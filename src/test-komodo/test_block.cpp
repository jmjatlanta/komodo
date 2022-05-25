#include "primitives/block.h"
#include "testutils.h"
#include "komodo_extern_globals.h"
#include "consensus/validation.h"
#include "coincontrol.h"
#include "miner.h"
#include "txdb.h"

#include <thread>
#include <gtest/gtest.h>

#include <boost/filesystem.hpp>

TEST(test_block, header_size_is_expected) {
    // Header with an empty Equihash solution.
    CBlockHeader header;
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << header;

    auto stream_size = CBlockHeader::HEADER_SIZE + 1;
    // ss.size is +1 due to data stream header of 1 byte
    EXPECT_EQ(ss.size(), stream_size);
}

TEST(test_block, TestStopAt)
{
    TestChain chain;
    auto notary = std::make_shared<TestWallet>(chain.getNotaryKey(), "notary");
    std::shared_ptr<CBlock> lastBlock = chain.generateBlock(notary); // genesis block
    ASSERT_GT( chain.GetIndex()->nHeight, 0 );
    lastBlock = chain.generateBlock(notary); // now we should be above 1
    ASSERT_GT( chain.GetIndex()->nHeight, 1);
    CBlock block;
    CValidationState state;
    KOMODO_STOPAT = 1;
    EXPECT_FALSE( chain.ConnectBlock(block, state, chain.GetIndex(), false, true) );
    KOMODO_STOPAT = 0; // to not stop other tests
}

TEST(test_block, TestConnectWithoutChecks)
{
    TestChain chain;
    auto notary = std::make_shared<TestWallet>(chain.getNotaryKey(), "notary");
    auto alice = std::make_shared<TestWallet>("alice");
    std::shared_ptr<CBlock> lastBlock = chain.generateBlock(notary); // genesis block
    ASSERT_GT( chain.GetIndex()->nHeight, 0 );
    // Add some transaction to a block
    int32_t newHeight = chain.GetIndex()->nHeight + 1;
    TransactionInProcess fundAlice = notary->CreateSpendTransaction(alice, 100000);
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
    block.vtx.push_back(fundAlice.transaction);
    CValidationState state;
    // create a new CBlockIndex to forward to ConnectBlock
    auto index = chain.GetIndex();
    CBlockIndex newIndex;
    newIndex.pprev = index;
    EXPECT_TRUE( chain.ConnectBlock(block, state, &newIndex, true, false) );
    if (!state.IsValid() )
        FAIL() << state.GetRejectReason();
}

TEST(test_block, TestSpendInSameBlock)
{
    //setConsoleDebugging(true);
    TestChain chain;
    auto notary = std::make_shared<TestWallet>(chain.getNotaryKey(), "notary");
    notary->SetBroadcastTransactions(true);
    auto alice = std::make_shared<TestWallet>("alice");
    alice->SetBroadcastTransactions(true);
    auto bob = std::make_shared<TestWallet>("bob");
    std::shared_ptr<CBlock> lastBlock = chain.generateBlock(notary); // genesis block
    ASSERT_GT( chain.GetIndex()->nHeight, 0 );
    // delay just a second to help with locktime
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // Start to build a block
    int32_t newHeight = chain.GetIndex()->nHeight + 1;
    TransactionInProcess fundAlice = notary->CreateSpendTransaction(alice, 100000, 0, true);
    // now have Alice move some funds to Bob in the same block
    CCoinControl useThisTransaction;
    COutPoint tx(fundAlice.transaction.GetHash(), 1);
    useThisTransaction.Select(tx);
    TransactionInProcess aliceToBob = alice->CreateSpendTransaction(bob, 50000, 5000, useThisTransaction);
    EXPECT_TRUE( alice->CommitTransaction(aliceToBob.transaction, aliceToBob.reserveKey) );
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // see if everything worked
    lastBlock = chain.generateBlock(notary);
    EXPECT_TRUE( lastBlock != nullptr);
    // balances should be correct
    EXPECT_EQ( bob->GetBalance() + bob->GetUnconfirmedBalance() + bob->GetImmatureBalance(), CAmount(50000));
    EXPECT_EQ( notary->GetBalance(), CAmount(10000000299905000));
    EXPECT_EQ( alice->GetBalance() + alice->GetUnconfirmedBalance() + alice->GetImmatureBalance(), CAmount(45000));
}

TEST(test_block, TestDoubleSpendInSameBlock)
{
    TestChain chain;
    auto notary = std::make_shared<TestWallet>(chain.getNotaryKey(), "notary");
    notary->SetBroadcastTransactions(true);
    auto alice = std::make_shared<TestWallet>("alice");
    alice->SetBroadcastTransactions(true);
    auto bob = std::make_shared<TestWallet>("bob");
    auto charlie = std::make_shared<TestWallet>("charlie");
    std::shared_ptr<CBlock> lastBlock = chain.generateBlock(notary); // genesis block
    ASSERT_GT( chain.GetIndex()->nHeight, 0 );
    // Start to build a block
    int32_t newHeight = chain.GetIndex()->nHeight + 1;
    TransactionInProcess fundAlice = notary->CreateSpendTransaction(alice, 100000, 0, true);
    EXPECT_EQ(mempool.size(), 1);
    // now have Alice move some funds to Bob in the same block
    {
        CCoinControl useThisTransaction;
        COutPoint tx(fundAlice.transaction.GetHash(), 1);
        useThisTransaction.Select(tx);
        TransactionInProcess aliceToBob = alice->CreateSpendTransaction(bob, 10000, 5000, useThisTransaction);
        EXPECT_TRUE(alice->CommitTransaction(aliceToBob.transaction, aliceToBob.reserveKey));
    }
    // alice attempts to double spend the vout and send something to charlie
    {
        CCoinControl useThisTransaction;
        COutPoint tx(fundAlice.transaction.GetHash(), 1);
        useThisTransaction.Select(tx);
        TransactionInProcess aliceToCharlie = alice->CreateSpendTransaction(charlie, 10000, 5000, useThisTransaction);
        CValidationState state;
        EXPECT_FALSE(alice->CommitTransaction(aliceToCharlie.transaction, aliceToCharlie.reserveKey, state));
        EXPECT_EQ(state.GetRejectReason(), "mempool conflict");
    }
    /*  
    EXPECT_EQ(mempool.size(), 3);
    CValidationState validationState;
    std::shared_ptr<CBlock> block = chain.generateBlock(notary, &validationState);
    EXPECT_EQ( block, nullptr );
    EXPECT_EQ( validationState.GetRejectReason(), "bad-txns-inputs-missingorspent");
    */
}

bool CalcPoW(CBlock *pblock);

TEST(test_block, TestProcessBlock)
{
    TestChain chain;
    EXPECT_EQ(chain.GetIndex()->nHeight, 0);
    auto notary = std::make_shared<TestWallet>(chain.getNotaryKey(), "notary");
    auto alice = std::make_shared<TestWallet>("alice");
    auto bob = std::make_shared<TestWallet>("bob");
    auto charlie = std::make_shared<TestWallet>("charlie");
    std::shared_ptr<CBlock> lastBlock = chain.generateBlock(notary); // gives notary everything
    EXPECT_EQ(chain.GetIndex()->nHeight, 1);
    chain.IncrementChainTime();
    // add a transaction to the mempool
    TransactionInProcess fundAlice = notary->CreateSpendTransaction(alice, 100000);
    EXPECT_TRUE( chain.acceptTx(fundAlice.transaction).IsValid() );
    // construct the block
    CBlock block;
    int32_t newHeight = chain.GetIndex()->nHeight + 1;
    CValidationState state;
    // no transactions
    EXPECT_FALSE( ProcessNewBlock(false, newHeight, state, nullptr, &block, false, nullptr) );
    EXPECT_EQ(state.GetRejectReason(), "bad-blk-length");
    EXPECT_EQ(chain.GetIndex()->nHeight, 2);
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

TEST(test_block, TestProcessBadBlock)
{
    TestChain chain;
    auto notary = std::make_shared<TestWallet>(chain.getNotaryKey(), "notary");
    auto alice = std::make_shared<TestWallet>("alice");
    auto bob = std::make_shared<TestWallet>("bob");
    auto charlie = std::make_shared<TestWallet>("charlie");
    std::shared_ptr<CBlock> lastBlock = chain.generateBlock(notary); // genesis block
    // add a transaction to the mempool
    TransactionInProcess fundAlice = notary->CreateSpendTransaction(alice, 100000);
    EXPECT_TRUE( chain.acceptTx(fundAlice.transaction).IsValid() );
    // construct the block
    CBlock block;
    int32_t newHeight = chain.GetIndex()->nHeight + 1;
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
        auto notary = std::make_shared<TestWallet>(chain.getNotaryKey(), "notary");
        auto alice = std::make_shared<TestWallet>("alice");
        aliceKey = alice->GetPrivKey();
        std::shared_ptr<CBlock> lastBlock = chain.generateBlock(alice); // genesis block
        auto idx = chain.GetIndex();
        int blockFile = idx->GetBlockPos().nFile;
        int lastHeight = idx->nHeight;
        // fill block00000.dat  
        while (blockFile == idx->GetBlockPos().nFile)
        {
            lastBlock = chain.generateBlock(alice);
            idx = chain.GetIndex();
            currentHeight = idx->nHeight;
            ASSERT_TRUE( lastHeight +1 == currentHeight);
            lastHeight = currentHeight;
            std::cout << "Mined block " << std::to_string(currentHeight) << "\n";
        }
        // fill block00001.dat
        blockFile = idx->GetBlockPos().nFile;
        while (blockFile == idx->GetBlockPos().nFile)
        {
            lastBlock = chain.generateBlock(alice);
            idx = chain.GetIndex();
            currentHeight = idx->nHeight;
            ASSERT_TRUE( lastHeight + 1 == currentHeight);
            lastHeight = currentHeight;
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
        EXPECT_EQ(idx->nHeight, currentHeight);
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

TEST(test_block, CorruptIndexFile)
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
        auto notary = std::make_shared<TestWallet>(chain.getNotaryKey(), "notary");
        auto alice = std::make_shared<TestWallet>("alice");
        aliceKey = alice->GetPrivKey();
        std::shared_ptr<CBlock> lastBlock = chain.generateBlock(alice); // genesis block
        auto idx = chain.GetIndex();
        int blockFile = idx->GetBlockPos().nFile;
        int lastHeight = idx->nHeight;
        // fill block00000.dat  
        while (blockFile == idx->GetBlockPos().nFile)
        {
            lastBlock = chain.generateBlock(alice);
            idx = chain.GetIndex();
            currentHeight = idx->nHeight;
            ASSERT_TRUE(lastHeight + 1 == currentHeight);
            lastHeight = currentHeight;
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
        EXPECT_EQ(chain.GetIndex()->nHeight, currentHeight);
    }
}

TEST(test_block, MissingIndexEntry)
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
        auto notary = std::make_shared<TestWallet>(chain.getNotaryKey(), "notary");
        auto alice = std::make_shared<TestWallet>("alice");
        auto lastBlock = chain.generateBlock(alice); // genesis block
        currentHeight = chain.GetIndex()->nHeight;
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
        auto notary = std::make_shared<TestWallet>(chain.getNotaryKey(), "notary");
        auto alice = std::make_shared<TestWallet>("alice");
        auto lastBlock = chain.generateBlock(alice);
        lastBlockHash = lastBlock->GetHash();
        lastTransactionHash = lastBlock->vtx[lastBlock->vtx.size()-1].GetHash();
        EXPECT_EQ( chain.GetIndex()->nHeight, currentHeight + 1);
        currentHeight = chain.GetIndex()->nHeight;
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
        EXPECT_EQ(chain.GetIndex()->nHeight, currentHeight);
        // we should be able to look up the block
        auto lastBlock = chain.GetBlock( chain.GetIndex(currentHeight) );
        EXPECT_EQ( lastBlock, nullptr ); // <-- a nullptr due to missing index entry
        // what happens if I attempt to mine a block and add it to the chain?
        auto notary = std::make_shared<TestWallet>(chain.getNotaryKey(), "notary");
        auto alice = std::make_shared<TestWallet>("alice");
        lastBlock = chain.generateBlock(alice);
        EXPECT_EQ( chain.GetIndex()->nHeight, currentHeight + 1);
        currentHeight = chain.GetIndex()->nHeight;
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
