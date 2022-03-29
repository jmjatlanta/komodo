#include "primitives/block.h"
#include "testutils.h"
#include "komodo_extern_globals.h"
#include "consensus/validation.h"
#include "miner.h"
#include "txdb.h"
#include <gtest/gtest.h>

#include <boost/filesystem.hpp>

TEST(BlockTest, header_size_is_expected) {
    // Header with an empty Equihash solution.
    CBlockHeader header;
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << header;

    auto stream_size = CBlockHeader::HEADER_SIZE + 1;
    // ss.size is +1 due to data stream header of 1 byte
    EXPECT_EQ(ss.size(), stream_size);
}

TEST(BlockTest, TestStopAt)
{
    TestChain chain;
    auto notary = chain.AddWallet(chain.getNotaryKey());
    std::shared_ptr<CBlock> lastBlock = chain.generateBlock(); // genesis block
    ASSERT_GT( chain.GetIndex()->GetHeight(), 0 );
    lastBlock = chain.generateBlock(); // now we should be above 1
    ASSERT_GT( chain.GetIndex()->GetHeight(), 1);
    CBlock block;
    CValidationState state;
    KOMODO_STOPAT = 1;
    EXPECT_FALSE( ConnectBlock(block, state, chain.GetIndex(), *chain.GetCoinsViewCache(), false, true) );
    KOMODO_STOPAT = 0; // to not stop other tests
}

TEST(BlockTest, TestConnectWithoutChecks)
{
    TestChain chain;
    auto notary = chain.AddWallet(chain.getNotaryKey());
    auto alice = chain.AddWallet();
    std::shared_ptr<CBlock> lastBlock = chain.generateBlock(alice); // genesis block
    ASSERT_GT( chain.GetIndex()->GetHeight(), 0 );
    // let funds mature
    lastBlock = chain.generateBlock(alice);
    // Add some transaction to a block
    int32_t newHeight = chain.GetIndex()->GetHeight() + 1;
    CTransaction fundAlice = alice->CreateSpendTransaction(notary, 100000);
    // construct the block
    CBlock block;
    // first a coinbase tx
    auto consensusParams = Params().GetConsensus();
    CMutableTransaction txNew = CreateNewContextualCMutableTransaction(consensusParams, newHeight);
    txNew.vin.resize(1);
    txNew.vin[0].prevout.SetNull();
    txNew.vin[0].scriptSig = (CScript() << newHeight << CScriptNum(1)) + COINBASE_FLAGS;
    txNew.vout.resize(1);
    txNew.vout[0].nValue = GetBlockSubsidy(newHeight,consensusParams);
    txNew.nExpiryHeight = 0;
    block.vtx.push_back(CTransaction(txNew));
    // then the actual tx
    block.vtx.push_back(fundAlice);
    CValidationState state;
    // create a new CBlockIndex to forward to ConnectBlock
    auto view = chain.GetCoinsViewCache();
    auto index = chain.GetIndex();
    CBlockIndex newIndex;
    newIndex.pprev = index;
    EXPECT_TRUE( ConnectBlock(block, state, &newIndex, *chain.GetCoinsViewCache(), true, false) );
    if (!state.IsValid() )
        FAIL() << state.GetRejectReason();
}

TEST(BlockTest, TestSpendInSameBlock)
{
    TestChain chain;
    auto notary = chain.AddWallet(chain.getNotaryKey());
    auto alice = chain.AddWallet();
    auto bob = chain.AddWallet();
    std::shared_ptr<CBlock> lastBlock = chain.generateBlock(alice); // genesis block
    ASSERT_GT( chain.GetIndex()->GetHeight(), 0 );
    // let coinbase mature
    lastBlock = chain.generateBlock(alice); // genesis block
    // Start to build a block
    int32_t newHeight = chain.GetIndex()->GetHeight() + 1;
    CTransaction fundAlice = alice->CreateSpendTransaction(notary, 100000);
    // now have Notary move some funds to Bob in the same block
    CMutableTransaction notaryToBobMutable;
    CTxIn notaryIn;
    notaryIn.prevout.hash = fundAlice.GetHash();
    notaryIn.prevout.n = 0;
    notaryToBobMutable.vin.push_back(notaryIn);
    CTxOut bobOut;
    bobOut.scriptPubKey = GetScriptForDestination(bob->GetPubKey());
    bobOut.nValue = 10000;
    notaryToBobMutable.vout.push_back(bobOut);
    CTxOut notaryRemainder;
    notaryRemainder.scriptPubKey = GetScriptForDestination(notary->GetPubKey());
    notaryRemainder.nValue = fundAlice.vout[0].nValue - 10000;
    notaryToBobMutable.vout.push_back(notaryRemainder);
    uint256 hash = SignatureHash(fundAlice.vout[0].scriptPubKey, notaryToBobMutable, 0, SIGHASH_ALL, 0, 0);
    notaryToBobMutable.vin[0].scriptSig << alice->Sign(hash, SIGHASH_ALL);
    CTransaction notaryToBobTx(notaryToBobMutable);
    // construct the block
    CBlock block;
    // first a coinbase tx
    auto consensusParams = Params().GetConsensus();
    CMutableTransaction txNew = CreateNewContextualCMutableTransaction(consensusParams, newHeight);
    txNew.vin.resize(1);
    txNew.vin[0].prevout.SetNull();
    txNew.vin[0].scriptSig = (CScript() << newHeight << CScriptNum(1)) + COINBASE_FLAGS;
    txNew.vout.resize(1);
    txNew.vout[0].nValue = GetBlockSubsidy(newHeight,consensusParams);
    txNew.nExpiryHeight = 0;
    block.vtx.push_back(CTransaction(txNew));
    // then the actual txs
    block.vtx.push_back(fundAlice);
    block.vtx.push_back(notaryToBobTx);
    CValidationState state;
    // create a new CBlockIndex to forward to ConnectBlock
    auto index = chain.GetIndex();
    CBlockIndex newIndex;
    newIndex.pprev = index;
    EXPECT_TRUE( ConnectBlock(block, state, &newIndex, *chain.GetCoinsViewCache(), true, false) );
    if (!state.IsValid() )
        FAIL() << state.GetRejectReason();
}

TEST(BlockTest, TestDoubleSpendInSameBlock)
{
    TestChain chain;
    auto notary = chain.AddWallet(chain.getNotaryKey());
    auto alice = chain.AddWallet();
    auto bob = chain.AddWallet();
    auto charlie = chain.AddWallet();
    std::shared_ptr<CBlock> lastBlock = chain.generateBlock(alice); // genesis block
    ASSERT_GT( chain.GetIndex()->GetHeight(), 0 );
    // let coinbase mature
    lastBlock = chain.generateBlock(alice); // genesis block
    // Start to build a block
    int32_t newHeight = chain.GetIndex()->GetHeight() + 1;
    CTransaction fundNotary = alice->CreateSpendTransaction(notary, 100000);
    // now have Alice move some funds to Bob in the same block
    CMutableTransaction aliceToBobMutable;
    CTxIn aliceIn;
    aliceIn.prevout.hash = fundNotary.GetHash();
    aliceIn.prevout.n = 0;
    aliceToBobMutable.vin.push_back(aliceIn);
    CTxOut bobOut;
    bobOut.scriptPubKey = GetScriptForDestination(bob->GetPubKey());
    bobOut.nValue = 10000;
    aliceToBobMutable.vout.push_back(bobOut);
    CTxOut aliceRemainder;
    aliceRemainder.scriptPubKey = GetScriptForDestination(alice->GetPubKey());
    aliceRemainder.nValue = fundNotary.vout[0].nValue - 10000;
    aliceToBobMutable.vout.push_back(aliceRemainder);
    uint256 hash = SignatureHash(fundNotary.vout[0].scriptPubKey, aliceToBobMutable, 0, SIGHASH_ALL, 0, 0);
    aliceToBobMutable.vin[0].scriptSig << alice->Sign(hash, SIGHASH_ALL);
    CTransaction aliceToBobTx(aliceToBobMutable);
    // alice attempts to double spend the vout and send something to charlie
    CMutableTransaction aliceToCharlieMutable;
    CTxIn aliceIn2;
    aliceIn2.prevout.hash = fundNotary.GetHash();
    aliceIn2.prevout.n = 0;
    aliceToCharlieMutable.vin.push_back(aliceIn2);
    CTxOut charlieOut;
    charlieOut.scriptPubKey = GetScriptForDestination(charlie->GetPubKey());
    charlieOut.nValue = 10000;
    aliceToCharlieMutable.vout.push_back(charlieOut);
    CTxOut aliceRemainder2;
    aliceRemainder2.scriptPubKey = GetScriptForDestination(alice->GetPubKey());
    aliceRemainder2.nValue = fundNotary.vout[0].nValue - 10000;
    aliceToCharlieMutable.vout.push_back(aliceRemainder2);
    hash = SignatureHash(fundNotary.vout[0].scriptPubKey, aliceToCharlieMutable, 0, SIGHASH_ALL, 0, 0);
    aliceToCharlieMutable.vin[0].scriptSig << alice->Sign(hash, SIGHASH_ALL);
    CTransaction aliceToCharlieTx(aliceToCharlieMutable);
    // construct the block
    CBlock block;
    // first a coinbase tx
    auto consensusParams = Params().GetConsensus();
    CMutableTransaction txNew = CreateNewContextualCMutableTransaction(consensusParams, newHeight);
    txNew.vin.resize(1);
    txNew.vin[0].prevout.SetNull();
    txNew.vin[0].scriptSig = (CScript() << newHeight << CScriptNum(1)) + COINBASE_FLAGS;
    txNew.vout.resize(1);
    txNew.vout[0].nValue = GetBlockSubsidy(newHeight,consensusParams);
    txNew.nExpiryHeight = 0;
    block.vtx.push_back(CTransaction(txNew));
    // then the actual txs
    block.vtx.push_back(fundNotary);
    block.vtx.push_back(aliceToBobTx);
    block.vtx.push_back(aliceToCharlieTx);
    CValidationState state;
    // create a new CBlockIndex to forward to ConnectBlock
    auto index = chain.GetIndex();
    CBlockIndex newIndex;
    newIndex.pprev = index;
    EXPECT_FALSE( ConnectBlock(block, state, &newIndex, *chain.GetCoinsViewCache(), true, false) );
    EXPECT_EQ(state.GetRejectReason(), "bad-txns-inputs-missingorspent");
}

bool CalcPoW(CBlock *pblock);

TEST(BlockTest, TestProcessBlock)
{
    TestChain chain;
    EXPECT_EQ(chain.GetIndex()->GetHeight(), 0);
    auto notary = chain.AddWallet(chain.getNotaryKey());
    auto alice = chain.AddWallet();
    auto bob = chain.AddWallet();
    auto charlie = chain.AddWallet();
    std::shared_ptr<CBlock> lastBlock = chain.generateBlock(alice); // gives alice everything
    // let coinbase mature
    lastBlock = chain.generateBlock(alice);
    EXPECT_EQ(chain.GetIndex()->GetHeight(), 2);
    chain.IncrementChainTime();
    // add a transaction to the mempool
    CTransaction fundAlice = alice->CreateSpendTransaction(bob, 100000);
    EXPECT_TRUE( chain.acceptTx(fundAlice).IsValid() );
    // construct the block
    CBlock block;
    int32_t newHeight = chain.GetIndex()->GetHeight() + 1;
    CValidationState state;
    // no transactions
    EXPECT_FALSE( ProcessNewBlock(false, newHeight, state, nullptr, &block, false, nullptr) );
    EXPECT_EQ(state.GetRejectReason(), "bad-blk-length");
    EXPECT_EQ(chain.GetIndex()->GetHeight(), 2);
    // add first a coinbase tx
    auto consensusParams = Params().GetConsensus();
    CMutableTransaction txNew = CreateNewContextualCMutableTransaction(consensusParams, newHeight);
    txNew.vin.resize(1);
    txNew.vin[0].prevout.SetNull();
    txNew.vin[0].scriptSig = (CScript() << newHeight << CScriptNum(1)) + COINBASE_FLAGS;
    txNew.vout.resize(1);
    txNew.vout[0].nValue = GetBlockSubsidy(newHeight,consensusParams);
    txNew.nExpiryHeight = 0;
    block.vtx.push_back(CTransaction(txNew));
    // no PoW, no merkle root should fail on merkle error
    EXPECT_FALSE( ProcessNewBlock(false, newHeight, state, nullptr, &block, false, nullptr) );
    EXPECT_EQ(state.GetRejectReason(), "bad-txnmrklroot");
    // Verify transaction is still in mempool
    EXPECT_EQ(mempool.size(), 1);
    // finish constructing the block
    block.nBits = GetNextWorkRequired( chain.GetIndex(), &block, Params().GetConsensus());
    block.nTime = GetTime();
    block.hashPrevBlock = lastBlock->GetHash();
    block.hashMerkleRoot = block.BuildMerkleTree();
    // Add the PoW
    EXPECT_TRUE(CalcPoW(&block));
    state = CValidationState();
    EXPECT_TRUE( ProcessNewBlock(false, newHeight, state, nullptr, &block, false, nullptr) );
    if (!state.IsValid())
        FAIL() << state.GetRejectReason();
    // Verify transaction is still in mempool
    EXPECT_EQ(mempool.size(), 1);
}

TEST(BlockTest, TestProcessBadBlock)
{
    TestChain chain;
    auto notary = chain.AddWallet(chain.getNotaryKey());
    auto alice = chain.AddWallet();
    auto bob = chain.AddWallet();
    auto charlie = chain.AddWallet();
    std::shared_ptr<CBlock> lastBlock = chain.generateBlock(alice); // genesis block
    // let coinbase mature
    lastBlock = chain.generateBlock(alice);
    // add a transaction to the mempool
    CTransaction fundAlice = alice->CreateSpendTransaction(notary, 100000);
    EXPECT_TRUE( chain.acceptTx(fundAlice).IsValid() );
    // construct the block
    CBlock block;
    int32_t newHeight = chain.GetIndex()->GetHeight() + 1;
    CValidationState state;
    // no transactions
    EXPECT_FALSE( ProcessNewBlock(false, newHeight, state, nullptr, &block, false, nullptr) );
    EXPECT_EQ(state.GetRejectReason(), "bad-blk-length");
    // add first a coinbase tx
    auto consensusParams = Params().GetConsensus();
    CMutableTransaction txNew = CreateNewContextualCMutableTransaction(consensusParams, newHeight);
    txNew.vin.resize(1);
    txNew.vin[0].prevout.SetNull();
    txNew.vin[0].scriptSig = (CScript() << newHeight << CScriptNum(1)) + COINBASE_FLAGS;
    txNew.vout.resize(1);
    txNew.vout[0].nValue = GetBlockSubsidy(newHeight,consensusParams);
    txNew.nExpiryHeight = 0;
    block.vtx.push_back(CTransaction(txNew));
    // Add no PoW, should fail on merkle error
    EXPECT_FALSE( ProcessNewBlock(false, newHeight, state, nullptr, &block, false, nullptr) );
    EXPECT_EQ(state.GetRejectReason(), "bad-txnmrklroot");
    // Verify transaction is still in mempool
    EXPECT_EQ(mempool.size(), 1);
}

void komodo_init(int32_t height); // in komodo_bitcoind.cpp

class PersistedTestChain : public TestChain
{
public:
    PersistedTestChain() : TestChain(CBaseChainParams::REGTEST, "", false)
    {
        removeDataOnDestruction = false;
    }
    PersistedTestChain(boost::filesystem::path data_dir, bool reindex = false) 
        : TestChain(CBaseChainParams::REGTEST, data_dir, false, reindex)
    {
        removeDataOnDestruction = false;
    }
    boost::filesystem::path GetDataDir() { return dataDir; }
    void RemoveOnDestruction(bool in) { removeDataOnDestruction = in; }
};

TEST(BlockTest, CorruptBlockFile)
{
    /****
     * in main.h set the sizes to something small to prevent this from running a VERY
     * long time. Something like:
     *  static const unsigned int MAX_BLOCKFILE_SIZE = 0x2000;
     *  static const unsigned int MAX_TEMPFILE_SIZE =  0x2000;
     *  static const unsigned int BLOCKFILE_CHUNK_SIZE = 0x2000;
     *  static const unsigned int UNDOFILE_CHUNK_SIZE = 0x2000;
     */
    boost::filesystem::path dataPath;
    CKey aliceKey;
    int32_t currentHeight = 0;
    {
        PersistedTestChain chain;
        dataPath = chain.GetDataDir();
        auto notary = chain.AddWallet(chain.getNotaryKey(), "notary");
        auto alice = chain.AddWallet("alice");
        aliceKey = alice->GetPrivKey();
        std::shared_ptr<CBlock> lastBlock = chain.generateBlock(alice); // genesis block
        auto idx = chain.GetIndex();
        int blockFile = idx->GetBlockPos().nFile;
        // fill block00000.dat  
        while (blockFile == idx->GetBlockPos().nFile)
        {
            lastBlock = chain.generateBlock(alice);
            idx = chain.GetIndex();
            currentHeight = idx->GetHeight();
            std::cout << "Mined block " << std::to_string(currentHeight) << "\n";
        }
        // fill block00001.dat
        blockFile = idx->GetBlockPos().nFile;
        while (blockFile == idx->GetBlockPos().nFile)
        {
            lastBlock = chain.generateBlock(alice);
            idx = chain.GetIndex();
            currentHeight = idx->GetHeight();
            std::cout << "Mined block " << std::to_string(currentHeight) << "\n";
        }
    }
    // screw up the 1st file (file 0)
    {
        auto inFile = dataPath / "regtest" / "blocks" / "blk00000.dat";
        boost::filesystem::resize_file( inFile, file_size(inFile) - 100 );
    }
    // attempt to read the database
    {
        PersistedTestChain chain(dataPath);
        /**
         * corruption in the blkxxxxx.dat causes an exception that was eaten by
         * txdb.cpp->txdb.cpp->LoadBlockIndexGuts->pcursor->GetKey, try/catch
         * within the GetKey(). The chain boots.
         */
        // we are at the correct height, as the index is okay.
        auto idx = chain.GetIndex();
        EXPECT_TRUE( idx != nullptr);
        EXPECT_EQ(idx->GetHeight(), currentHeight);
        // Attempting to retrieve any block fails.
        for(int i = 1; i <= currentHeight; ++i)
        {
            idx = chain.GetIndex(i);
            EXPECT_TRUE(idx != nullptr);
            auto blk = chain.GetBlock(idx);
            EXPECT_TRUE(blk != nullptr);
        }
    }
}

boost::filesystem::path find_file(const boost::filesystem::path& dir, const std::string& extension)
{
    boost::filesystem::path retval;
    for( auto f : boost::filesystem::directory_iterator(dir) )
    {
        if (f.path().extension() == extension)
            retval = f.path();
    }
    return retval;
}

boost::filesystem::path index_path(const boost::filesystem::path& dataPath)
{
    return dataPath / "regtest" / "blocks" / "index";
}

TEST(BlockTest, CorruptIndexFile)
{
    /****
     * in main.h set the sizes to something small to prevent this from running a VERY
     * long time. Something like:
     *  static const unsigned int MAX_BLOCKFILE_SIZE = 0x2000;
     *  static const unsigned int MAX_TEMPFILE_SIZE =  0x2000;
     *  static const unsigned int BLOCKFILE_CHUNK_SIZE = 0x2000;
     *  static const unsigned int UNDOFILE_CHUNK_SIZE = 0x2000;
     */
    boost::filesystem::path dataPath;
    CKey aliceKey;
    int32_t currentHeight = 0;
    {
        PersistedTestChain chain;
        dataPath = chain.GetDataDir();
        auto notary = chain.AddWallet(chain.getNotaryKey(), "notary");
        auto alice = chain.AddWallet("alice");
        aliceKey = alice->GetPrivKey();
        std::shared_ptr<CBlock> lastBlock = chain.generateBlock(alice); // genesis block
        auto idx = chain.GetIndex();
        int blockFile = idx->GetBlockPos().nFile;
        // fill block00000.dat  
        while (blockFile == idx->GetBlockPos().nFile)
        {
            lastBlock = chain.generateBlock(alice);
            idx = chain.GetIndex();
            currentHeight = idx->GetHeight();
            std::cout << "Mined block " << std::to_string(currentHeight) << "\n";
        }
    }
    // screw up the ldb file
    {
        boost::filesystem::path ldb = find_file(index_path(dataPath), ".log");
        ASSERT_TRUE(boost::filesystem::exists(ldb));
        // chop 3 bytes off the end of the file
        boost::filesystem::resize_file( ldb, file_size(ldb) - 3 );
    }
    // attempt to read the database
    {
        EXPECT_THROW(PersistedTestChain chain(dataPath), std::logic_error);
        /**
         * corruption in the index/xxxxxx.log causes an exception that was eaten by
         * txdb.cpp->txdb.cpp->LoadBlockIndexGuts->pcursor->GetKey, the chain started
         * but with 0 blocks. Now it throws. Try to recover.
         */
        PersistedTestChain chain(dataPath, true);
        chain.RemoveOnDestruction(true); // last one
        // we should have the right number of blocks
        EXPECT_TRUE( chain.GetIndex() != nullptr );
        EXPECT_EQ(chain.GetIndex()->GetHeight(), currentHeight);
    }
}

TEST(BlockTest, MissingIndexEntry)
{
    /****
     * in main.h set the sizes to something small to prevent this from running a VERY
     * long time. Something like:
     *  static const unsigned int MAX_BLOCKFILE_SIZE = 0x2000;
     *  static const unsigned int MAX_TEMPFILE_SIZE =  0x2000;
     *  static const unsigned int BLOCKFILE_CHUNK_SIZE = 0x2000;
     *  static const unsigned int UNDOFILE_CHUNK_SIZE = 0x2000;
     */
    boost::filesystem::path dataPath;
    int32_t currentHeight = 0;
    boost::filesystem::path copiedLogFile;
    uint256 firstBlockHash;
    {
        PersistedTestChain chain;
        dataPath = chain.GetDataDir();
        auto notary = chain.AddWallet(chain.getNotaryKey(), "notary");
        auto alice = chain.AddWallet("alice");
        auto lastBlock = chain.generateBlock(alice); // genesis block
        currentHeight = chain.GetIndex()->GetHeight();
        firstBlockHash = lastBlock->GetHash();
    }
    // save off a copy of the .log file
    {
        // find the ldb file
        boost::filesystem::path ldb = find_file(index_path(dataPath), ".log");
        ASSERT_TRUE(boost::filesystem::exists(ldb));
        copiedLogFile = dataPath / "regtest" / "blocks" / ldb.filename();
        boost::filesystem::copy(ldb, copiedLogFile);
    }
    // store info about the last block to be created
    uint256 lastBlockHash;
    uint256 lastTransactionHash;
    // create the last block
    {
        PersistedTestChain chain(dataPath);
        auto notary = chain.AddWallet(chain.getNotaryKey(), "notary");
        auto alice = chain.AddWallet("alice");
        auto lastBlock = chain.generateBlock(alice);
        lastBlockHash = lastBlock->GetHash();
        lastTransactionHash = lastBlock->vtx[lastBlock->vtx.size()-1].GetHash();
        EXPECT_EQ( chain.GetIndex()->GetHeight(), currentHeight + 1);
        currentHeight = chain.GetIndex()->GetHeight();
    }
    // replace the .log file with the one we copied
    {
        // find the ldb file
        boost::filesystem::path ldb = find_file(index_path(dataPath), ".log");
        ASSERT_TRUE(boost::filesystem::exists(ldb) );
        boost::filesystem::copy_file(copiedLogFile, ldb, boost::filesystem::copy_option::overwrite_if_exists);
    }
    // attempt to read the database
    {
        /**
         * The index has a missing entry, but the chain starts fine.
         * The chain has the correct number of blocks. But looking
         * it up by chain height leads to nullptr
         */
        PersistedTestChain chain(dataPath, true);
        // we should have the right number of blocks
        EXPECT_TRUE( chain.GetIndex() != nullptr );
        EXPECT_EQ(chain.GetIndex()->GetHeight(), currentHeight);
        // we should be able to look up the block
        auto lastBlock = chain.GetBlock( chain.GetIndex(currentHeight) );
        EXPECT_EQ( lastBlock, nullptr ); // <-- a nullptr due to missing index entry
        // what happens if I attempt to mine a block and add it to the chain?
        auto notary = chain.AddWallet(chain.getNotaryKey(), "notary");
        auto alice = chain.AddWallet("alice");
        lastBlock = chain.generateBlock(alice);
        EXPECT_EQ( chain.GetIndex()->GetHeight(), currentHeight + 1);
        currentHeight = chain.GetIndex()->GetHeight();
        // attempt some lookups of the new block
        auto idx = chain.GetIndex();
        auto newestBlock = chain.GetBlock( idx );
        EXPECT_NE( newestBlock, nullptr );
        // The new block references the last (we have not forked).
        EXPECT_EQ( newestBlock->GetBlockHeader().hashPrevBlock, lastBlockHash );
        // We are still unable to look up the block with the missing index entry.
        lastBlock = chain.GetBlock( chain.GetIndex( currentHeight - 1 ) );
        EXPECT_EQ( lastBlock, nullptr );
    }
}
