#include "cc/CCHeir.h"
#include "testutils.h"
#include "rpc/server.h"
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <iomanip>
#include <thread>

namespace TestHeir
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

TEST(TestHeir, TestHeirCoin)
{
    ASSETCHAINS_CC = 1;
    mapArgs["-addressindex"] = "1";
    mapArgs["-spentindex"] = "1";
    TestChain chain;
    // make some test wallets
    std::shared_ptr<TestWallet> alice = chain.AddWallet("alice");
    std::shared_ptr<TestWallet> notary = chain.AddWallet(chain.getNotaryKey());
    notary->SetAsMain();
    // now switching wallets should be a matter of changing pwalletMain
    chain.generateBlock();
    notary->Transfer(alice, 100000);
    chain.generateBlock();

    // get some basic info about heirs
    UniValue params(UniValue::VARR);
    params.push_back( UniValue(UniValue::VSTR, to_string(notary->GetPubKey())));
    UniValue hAddress = heiraddress(params, false, notary->GetPubKey());
    std::cout << "Heir Data:\n" << to_string(hAddress) << "\n";

    // Alice is the heir to the notary
    UniValue createHeir(UniValue::VARR);
    createHeir.push_back(UniValue(UniValue::VSTR, "1000"));
    createHeir.push_back(UniValue(UniValue::VSTR, "UNITHEIR"));
    createHeir.push_back(UniValue(UniValue::VSTR, to_string(alice->GetPubKey())));
    createHeir.push_back(UniValue(UniValue::VSTR, "10"));
    createHeir.push_back(UniValue(UniValue::VSTR, "TESTMEMO"));
    // this generates the needed tx data, but does not send
    UniValue fundResult = heirfund(createHeir, false, notary->GetPubKey());
    EXPECT_EQ(fundResult["result"].get_str(), "success");

    // now we send
    UniValue send(UniValue::VARR);
    send.push_back( fundResult["hex"] );
    fundResult = sendrawtransaction(send, false, notary->GetPubKey() );
    EXPECT_GT(fundResult.get_str().size(), 63);
    chain.generateBlock();

    // get heirinfo
    {
        UniValue send(UniValue::VARR);
        send.push_back( fundResult );
        UniValue infoResult = heirinfo(send, false, notary->GetPubKey());
        std::cout << "HeirInfo:\n" << to_string(infoResult) << "\n";
    }

    // Alice tries to get the funds immediately (this should fail)
    alice->SetAsMain();
    UniValue claimHeir(UniValue::VARR);
    claimHeir.push_back( UniValue(UniValue::VSTR, "1000"));
    claimHeir.push_back( fundResult ); // the txid
    UniValue claimResult = heirclaim(claimHeir, false, alice->GetPubKey() );
    EXPECT_EQ(claimResult["result"].get_str(), "error");
    EXPECT_EQ(claimResult["error"].get_str(), "spending is not allowed yet for the heir");

    // wait 10 blocks
    for(int i = 0; i < 10; ++i)
        chain.generateBlock();

    // now Alice tries again
    claimResult = heirclaim(claimHeir, false, alice->GetPubKey() );
    EXPECT_EQ(claimResult["result"].get_str(), "success");
}

TEST(TestHeir, TestHeirToken)
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
        chain.generateBlock();
    }

    // Alice is the heir to the notary
    UniValue fundResult;
    {
        UniValue createHeir(UniValue::VARR);
        createHeir.push_back(UniValue(UniValue::VSTR, "1000"));
        createHeir.push_back(UniValue(UniValue::VSTR, "UNITHEIR"));
        createHeir.push_back(UniValue(UniValue::VSTR, to_string(alice->GetPubKey())));
        createHeir.push_back(UniValue(UniValue::VSTR, "10"));
        createHeir.push_back(UniValue(UniValue::VSTR, "TESTMEMO"));
        createHeir.push_back(createTokenResult);
        // this generates the needed tx data, but does not send
        fundResult = heirfund(createHeir, false, notary->GetPubKey());
        EXPECT_EQ(fundResult["result"].get_str(), "success");
    }

    // now we send
    {
        UniValue send(UniValue::VARR);
        send.push_back( fundResult["hex"] );
        fundResult = sendrawtransaction(send, false, notary->GetPubKey() );
        EXPECT_GT(fundResult.get_str().size(), 63);
        chain.generateBlock();
    }

    // get heirinfo
    {
        UniValue send(UniValue::VARR);
        send.push_back( fundResult );
        UniValue infoResult = heirinfo(send, false, notary->GetPubKey());
        std::cout << "HeirInfo:\n" << to_string(infoResult) << "\n";
    }

    // Alice tries to get the funds immediately (this should fail)
    alice->SetAsMain();
    UniValue claimHeir(UniValue::VARR);
    claimHeir.push_back( UniValue(UniValue::VSTR, "1000"));
    claimHeir.push_back( fundResult ); // the txid
    UniValue claimResult = heirclaim(claimHeir, false, alice->GetPubKey() );
    EXPECT_EQ(claimResult["result"].get_str(), "error");
    EXPECT_EQ(claimResult["error"].get_str(), "spending is not allowed yet for the heir");

    // wait 10 blocks
    for(int i = 0; i < 10; ++i)
        chain.generateBlock();

    // now Alice tries again
    claimResult = heirclaim(claimHeir, false, alice->GetPubKey() );
    EXPECT_EQ(claimResult["result"].get_str(), "success");
}

} // namespace TestHeir