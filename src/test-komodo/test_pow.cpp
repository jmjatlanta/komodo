
#include "chain.h"
#include "chainparams.h"
#include "pow.h"
#include "random.h"
#include <gtest/gtest.h>

TEST(PoW, DifficultyAveraging) {
    SelectParams(CBaseChainParams::MAIN);
    const Consensus::Params& params = Params().GetConsensus();
    size_t lastBlk = 2*params.nPowAveragingWindow;
    size_t firstBlk = lastBlk - params.nPowAveragingWindow;

    // Start with blocks evenly-spaced and equal difficulty
    std::vector<CBlockIndex> blocks(lastBlk+1);
    for (int i = 0; i <= lastBlk; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].SetHeight(i);
        blocks[i].nTime = 1269211443 + i * params.nPowTargetSpacing;
        blocks[i].nBits = 0x1e7fffff; /* target 0x007fffff000... */
        blocks[i].chainPower = i ? (CChainPower(&blocks[i]) + blocks[i - 1].chainPower) + GetBlockProof(blocks[i - 1]) : CChainPower(&blocks[i]);
    }

    // Result should be the same as if last difficulty was used
    arith_uint256 bnAvg;
    bnAvg.SetCompact(blocks[lastBlk].nBits);
    EXPECT_EQ(CalculateNextWorkRequired(bnAvg,
                                        blocks[lastBlk].GetMedianTimePast(),
                                        blocks[firstBlk].GetMedianTimePast(),
                                        params),
              GetNextWorkRequired(&blocks[lastBlk], nullptr, params));
    // Result should be unchanged, modulo integer division precision loss
    arith_uint256 bnRes;
    bnRes.SetCompact(0x1e7fffff);
    bnRes /= params.AveragingWindowTimespan();
    bnRes *= params.AveragingWindowTimespan();
    EXPECT_EQ(bnRes.GetCompact(), GetNextWorkRequired(&blocks[lastBlk], nullptr, params));

    // Randomise the final block time (plus 1 to ensure it is always different)
    blocks[lastBlk].nTime += GetRand(params.nPowTargetSpacing/2) + 1;

    // Result should be the same as if last difficulty was used
    bnAvg.SetCompact(blocks[lastBlk].nBits);
    EXPECT_EQ(CalculateNextWorkRequired(bnAvg,
                                        blocks[lastBlk].GetMedianTimePast(),
                                        blocks[firstBlk].GetMedianTimePast(),
                                        params),
              GetNextWorkRequired(&blocks[lastBlk], nullptr, params));
    // Result should not be unchanged
    EXPECT_NE(0x1e7fffff, GetNextWorkRequired(&blocks[lastBlk], nullptr, params));

    // Change the final block difficulty
    blocks[lastBlk].nBits = 0x1e0fffff;

    // Result should not be the same as if last difficulty was used
    bnAvg.SetCompact(blocks[lastBlk].nBits);
    EXPECT_NE(CalculateNextWorkRequired(bnAvg,
                                        blocks[lastBlk].GetMedianTimePast(),
                                        blocks[firstBlk].GetMedianTimePast(),
                                        params),
              GetNextWorkRequired(&blocks[lastBlk], nullptr, params));

    // Result should be the same as if the average difficulty was used
    arith_uint256 average = UintToArith256(uint256S("0000796968696969696969696969696969696969696969696969696969696969"));
    EXPECT_EQ(CalculateNextWorkRequired(average,
                                        blocks[lastBlk].GetMedianTimePast(),
                                        blocks[firstBlk].GetMedianTimePast(),
                                        params),
              GetNextWorkRequired(&blocks[lastBlk], nullptr, params));
}

TEST(PoW, MinDifficultyRules) {
    SelectParams(CBaseChainParams::TESTNET);
    const Consensus::Params& params = Params().GetConsensus();
    size_t lastBlk = 2*params.nPowAveragingWindow;
    const uint32_t startTime = 1269211443;

    // Start with blocks evenly-spaced and equal difficulty
    std::vector<CBlockIndex> blocks(lastBlk+1);
    uint32_t nextTime = startTime;
    for (int i = 0; i <= lastBlk; i++) {
        nextTime = nextTime + params.nPowTargetSpacing;
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].SetHeight(params.nPowAllowMinDifficultyBlocksAfterHeight.get() + i);
        blocks[i].nTime = nextTime;
        blocks[i].nBits = 0x1e7fffff; /* target 0x007fffff000... */
        blocks[i].chainPower.chainWork = i ? blocks[i - 1].chainPower.chainWork 
                + GetBlockProof(blocks[i - 1]).chainWork : arith_uint256(0);
    }

    // Create a new block at the target spacing
    CBlockHeader next;
    next.nTime = blocks[lastBlk].nTime + params.nPowTargetSpacing;

    // Result should be unchanged, modulo integer division precision loss
    arith_uint256 bnRes;
    bnRes.SetCompact(0x1e7fffff);
    bnRes /= params.AveragingWindowTimespan();
    bnRes *= params.AveragingWindowTimespan();
    EXPECT_EQ(GetNextWorkRequired(&blocks[lastBlk], &next, params), bnRes.GetCompact());

    // Delay last block a bit, time warp protection should prevent any change
    next.nTime += params.nPowTargetSpacing * 5;

    // Result should be unchanged, modulo integer division precision loss
    EXPECT_EQ(GetNextWorkRequired(&blocks[lastBlk], &next, params), bnRes.GetCompact());

    // Delay last block to a huge number. Result should be unchanged, time warp protection
    next.nTime = std::numeric_limits<uint32_t>::max();
    EXPECT_EQ(GetNextWorkRequired(&blocks[lastBlk], &next, params), bnRes.GetCompact());

    // space all blocks out so the median is above the limits and difficulty should drop
    nextTime = startTime;
    for (int i = 0; i <= lastBlk; i++) {
        nextTime = nextTime + ( params.MaxActualTimespan() / params.nPowAveragingWindow + 1);
        blocks[i].nTime = nextTime;
        blocks[i].chainPower.chainWork = i ? blocks[i - 1].chainPower.chainWork 
                + GetBlockProof(blocks[i - 1]).chainWork : arith_uint256(0);
    }

    // difficulty should have decreased ( nBits increased )
    EXPECT_GT(GetNextWorkRequired(&blocks[lastBlk], &next, params),
            bnRes.GetCompact());

    // diffuculty should never decrease below minimum
    arith_uint256 minWork = UintToArith256(params.powLimit);
    for (int i = 0; i <= lastBlk; i++) {
        blocks[i].nBits = minWork.GetCompact();
        blocks[i].chainPower.chainWork = i ? blocks[i - 1].chainPower.chainWork 
                + GetBlockProof(blocks[i - 1]).chainWork : arith_uint256(0);
    }
    EXPECT_EQ(GetNextWorkRequired(&blocks[lastBlk], &next, params), minWork.GetCompact());

    // space all blocks out so the median is under limits and difficulty should increase
    nextTime = startTime;
    for (int i = 0; i <= lastBlk; i++) {
        nextTime = nextTime + (params.MinActualTimespan() / params.nPowAveragingWindow - 1);
        blocks[i].nTime = nextTime;
        blocks[i].nBits = 0x1e7fffff; /* target 0x007fffff000... */
        blocks[i].chainPower.chainWork = i ? blocks[i - 1].chainPower.chainWork 
                + GetBlockProof(blocks[i - 1]).chainWork : arith_uint256(0);
    }

    // difficulty should have increased ( nBits decreased )
    EXPECT_LT(GetNextWorkRequired(&blocks[lastBlk], &next, params),
            bnRes.GetCompact());

}

arith_uint256 zawy_TSA_EMA(int32_t height,int32_t tipdiff,arith_uint256 prevTarget);

TEST(PoW, AdaptivePoW2)
{
    // tested with K==1000000 and T = ASSETCHAINS_BLOCKTIME (which was 60)
    int32_t height = 1;
    int32_t tipdiff = 1;
    arith_uint256 prevTarget;
    arith_uint256 expectedTarget;

    // heights with low targets do not matter
    arith_uint256 newTarget = zawy_TSA_EMA(height, tipdiff, prevTarget);
    EXPECT_EQ(prevTarget, newTarget);
    EXPECT_EQ(newTarget, expectedTarget);
    height++;
    for(; height < 2500; ++height)
    {
        newTarget = zawy_TSA_EMA(height, tipdiff, prevTarget);
        EXPECT_EQ(prevTarget, newTarget);
        EXPECT_EQ(expectedTarget, newTarget);
        prevTarget = newTarget;
    }

    // higher span to tip adjusts target lower
    prevTarget = 600000000;
    // Anything less than 4 is adjusted to 4
    arith_uint256 newTarget4 = zawy_TSA_EMA(height, 4, prevTarget);
    EXPECT_NE(newTarget4, arith_uint256());
    arith_uint256 newTarget100 = zawy_TSA_EMA(height, 100, prevTarget);
    EXPECT_LT(newTarget100, newTarget4);
    arith_uint256 newTarget1000 = zawy_TSA_EMA(height, 1000, prevTarget);
    EXPECT_NE(newTarget100, newTarget1000);
    EXPECT_LT(newTarget1000, newTarget100);
    EXPECT_EQ(newTarget1000, arith_uint256());
}
