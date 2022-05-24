#include "testutils.h"
#include "chainparams.h"
#include "komodo_notary.h"
#include "miner.h"

#include <gtest/gtest.h>

bool CalcPoW(CBlock *pblock); // generate PoW on a block

namespace TestNotary
{

/***
 * A little class to help with the different formats keys come in
 */
class my_key
{
public:
    my_key(uint8_t in[33])
    {
        for(int i = 0; i < 33; ++i)
            key.push_back(in[i]);
    }
    my_key(const std::string& in)
    {
        for(int i = 0; i < 33; ++i)
            key.push_back( 
                    (unsigned int)strtol(in.substr(i*2, 2).c_str(), nullptr, 16) );
    }
    bool fill(uint8_t in[33])
    {
        memcpy(in, key.data(), 33);
        return true;
    }
private:
    std::vector<uint8_t> key;
    friend bool operator==(const my_key& lhs, const my_key& rhs);
};

bool operator==(const my_key& lhs, const my_key& rhs)
{
    if (lhs.key == rhs.key)
        return true;
    return false;
}

TEST(TestNotary, KomodoNotaries)
{
    // Test komodo_notaries(), getkmdseason()
    ASSETCHAINS_SYMBOL[0] = 0;
    uint8_t pubkeys[64][33];
    int32_t height = 0;
    uint32_t timestamp = 0;
    {
        TestChain testChain;
        // Get the pubkeys of the first era
        int32_t result = komodo_notaries(pubkeys, height, timestamp);
        EXPECT_EQ(result, 36);
        // the first notary didn't change between era 1 and 2, so look at the 2nd notary
        EXPECT_EQ(my_key(pubkeys[1]), my_key("02ebfc784a4ba768aad88d44d1045d240d47b26e248cafaf1c5169a42d7a61d344"));
        // make sure the era hasn't changed before it is supposed to
        for(;height <= Params().KomodoNotariesHardcoded()-1; ++height)
        {
            result = komodo_notaries(pubkeys, height, timestamp);
            EXPECT_EQ(result, 36);
            EXPECT_EQ(getkmdseason(height), 1);
            EXPECT_EQ(my_key(pubkeys[1]), my_key("02ebfc784a4ba768aad88d44d1045d240d47b26e248cafaf1c5169a42d7a61d344"));
        }
        EXPECT_EQ(height, Params().KomodoNotariesHardcoded());
        // at 180000 we start using notaries_elected(komodo_defs.h) instead of Notaries_genesis(komodo_notary.cpp)
        for(;height <= Params().S1HardforkHeight(); ++height)
        {
            result = komodo_notaries(pubkeys, height, timestamp);
            EXPECT_EQ(result, 64);
            EXPECT_EQ(getkmdseason(height), 1);
            EXPECT_EQ(my_key(pubkeys[1]), my_key("02ebfc784a4ba768aad88d44d1045d240d47b26e248cafaf1c5169a42d7a61d344"));
        }
        // make sure the era changes when it was supposed to, and we have a new key
        EXPECT_EQ(height, Params().S1HardforkHeight() + 1);
        result = komodo_notaries(pubkeys, height, timestamp);
        EXPECT_EQ(result, 64);
        EXPECT_EQ(getkmdseason(height), 2);
        EXPECT_EQ(my_key(pubkeys[1]), my_key("030f34af4b908fb8eb2099accb56b8d157d49f6cfb691baa80fdd34f385efed961"));
    }
    // now try the same thing with notaries_staked, which uses timestamp instead of height
    // NOTE: If height is passed instead of timestamp, the timestamp is computed based on block timestamps
    // notaries come from notaries_STAKED(notaries_staked.h)
    // also tests STAKED_era()
    height = 0;
    timestamp = 1;
    {
        TestChain testChain;
        strcpy(ASSETCHAINS_SYMBOL, "LABS");
        // we should be in era 1 now
        int32_t result = komodo_notaries(pubkeys, height, timestamp);
        EXPECT_EQ(result, 22);
        EXPECT_EQ(STAKED_era(timestamp), 1);
        EXPECT_EQ(my_key(pubkeys[13]), my_key("03669457b2934d98b5761121dd01b243aed336479625b293be9f8c43a6ae7aaeff"));
        timestamp = 1572523200;
        EXPECT_EQ(result, 22);
        EXPECT_EQ(STAKED_era(timestamp), 1);
        EXPECT_EQ(my_key(pubkeys[13]), my_key("03669457b2934d98b5761121dd01b243aed336479625b293be9f8c43a6ae7aaeff"));
        // Moving to era 2 should change the notaries. But there is a gap of 777 that uses komodo notaries for some reason
        // (NOTE: the first 12 are the same, so use the 13th)
        timestamp++;
        EXPECT_EQ(timestamp, 1572523201);
        result = komodo_notaries(pubkeys, height, timestamp);
        EXPECT_EQ(result, 64);
        EXPECT_EQ(STAKED_era(timestamp), 0);
        EXPECT_EQ(pubkeys[13][0], 0);
        // advance past the gap
        timestamp += 778;
        result = komodo_notaries(pubkeys, height, timestamp);
        EXPECT_EQ(result, 24);
        EXPECT_EQ(STAKED_era(timestamp), 2);
        EXPECT_EQ(my_key(pubkeys[13]), my_key("02d1dd4c5d5c00039322295aa965f9787a87d234ed4f8174231bbd6162e384eba8"));

        // now test getacseason()
        EXPECT_EQ(getacseason(0), 1);
        EXPECT_EQ(getacseason(1), 1);
        EXPECT_EQ(getacseason(1525132800), 1);
        EXPECT_EQ(getacseason(1525132801), 2);
        EXPECT_EQ(getacseason(1751328000), 6);
        EXPECT_EQ(getacseason(1751328001), 0);
    }
}

TEST(TestNotary, ElectedNotary)
{
    // exercise the routine that checks to see if a particular public key is a notary at the current height

    // setup
    TestChain testChain;
    my_key first_era("02ebfc784a4ba768aad88d44d1045d240d47b26e248cafaf1c5169a42d7a61d344");
    my_key second_era("030f34af4b908fb8eb2099accb56b8d157d49f6cfb691baa80fdd34f385efed961");

    int32_t numnotaries;
    uint8_t pubkey[33];
    first_era.fill(pubkey);
    int32_t height = 0;
    uint32_t timestamp = 0;

    // check the KMD chain, first era
    ASSETCHAINS_SYMBOL[0] = 0;
    int32_t result = komodo_electednotary(&numnotaries, pubkey, height, timestamp);
    EXPECT_EQ(result, 1);
    EXPECT_EQ(numnotaries, 36);
    // now try a wrong key
    second_era.fill(pubkey);
    result = komodo_electednotary(&numnotaries, pubkey, height, timestamp);
    EXPECT_EQ(result, -1);
    EXPECT_EQ(numnotaries, 36);

    // KMD chain, second era
    height = 814001;
    result = komodo_electednotary(&numnotaries, pubkey, height, timestamp);
    EXPECT_EQ(result, 1);
    EXPECT_EQ(numnotaries, 64);
    // now try a wrong key
    first_era.fill(pubkey);
    result = komodo_electednotary(&numnotaries, pubkey, height, timestamp);
    EXPECT_EQ(result, -1);
    EXPECT_EQ(numnotaries, 64);
}

uint32_t GetCoinbaseNonce(const CBlock& in)
{
    uint32_t retVal = 0;
    std::vector<std::vector<unsigned char> > results;
    CScript scr = in.vtx[0].vin[0].scriptSig;
    scr.GetPushedData(scr.begin(), results);
    if (results.size() >= 2)
    {
        CScriptNum scr(results[1], false);
        retVal = scr.getint();
    }
    
    return retVal;
}

TEST(TestNotary, HardforkActiveDecember2019)
{
    /*
    {
        // ATM, we cannot change the consts that this method uses, hence this test takes a
        // very long time. In the future, it is hoped we can be a bit more flexible with this
        ASSETCHAINS_SYMBOL[0] = 0;

        TestChain testChain;
        auto height = testChain.GetIndex()->nHeight;
        while (height < nDecemberHardforkHeight)
        {
            testChain.generateBlock();
            height = testChain.GetIndex()->nHeight;
            EXPECT_FALSE( komodo_hardfork_active(0) );
        }
        while (height < nDecemberHardforkHeight + 10)
        {
            testChain.generateBlock();
            EXPECT_TRUE( komodo_hardfork_active(0) );
            height = testChain.GetIndex()->nHeight;
        }
    }
    {
        // a notary miner should set ExtraNonce to 1 after hardfork
        // This is hard to test as it is in miner.cpp::BitcoinMiner()
        // TODO: Break that method apart. Note that some of that code
        // looks to be a duplicate of rpc/mining.cpp::generate()
        TestChain testChain;
        // at first, anything within nExtraNonce is valid
        std::shared_ptr<TestWallet> notaryWallet = std::make_shared<TestWallet>( testChain.getNotaryKey() );
        for( int i = 0; i < nDecemberHardforkHeight; ++i)
        {
            auto block = testChain.BuildBlock(notaryWallet);
            unsigned int nExtraNonce = GetCoinbaseNonce(block);
            for (int x = 0; x < i % 5; ++x)
            {
                IncrementExtraNonce(&block, testChain.GetIndex(), nExtraNonce);
            }
            block = testChain.generateBlock(block);
            EXPECT_EQ( GetCoinbaseNonce(block), nExtraNonce);
        }
        // progress past hardfork
        for( int i = chainActive.Height(); i < nDecemberHardforkHeight+10; ++i)
        {
            auto block = testChain.BuildBlock(notaryWallet);
            unsigned int nExtraNonce = GetCoinbaseNonce(block);
            for (int x = 0; x < i % 5; ++x)
            {
                IncrementExtraNonce(&block, testChain.GetIndex(), nExtraNonce);
            }
            block = testChain.generateBlock(block);
            EXPECT_EQ( GetCoinbaseNonce(block), nExtraNonce);
        }
        // TODO: after another hardfork, even non-notaries should not have the nonce
        // set to anything but 1.
    }
    */
    {
        // with asset chains, time is used instead of block height
        strcpy(ASSETCHAINS_SYMBOL, "JMJ");
        
        TestChain testChain;
        EXPECT_FALSE( komodo_hardfork_active(0) );
        EXPECT_FALSE( komodo_hardfork_active( Params().StakedDecemberHardforkTimestamp() ) );
        EXPECT_TRUE(  komodo_hardfork_active( Params().StakedDecemberHardforkTimestamp() + 1) );
        EXPECT_TRUE(  komodo_hardfork_active( Params().StakedDecemberHardforkTimestamp() + 10) );
        ASSETCHAINS_SYMBOL[0] = 0;
    }
    // test komodo_newStakerActive based on timestamp
    {
        TestChain testChain;
        auto notary = std::make_shared<TestWallet>(testChain.getNotaryKey(), "notary");
        auto alice = std::make_shared<TestWallet>("alice");
        auto height = testChain.GetIndex()->nHeight;
        uint32_t timestamp = 0;
        // both are too low
        EXPECT_FALSE( komodo_newStakerActive(height, timestamp) );
        // timestamp is the same as activation timestamp
        timestamp = Params().StakedDecemberHardforkTimestamp();
        EXPECT_FALSE( komodo_newStakerActive(height, timestamp) );
        // timestamp is 1 more than activation timestamp
        ++timestamp;
        EXPECT_TRUE( komodo_newStakerActive(height, timestamp) );
        // now base on heights
        timestamp = 0;
        EXPECT_FALSE( komodo_newStakerActive(height, timestamp) );
        // because we are well beyond the hardfork time, making just 
        // 1 block should cause the hardfork
        testChain.generateBlock(alice);
        height = testChain.GetIndex()->nHeight;
        EXPECT_TRUE( komodo_newStakerActive(height, timestamp) );
    }
    {
        // the CDiskBlockIndex adds segid to serialization if
        // the chain is staked and the hardfork has happened
        // for non-staked chains, CDiskBlockIndex does not change
        ASSETCHAINS_STAKED = 0;
        CDiskBlockIndex idx;
        idx.nTime = 0;
        CDataStream s1(SER_DISK, CLIENT_VERSION);
        idx.Serialize(s1);
        idx.nTime = Params().StakedDecemberHardforkTimestamp() + 10;
        CDataStream s2(SER_DISK, CLIENT_VERSION);
        idx.Serialize(s2);
        EXPECT_EQ(s2.size(), s1.size());
        // for staked chains, the serialized CDiskBlockIndex includes segid
        ASSETCHAINS_STAKED = 1;
        CDiskBlockIndex idx2;
        idx2.nTime = Params().StakedDecemberHardforkTimestamp(); // still no hardfork until next second
        CDataStream s3(SER_DISK, CLIENT_VERSION);
        idx2.Serialize(s3);
        idx2.nTime = Params().StakedDecemberHardforkTimestamp() + 10;
        CDataStream s4(SER_DISK, CLIENT_VERSION);
        idx2.Serialize(s4);
        EXPECT_EQ(s2.size(), s3.size());
        // hardfork happened, s4 should be 1 byte bigger
        EXPECT_EQ(s4.size(), s3.size() + 1);
    }
}

/***
 * Prints out some details of a block
 */
void displayBlock(const TestChain& testChain, std::shared_ptr<CBlock> block, bool withTransactions = false)
{
    static uint32_t lastBlockTime;
    auto height = testChain.GetIndex()->nHeight;
    uint32_t currentBlockTime = time(nullptr);
    if (height == 1)
        lastBlockTime = currentBlockTime;
    std::cout << "Block " << block->GetHash().ToString() 
            << " contains " << std::to_string( block->vtx.size() ) 
            << " transactions, and a difficulty of " 
            << std::to_string( block->GetBlockHeader().nBits)
            << ". There are " << std::to_string( currentBlockTime - lastBlockTime)
            << " seconds between the last block and this one.\n";
    lastBlockTime = currentBlockTime;
    if (withTransactions)
        for (auto& tx : block->vtx)
        {
            std::cout << " Transaction hash: " << tx.GetHash().ToString() << "\n";
            std::cout << "  Inputs:\n";
            for( auto i : tx.vin)
            {
                std::cout << "  Hash of prevout: " << i.prevout.hash.ToString() << "\n";
            }
            std::cout << "  Outputs:\n";
            for( auto o : tx.vout )
            {
                std::cout << "  Hash of out: " << o.GetHash().ToString() << "\n";
            }
        }
}

TEST(TestNotary, DISABLED_NotaryMining)
{
    /***
     * This test proves the ability for notary nodes to mine
     * blocks at reduced difficulty. It is disabled due to needing
     * to build a large number of blocks (67) before the real test
     * can begin.
     */
    // setup LogPrint logging
    fDebug = true;
    fPrintToConsole = true;
    mapMultiArgs["-debug"] = testing::internal::Strings{"mining", "pow"};
    TestChain testChain;
    std::shared_ptr<TestWallet> notary = std::make_shared<TestWallet>(testChain.getNotaryKey(), "notary");
    std::shared_ptr<TestWallet> alice = std::make_shared<TestWallet>("alice");

    // Alice should mine some blocks
    std::shared_ptr<CBlock> lastBlock;
    uint32_t lastHeight = testChain.GetIndex()->nHeight;
    for(int i = 0; i < 67; ++i)
    {
        lastBlock = testChain.generateBlock(alice);
        lastHeight = testChain.GetIndex()->nHeight;
        // this makes some txs for notary mining
        if (i > 1 && i < 55) // going above overwinter (ht.61) makes existing transactions invalid (need to research)
        {
            try
            {
                alice->Transfer(notary, 10000);
            }
            catch(const std::logic_error& le)
            {
                FAIL() << "Unable to transfer from Alice to Notary: " << le.what() << "\n";
            }
        }
    }
    uint32_t prevBits = lastBlock->GetBlockHeader().nBits;
    // a notary should be able to mine with a lower difficulty
    lastBlock = testChain.generateBlock(notary);
    EXPECT_EQ(testChain.GetIndex()->nHeight, lastHeight + 1);
    /* 
       nBits is not an indication that THIS block has a lower difficulty.
       nBits it set based on the needed difficulty. If a notary is able to
       do it at a lower difficulty, it does. But nBits is not adjusted. 
       Instead, code within CheckProofOfWork "does the right thing"
       and verifies that if this block is from a notary, and it is their turn,
       the PoW complies with at least the minimum hash target.
    */
    arith_uint256 bnTarget;
    bnTarget.SetCompact(lastBlock->GetBlockHeader().nBits);
    arith_uint256 actual = UintToArith256( lastBlock->GetBlockHeader().GetHash() );
    arith_uint256 powLimit = UintToArith256(Params().GetConsensus().powLimit);
    // The PoW algo requires that the computed hash is less (arithmetically) than the limit
    EXPECT_GT( powLimit, bnTarget ); // the lowest allowed network limit is greater than the block nBits
    EXPECT_GT( powLimit, actual ); // the lowest allowed network limit is greater than the actual mined hash
    EXPECT_GT( bnTarget, actual ); // the nBits in the header is greater than the actual mined hash
    lastHeight = testChain.GetIndex()->nHeight;
    // a non-notary should be back at the regular difficulty
    lastBlock = testChain.generateBlock(alice);
    EXPECT_EQ( testChain.GetIndex()->nHeight, lastHeight + 1);
}

/***
 * Checking the Genesis block
 */
TEST(TestNotary, DISABLED_GenesisBlock)
{
    TestChain testChain;
    std::shared_ptr<TestWallet> notary = std::make_shared<TestWallet>( testChain.getNotaryKey(), "notary" );
    std::shared_ptr<TestWallet> alice = std::make_shared<TestWallet>("alice");
    CBlock genesis = Params().GenesisBlock();
    genesis.nNonce = ArithToUint256((UintToArith256(genesis.nNonce)-1));
    ASSERT_TRUE( CalcPoW(&genesis) );
    // display solution and nonce
    std::cout << "Nonce: " << genesis.nNonce.ToString() << "\n";
    std::cout << "Solution: " << genesis.GetBlockHeader().hashMerkleRoot.ToString() << "\n";

    // attempt to mine blocks
    for(int i = 0; i < 2; ++i)
    {
        std::cout << "Mining block " << std::to_string(i+1) << "...\n";
        testChain.generateBlock( alice );
    }
}

/****
 * Checking some things in the TestWallet
 */
TEST(TestNotary, DISABLED_Wallet)
{
    TestChain testChain;
    std::shared_ptr<TestWallet> notary = std::make_shared<TestWallet>(testChain.getNotaryKey(), "notary");
    std::shared_ptr<TestWallet> alice = std::make_shared<TestWallet>("alice");

    // Alice should mine some blocks
    std::shared_ptr<CBlock> lastBlock;
    for(int i = 0; i < 20; ++i)
    {
        lastBlock = testChain.generateBlock(alice);
        displayBlock(testChain, lastBlock, true);
        // this makes some txs for notary mining
        if (i > 0)
        {
            try
            {
                alice->Transfer(notary, 10000);
            }
            catch(const std::logic_error& le)
            {
                FAIL() << "Unable to transfer from Alice to Notary: " << le.what() << "\n";
            }
        }
        }
    uint32_t prevBits = lastBlock->GetBlockHeader().nBits;
    // a notary should be able to mine with a lower difficulty
    lastBlock = testChain.generateBlock(notary);
    displayBlock(testChain, lastBlock);
    auto notaryBits = lastBlock->GetBlockHeader().nBits;
    EXPECT_LT(notaryBits, prevBits);
    // a non-notary should be back at the regular difficulty
    lastBlock = testChain.generateBlock(alice);
    displayBlock(testChain, lastBlock);
    EXPECT_GT(prevBits, notaryBits);
}

} // namespace TestNotary