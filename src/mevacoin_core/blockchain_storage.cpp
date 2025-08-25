// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#include "blockchain_storage.h"

#include "memory_blockchain_storage.h"
#include "swapped_blockchain_storage.h"

using namespace mevacoin;

BlockchainStorage::BlockchainStorage(uint32_t reserveSize) : internalStorage(new MemoryBlockchainStorage(reserveSize))
{
}

BlockchainStorage::BlockchainStorage(const std::string &indexFileName, const std::string &dataFileName) : internalStorage(new SwappedBlockchainStorage(indexFileName, dataFileName))
{
}

BlockchainStorage::BlockchainStorage(std::unique_ptr<IBlockchainStorageInternal> storage) : internalStorage(std::move(storage))
{
}

BlockchainStorage::~BlockchainStorage()
{
}

void BlockchainStorage::pushBlock(RawBlock &&rawBlock)
{
    internalStorage->pushBlock(std::move(rawBlock));
}

RawBlock BlockchainStorage::getBlockByIndex(uint32_t index) const
{
    return internalStorage->getBlockByIndex(index);
}

uint32_t BlockchainStorage::getBlockCount() const
{
    return internalStorage->getBlockCount();
}

// Returns MemoryBlockchainStorage with elements from [splitIndex, blocks.size() - 1].
// Original MemoryBlockchainStorage will contain elements from [0, splitIndex - 1].
std::unique_ptr<BlockchainStorage> BlockchainStorage::splitStorage(uint32_t splitIndex)
{
    std::unique_ptr<BlockchainStorage> newStorage(new BlockchainStorage(internalStorage->splitStorage(splitIndex)));
    return newStorage;
}
