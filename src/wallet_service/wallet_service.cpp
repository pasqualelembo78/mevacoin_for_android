// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#include "wallet_service.h"

#include <future>
#include <assert.h>
#include <sstream>
#include <unordered_set>
#include <tuple>

#include <boost/filesystem/operations.hpp>

#include <syst/timer.h>
#include <syst/interrupted_exception.h>
#include "common/base58.h"
#include "common/util.h"

#include "crypto/crypto.h"
#include "mevacoin.h"
#include "mevacoin_core/mevacoin_format_utils.h"
#include "mevacoin_core/mevacoin_basic_impl.h"
#include "mevacoin_core/mevacoin_tools.h"
#include "mevacoin_core/transaction_extra.h"
#include "mevacoin_core/account.h"
#include "mevacoin_core/mixins.h"

#include <syst/event_lock.h>
#include <syst/remote_context.h>

#include "payment_service_json_rpc_messages.h"
#include "node_factory.h"

#include "wallet/wallet_green.h"
#include "wallet/wallet_errors.h"
#include "wallet/wallet_utils.h"
#include "wallet_service_error_category.h"

#include "mnemonics/mnemonics.h"

namespace payment_service
{

    namespace
    {

        bool checkPaymentId(const std::string &paymentId)
        {
            if (paymentId.size() != 64)
            {
                return false;
            }

            return std::all_of(paymentId.begin(), paymentId.end(), [](const char c)
                               {
    if (c >= '0' && c <= '9') {
      return true;
    }

    if (c >= 'a' && c <= 'f') {
      return true;
    }

    if (c >= 'A' && c <= 'F') {
      return true;
    }

    return false; });
        }

        crypto::Hash parsePaymentId(const std::string &paymentIdStr)
        {
            if (!checkPaymentId(paymentIdStr))
            {
                throw std::system_error(make_error_code(mevacoin::error::WalletServiceErrorCode::WRONG_PAYMENT_ID_FORMAT));
            }

            crypto::Hash paymentId;
            bool r = common::podFromHex(paymentIdStr, paymentId);
            if (r)
            {
            }
            assert(r);

            return paymentId;
        }

        bool getPaymentIdFromExtra(const std::string &binaryString, crypto::Hash &paymentId)
        {
            return mevacoin::getPaymentIdFromTxExtra(common::asBinaryArray(binaryString), paymentId);
        }

        std::string getPaymentIdStringFromExtra(const std::string &binaryString)
        {
            crypto::Hash paymentId;

            try
            {
                if (!getPaymentIdFromExtra(binaryString, paymentId))
                {
                    return std::string();
                }
            }
            catch (std::exception &)
            {
                return std::string();
            }

            return common::podToHex(paymentId);
        }

    }

    struct TransactionsInBlockInfoFilter
    {
        TransactionsInBlockInfoFilter(const std::vector<std::string> &addressesVec, const std::string &paymentIdStr)
        {
            addresses.insert(addressesVec.begin(), addressesVec.end());

            if (!paymentIdStr.empty())
            {
                paymentId = parsePaymentId(paymentIdStr);
                havePaymentId = true;
            }
            else
            {
                havePaymentId = false;
            }
        }

        bool checkTransaction(const mevacoin::WalletTransactionWithTransfers &transaction) const
        {
            if (havePaymentId)
            {
                crypto::Hash transactionPaymentId;
                if (!getPaymentIdFromExtra(transaction.transaction.extra, transactionPaymentId))
                {
                    return false;
                }

                if (paymentId != transactionPaymentId)
                {
                    return false;
                }
            }

            if (addresses.empty())
            {
                return true;
            }

            bool haveAddress = false;
            for (const mevacoin::WalletTransfer &transfer : transaction.transfers)
            {
                if (addresses.find(transfer.address) != addresses.end())
                {
                    haveAddress = true;
                    break;
                }
            }

            return haveAddress;
        }

        std::unordered_set<std::string> addresses;
        bool havePaymentId = false;
        crypto::Hash paymentId;
    };

    namespace
    {

        void addPaymentIdToExtra(const std::string &paymentId, std::string &extra)
        {
            std::vector<uint8_t> extraVector;
            if (!mevacoin::createTxExtraWithPaymentId(paymentId, extraVector))
            {
                throw std::system_error(make_error_code(mevacoin::error::BAD_PAYMENT_ID));
            }

            std::copy(extraVector.begin(), extraVector.end(), std::back_inserter(extra));
        }

        void validatePaymentId(const std::string &paymentId, logging::LoggerRef logger)
        {
            if (!checkPaymentId(paymentId))
            {
                logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Can't validate payment id: " << paymentId;
                throw std::system_error(make_error_code(mevacoin::error::WalletServiceErrorCode::WRONG_PAYMENT_ID_FORMAT));
            }
        }

        crypto::Hash parseHash(const std::string &hashString, logging::LoggerRef logger)
        {
            crypto::Hash hash;

            if (!common::podFromHex(hashString, hash))
            {
                logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Can't parse hash string " << hashString;
                throw std::system_error(make_error_code(mevacoin::error::WalletServiceErrorCode::WRONG_HASH_FORMAT));
            }

            return hash;
        }

        std::vector<mevacoin::TransactionsInBlockInfo> filterTransactions(
            const std::vector<mevacoin::TransactionsInBlockInfo> &blocks,
            const TransactionsInBlockInfoFilter &filter)
        {

            std::vector<mevacoin::TransactionsInBlockInfo> result;

            for (const auto &block : blocks)
            {
                mevacoin::TransactionsInBlockInfo item;
                item.blockHash = block.blockHash;

                for (const auto &transaction : block.transactions)
                {
                    if (transaction.transaction.state != mevacoin::WalletTransactionState::DELETED && filter.checkTransaction(transaction))
                    {
                        item.transactions.push_back(transaction);
                    }
                }

                if (!block.transactions.empty())
                {
                    result.push_back(std::move(item));
                }
            }

            return result;
        }

        payment_service::TransactionRpcInfo convertTransactionWithTransfersToTransactionRpcInfo(
            const mevacoin::WalletTransactionWithTransfers &transactionWithTransfers)
        {

            payment_service::TransactionRpcInfo transactionInfo;

            transactionInfo.state = static_cast<uint8_t>(transactionWithTransfers.transaction.state);
            transactionInfo.transactionHash = common::podToHex(transactionWithTransfers.transaction.hash);
            transactionInfo.blockIndex = transactionWithTransfers.transaction.blockHeight;
            transactionInfo.timestamp = transactionWithTransfers.transaction.timestamp;
            transactionInfo.isBase = transactionWithTransfers.transaction.isBase;
            transactionInfo.unlockTime = transactionWithTransfers.transaction.unlockTime;
            transactionInfo.amount = transactionWithTransfers.transaction.totalAmount;
            transactionInfo.fee = transactionWithTransfers.transaction.fee;
            transactionInfo.extra = common::toHex(transactionWithTransfers.transaction.extra.data(), transactionWithTransfers.transaction.extra.size());
            transactionInfo.paymentId = getPaymentIdStringFromExtra(transactionWithTransfers.transaction.extra);

            for (const mevacoin::WalletTransfer &transfer : transactionWithTransfers.transfers)
            {
                payment_service::TransferRpcInfo rpcTransfer;
                rpcTransfer.address = transfer.address;
                rpcTransfer.amount = transfer.amount;
                rpcTransfer.type = static_cast<uint8_t>(transfer.type);

                transactionInfo.transfers.push_back(std::move(rpcTransfer));
            }

            return transactionInfo;
        }

        std::vector<payment_service::TransactionsInBlockRpcInfo> convertTransactionsInBlockInfoToTransactionsInBlockRpcInfo(
            const std::vector<mevacoin::TransactionsInBlockInfo> &blocks)
        {

            std::vector<payment_service::TransactionsInBlockRpcInfo> rpcBlocks;
            rpcBlocks.reserve(blocks.size());
            for (const auto &block : blocks)
            {
                payment_service::TransactionsInBlockRpcInfo rpcBlock;
                rpcBlock.blockHash = common::podToHex(block.blockHash);

                for (const mevacoin::WalletTransactionWithTransfers &transactionWithTransfers : block.transactions)
                {
                    payment_service::TransactionRpcInfo transactionInfo = convertTransactionWithTransfersToTransactionRpcInfo(transactionWithTransfers);
                    rpcBlock.transactions.push_back(std::move(transactionInfo));
                }

                rpcBlocks.push_back(std::move(rpcBlock));
            }

            return rpcBlocks;
        }

        std::vector<payment_service::TransactionHashesInBlockRpcInfo> convertTransactionsInBlockInfoToTransactionHashesInBlockRpcInfo(
            const std::vector<mevacoin::TransactionsInBlockInfo> &blocks)
        {

            std::vector<payment_service::TransactionHashesInBlockRpcInfo> transactionHashes;
            transactionHashes.reserve(blocks.size());
            for (const mevacoin::TransactionsInBlockInfo &block : blocks)
            {
                payment_service::TransactionHashesInBlockRpcInfo item;
                item.blockHash = common::podToHex(block.blockHash);

                for (const mevacoin::WalletTransactionWithTransfers &transaction : block.transactions)
                {
                    item.transactionHashes.emplace_back(common::podToHex(transaction.transaction.hash));
                }

                transactionHashes.push_back(std::move(item));
            }

            return transactionHashes;
        }

        void validateAddresses(const std::vector<std::string> &addresses, const mevacoin::Currency &currency, logging::LoggerRef logger)
        {
            for (const auto &address : addresses)
            {
                if (!mevacoin::validateAddress(address, currency))
                {
                    logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Can't validate address " << address;
                    throw std::system_error(make_error_code(mevacoin::error::BAD_ADDRESS));
                }
            }
        }

        std::tuple<std::string, std::string> decodeIntegratedAddress(const std::string &integratedAddr, const mevacoin::Currency &currency, logging::LoggerRef logger)
        {
            std::string decoded;
            uint64_t prefix;

            /* Need to be able to decode the string as an address */
            if (!tools::base58::decode_addr(integratedAddr, prefix, decoded))
            {
                throw std::system_error(make_error_code(mevacoin::error::BAD_ADDRESS));
            }

            /* The prefix needs to be the same as the base58 prefix */
            if (prefix !=
                mevacoin::parameters::MEVACOIN_PUBLIC_ADDRESS_BASE58_PREFIX)
            {
                throw std::system_error(make_error_code(mevacoin::error::BAD_ADDRESS));
            }

            const uint64_t paymentIDLen = 64;
            /* Grab the payment ID from the decoded address */
            std::string paymentID = decoded.substr(0, paymentIDLen);

            /* Check the extracted payment ID is good. */
            validatePaymentId(paymentID, logger);

            /* The binary array encoded keys are the rest of the address */
            std::string keys = decoded.substr(paymentIDLen, std::string::npos);

            mevacoin::AccountPublicAddress addr;
            mevacoin::BinaryArray ba = common::asBinaryArray(keys);

            if (!mevacoin::fromBinaryArray(addr, ba))
            {
                throw std::system_error(make_error_code(mevacoin::error::BAD_ADDRESS));
            }

            /* Parse the AccountPublicAddress into a standard wallet address */
            /* Use the calculated prefix from earlier for less typing :p */
            std::string address = mevacoin::getAccountAddressAsStr(prefix, addr);

            /* Check the extracted address is good. */
            validateAddresses({address}, currency, logger);

            return std::make_tuple(address, paymentID);
        }

        std::string getValidatedTransactionExtraString(const std::string &extraString)
        {
            std::vector<uint8_t> binary;
            if (!common::fromHex(extraString, binary))
            {
                throw std::system_error(make_error_code(mevacoin::error::BAD_TRANSACTION_EXTRA));
            }

            return common::asString(binary);
        }

        std::vector<std::string> collectDestinationAddresses(const std::vector<payment_service::WalletRpcOrder> &orders)
        {
            std::vector<std::string> result;

            result.reserve(orders.size());
            for (const auto &order : orders)
            {
                result.push_back(order.address);
            }

            return result;
        }

        std::vector<mevacoin::WalletOrder> convertWalletRpcOrdersToWalletOrders(const std::vector<payment_service::WalletRpcOrder> &orders, const std::string nodeAddress, const uint32_t nodeFee)
        {
            std::vector<mevacoin::WalletOrder> result;

            if (!nodeAddress.empty() && nodeFee != 0)
            {
                result.reserve(orders.size() + 1);
                result.emplace_back(mevacoin::WalletOrder{nodeAddress, nodeFee});
            }
            else
            {
                result.reserve(orders.size());
            }

            for (const auto &order : orders)
            {
                result.emplace_back(mevacoin::WalletOrder{order.address, order.amount});
            }

            return result;
        }

    }

    void generateNewWallet(const mevacoin::Currency &currency, const WalletConfiguration &conf, std::shared_ptr<logging::ILogger> logger, syst::Dispatcher &dispatcher)
    {
        logging::LoggerRef log(logger, "generateNewWallet");

        mevacoin::INode *nodeStub = NodeFactory::createNodeStub();
        std::unique_ptr<mevacoin::INode> nodeGuard(nodeStub);

        mevacoin::IWallet *wallet = new mevacoin::WalletGreen(dispatcher, currency, *nodeStub, logger);
        std::unique_ptr<mevacoin::IWallet> walletGuard(wallet);

        std::string address;
        if (conf.secretSpendKey.empty() && conf.secretViewKey.empty() && conf.mnemonicSeed.empty())
        {
            log(logging::INFO, logging::BRIGHT_WHITE) << "Generating new wallet";

            crypto::SecretKey private_view_key;
            mevacoin::KeyPair spendKey;

            crypto::generate_keys(spendKey.publicKey, spendKey.secretKey);
            mevacoin::AccountBase::generateViewFromSpend(spendKey.secretKey, private_view_key);

            wallet->initializeWithViewKey(conf.walletFile, conf.walletPassword, private_view_key, 0, true);
            address = wallet->createAddress(spendKey.secretKey, 0, true);

            log(logging::INFO, logging::BRIGHT_WHITE) << "New wallet is generated. Address: " << address;
        }
        else if (!conf.mnemonicSeed.empty())
        {
            log(logging::INFO, logging::BRIGHT_WHITE) << "Attempting to import wallet from mnemonic seed";

            auto [error, private_spend_key] = Mnemonics::MnemonicToPrivateKey(conf.mnemonicSeed);

            if (error)
            {
                log(logging::ERROR, logging::BRIGHT_RED) << error;
                return;
            }

            crypto::SecretKey private_view_key;

            mevacoin::AccountBase::generateViewFromSpend(private_spend_key, private_view_key);

            wallet->initializeWithViewKey(conf.walletFile, conf.walletPassword, private_view_key, conf.scanHeight, false);

            address = wallet->createAddress(private_spend_key, conf.scanHeight, false);

            log(logging::INFO, logging::BRIGHT_WHITE) << "Imported wallet successfully.";
        }
        else
        {
            if (conf.secretSpendKey.empty() || conf.secretViewKey.empty())
            {
                log(logging::ERROR, logging::BRIGHT_RED) << "Need both secret spend key and secret view key.";
                return;
            }
            else
            {
                log(logging::INFO, logging::BRIGHT_WHITE) << "Attemping to import wallet from keys";
                crypto::Hash private_spend_key_hash;
                crypto::Hash private_view_key_hash;
                uint64_t size;
                if (!common::fromHex(conf.secretSpendKey, &private_spend_key_hash, sizeof(private_spend_key_hash), size) || size != sizeof(private_spend_key_hash))
                {
                    log(logging::ERROR, logging::BRIGHT_RED) << "Invalid spend key";
                    return;
                }
                if (!common::fromHex(conf.secretViewKey, &private_view_key_hash, sizeof(private_view_key_hash), size) || size != sizeof(private_spend_key_hash))
                {
                    log(logging::ERROR, logging::BRIGHT_RED) << "Invalid view key";
                    return;
                }
                crypto::SecretKey private_spend_key = *(struct crypto::SecretKey *)&private_spend_key_hash;
                crypto::SecretKey private_view_key = *(struct crypto::SecretKey *)&private_view_key_hash;

                wallet->initializeWithViewKey(conf.walletFile, conf.walletPassword, private_view_key, conf.scanHeight, false);
                address = wallet->createAddress(private_spend_key, conf.scanHeight, false);
                log(logging::INFO, logging::BRIGHT_WHITE) << "Imported wallet successfully.";
            }
        }

        wallet->save(mevacoin::WalletSaveLevel::SAVE_KEYS_ONLY);
        log(logging::INFO, logging::BRIGHT_WHITE) << "Wallet is saved";
    }

    WalletService::WalletService(const mevacoin::Currency &currency, syst::Dispatcher &sys, mevacoin::INode &node,
                                 mevacoin::IWallet &wallet, mevacoin::IFusionManager &fusionManager, const WalletConfiguration &conf, std::shared_ptr<logging::ILogger> logger) : currency(currency),
                                                                                                                                                                                      wallet(wallet),
                                                                                                                                                                                      fusionManager(fusionManager),
                                                                                                                                                                                      node(node),
                                                                                                                                                                                      config(conf),
                                                                                                                                                                                      inited(false),
                                                                                                                                                                                      logger(logger, "WalletService"),
                                                                                                                                                                                      dispatcher(sys),
                                                                                                                                                                                      readyEvent(dispatcher),
                                                                                                                                                                                      refreshContext(dispatcher)
    {
        readyEvent.set();
    }

    WalletService::~WalletService()
    {
        if (inited)
        {
            wallet.stop();
            refreshContext.wait();
            wallet.shutdown();
        }
    }

    void WalletService::init()
    {
        loadWallet();
        loadTransactionIdIndex();

        getNodeFee();
        refreshContext.spawn([this]
                             { refresh(); });

        inited = true;
    }

    void WalletService::getNodeFee()
    {
        logger(logging::DEBUGGING) << "Trying to retrieve node fee information." << std::endl;

        m_node_address = node.feeAddress();
        m_node_fee = node.feeAmount();

        if (!m_node_address.empty() && m_node_fee != 0)
        {
            // Partially borrowed from <zedwallet/Tools.h>
            uint32_t div = static_cast<uint32_t>(pow(10, mevacoin::parameters::MEVACOIN_DISPLAY_DECIMAL_POINT));
            uint32_t coins = m_node_fee / div;
            uint32_t cents = m_node_fee % div;
            std::stringstream stream;
            stream << std::setfill('0') << std::setw(mevacoin::parameters::MEVACOIN_DISPLAY_DECIMAL_POINT) << cents;
            std::string amount = std::to_string(coins) + "." + stream.str();

            logger(logging::INFO, logging::RED) << "You have connected to a node that charges "
                                                << "a fee to send transactions." << std::endl;

            logger(logging::INFO, logging::RED) << "The fee for sending transactions is: " << amount << " per transaction." << std::endl;

            logger(logging::INFO, logging::RED) << "If you don't want to pay the node fee, please "
                                                << "relaunch this program and specify a different "
                                                << "node or run your own." << std::endl;
        }
    }

    void WalletService::saveWallet()
    {
        wallet.save();
        logger(logging::INFO, logging::BRIGHT_WHITE) << "Wallet is saved";
    }

    void WalletService::loadWallet()
    {
        logger(logging::INFO, logging::BRIGHT_WHITE) << "Loading wallet";
        wallet.load(config.walletFile, config.walletPassword);
        logger(logging::INFO, logging::BRIGHT_WHITE) << "Wallet loading is finished.";
    }

    void WalletService::loadTransactionIdIndex()
    {
        transactionIdIndex.clear();

        for (size_t i = 0; i < wallet.getTransactionCount(); ++i)
        {
            transactionIdIndex.emplace(common::podToHex(wallet.getTransaction(i).hash), i);
        }
    }

    std::error_code WalletService::saveWalletNoThrow()
    {
        try
        {
            syst::EventLock lk(readyEvent);

            logger(logging::INFO, logging::BRIGHT_WHITE) << "Saving wallet...";

            if (!inited)
            {
                logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Save impossible: Wallet Service is not initialized";
                return make_error_code(mevacoin::error::NOT_INITIALIZED);
            }

            saveWallet();
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while saving wallet: " << x.what();
            return x.code();
        }
        catch (std::exception &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while saving wallet: " << x.what();
            return make_error_code(mevacoin::error::INTERNAL_WALLET_ERROR);
        }

        return std::error_code();
    }

    std::error_code WalletService::exportWallet(const std::string &fileName)
    {
        try
        {
            syst::EventLock lk(readyEvent);

            if (!inited)
            {
                logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Export impossible: Wallet Service is not initialized";
                return make_error_code(mevacoin::error::NOT_INITIALIZED);
            }

            boost::filesystem::path walletPath(config.walletFile);
            boost::filesystem::path exportPath = walletPath.parent_path() / fileName;

            logger(logging::INFO, logging::BRIGHT_WHITE) << "Exporting wallet to " << exportPath.string();
            wallet.exportWallet(exportPath.string());
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while exporting wallet: " << x.what();
            return x.code();
        }
        catch (std::exception &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while exporting wallet: " << x.what();
            return make_error_code(mevacoin::error::INTERNAL_WALLET_ERROR);
        }

        return std::error_code();
    }

    std::error_code WalletService::resetWallet(const uint64_t scanHeight)
    {
        try
        {
            syst::EventLock lk(readyEvent);

            logger(logging::INFO, logging::BRIGHT_WHITE) << "Resetting wallet";

            if (!inited)
            {
                logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Reset impossible: Wallet Service is not initialized";
                return make_error_code(mevacoin::error::NOT_INITIALIZED);
            }

            reset(scanHeight);
            logger(logging::INFO, logging::BRIGHT_WHITE) << "Wallet has been reset";
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while resetting wallet: " << x.what();
            return x.code();
        }
        catch (std::exception &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while resetting wallet: " << x.what();
            return make_error_code(mevacoin::error::INTERNAL_WALLET_ERROR);
        }

        return std::error_code();
    }

    std::error_code WalletService::createAddress(const std::string &spendSecretKeyText, uint64_t scanHeight, bool newAddress, std::string &address)
    {
        try
        {
            syst::EventLock lk(readyEvent);

            logger(logging::DEBUGGING) << "Creating address";

            crypto::SecretKey secretKey;
            if (!common::podFromHex(spendSecretKeyText, secretKey))
            {
                logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Wrong key format: " << spendSecretKeyText;
                return make_error_code(mevacoin::error::WalletServiceErrorCode::WRONG_KEY_FORMAT);
            }

            address = wallet.createAddress(secretKey, scanHeight, newAddress);
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while creating address: " << x.what();
            return x.code();
        }

        logger(logging::DEBUGGING) << "Created address " << address;

        return std::error_code();
    }

    std::error_code WalletService::createAddressList(const std::vector<std::string> &spendSecretKeysText, uint64_t scanHeight, bool newAddress, std::vector<std::string> &addresses)
    {
        try
        {
            syst::EventLock lk(readyEvent);

            logger(logging::DEBUGGING) << "Creating " << spendSecretKeysText.size() << " addresses...";

            std::vector<crypto::SecretKey> secretKeys;
            std::unordered_set<std::string> unique;
            secretKeys.reserve(spendSecretKeysText.size());
            unique.reserve(spendSecretKeysText.size());
            for (auto &keyText : spendSecretKeysText)
            {
                auto insertResult = unique.insert(keyText);
                if (!insertResult.second)
                {
                    logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Not unique key";
                    return make_error_code(mevacoin::error::WalletServiceErrorCode::DUPLICATE_KEY);
                }

                crypto::SecretKey key;
                if (!common::podFromHex(keyText, key))
                {
                    logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Wrong key format: " << keyText;
                    return make_error_code(mevacoin::error::WalletServiceErrorCode::WRONG_KEY_FORMAT);
                }

                secretKeys.push_back(std::move(key));
            }

            addresses = wallet.createAddressList(secretKeys, scanHeight, newAddress);
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while creating addresses: " << x.what();
            return x.code();
        }

        logger(logging::DEBUGGING) << "Created " << addresses.size() << " addresses";

        return std::error_code();
    }

    std::error_code WalletService::createAddress(std::string &address)
    {
        try
        {
            syst::EventLock lk(readyEvent);

            logger(logging::DEBUGGING) << "Creating address";

            address = wallet.createAddress();
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while creating address: " << x.what();
            return x.code();
        }

        logger(logging::DEBUGGING) << "Created address " << address;

        return std::error_code();
    }

    std::error_code WalletService::createTrackingAddress(const std::string &spendPublicKeyText, uint64_t scanHeight, bool newAddress, std::string &address)
    {
        try
        {
            syst::EventLock lk(readyEvent);

            logger(logging::DEBUGGING) << "Creating tracking address";

            crypto::PublicKey publicKey;
            if (!common::podFromHex(spendPublicKeyText, publicKey))
            {
                logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Wrong key format: " << spendPublicKeyText;
                return make_error_code(mevacoin::error::WalletServiceErrorCode::WRONG_KEY_FORMAT);
            }

            address = wallet.createAddress(publicKey, scanHeight, true);
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while creating tracking address: " << x.what();
            return x.code();
        }

        logger(logging::DEBUGGING) << "Created address " << address;
        return std::error_code();
    }

    std::error_code WalletService::deleteAddress(const std::string &address)
    {
        try
        {
            syst::EventLock lk(readyEvent);

            logger(logging::DEBUGGING) << "Delete address request came";
            wallet.deleteAddress(address);
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while deleting address: " << x.what();
            return x.code();
        }

        logger(logging::DEBUGGING) << "Address " << address << " successfully deleted";
        return std::error_code();
    }

    std::error_code WalletService::getSpendkeys(const std::string &address, std::string &publicSpendKeyText, std::string &secretSpendKeyText)
    {
        try
        {
            syst::EventLock lk(readyEvent);

            mevacoin::KeyPair key = wallet.getAddressSpendKey(address);

            publicSpendKeyText = common::podToHex(key.publicKey);
            secretSpendKeyText = common::podToHex(key.secretKey);
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while getting spend key: " << x.what();
            return x.code();
        }

        return std::error_code();
    }

    std::error_code WalletService::getBalance(const std::string &address, uint64_t &availableBalance, uint64_t &lockedAmount)
    {
        try
        {
            syst::EventLock lk(readyEvent);
            logger(logging::DEBUGGING) << "Getting balance for address " << address;

            availableBalance = wallet.getActualBalance(address);
            lockedAmount = wallet.getPendingBalance(address);
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while getting balance: " << x.what();
            return x.code();
        }

        logger(logging::DEBUGGING) << address << " actual balance: " << availableBalance << ", pending: " << lockedAmount;
        return std::error_code();
    }

    std::error_code WalletService::getBalance(uint64_t &availableBalance, uint64_t &lockedAmount)
    {
        try
        {
            syst::EventLock lk(readyEvent);
            logger(logging::DEBUGGING) << "Getting wallet balance";

            availableBalance = wallet.getActualBalance();
            lockedAmount = wallet.getPendingBalance();
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while getting balance: " << x.what();
            return x.code();
        }

        logger(logging::DEBUGGING) << "Wallet actual balance: " << availableBalance << ", pending: " << lockedAmount;
        return std::error_code();
    }

    std::error_code WalletService::getBlockHashes(uint32_t firstBlockIndex, uint32_t blockCount, std::vector<std::string> &blockHashes)
    {
        try
        {
            syst::EventLock lk(readyEvent);
            std::vector<crypto::Hash> hashes = wallet.getBlockHashes(firstBlockIndex, blockCount);

            blockHashes.reserve(hashes.size());
            for (const auto &hash : hashes)
            {
                blockHashes.push_back(common::podToHex(hash));
            }
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while getting block hashes: " << x.what();
            return x.code();
        }

        return std::error_code();
    }

    std::error_code WalletService::getViewKey(std::string &viewSecretKey)
    {
        try
        {
            syst::EventLock lk(readyEvent);
            mevacoin::KeyPair viewKey = wallet.getViewKey();
            viewSecretKey = common::podToHex(viewKey.secretKey);
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while getting view key: " << x.what();
            return x.code();
        }

        return std::error_code();
    }

    std::error_code WalletService::getMnemonicSeed(const std::string &address, std::string &mnemonicSeed)
    {
        try
        {
            syst::EventLock lk(readyEvent);
            mevacoin::KeyPair key = wallet.getAddressSpendKey(address);
            mevacoin::KeyPair viewKey = wallet.getViewKey();

            crypto::SecretKey deterministic_private_view_key;

            mevacoin::AccountBase::generateViewFromSpend(key.secretKey, deterministic_private_view_key);

            bool deterministic_private_keys = deterministic_private_view_key == viewKey.secretKey;

            if (deterministic_private_keys)
            {
                mnemonicSeed = Mnemonics::PrivateKeyToMnemonic(key.secretKey);
            }
            else
            {
                /* Have to be able to derive view key from spend key to create a mnemonic
                   seed, due to being able to generate multiple addresses we can't do
                   this in walletd as the default */
                logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Your private keys are not deterministic and so a mnemonic seed cannot be generated!";
                return make_error_code(mevacoin::error::WalletServiceErrorCode::KEYS_NOT_DETERMINISTIC);
            }
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while getting mnemonic seed: " << x.what();
            return x.code();
        }

        return std::error_code();
    }

    std::error_code WalletService::getTransactionHashes(const std::vector<std::string> &addresses, const std::string &blockHashString,
                                                        uint32_t blockCount, const std::string &paymentId, std::vector<TransactionHashesInBlockRpcInfo> &transactionHashes)
    {
        try
        {
            syst::EventLock lk(readyEvent);
            validateAddresses(addresses, currency, logger);

            if (!paymentId.empty())
            {
                validatePaymentId(paymentId, logger);
            }

            TransactionsInBlockInfoFilter transactionFilter(addresses, paymentId);
            crypto::Hash blockHash = parseHash(blockHashString, logger);

            transactionHashes = getRpcTransactionHashes(blockHash, blockCount, transactionFilter);
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while getting transactions: " << x.what();
            return x.code();
        }
        catch (std::exception &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while getting transactions: " << x.what();
            return make_error_code(mevacoin::error::INTERNAL_WALLET_ERROR);
        }

        return std::error_code();
    }

    std::error_code WalletService::getTransactionHashes(const std::vector<std::string> &addresses, uint32_t firstBlockIndex,
                                                        uint32_t blockCount, const std::string &paymentId, std::vector<TransactionHashesInBlockRpcInfo> &transactionHashes)
    {
        try
        {
            syst::EventLock lk(readyEvent);
            validateAddresses(addresses, currency, logger);

            if (!paymentId.empty())
            {
                validatePaymentId(paymentId, logger);
            }

            TransactionsInBlockInfoFilter transactionFilter(addresses, paymentId);
            transactionHashes = getRpcTransactionHashes(firstBlockIndex, blockCount, transactionFilter);
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while getting transactions: " << x.what();
            return x.code();
        }
        catch (std::exception &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while getting transactions: " << x.what();
            return make_error_code(mevacoin::error::INTERNAL_WALLET_ERROR);
        }

        return std::error_code();
    }

    std::error_code WalletService::getTransactions(const std::vector<std::string> &addresses, const std::string &blockHashString,
                                                   uint32_t blockCount, const std::string &paymentId, std::vector<TransactionsInBlockRpcInfo> &transactions)
    {
        try
        {
            syst::EventLock lk(readyEvent);
            validateAddresses(addresses, currency, logger);

            if (!paymentId.empty())
            {
                validatePaymentId(paymentId, logger);
            }

            TransactionsInBlockInfoFilter transactionFilter(addresses, paymentId);

            crypto::Hash blockHash = parseHash(blockHashString, logger);

            transactions = getRpcTransactions(blockHash, blockCount, transactionFilter);
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while getting transactions: " << x.what();
            return x.code();
        }
        catch (std::exception &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while getting transactions: " << x.what();
            return make_error_code(mevacoin::error::INTERNAL_WALLET_ERROR);
        }

        return std::error_code();
    }

    std::error_code WalletService::getTransactions(const std::vector<std::string> &addresses, uint32_t firstBlockIndex,
                                                   uint32_t blockCount, const std::string &paymentId, std::vector<TransactionsInBlockRpcInfo> &transactions)
    {
        try
        {
            syst::EventLock lk(readyEvent);
            validateAddresses(addresses, currency, logger);

            if (!paymentId.empty())
            {
                validatePaymentId(paymentId, logger);
            }

            TransactionsInBlockInfoFilter transactionFilter(addresses, paymentId);

            transactions = getRpcTransactions(firstBlockIndex, blockCount, transactionFilter);
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while getting transactions: " << x.what();
            return x.code();
        }
        catch (std::exception &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while getting transactions: " << x.what();
            return make_error_code(mevacoin::error::INTERNAL_WALLET_ERROR);
        }

        return std::error_code();
    }

    std::error_code WalletService::getTransaction(const std::string &transactionHash, TransactionRpcInfo &transaction)
    {
        try
        {
            syst::EventLock lk(readyEvent);
            crypto::Hash hash = parseHash(transactionHash, logger);

            mevacoin::WalletTransactionWithTransfers transactionWithTransfers = wallet.getTransaction(hash);

            if (transactionWithTransfers.transaction.state == mevacoin::WalletTransactionState::DELETED)
            {
                logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Transaction " << transactionHash << " is deleted";
                return make_error_code(mevacoin::error::OBJECT_NOT_FOUND);
            }

            transaction = convertTransactionWithTransfersToTransactionRpcInfo(transactionWithTransfers);
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while getting transaction: " << x.what();
            return x.code();
        }
        catch (std::exception &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while getting transaction: " << x.what();
            return make_error_code(mevacoin::error::INTERNAL_WALLET_ERROR);
        }

        return std::error_code();
    }

    std::error_code WalletService::getAddresses(std::vector<std::string> &addresses)
    {
        try
        {
            syst::EventLock lk(readyEvent);

            addresses.clear();
            addresses.reserve(wallet.getAddressCount());

            for (size_t i = 0; i < wallet.getAddressCount(); ++i)
            {
                addresses.push_back(wallet.getAddress(i));
            }
        }
        catch (std::exception &e)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Can't get addresses: " << e.what();
            return make_error_code(mevacoin::error::INTERNAL_WALLET_ERROR);
        }

        return std::error_code();
    }

    std::error_code WalletService::sendTransaction(SendTransaction::Request &request, std::string &transactionHash)
    {
        try
        {
            syst::EventLock lk(readyEvent);

            /* Integrated address payment ID's are uppercase - lets convert the input
               payment ID to upper so we can compare with more ease */
            std::transform(request.paymentId.begin(), request.paymentId.end(), request.paymentId.begin(), ::toupper);

            std::vector<std::string> paymentIDs;

            for (auto &transfer : request.transfers)
            {
                std::string addr = transfer.address;

                /* It's not a standard address. Is it an integrated address? */
                if (!mevacoin::validateAddress(addr, currency))
                {
                    std::string address, paymentID;
                    std::tie(address, paymentID) = decodeIntegratedAddress(addr, currency, logger);

                    /* A payment ID was specified with the transaction, and it is not
                       the same as the decoded one -> we can't send a transaction
                       with two different payment ID's! */
                    if (request.paymentId != "" && request.paymentId != paymentID)
                    {
                        throw std::system_error(make_error_code(mevacoin::error::CONFLICTING_PAYMENT_IDS));
                    }

                    /* Replace the integrated transfer address with the actual
                       decoded address */
                    transfer.address = address;

                    paymentIDs.push_back(paymentID);
                }
            }

            /* Only one integrated address specified, set the payment ID to the
               decoded value */
            if (paymentIDs.size() == 1)
            {
                request.paymentId = paymentIDs[0];
            }
            else if (paymentIDs.size() > 1)
            {
                /* Are all the specified payment IDs equal? */
                if (!std::equal(paymentIDs.begin() + 1, paymentIDs.end(), paymentIDs.begin()))
                {
                    throw std::system_error(make_error_code(mevacoin::error::CONFLICTING_PAYMENT_IDS));
                }

                /* They are all equal, set the payment ID to the decoded value */
                request.paymentId = paymentIDs[0];
            }

            validateAddresses(request.sourceAddresses, currency, logger);
            validateAddresses(collectDestinationAddresses(request.transfers), currency, logger);
            if (!request.changeAddress.empty())
            {
                validateAddresses({request.changeAddress}, currency, logger);
            }

            auto [success, error, error_code] = mevacoin::Mixins::validate(request.anonymity, node.getLastKnownBlockHeight());

            if (!success)
            {
                logger(logging::WARNING, logging::BRIGHT_YELLOW) << error;
                throw std::system_error(error_code);
            }

            mevacoin::TransactionParameters sendParams;
            if (!request.paymentId.empty())
            {
                addPaymentIdToExtra(request.paymentId, sendParams.extra);
            }
            else
            {
                sendParams.extra = getValidatedTransactionExtraString(request.extra);
            }

            sendParams.sourceAddresses = request.sourceAddresses;
            sendParams.destinations = convertWalletRpcOrdersToWalletOrders(request.transfers, m_node_address, m_node_fee);
            sendParams.fee = request.fee;
            sendParams.mixIn = request.anonymity;
            sendParams.unlockTimestamp = request.unlockTime;
            sendParams.changeDestination = request.changeAddress;

            size_t transactionId = wallet.transfer(sendParams);
            transactionHash = common::podToHex(wallet.getTransaction(transactionId).hash);

            logger(logging::DEBUGGING) << "Transaction " << transactionHash << " has been sent";
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while sending transaction: " << x.what();
            return x.code();
        }
        catch (std::exception &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while sending transaction: " << x.what();
            return make_error_code(mevacoin::error::INTERNAL_WALLET_ERROR);
        }

        return std::error_code();
    }

    std::error_code WalletService::createDelayedTransaction(CreateDelayedTransaction::Request &request, std::string &transactionHash)
    {
        try
        {
            syst::EventLock lk(readyEvent);

            /* Integrated address payment ID's are uppercase - lets convert the input
               payment ID to upper so we can compare with more ease */
            std::transform(request.paymentId.begin(), request.paymentId.end(), request.paymentId.begin(), ::toupper);

            std::vector<std::string> paymentIDs;

            for (auto &transfer : request.transfers)
            {
                std::string addr = transfer.address;

                /* It's not a standard address. Is it an integrated address? */
                if (!mevacoin::validateAddress(addr, currency))
                {
                    std::string address, paymentID;
                    std::tie(address, paymentID) = decodeIntegratedAddress(addr, currency, logger);

                    /* A payment ID was specified with the transaction, and it is not
                       the same as the decoded one -> we can't send a transaction
                       with two different payment ID's! */
                    if (request.paymentId != "" && request.paymentId != paymentID)
                    {
                        throw std::system_error(make_error_code(mevacoin::error::CONFLICTING_PAYMENT_IDS));
                    }

                    /* Replace the integrated transfer address with the actual
                       decoded address */
                    transfer.address = address;

                    paymentIDs.push_back(paymentID);
                }
            }

            /* Only one integrated address specified, set the payment ID to the
               decoded value */
            if (paymentIDs.size() == 1)
            {
                request.paymentId = paymentIDs[0];
            }
            else if (paymentIDs.size() > 1)
            {
                /* Are all the specified payment IDs equal? */
                if (!std::equal(paymentIDs.begin() + 1, paymentIDs.end(), paymentIDs.begin()))
                {
                    throw std::system_error(make_error_code(mevacoin::error::CONFLICTING_PAYMENT_IDS));
                }

                /* They are all equal, set the payment ID to the decoded value */
                request.paymentId = paymentIDs[0];
            }

            validateAddresses(request.addresses, currency, logger);
            validateAddresses(collectDestinationAddresses(request.transfers), currency, logger);
            if (!request.changeAddress.empty())
            {
                validateAddresses({request.changeAddress}, currency, logger);
            }

            mevacoin::TransactionParameters sendParams;
            if (!request.paymentId.empty())
            {
                addPaymentIdToExtra(request.paymentId, sendParams.extra);
            }
            else
            {
                sendParams.extra = common::asString(common::fromHex(request.extra));
            }

            sendParams.sourceAddresses = request.addresses;
            sendParams.destinations = convertWalletRpcOrdersToWalletOrders(request.transfers, m_node_address, m_node_fee);
            sendParams.fee = request.fee;
            sendParams.mixIn = request.anonymity;
            sendParams.unlockTimestamp = request.unlockTime;
            sendParams.changeDestination = request.changeAddress;

            size_t transactionId = wallet.makeTransaction(sendParams);
            transactionHash = common::podToHex(wallet.getTransaction(transactionId).hash);

            logger(logging::DEBUGGING) << "Delayed transaction " << transactionHash << " has been created";
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while creating delayed transaction: " << x.what();
            return x.code();
        }
        catch (std::exception &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while creating delayed transaction: " << x.what();
            return make_error_code(mevacoin::error::INTERNAL_WALLET_ERROR);
        }

        return std::error_code();
    }

    std::error_code WalletService::getDelayedTransactionHashes(std::vector<std::string> &transactionHashes)
    {
        try
        {
            syst::EventLock lk(readyEvent);

            std::vector<size_t> transactionIds = wallet.getDelayedTransactionIds();
            transactionHashes.reserve(transactionIds.size());

            for (auto id : transactionIds)
            {
                transactionHashes.emplace_back(common::podToHex(wallet.getTransaction(id).hash));
            }
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while getting delayed transaction hashes: " << x.what();
            return x.code();
        }
        catch (std::exception &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while getting delayed transaction hashes: " << x.what();
            return make_error_code(mevacoin::error::INTERNAL_WALLET_ERROR);
        }

        return std::error_code();
    }

    std::error_code WalletService::deleteDelayedTransaction(const std::string &transactionHash)
    {
        try
        {
            syst::EventLock lk(readyEvent);

            parseHash(transactionHash, logger); // validate transactionHash parameter

            auto idIt = transactionIdIndex.find(transactionHash);
            if (idIt == transactionIdIndex.end())
            {
                return make_error_code(mevacoin::error::WalletServiceErrorCode::OBJECT_NOT_FOUND);
            }

            size_t transactionId = idIt->second;
            wallet.rollbackUncommitedTransaction(transactionId);

            logger(logging::DEBUGGING) << "Delayed transaction " << transactionHash << " has been canceled";
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while deleting delayed transaction hashes: " << x.what();
            return x.code();
        }
        catch (std::exception &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while deleting delayed transaction hashes: " << x.what();
            return make_error_code(mevacoin::error::INTERNAL_WALLET_ERROR);
        }

        return std::error_code();
    }

    std::error_code WalletService::sendDelayedTransaction(const std::string &transactionHash)
    {
        try
        {
            syst::EventLock lk(readyEvent);

            parseHash(transactionHash, logger); // validate transactionHash parameter

            auto idIt = transactionIdIndex.find(transactionHash);
            if (idIt == transactionIdIndex.end())
            {
                return make_error_code(mevacoin::error::WalletServiceErrorCode::OBJECT_NOT_FOUND);
            }

            size_t transactionId = idIt->second;
            wallet.commitTransaction(transactionId);

            logger(logging::DEBUGGING) << "Delayed transaction " << transactionHash << " has been sent";
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while sending delayed transaction hashes: " << x.what();
            return x.code();
        }
        catch (std::exception &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while sending delayed transaction hashes: " << x.what();
            return make_error_code(mevacoin::error::INTERNAL_WALLET_ERROR);
        }

        return std::error_code();
    }

    std::error_code WalletService::getUnconfirmedTransactionHashes(const std::vector<std::string> &addresses, std::vector<std::string> &transactionHashes)
    {
        try
        {
            syst::EventLock lk(readyEvent);

            validateAddresses(addresses, currency, logger);

            std::vector<mevacoin::WalletTransactionWithTransfers> transactions = wallet.getUnconfirmedTransactions();

            TransactionsInBlockInfoFilter transactionFilter(addresses, "");

            for (const auto &transaction : transactions)
            {
                if (transactionFilter.checkTransaction(transaction))
                {
                    transactionHashes.emplace_back(common::podToHex(transaction.transaction.hash));
                }
            }
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while getting unconfirmed transaction hashes: " << x.what();
            return x.code();
        }
        catch (std::exception &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while getting unconfirmed transaction hashes: " << x.what();
            return make_error_code(mevacoin::error::INTERNAL_WALLET_ERROR);
        }

        return std::error_code();
    }

    /* blockCount = the blocks the wallet has synced. knownBlockCount = the top block the daemon knows of. localDaemonBlockCount = the blocks the daemon has synced. */
    std::error_code WalletService::getStatus(uint32_t &blockCount, uint32_t &knownBlockCount, uint64_t &localDaemonBlockCount, std::string &lastBlockHash, uint32_t &peerCount)
    {
        try
        {
            syst::EventLock lk(readyEvent);

            syst::RemoteContext<std::tuple<uint32_t, uint64_t, uint32_t>> remoteContext(dispatcher, [this]()
                                                                                        {
      /* Daemon remote height, daemon local height, peer count */
      return std::make_tuple(node.getKnownBlockCount(), node.getNodeHeight(), static_cast<uint32_t>(node.getPeerCount())); });

            std::tie(knownBlockCount, localDaemonBlockCount, peerCount) = remoteContext.get();

            blockCount = wallet.getBlockCount();

            auto lastHashes = wallet.getBlockHashes(blockCount - 1, 1);
            lastBlockHash = common::podToHex(lastHashes.back());
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while getting status: " << x.what();
            return x.code();
        }
        catch (std::exception &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while getting status: " << x.what();
            return make_error_code(mevacoin::error::INTERNAL_WALLET_ERROR);
        }

        return std::error_code();
    }

    std::error_code WalletService::sendFusionTransaction(uint64_t threshold, uint32_t anonymity, const std::vector<std::string> &addresses,
                                                         const std::string &destinationAddress, std::string &transactionHash)
    {

        try
        {
            syst::EventLock lk(readyEvent);

            validateAddresses(addresses, currency, logger);
            if (!destinationAddress.empty())
            {
                validateAddresses({destinationAddress}, currency, logger);
            }

            size_t transactionId = fusionManager.createFusionTransaction(threshold, anonymity, addresses, destinationAddress);
            transactionHash = common::podToHex(wallet.getTransaction(transactionId).hash);

            logger(logging::DEBUGGING) << "Fusion transaction " << transactionHash << " has been sent";
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while sending fusion transaction: " << x.what();
            return x.code();
        }
        catch (std::exception &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while sending fusion transaction: " << x.what();
            return make_error_code(mevacoin::error::INTERNAL_WALLET_ERROR);
        }

        return std::error_code();
    }

    std::error_code WalletService::estimateFusion(uint64_t threshold, const std::vector<std::string> &addresses,
                                                  uint32_t &fusionReadyCount, uint32_t &totalOutputCount)
    {

        try
        {
            syst::EventLock lk(readyEvent);

            validateAddresses(addresses, currency, logger);

            auto estimateResult = fusionManager.estimate(threshold, addresses);
            fusionReadyCount = static_cast<uint32_t>(estimateResult.fusionReadyCount);
            totalOutputCount = static_cast<uint32_t>(estimateResult.totalOutputCount);
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Failed to estimate number of fusion outputs: " << x.what();
            return x.code();
        }
        catch (std::exception &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Failed to estimate number of fusion outputs: " << x.what();
            return make_error_code(mevacoin::error::INTERNAL_WALLET_ERROR);
        }

        return std::error_code();
    }

    std::error_code WalletService::createIntegratedAddress(const std::string &address, const std::string &paymentId, std::string &integratedAddress)
    {
        try
        {
            syst::EventLock lk(readyEvent);

            validateAddresses({address}, currency, logger);
            validatePaymentId(paymentId, logger);
        }
        catch (std::system_error &x)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "Error while creating integrated address: " << x.what();
            return x.code();
        }

        uint64_t prefix;

        mevacoin::AccountPublicAddress addr;

        /* Get the private + public key from the address */
        mevacoin::parseAccountAddressString(prefix, addr, address);

        /* Pack as a binary array */
        mevacoin::BinaryArray ba;
        mevacoin::toBinaryArray(addr, ba);
        std::string keys = common::asString(ba);

        /* Encode prefix + paymentID + keys as an address */
        integratedAddress = tools::base58::encode_addr(
            mevacoin::parameters::MEVACOIN_PUBLIC_ADDRESS_BASE58_PREFIX,
            paymentId + keys);

        return std::error_code();
    }

    std::error_code WalletService::getFeeInfo(std::string &address, uint32_t &amount)
    {
        address = m_node_address;
        amount = m_node_fee;

        return std::error_code();
    }

    uint64_t WalletService::getDefaultMixin() const
    {
        return mevacoin::getDefaultMixinByHeight(node.getLastKnownBlockHeight());
    }

    void WalletService::refresh()
    {
        try
        {
            logger(logging::DEBUGGING) << "Refresh is started";
            for (;;)
            {
                auto event = wallet.getEvent();
                if (event.type == mevacoin::TRANSACTION_CREATED)
                {
                    size_t transactionId = event.transactionCreated.transactionIndex;
                    transactionIdIndex.emplace(common::podToHex(wallet.getTransaction(transactionId).hash), transactionId);
                }
            }
        }
        catch (std::system_error &e)
        {
            logger(logging::DEBUGGING) << "refresh is stopped: " << e.what();
        }
        catch (std::exception &e)
        {
            logger(logging::WARNING, logging::BRIGHT_YELLOW) << "exception thrown in refresh(): " << e.what();
        }
    }

    void WalletService::reset(const uint64_t scanHeight)
    {
        wallet.reset(scanHeight);
    }

    std::vector<mevacoin::TransactionsInBlockInfo> WalletService::getTransactions(const crypto::Hash &blockHash, size_t blockCount) const
    {
        std::vector<mevacoin::TransactionsInBlockInfo> result = wallet.getTransactions(blockHash, blockCount);
        if (result.empty())
        {
            throw std::system_error(make_error_code(mevacoin::error::WalletServiceErrorCode::OBJECT_NOT_FOUND));
        }

        return result;
    }

    std::vector<mevacoin::TransactionsInBlockInfo> WalletService::getTransactions(uint32_t firstBlockIndex, size_t blockCount) const
    {
        std::vector<mevacoin::TransactionsInBlockInfo> result = wallet.getTransactions(firstBlockIndex, blockCount);
        if (result.empty())
        {
            throw std::system_error(make_error_code(mevacoin::error::WalletServiceErrorCode::OBJECT_NOT_FOUND));
        }

        return result;
    }

    std::vector<TransactionHashesInBlockRpcInfo> WalletService::getRpcTransactionHashes(const crypto::Hash &blockHash, size_t blockCount, const TransactionsInBlockInfoFilter &filter) const
    {
        std::vector<mevacoin::TransactionsInBlockInfo> allTransactions = getTransactions(blockHash, blockCount);
        std::vector<mevacoin::TransactionsInBlockInfo> filteredTransactions = filterTransactions(allTransactions, filter);
        return convertTransactionsInBlockInfoToTransactionHashesInBlockRpcInfo(filteredTransactions);
    }

    std::vector<TransactionHashesInBlockRpcInfo> WalletService::getRpcTransactionHashes(uint32_t firstBlockIndex, size_t blockCount, const TransactionsInBlockInfoFilter &filter) const
    {
        std::vector<mevacoin::TransactionsInBlockInfo> allTransactions = getTransactions(firstBlockIndex, blockCount);
        std::vector<mevacoin::TransactionsInBlockInfo> filteredTransactions = filterTransactions(allTransactions, filter);
        return convertTransactionsInBlockInfoToTransactionHashesInBlockRpcInfo(filteredTransactions);
    }

    std::vector<TransactionsInBlockRpcInfo> WalletService::getRpcTransactions(const crypto::Hash &blockHash, size_t blockCount, const TransactionsInBlockInfoFilter &filter) const
    {
        std::vector<mevacoin::TransactionsInBlockInfo> allTransactions = getTransactions(blockHash, blockCount);
        std::vector<mevacoin::TransactionsInBlockInfo> filteredTransactions = filterTransactions(allTransactions, filter);
        return convertTransactionsInBlockInfoToTransactionsInBlockRpcInfo(filteredTransactions);
    }

    std::vector<TransactionsInBlockRpcInfo> WalletService::getRpcTransactions(uint32_t firstBlockIndex, size_t blockCount, const TransactionsInBlockInfoFilter &filter) const
    {
        std::vector<mevacoin::TransactionsInBlockInfo> allTransactions = getTransactions(firstBlockIndex, blockCount);
        std::vector<mevacoin::TransactionsInBlockInfo> filteredTransactions = filterTransactions(allTransactions, filter);
        return convertTransactionsInBlockInfoToTransactionsInBlockRpcInfo(filteredTransactions);
    }

    std::error_code WalletService::validateAddress(const std::string &address, bool &isValid)
    {
        isValid = mevacoin::validateAddress(address, currency);
        return std::error_code();  // Return success
    }

} // namespace payment_service
