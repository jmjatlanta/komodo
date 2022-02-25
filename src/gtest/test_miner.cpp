#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "chainparams.h"
#include "key.h"
#include "miner.h"
#include "util.h"
#include "wallet/wallet.h"

#include <boost/optional.hpp>

using ::testing::Return;

class MockReserveKey : public CReserveKey {
public:
    MockReserveKey() : CReserveKey(nullptr) { }

    MOCK_METHOD1(GetReservedKey, bool(CPubKey &pubkey));
};

TEST(Miner, GetMinerScriptPubKey) {
    SelectParams(CBaseChainParams::MAIN);

    boost::optional<CScript> scriptPubKey;
    MockReserveKey reservekey;
    EXPECT_CALL(reservekey, GetReservedKey(::testing::_))
        .WillRepeatedly(Return(false));

    // No miner address set
    scriptPubKey = GetMinerScriptPubKey(reservekey);
    EXPECT_FALSE((bool) scriptPubKey);

    mapArgs["-mineraddress"] = "notAnAddress";
    scriptPubKey = GetMinerScriptPubKey(reservekey);
    EXPECT_FALSE((bool) scriptPubKey);

    // Partial address
    mapArgs["-mineraddress"] = "t1T8yaLVhNqxA5KJcmiqq";
    scriptPubKey = GetMinerScriptPubKey(reservekey);
    EXPECT_FALSE((bool) scriptPubKey);

    // Typo in address
    mapArgs["-mineraddress"] = "t1TByaLVhNqxA5KJcmiqqFN88e8DNp2PBfF";
    scriptPubKey = GetMinerScriptPubKey(reservekey);
    EXPECT_FALSE((bool) scriptPubKey);

    // Set up expected scriptPubKey for t1T8yaLVhNqxA5KJcmiqqFN88e8DNp2PBfF
    CKeyID keyID;
    keyID.SetHex("eb88f1c65b39a823479ac9c7db2f4a865960a165");
    CScript expectedScriptPubKey = CScript() << OP_DUP << OP_HASH160 << ToByteVector(keyID) << OP_EQUALVERIFY << OP_CHECKSIG;

    // Valid address
    mapArgs["-mineraddress"] = "t1T8yaLVhNqxA5KJcmiqqFN88e8DNp2PBfF";
    scriptPubKey = GetMinerScriptPubKey(reservekey);
    EXPECT_TRUE((bool) scriptPubKey);
    EXPECT_EQ(expectedScriptPubKey, *scriptPubKey);

    // Valid address with leading whitespace
    mapArgs["-mineraddress"] = "  t1T8yaLVhNqxA5KJcmiqqFN88e8DNp2PBfF";
    scriptPubKey = GetMinerScriptPubKey(reservekey);
    EXPECT_TRUE((bool) scriptPubKey);
    EXPECT_EQ(expectedScriptPubKey, *scriptPubKey);

    // Valid address with trailing whitespace
    mapArgs["-mineraddress"] = "t1T8yaLVhNqxA5KJcmiqqFN88e8DNp2PBfF  ";
    scriptPubKey = GetMinerScriptPubKey(reservekey);
    EXPECT_TRUE((bool) scriptPubKey);
    EXPECT_EQ(expectedScriptPubKey, *scriptPubKey);
}
