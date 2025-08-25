// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstddef>

namespace mevacoin
{

    class ICryptoNoteProtocolObserver
    {
    public:
        virtual void peerCountUpdated(size_t count) {}
        virtual void lastKnownBlockHeightUpdated(uint32_t height) {}
        virtual void blockchainSynchronized(uint32_t topHeight) {}
    };

} // namespace mevacoin
