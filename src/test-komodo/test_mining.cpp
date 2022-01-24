#include "chainparams.h"
#include "pow.h"
#include "net.h" // for CNode testing
#include <gtest/gtest.h>

bool CalcPoW(CBlock *pblock);

namespace TestMining
{

void printVector(const std::vector<uint8_t>& in, std::ostream& out)
{
    std::for_each(in.begin(), in.end(), [&out](const uint8_t val) {
        char c[3];
        sprintf(c, "%02x", val);
        out << c;
    });
}

/****
 * Mine the genesis block to verify the solution, nonce, and hash match
 */
void MineAndCheckGenesis(CBaseChainParams::Network network)
{
    SelectParams(network);

    const CChainParams& params = Params();

    const CBlock& const_genesis = params.GenesisBlock();
    CBlock& genesis = const_cast<CBlock&>(const_genesis);
    uint256 originalHash = genesis.GetHash();
    arith_uint256 originalNonce = arith_uint256( genesis.nNonce.GetHex() );
    std::vector<uint8_t> originalSolution = genesis.nSolution;

    // CalcPoW always bumps the genesis by at least one, so back it up by one
    genesis.nNonce = ArithToUint256(originalNonce - 1);
    EXPECT_TRUE( CalcPoW(&genesis) );

    EXPECT_EQ( originalHash, genesis.GetHash() );
    EXPECT_EQ( originalSolution, genesis.nSolution );
    EXPECT_EQ( originalNonce.GetHex(), genesis.nNonce.GetHex());
    std::cout << "New Solution: ";
    printVector(genesis.nSolution, std::cout);
    std::cout << "\n";
}

TEST(TestMining, GenesisBlock)
{
    MineAndCheckGenesis(CBaseChainParams::TESTNET);
    //CheckHash(CBaseChainParams::REGTEST);
}

TEST(TestMining, GenesisSolution)
{
    const CChainParams& params = Params();
    const CBlock& genesis = params.GenesisBlock();
    EXPECT_TRUE( CheckEquihashSolution(&genesis, params) );
}

TEST(TestMining, TestWhitelist)
{
    std::string testString("192.168.0.0/16");
    CSubNet net1(testString);
    CNode::AddWhitelistedRange(net1);
    CNetAddr addr1("192.168.0.6");
    EXPECT_TRUE(CNode::IsWhitelistedRange( addr1 ));
    addr1 = CNetAddr("192.168.254.254");
    EXPECT_TRUE(CNode::IsWhitelistedRange( addr1 ));
    CNetAddr addr2("192.169.0.6");
    EXPECT_FALSE(CNode::IsWhitelistedRange( addr2 ));
}

} // namespace TestMining