// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#include "mevacoin_core/mevacoin_basic.h"
#include "itransaction.h"

namespace mevacoin
{

    bool checkInputsKeyimagesDiff(const mevacoin::TransactionPrefix &tx);

    // TransactionInput helper functions
    size_t getRequiredSignaturesCount(const TransactionInput &in);
    uint64_t getTransactionInputAmount(const TransactionInput &in);
    TransactionTypes::InputType getTransactionInputType(const TransactionInput &in);
    const TransactionInput &getInputChecked(const mevacoin::TransactionPrefix &transaction, size_t index);
    const TransactionInput &getInputChecked(const mevacoin::TransactionPrefix &transaction, size_t index, TransactionTypes::InputType type);

    // TransactionOutput helper functions
    TransactionTypes::OutputType getTransactionOutputType(const TransactionOutputTarget &out);
    const TransactionOutput &getOutputChecked(const mevacoin::TransactionPrefix &transaction, size_t index);
    const TransactionOutput &getOutputChecked(const mevacoin::TransactionPrefix &transaction, size_t index, TransactionTypes::OutputType type);

    bool findOutputsToAccount(const mevacoin::TransactionPrefix &transaction, const AccountPublicAddress &addr,
                              const crypto::SecretKey &viewSecretKey, std::vector<uint32_t> &out, uint64_t &amount);

} // namespace mevacoin
