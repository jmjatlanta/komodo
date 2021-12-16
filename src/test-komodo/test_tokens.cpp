#include "cc/CCtokens.h"
#include "testutils.h"
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <iomanip>

extern CWallet *pwalletMain;

namespace TestTokens
{

std::string to_string(const UniValue& in)
{
    std::stringstream ss;
    if (in.isObject())
    {
        auto keys = in.getKeys();
        auto values = in.getValues();
        for(uint32_t i = 0; i < in.size(); ++i)
        {
            auto key = keys[i];
            auto value = values[i];
            ss << "Key: " << key << " Value: " << to_string(value) << "\n";
        }
    }
    if (in.isStr())
        ss << in.get_str();
    if (in.isNum())
        ss << std::to_string(in.get_real());
    return ss.str();
}

std::string to_string(const CPubKey& in)
{
    std::stringstream pubkeystream;
    std::for_each(in.begin(), in.end(), [&pubkeystream](const unsigned char c){ pubkeystream << std::hex << std::setw(2) << std::setfill('0') << (int)c; });
    return pubkeystream.str();
}


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
    std::shared_ptr<TestWallet> alice = chain.AddWallet("alice");
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

TEST(TestTokens, CreateToken)
{
    ASSETCHAINS_CC = 1;
    mapArgs["-addressindex"] = "1";
    mapArgs["-spentindex"] = "1";
    TestChain chain;
    // make some test wallets
    std::shared_ptr<TestWallet> alice = chain.AddWallet("alice");
    std::shared_ptr<TestWallet> notary = chain.AddWallet(chain.getNotaryKey());
    notary->SetAsMain();
    chain.generateBlock();
    notary->Transfer(alice, 100000);
    chain.generateBlock();

    // create token
    UniValue createTokenResult;
    {
        UniValue createToken(UniValue::VARR);
        createToken.push_back( UniValue(UniValue::VSTR, "TESTTOKEN"));
        createToken.push_back( UniValue(UniValue::VSTR, "100000"));
        createTokenResult = tokencreate(createToken, false, notary->GetPubKey());
        EXPECT_EQ(createTokenResult["result"].get_str(), "success");
        UniValue send(UniValue::VARR);
        send.push_back( createTokenResult["hex"] );
        createTokenResult = sendrawtransaction(send, false, notary->GetPubKey());
        EXPECT_GT(createTokenResult.get_str().size(), 63);
        std::cout << "createToken txid: " << to_string(createTokenResult) << "\n";
        chain.generateBlock();
    }

    // transfer some tokens to Alice
    {
        UniValue transfer(UniValue::VARR);
        transfer.push_back( createTokenResult );
        transfer.push_back( UniValue(UniValue::VSTR, to_string(alice->GetPubKey())));
        transfer.push_back( UniValue(UniValue::VSTR, "100000") );
        UniValue transferResult = tokentransfer(transfer, false, notary->GetPubKey());
        EXPECT_EQ(transferResult["result"].get_str(), "success");
        UniValue send(UniValue::VARR);
        send.push_back( transferResult["hex"] );
        transferResult = sendrawtransaction(send, false, notary->GetPubKey() );
        EXPECT_GT( transferResult.get_str().size(), 63 );
    }

}

} // namespace TestTokens