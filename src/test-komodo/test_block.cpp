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
    CBlock lastBlock = chain.generateBlock(); // genesis block
    ASSERT_GT( chain.GetIndex()->GetHeight(), 0 );
    lastBlock = chain.generateBlock(); // now we should be above 1
    ASSERT_GT( chain.GetIndex()->GetHeight(), 1);
    CBlock block;
    CValidationState state;
    KOMODO_STOPAT = 1;
    EXPECT_FALSE( ConnectBlock(block, state, chain.GetIndex(), *chain.GetCoinsViewCache(), false, true) );
    KOMODO_STOPAT = 0; // to not stop other tests
}

TEST(block_tests, TestConnectWithoutChecks)
{
    TestChain chain;
    auto notary = chain.AddWallet(chain.getNotaryKey());
    auto alice = chain.AddWallet();
    CBlock lastBlock = chain.generateBlock(); // genesis block
    ASSERT_GT( chain.GetIndex()->GetHeight(), 0 );
    // Add some transaction to a block
    int32_t newHeight = chain.GetIndex()->GetHeight() + 1;
    CTransaction fundAlice = notary->CreateSpendTransaction(alice, 100000);
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

TEST(block_tests, TestSpendInSameBlock)
{
    TestChain chain;
    auto notary = chain.AddWallet(chain.getNotaryKey());
    auto alice = chain.AddWallet();
    auto bob = chain.AddWallet();
    CBlock lastBlock = chain.generateBlock(); // genesis block
    ASSERT_GT( chain.GetIndex()->GetHeight(), 0 );
    // Start to build a block
    int32_t newHeight = chain.GetIndex()->GetHeight() + 1;
    CTransaction fundAlice = notary->CreateSpendTransaction(alice, 100000);
    // now have Alice move some funds to Bob in the same block
    CMutableTransaction aliceToBobMutable;
    CTxIn aliceIn;
    aliceIn.prevout.hash = fundAlice.GetHash();
    aliceIn.prevout.n = 0;
    aliceToBobMutable.vin.push_back(aliceIn);
    CTxOut bobOut;
    bobOut.scriptPubKey = GetScriptForDestination(bob->GetPubKey());
    bobOut.nValue = 10000;
    aliceToBobMutable.vout.push_back(bobOut);
    CTxOut aliceRemainder;
    aliceRemainder.scriptPubKey = GetScriptForDestination(alice->GetPubKey());
    aliceRemainder.nValue = fundAlice.vout[0].nValue - 10000;
    aliceToBobMutable.vout.push_back(aliceRemainder);
    uint256 hash = SignatureHash(fundAlice.vout[0].scriptPubKey, aliceToBobMutable, 0, SIGHASH_ALL, 0, 0);
    aliceToBobMutable.vin[0].scriptSig << alice->Sign(hash, SIGHASH_ALL);
    CTransaction aliceToBobTx(aliceToBobMutable);
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
    block.vtx.push_back(aliceToBobTx);
    CValidationState state;
    // create a new CBlockIndex to forward to ConnectBlock
    auto index = chain.GetIndex();
    CBlockIndex newIndex;
    newIndex.pprev = index;
    EXPECT_TRUE( ConnectBlock(block, state, &newIndex, *chain.GetCoinsViewCache(), true, false) );
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
    CBlock lastBlock = chain.generateBlock(); // genesis block
    ASSERT_GT( chain.GetIndex()->GetHeight(), 0 );
    // Start to build a block
    int32_t newHeight = chain.GetIndex()->GetHeight() + 1;
    CTransaction fundAlice = notary->CreateSpendTransaction(alice, 100000);
    // now have Alice move some funds to Bob in the same block
    CMutableTransaction aliceToBobMutable;
    CTxIn aliceIn;
    aliceIn.prevout.hash = fundAlice.GetHash();
    aliceIn.prevout.n = 0;
    aliceToBobMutable.vin.push_back(aliceIn);
    CTxOut bobOut;
    bobOut.scriptPubKey = GetScriptForDestination(bob->GetPubKey());
    bobOut.nValue = 10000;
    aliceToBobMutable.vout.push_back(bobOut);
    CTxOut aliceRemainder;
    aliceRemainder.scriptPubKey = GetScriptForDestination(alice->GetPubKey());
    aliceRemainder.nValue = fundAlice.vout[0].nValue - 10000;
    aliceToBobMutable.vout.push_back(aliceRemainder);
    uint256 hash = SignatureHash(fundAlice.vout[0].scriptPubKey, aliceToBobMutable, 0, SIGHASH_ALL, 0, 0);
    aliceToBobMutable.vin[0].scriptSig << alice->Sign(hash, SIGHASH_ALL);
    CTransaction aliceToBobTx(aliceToBobMutable);
    // alice attempts to double spend the vout and send something to charlie
    CMutableTransaction aliceToCharlieMutable;
    CTxIn aliceIn2;
    aliceIn2.prevout.hash = fundAlice.GetHash();
    aliceIn2.prevout.n = 0;
    aliceToCharlieMutable.vin.push_back(aliceIn2);
    CTxOut charlieOut;
    charlieOut.scriptPubKey = GetScriptForDestination(charlie->GetPubKey());
    charlieOut.nValue = 10000;
    aliceToCharlieMutable.vout.push_back(charlieOut);
    CTxOut aliceRemainder2;
    aliceRemainder2.scriptPubKey = GetScriptForDestination(alice->GetPubKey());
    aliceRemainder2.nValue = fundAlice.vout[0].nValue - 10000;
    aliceToCharlieMutable.vout.push_back(aliceRemainder2);
    hash = SignatureHash(fundAlice.vout[0].scriptPubKey, aliceToCharlieMutable, 0, SIGHASH_ALL, 0, 0);
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
    block.vtx.push_back(fundAlice);
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

TEST(block_tests, TestProcessBlock)
{
    TestChain chain;
    EXPECT_EQ(chain.GetIndex()->GetHeight(), 0);
    auto notary = chain.AddWallet(chain.getNotaryKey());
    auto alice = chain.AddWallet();
    auto bob = chain.AddWallet();
    auto charlie = chain.AddWallet();
    CBlock lastBlock = chain.generateBlock(); // gives notary everything
    EXPECT_EQ(chain.GetIndex()->GetHeight(), 1);
    chain.IncrementChainTime();
    auto notaryPrevOut = notary->GetAvailable(100000);
    // add a transaction to the mempool
    CTransaction fundAlice = notary->CreateSpendTransaction(alice, 100000);
    EXPECT_TRUE( chain.acceptTx(fundAlice).IsValid() );
    // construct the block
    CBlock block;
    int32_t newHeight = chain.GetIndex()->GetHeight() + 1;
    CValidationState state;
    // no transactions
    EXPECT_FALSE( ProcessNewBlock(false, newHeight, state, nullptr, &block, false, nullptr) );
    EXPECT_EQ(state.GetRejectReason(), "bad-blk-length");
    EXPECT_EQ(chain.GetIndex()->GetHeight(), 1);
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
    block.hashPrevBlock = lastBlock.GetHash();
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

TEST(block_tests, TestForkBlock)
{
    TestChain chain;
    bool forceProcessing = true; // true = local or whitelisted peers, false = non-requested blocks
    EXPECT_EQ(chain.GetIndex()->GetHeight(), 0);
    auto notary = chain.AddWallet(chain.getNotaryKey());
    auto alice = chain.AddWallet();
    auto bob = chain.AddWallet();
    auto charlie = chain.AddWallet();
    CBlock lastBlock = chain.generateBlock(); // gives notary everything
    EXPECT_EQ(chain.GetIndex()->GetHeight(), 1);
    chain.IncrementChainTime();
    auto notaryPrevOut = notary->GetAvailable(100000);
    // add a transaction to the mempool
    CTransaction fundAlice = notary->CreateSpendTransaction(alice, 100000);
    EXPECT_TRUE( chain.acceptTx(fundAlice).IsValid() );
    // construct 2 blocks at the same height to fork chain
    int32_t newHeight = chain.GetIndex()->GetHeight() + 1;
    CBlock aBlock;
    // construct a block and connect to "lastBlock" on chain "A". This does nothing to the tx in mempool
    {
        // add first a coinbase tx
        auto consensusParams = Params().GetConsensus();
        CMutableTransaction txNew = CreateNewContextualCMutableTransaction(consensusParams, newHeight);
        txNew.vin.resize(1);
        txNew.vin[0].prevout.SetNull();
        txNew.vin[0].scriptSig = (CScript() << newHeight << CScriptNum(1)) + COINBASE_FLAGS;
        txNew.vout.resize(1);
        txNew.vout[0].nValue = GetBlockSubsidy(newHeight,consensusParams);
        txNew.nExpiryHeight = 0;
        aBlock.vtx.push_back(CTransaction(txNew));
        // finish constructing the block
        aBlock.nBits = GetNextWorkRequired( chain.GetIndex(), &aBlock, Params().GetConsensus());
        aBlock.nTime = GetTime();
        aBlock.hashPrevBlock = lastBlock.GetHash();
        aBlock.hashMerkleRoot = aBlock.BuildMerkleTree();
        // Add the PoW
        EXPECT_TRUE(CalcPoW(&aBlock));
        CValidationState state;
        EXPECT_TRUE( ProcessNewBlock(false, newHeight, state, nullptr, &aBlock, forceProcessing, nullptr) );
        if (!state.IsValid())
            FAIL() << state.GetRejectReason();
    }
    // construct a different block at same height to make fork B. This spends the mempool tx
    CBlock bBlock;
    {
        // add first a coinbase tx
        auto consensusParams = Params().GetConsensus();
        CMutableTransaction txNew = CreateNewContextualCMutableTransaction(consensusParams, newHeight);
        txNew.vin.resize(1);
        txNew.vin[0].prevout.SetNull();
        txNew.vin[0].scriptSig = (CScript() << newHeight << CScriptNum(1)) + COINBASE_FLAGS;
        txNew.vout.resize(1);
        txNew.vout[0].nValue = GetBlockSubsidy(newHeight,consensusParams);
        txNew.nExpiryHeight = 0;
        bBlock.vtx.push_back(CTransaction(txNew));
        // add the transaction that was in the mempool
        bBlock.vtx.push_back(fundAlice);
        // finish constructing the block
        bBlock.nBits = GetNextWorkRequired( chain.GetIndex(), &bBlock, Params().GetConsensus());
        bBlock.nTime = GetTime();
        bBlock.hashPrevBlock = lastBlock.GetHash();
        bBlock.hashMerkleRoot = bBlock.BuildMerkleTree();
        // Add the PoW
        EXPECT_TRUE(CalcPoW(&bBlock));
        CValidationState state;
        EXPECT_TRUE( ProcessNewBlock(false, newHeight, state, nullptr, &bBlock, forceProcessing, nullptr) );
        if (!state.IsValid())
            FAIL() << state.GetRejectReason();
        // The transaction still exists in the mempool, chain b is not tip
        EXPECT_EQ(mempool.size(), 1);
    }
    newHeight += 1;
    chain.IncrementChainTime();
    CBlock a2Block;
    // add another block to chain "A" but that does not spend Alice's coin
    {
        // add first a coinbase tx
        auto consensusParams = Params().GetConsensus();
        CMutableTransaction txNew = CreateNewContextualCMutableTransaction(consensusParams, newHeight);
        txNew.vin.resize(1);
        txNew.vin[0].prevout.SetNull();
        txNew.vin[0].scriptSig = (CScript() << newHeight << CScriptNum(1)) + COINBASE_FLAGS;
        txNew.vout.resize(1);
        txNew.vout[0].nValue = GetBlockSubsidy(newHeight,consensusParams);
        txNew.nExpiryHeight = 0;
        a2Block.vtx.push_back(CTransaction(txNew));
        // finish constructing the block
        a2Block.nBits = GetNextWorkRequired( chain.GetIndex(), &a2Block, Params().GetConsensus());
        a2Block.nTime = GetTime();
        a2Block.hashPrevBlock = aBlock.GetHash();
        a2Block.hashMerkleRoot = a2Block.BuildMerkleTree();
        // Add the PoW
        EXPECT_TRUE(CalcPoW(&a2Block));
        CValidationState state;
        EXPECT_TRUE( ProcessNewBlock(false, newHeight, state, nullptr, &a2Block, forceProcessing, nullptr) );
        if (!state.IsValid())
            FAIL() << state.GetRejectReason();
        // The transaction still exists in the mempool, chain A is still the longest chain
        EXPECT_EQ(mempool.size(), 1);
    }
    // add a block to chain "B" that tries to doublespend the mempool tx
    CBlock b2Block;
    {
        // add first a coinbase tx
        auto consensusParams = Params().GetConsensus();
        CMutableTransaction txNew = CreateNewContextualCMutableTransaction(consensusParams, newHeight);
        txNew.vin.resize(1);
        txNew.vin[0].prevout.SetNull();
        txNew.vin[0].scriptSig = (CScript() << newHeight << CScriptNum(1)) + COINBASE_FLAGS;
        txNew.vout.resize(1);
        txNew.vout[0].nValue = GetBlockSubsidy(newHeight,consensusParams);
        txNew.nExpiryHeight = 0;
        b2Block.vtx.push_back(CTransaction(txNew));
        // add the transaction that was in the mempool (double spending)
        b2Block.vtx.push_back(fundAlice);
        // finish constructing the block
        b2Block.nBits = GetNextWorkRequired( chain.GetIndex(), &bBlock, Params().GetConsensus());
        b2Block.nTime = GetTime();
        b2Block.hashPrevBlock = bBlock.GetHash();
        b2Block.hashMerkleRoot = b2Block.BuildMerkleTree();
        // Add the PoW
        EXPECT_TRUE(CalcPoW(&b2Block));
        CValidationState state;
        EXPECT_TRUE( ProcessNewBlock(false, newHeight, state, nullptr, &b2Block, forceProcessing, nullptr) );
        if (!state.IsValid())
            FAIL() << state.GetRejectReason();
        // The transaction still exists in the mempool, chain b is not tip
        EXPECT_EQ(mempool.size(), 1);
    }
    // add another block that makes chain "B" the longest chain
    newHeight += 1;
    chain.IncrementChainTime();
    {
        CBlock b3Block;
        // add first a coinbase tx
        auto consensusParams = Params().GetConsensus();
        CMutableTransaction txNew = CreateNewContextualCMutableTransaction(consensusParams, newHeight);
        txNew.vin.resize(1);
        txNew.vin[0].prevout.SetNull();
        txNew.vin[0].scriptSig = (CScript() << newHeight << CScriptNum(1)) + COINBASE_FLAGS;
        txNew.vout.resize(1);
        txNew.vout[0].nValue = GetBlockSubsidy(newHeight,consensusParams);
        txNew.nExpiryHeight = 0;
        b3Block.vtx.push_back(CTransaction(txNew));
        // finish constructing the block
        b3Block.nBits = GetNextWorkRequired( chain.GetIndex(), &b2Block, Params().GetConsensus());
        b3Block.nTime = GetTime();
        b3Block.hashPrevBlock = b2Block.GetHash();
        b3Block.hashMerkleRoot = b3Block.BuildMerkleTree();
        // Add the PoW
        EXPECT_TRUE(CalcPoW(&b3Block));
        CValidationState state;
        EXPECT_TRUE( ProcessNewBlock(false, newHeight, state, nullptr, &b3Block, forceProcessing, nullptr) );
        if (!state.IsValid())
            FAIL() << state.GetRejectReason();
        // the chain should fail consensus. Therefore chain A is still the longest chain
        // verify that the tip is still "chain A"
        EXPECT_EQ(chainActive.Height(), newHeight-1);
        EXPECT_EQ(chainActive.Tip()->GetBlockHash(), a2Block.GetHash());
    }
    // Verify transaction is still in the mempool
    EXPECT_EQ(mempool.size(), 1);
}

TEST(block_tests, TestBlockRemovesMempoolTx)
{
    TestChain chain;
    EXPECT_EQ(chain.GetIndex()->GetHeight(), 0);
    auto notary = chain.AddWallet(chain.getNotaryKey());
    auto alice = chain.AddWallet();
    auto bob = chain.AddWallet();
    auto charlie = chain.AddWallet();
    CBlock lastBlock = chain.generateBlock(); // gives notary everything
    EXPECT_EQ(chain.GetIndex()->GetHeight(), 1);
    chain.IncrementChainTime();
    auto notaryPrevOut = notary->GetAvailable(100000);
    // add a transaction to the mempool
    CTransaction fundAlice = notary->CreateSpendTransaction(alice, 100000);
    EXPECT_TRUE( chain.acceptTx(fundAlice).IsValid() );
    auto consensusParams = Params().GetConsensus();
    // construct 2 blocks at the same height to fork chain
    int32_t newHeight = chain.GetIndex()->GetHeight() + 1;
    // transaction on chain A that spends the mempool tx
    {
        CBlock aBlock;
        // add first a coinbase tx
        CMutableTransaction txNew = CreateNewContextualCMutableTransaction(consensusParams, newHeight);
        txNew.vin.resize(1);
        txNew.vin[0].prevout.SetNull();
        txNew.vin[0].scriptSig = (CScript() << newHeight << CScriptNum(1)) + COINBASE_FLAGS;
        txNew.vout.resize(1);
        txNew.vout[0].nValue = GetBlockSubsidy(newHeight,consensusParams);
        txNew.nExpiryHeight = 0;
        aBlock.vtx.push_back(CTransaction(txNew));
        // add the transaction that was in the mempool
        aBlock.vtx.push_back(fundAlice);
        // finish constructing the block
        aBlock.nBits = GetNextWorkRequired( chain.GetIndex(), &aBlock, Params().GetConsensus());
        aBlock.nTime = GetTime();
        aBlock.hashPrevBlock = lastBlock.GetHash();
        aBlock.hashMerkleRoot = aBlock.BuildMerkleTree();
        // Add the PoW
        EXPECT_TRUE(CalcPoW(&aBlock));
        CValidationState state;
        EXPECT_TRUE( ProcessNewBlock(false, newHeight, state, nullptr, &aBlock, true, nullptr) );
        if (!state.IsValid())
            FAIL() << state.GetRejectReason();
        // The transaction no longer exists in the mempool
        EXPECT_EQ(mempool.size(), 0);
    }
    // try to use the transaction on another fork
    CBlock b1Block;
    {
        // add first a coinbase tx
        CMutableTransaction txNew = CreateNewContextualCMutableTransaction(consensusParams, newHeight);
        txNew.vin.resize(1);
        txNew.vin[0].prevout.SetNull();
        txNew.vin[0].scriptSig = (CScript() << newHeight << CScriptNum(1)) + COINBASE_FLAGS;
        txNew.vout.resize(1);
        txNew.vout[0].nValue = GetBlockSubsidy(newHeight,consensusParams);
        txNew.nExpiryHeight = 0;
        b1Block.vtx.push_back(CTransaction(txNew));
        // add the transaction that was in the mempool (double spending)
        b1Block.vtx.push_back(fundAlice);
        // finish constructing the block
        b1Block.nBits = GetNextWorkRequired( chain.GetIndex(), &b1Block, Params().GetConsensus());
        b1Block.nTime = GetTime();
        b1Block.hashPrevBlock = lastBlock.GetHash();
        b1Block.hashMerkleRoot = b1Block.BuildMerkleTree();
        // Add the PoW
        EXPECT_TRUE(CalcPoW(&b1Block));
        CValidationState state;
        EXPECT_TRUE( ProcessNewBlock(false, newHeight, state, nullptr, &b1Block, true, nullptr) );
        if (!state.IsValid())
            FAIL() << state.GetRejectReason();
        // The transaction still does not exist in mempool, but does exist on both chain A and B
        EXPECT_EQ(mempool.size(), 0);
    }
    // now activate chain B by making it the longest chain
    newHeight += 1;
    chain.IncrementChainTime();
    {
        CBlock b2Block;
        CMutableTransaction txNew = CreateNewContextualCMutableTransaction(consensusParams, newHeight);
        txNew.vin.resize(1);
        txNew.vin[0].prevout.SetNull();
        txNew.vin[0].scriptSig = (CScript() << newHeight << CScriptNum(1)) + COINBASE_FLAGS;
        txNew.vout.resize(1);
        txNew.vout[0].nValue = GetBlockSubsidy(newHeight,consensusParams);
        txNew.nExpiryHeight = 0;
        b2Block.vtx.push_back(CTransaction(txNew));
        b2Block.nBits = GetNextWorkRequired( chain.GetIndex(), &b2Block, Params().GetConsensus());
        b2Block.nTime = GetTime();
        b2Block.hashPrevBlock = b1Block.GetHash();
        b2Block.hashMerkleRoot = b2Block.BuildMerkleTree();
        // Add the PoW
        EXPECT_TRUE(CalcPoW(&b2Block));
        CValidationState state;
        EXPECT_TRUE( ProcessNewBlock(false, newHeight, state, nullptr, &b2Block, true, nullptr) );
        if (!state.IsValid())
            FAIL() << state.GetRejectReason();
        // The transaction still does not exist in mempool, but does exist on both chain A and B
        EXPECT_EQ(mempool.size(), 0);
        // the tip should be on chain B
        EXPECT_EQ( chainActive.Tip()->GetBlockHash(), b2Block.GetHash() );
    }
}

TEST(block_tests, TestProcessBadBlock)
{
    TestChain chain;
    auto notary = chain.AddWallet(chain.getNotaryKey());
    auto alice = chain.AddWallet();
    auto bob = chain.AddWallet();
    auto charlie = chain.AddWallet();
    CBlock lastBlock = chain.generateBlock(); // genesis block
    auto notaryPrevOut = notary->GetAvailable(100000);
    // add a transaction to the mempool
    CTransaction fundAlice = notary->CreateSpendTransaction(alice, 100000);
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