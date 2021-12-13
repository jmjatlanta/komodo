#include "cc/CCtokens.h"
#include "testutils.h"
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <iomanip>

extern CWallet *pwalletMain;

namespace TestTokens
{

TEST(TestTokens, TestMakeCC1vout)
{
    // Test equality of a CTxOut
    CPubKey pubKey;
    CAmount amount = 100000;
    uint8_t evalCode = EVAL_ASSETS;
    CTxOut tokenOut1 = CCTokens::MakeCC1vout(evalCode, amount, pubKey);
    CTxOut tokenOut2 = CCTokens::MakeCC1vout(evalCode, amount, pubKey);
    EXPECT_EQ(tokenOut1, tokenOut2);
    CTxOut tokenOut3 = CCTokens::MakeCC1vout(evalCode, amount + 1, pubKey);
    EXPECT_NE(tokenOut1, tokenOut3);

    // the MakeCC1vout of CCTokens should be different than the generic MakeCC1vout
    CTxOut genericOut1 = MakeCC1vout(evalCode, amount, pubKey);
    EXPECT_NE(tokenOut1, genericOut1);

    // make sure CreateToken is using the right MakeCC1vout
    std::vector<unsigned char> expected;
    ASSETCHAINS_CC = 1;
    mapArgs["-addressindex"] = "1";
    TestChain chain;
    // set pwalletMain
    std::shared_ptr<CWallet> notaryWallet = std::make_shared<CWallet>();
    notaryWallet->AddKeyPubKey(chain.getNotaryKey(), chain.getNotaryKey().GetPubKey());
    pwalletMain = notaryWallet.get();
    // make some wallets
    std::shared_ptr<TestWallet> notary = chain.AddWallet(chain.getNotaryKey());
    std::shared_ptr<TestWallet> alice = chain.AddWallet();
    chain.generateBlock();
    notary->Transfer(alice, 100000);
    chain.generateBlock();
    std::string result1;
    std::string result2;
    {
        result1 = CCTokens::CreateToken(1, 1000000, "MyToken", "My Test Token", 
                std::vector<unsigned char>());
        /*
        std::cout << "String width: " << std::to_string(result1.size()) << std::endl;
        for(auto c : result1)
        {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)c << " ";
        }
        std::cout << std::endl;
        */
    }
    {
        result2 = CCTokens::CreateToken(1, 1000000, "MyToken", "My Test Token", 
                std::vector<unsigned char>());
    }
    EXPECT_EQ(result1, result2);
}

} // namespace TestTokens