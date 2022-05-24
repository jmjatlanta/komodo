#include "primitives/block.h"
#include "testutils.h"
#include "komodo_extern_globals.h"
#include "consensus/validation.h"
#include "miner.h"

#include <gtest/gtest.h>


TEST(block_tests, header_size_is_expected) {
    // Header with an empty Equihash solution.
    CBlockHeader header;
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << header;

    auto stream_size = CBlockHeader::HEADER_SIZE + 1;
    // ss.size is +1 due to data stream header of 1 byte
    EXPECT_EQ(ss.size(), stream_size);
}

TEST(block_tests, TestStopAt)
{
    TestChain chain;
    auto notary = chain.AddWallet(chain.getNotaryKey());
    std::shared_ptr<CBlock> lastBlock = chain.generateBlock(); // genesis block
    ASSERT_GT( chain.GetIndex()->nHeight, 0 );
    lastBlock = chain.generateBlock(); // now we should be above 1
    ASSERT_GT( chain.GetIndex()->nHeight, 1);
    CBlock block;
    CValidationState state;
    KOMODO_STOPAT = 1;
    EXPECT_FALSE( chain.ConnectBlock(block, state, chain.GetIndex(), false, true) );
    KOMODO_STOPAT = 0; // to not stop other tests
}

TEST(block_tests, TestConnectWithoutChecks)
{
    TestChain chain;
    auto notary = chain.AddWallet(chain.getNotaryKey());
    auto alice = chain.AddWallet();
    std::shared_ptr<CBlock> lastBlock = chain.generateBlock(alice); // genesis block
    ASSERT_GT( chain.GetIndex()->nHeight, 0 );
    // Add some transaction to a block
    int32_t newHeight = chain.GetIndex()->nHeight + 1;
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
    auto index = chain.GetIndex();
    CBlockIndex newIndex;
    newIndex.pprev = index;
    EXPECT_TRUE( chain.ConnectBlock(block, state, &newIndex, true, false) );
    if (!state.IsValid() )
        FAIL() << state.GetRejectReason();
}

TEST(block_tests, TestSpendInSameBlock)
{
    TestChain chain;
    auto notary = chain.AddWallet(chain.getNotaryKey());
    auto alice = chain.AddWallet();
    auto bob = chain.AddWallet();
    std::shared_ptr<CBlock> lastBlock = chain.generateBlock(alice); // genesis block
    ASSERT_GT( chain.GetIndex()->nHeight, 0 );
    // Start to build a block
    int32_t newHeight = chain.GetIndex()->nHeight + 1;
    CTransaction fundNotary = alice->CreateSpendTransaction(notary, 100000);
    // now have Notary move some funds to Bob in the same block
    CMutableTransaction notaryToBobMutable;
    CTxIn notaryIn;
    notaryIn.prevout.hash = fundNotary.GetHash();
    notaryIn.prevout.n = 1;
    notaryToBobMutable.vin.push_back(notaryIn);
    CTxOut bobOut;
    bobOut.scriptPubKey = GetScriptForDestination(bob->GetPubKey());
    bobOut.nValue = 10000;
    notaryToBobMutable.vout.push_back(bobOut);
    CTxOut notaryRemainder;
    notaryRemainder.scriptPubKey = GetScriptForDestination(notary->GetPubKey());
    notaryRemainder.nValue = fundNotary.vout[1].nValue - 10000;
    notaryToBobMutable.vout.push_back(notaryRemainder);
    uint256 hash = SignatureHash(fundNotary.vout[1].scriptPubKey, notaryToBobMutable, 0, SIGHASH_ALL, 0, 0);
    notaryToBobMutable.vin[0].scriptSig << notary->Sign(hash, SIGHASH_ALL);
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
    block.vtx.push_back(fundNotary);
    block.vtx.push_back(notaryToBobTx);
    CValidationState state;
    // create a new CBlockIndex to forward to ConnectBlock
    auto index = chain.GetIndex();
    CBlockIndex newIndex;
    newIndex.pprev = index;
    EXPECT_TRUE( chain.ConnectBlock(block, state, &newIndex, true, false) );
    if (!state.IsValid() )
        FAIL() << state.GetRejectReason();
}

TEST(block_tests, TestDoubleSpendInSameBlock)
{
    TestChain chain;
    auto notary = chain.AddWallet(chain.getNotaryKey());
    auto alice = chain.AddWallet();
    auto bob = chain.AddWallet();
    auto charlie = chain.AddWallet();
    std::shared_ptr<CBlock> lastBlock = chain.generateBlock(alice); // genesis block
    ASSERT_GT( chain.GetIndex()->nHeight, 0 );
    // Start to build a block
    int32_t newHeight = chain.GetIndex()->nHeight + 1;
    CTransaction fundNotary = alice->CreateSpendTransaction(notary, 100000);
    // now have Notary move some funds to Bob in the same block
    CMutableTransaction notaryToBobMutable;
    CTxIn notaryIn;
    notaryIn.prevout.hash = fundNotary.GetHash();
    notaryIn.prevout.n = 1;
    notaryToBobMutable.vin.push_back(notaryIn);
    CTxOut bobOut;
    bobOut.scriptPubKey = GetScriptForDestination(bob->GetPubKey());
    bobOut.nValue = 10000;
    notaryToBobMutable.vout.push_back(bobOut);
    CTxOut notaryRemainder;
    notaryRemainder.scriptPubKey = GetScriptForDestination(notary->GetPubKey());
    notaryRemainder.nValue = fundNotary.vout[1].nValue - 10000;
    notaryToBobMutable.vout.push_back(notaryRemainder);
    uint256 hash = SignatureHash(fundNotary.vout[1].scriptPubKey, notaryToBobMutable, 0, SIGHASH_ALL, 0, 0);
    notaryToBobMutable.vin[0].scriptSig << notary->Sign(hash, SIGHASH_ALL);
    CTransaction notaryToBobTx(notaryToBobMutable);
    // notary attempts to double spend the vout and send something to charlie
    CMutableTransaction notaryToCharlieMutable;
    CTxIn notaryIn2;
    notaryIn2.prevout.hash = fundNotary.GetHash();
    notaryIn2.prevout.n = 1;
    notaryToCharlieMutable.vin.push_back(notaryIn2);
    CTxOut charlieOut;
    charlieOut.scriptPubKey = GetScriptForDestination(charlie->GetPubKey());
    charlieOut.nValue = 10000;
    notaryToCharlieMutable.vout.push_back(charlieOut);
    CTxOut notaryRemainder2;
    notaryRemainder2.scriptPubKey = GetScriptForDestination(notary->GetPubKey());
    notaryRemainder2.nValue = fundNotary.vout[1].nValue - 10000;
    notaryToCharlieMutable.vout.push_back(notaryRemainder2);
    hash = SignatureHash(fundNotary.vout[1].scriptPubKey, notaryToCharlieMutable, 0, SIGHASH_ALL, 0, 0);
    notaryToCharlieMutable.vin[0].scriptSig << notary->Sign(hash, SIGHASH_ALL);
    CTransaction notaryToCharlieTx(notaryToCharlieMutable);
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
    block.vtx.push_back(notaryToBobTx);
    block.vtx.push_back(notaryToCharlieTx);
    CValidationState state;
    // create a new CBlockIndex to forward to ConnectBlock
    auto index = chain.GetIndex();
    CBlockIndex newIndex;
    newIndex.pprev = index;
    EXPECT_FALSE( chain.ConnectBlock(block, state, &newIndex, true, false) );
    EXPECT_EQ(state.GetRejectReason(), "bad-txns-inputs-missingorspent");
}

bool CalcPoW(CBlock *pblock);

TEST(block_tests, TestProcessBlock)
{
    TestChain chain;
    EXPECT_EQ(chain.GetIndex()->nHeight, 0);
    auto notary = chain.AddWallet(chain.getNotaryKey());
    auto alice = chain.AddWallet();
    auto bob = chain.AddWallet();
    auto charlie = chain.AddWallet();
    auto lastBlock = chain.generateBlock(alice); // gives alice everything
    EXPECT_EQ(chain.GetIndex()->nHeight, 1);
    chain.IncrementChainTime();
    // add a transaction to the mempool
    CTransaction fundBob = alice->CreateSpendTransaction(bob, 100000);
    EXPECT_TRUE( chain.acceptTx(fundBob).IsValid() );
    // construct the block
    CBlock block;
    int32_t newHeight = chain.GetIndex()->nHeight + 1;
    CValidationState state;
    // no transactions
    EXPECT_FALSE( ProcessNewBlock(false, newHeight, state, nullptr, &block, false, nullptr) );
    EXPECT_EQ(state.GetRejectReason(), "bad-blk-length");
    EXPECT_EQ(chain.GetIndex()->nHeight, 1);
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

TEST(block_tests, TestProcessBadBlock)
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