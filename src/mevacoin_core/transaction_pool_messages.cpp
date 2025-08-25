// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#include <mevacoin_core/transaction_pool_messages.h>

namespace mevacoin
{

    TransactionPoolMessage::TransactionPoolMessage(const AddTransaction &at)
        : type(TransactionMessageType::AddTransactionType), addTransaction(at)
    {
    }

    TransactionPoolMessage::TransactionPoolMessage(const DeleteTransaction &dt)
        : type(TransactionMessageType::DeleteTransactionType), deleteTransaction(dt)
    {
    }

    // pattern match
    void TransactionPoolMessage::match(std::function<void(const AddTransaction &)> &&addTxVisitor,
                                       std::function<void(const DeleteTransaction &)> &&delTxVisitor)
    {
        switch (getType())
        {
        case TransactionMessageType::AddTransactionType:
            addTxVisitor(addTransaction);
            break;
        case TransactionMessageType::DeleteTransactionType:
            delTxVisitor(deleteTransaction);
            break;
        }
    }

    // API with explicit type handling
    TransactionMessageType TransactionPoolMessage::getType() const
    {
        return type;
    }

    AddTransaction TransactionPoolMessage::getAddTransaction() const
    {
        assert(getType() == TransactionMessageType::AddTransactionType);
        return addTransaction;
    }

    DeleteTransaction TransactionPoolMessage::getDeleteTransaction() const
    {
        assert(getType() == TransactionMessageType::DeleteTransactionType);
        return deleteTransaction;
    }

}
