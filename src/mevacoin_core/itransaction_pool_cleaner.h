// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#pragma once

#include <vector>

#include "mevacoin_core/itransaction_pool.h"

namespace crypto
{
    struct Hash;
}

namespace mevacoin
{

    class ITransactionPoolCleanWrapper : public ITransactionPool
    {
    public:
        virtual ~ITransactionPoolCleanWrapper() {}

        virtual std::vector<crypto::Hash> clean(const uint32_t height) = 0;
    };

} // namespace mevacoin
