#include <gtest/gtest.h>
#include "testutils.h"
#include "miner.h"

extern CWallet *pwalletMain;

TEST(TestStaked, komodo_staked)
{
    // set up the environment
    ASSETCHAINS_SYMBOL[0] = 'A';
    mapArgs["-gen"] = "1";
    TestChain chain;
    std::shared_ptr<TestWallet> notary = std::make_shared<TestWallet>(chain.getNotaryKey(), "notary");
    pwalletMain = notary.get();

    // generate some blocks
    for(auto i = 0; i < 100; ++i)
        chain.generateBlock(notary);
    
    // now test to make sure komodo_staked works...
    {
        CReserveKey reservekey(notary.get());
        LOCK(cs_main);
        CBlockTemplate *ptr = CreateNewBlockWithKey(reservekey, chain.GetIndex()->nHeight + 1, 0, true);
        ASSERT_TRUE( ptr != nullptr);
        ASSERT_EQ( ptr->block.vtx.size(), 2);
        ASSERT_EQ( ptr->block.vtx[1].vout.size(), 2);
        ASSERT_EQ( ptr->block.vtx[1].vout[1].nValue, 0);
    }

    // clean up the environment
    ASSETCHAINS_SYMBOL[0] = 0;
    pwalletMain = nullptr;
    mapArgs.erase("-gen");
}
