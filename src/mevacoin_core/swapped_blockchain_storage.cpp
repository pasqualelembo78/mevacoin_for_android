// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#include "swapped_blockchain_storage.h"

#include <cassert>

#include "mevacoin_core/mevacoin_serialization.h"
#include "icore_definitions.h"
#include "memory_blockchain_storage.h"
#include "serialization/serialization_overloads.h"

namespace mevacoin
{

    SwappedBlockchainStorage::SwappedBlockchainStorage(const std::string &indexFileName, const std::string &dataFileName)
    {
        if (!blocks.open(dataFileName, indexFileName, 1024))
        {
            throw std::runtime_error("Can't open blockchain storage files.");
        }
    }

    SwappedBlockchainStorage::~SwappedBlockchainStorage()
    {
        blocks.close();
    }

    void SwappedBlockchainStorage::pushBlock(RawBlock &&rawBlock)
    {
        blocks.push_back(rawBlock);
    }

    RawBlock SwappedBlockchainStorage::getBlockByIndex(uint32_t index) const
    {
        assert(index < getBlockCount());
        return blocks[index];
    }

    uint32_t SwappedBlockchainStorage::getBlockCount() const
    {
        return static_cast<uint32_t>(blocks.size());
    }

    // Returns MemoryBlockchainStorage with elements from [splitIndex, blocks.size() - 1].
    // Original SwappedBlockchainStorage will contain elements from [0, splitIndex - 1].
    std::unique_ptr<BlockchainStorage::IBlockchainStorageInternal> SwappedBlockchainStorage::splitStorage(uint32_t splitIndex)
    {
        assert(splitIndex > 0);
        assert(splitIndex < blocks.size());
        std::unique_ptr<MemoryBlockchainStorage> newStorage = std::unique_ptr<MemoryBlockchainStorage>(new MemoryBlockchainStorage(splitIndex));

        uint64_t blocksCount = blocks.size();

        for (uint64_t i = splitIndex; i < blocksCount; ++i)
        {
            newStorage->pushBlock(RawBlock(blocks[i]));
        }

        for (uint64_t i = 0; i < blocksCount - splitIndex; ++i)
        {
            blocks.pop_back();
        }

        return std::move(newStorage);
    }

}
