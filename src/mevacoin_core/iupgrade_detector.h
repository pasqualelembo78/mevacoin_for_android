// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#pragma once
#include <cstdint>
#include <mevacoin_core/currency.h>

namespace mevacoin
{

    class IUpgradeDetector
    {
    public:
        enum : uint32_t
        {
            UNDEF_HEIGHT = static_cast<uint32_t>(-1)
        };

        virtual uint8_t targetVersion() const = 0;
        virtual uint32_t upgradeIndex() const = 0;
        virtual ~IUpgradeDetector() {}
    };

    std::unique_ptr<IUpgradeDetector> makeUpgradeDetector(uint8_t targetVersion, uint32_t upgradeIndex);

}
