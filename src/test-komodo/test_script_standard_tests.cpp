#include <gtest/gtest.h>
#include "key.h"
#include "keystore.h"
#include "wallet/wallet_ismine.h"
#include "script/script.h"
#include "script/script_error.h"
#include "script/standard.h"
#include "utilstrencodings.h"
#include "cc/CChtlc.h"
#include "testutils.h"

extern CKey notaryKey;

namespace TestScriptStandardTests {

    class TestScriptStandardTests : public ::testing::Test {};

    inline std::string txnouttypeToString(txnouttype t)
    {
        std::string res ;

        switch (t)
        {
            case TX_NONSTANDARD:
                res = "TX_NONSTANDARD";
                break;
            case TX_NULL_DATA:
                res = "TX_NULL_DATA";
                break;
            case TX_PUBKEY:
                res = "TX_PUBKEY";
                break;
            case TX_PUBKEYHASH:
                res = "TX_PUBKEYHASH";
                break;
            case TX_MULTISIG:
                res = "TX_MULTISIG";
                break;
            case TX_SCRIPTHASH:
                res = "TX_SCRIPTHASH";
                break;
            case TX_CRYPTOCONDITION:
                res = "TX_CRYPTOCONDITION";
                break;
            default:
                res = "UNKNOWN";
            }

        return res;
    }

    TEST(TestScriptStandardTests, script_standard_Solver_success) {

        CKey keys[3];
        CPubKey pubkeys[3];
        for (int i = 0; i < 3; i++) {
            keys[i].MakeNewKey(true);
            pubkeys[i] = keys[i].GetPubKey();
        }

        CScript s;
        txnouttype whichType;
        std::vector<std::vector<unsigned char> > solutions;

        // TX_PUBKEY
        s.clear();
        s << ToByteVector(pubkeys[0]) << OP_CHECKSIG;
        ASSERT_TRUE(Solver(s, whichType, solutions));
        ASSERT_EQ(whichType, TX_PUBKEY);
        ASSERT_EQ(solutions.size(), 1);
        ASSERT_TRUE(solutions[0] == ToByteVector(pubkeys[0]));

         // TX_PUBKEYHASH
        s.clear();
        s << OP_DUP << OP_HASH160 << ToByteVector(pubkeys[0].GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;
        ASSERT_TRUE(Solver(s, whichType, solutions));
        ASSERT_EQ(whichType, TX_PUBKEYHASH);
        ASSERT_EQ(solutions.size(), 1);
        ASSERT_TRUE(solutions[0] == ToByteVector(pubkeys[0].GetID()));

        // solutions.clear();

        // TX_SCRIPTHASH
        CScript redeemScript(s); // initialize with leftover P2PKH script
        s.clear();
        s << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;
        ASSERT_TRUE(Solver(s, whichType, solutions));
        ASSERT_EQ(whichType, TX_SCRIPTHASH);
        ASSERT_EQ(solutions.size(), 1);
        ASSERT_TRUE(solutions[0] == ToByteVector(CScriptID(redeemScript)));

        // TX_MULTISIG
        s.clear();
        s << OP_1 <<
            ToByteVector(pubkeys[0]) <<
            ToByteVector(pubkeys[1]) <<
            OP_2 << OP_CHECKMULTISIG;
        ASSERT_TRUE(Solver(s, whichType, solutions));
        ASSERT_EQ(whichType, TX_MULTISIG);
        ASSERT_EQ(solutions.size(), 4);
        ASSERT_TRUE(solutions[0] == std::vector<unsigned char>({1}));
        ASSERT_TRUE(solutions[1] == ToByteVector(pubkeys[0]));
        ASSERT_TRUE(solutions[2] == ToByteVector(pubkeys[1]));
        ASSERT_TRUE(solutions[3] == std::vector<unsigned char>({2}));

        s.clear();
        s << OP_2 <<
            ToByteVector(pubkeys[0]) <<
            ToByteVector(pubkeys[1]) <<
            ToByteVector(pubkeys[2]) <<
            OP_3 << OP_CHECKMULTISIG;
        ASSERT_TRUE(Solver(s, whichType, solutions));
        ASSERT_EQ(whichType, TX_MULTISIG);
        ASSERT_EQ(solutions.size(), 5);
        ASSERT_TRUE(solutions[0] == std::vector<unsigned char>({2}));
        ASSERT_TRUE(solutions[1] == ToByteVector(pubkeys[0]));
        ASSERT_TRUE(solutions[2] == ToByteVector(pubkeys[1]));
        ASSERT_TRUE(solutions[3] == ToByteVector(pubkeys[2]));
        ASSERT_TRUE(solutions[4] == std::vector<unsigned char>({3}));

        // TX_NULL_DATA
        s.clear();
        s << OP_RETURN <<
            std::vector<unsigned char>({0}) <<
            std::vector<unsigned char>({75}) <<
            std::vector<unsigned char>({255});
        ASSERT_TRUE(Solver(s, whichType, solutions));
        ASSERT_EQ(whichType, TX_NULL_DATA);
        ASSERT_EQ(solutions.size(), 0);

        // TX_NONSTANDARD
        s.clear();
        s << OP_9 << OP_ADD << OP_11 << OP_EQUAL;
        ASSERT_TRUE(!Solver(s, whichType, solutions));
        ASSERT_EQ(whichType, TX_NONSTANDARD);

    }

    TEST(TestScriptStandardTests, script_standard_Solver_failure) {
        CKey key;
        CPubKey pubkey;
        key.MakeNewKey(true);
        pubkey = key.GetPubKey();

        CScript s;
        txnouttype whichType;
        std::vector<std::vector<unsigned char> > solutions;

        // TX_PUBKEY with incorrectly sized pubkey
        s.clear();
        s << std::vector<unsigned char>(30, 0x01) << OP_CHECKSIG;
        ASSERT_TRUE(!Solver(s, whichType, solutions));

        // TX_PUBKEYHASH with incorrectly sized key hash
        s.clear();
        s << OP_DUP << OP_HASH160 << ToByteVector(pubkey) << OP_EQUALVERIFY << OP_CHECKSIG;
        ASSERT_TRUE(!Solver(s, whichType, solutions));

        // TX_SCRIPTHASH with incorrectly sized script hash
        s.clear();
        s << OP_HASH160 << std::vector<unsigned char>(21, 0x01) << OP_EQUAL;
        ASSERT_TRUE(!Solver(s, whichType, solutions));

        // TX_MULTISIG 0/2
        s.clear();
        s << OP_0 << ToByteVector(pubkey) << OP_1 << OP_CHECKMULTISIG;
        ASSERT_TRUE(!Solver(s, whichType, solutions));

        // TX_MULTISIG 2/1
        s.clear();
        s << OP_2 << ToByteVector(pubkey) << OP_1 << OP_CHECKMULTISIG;
        ASSERT_TRUE(!Solver(s, whichType, solutions));

        // TX_MULTISIG n = 2 with 1 pubkey
        s.clear();
        s << OP_1 << ToByteVector(pubkey) << OP_2 << OP_CHECKMULTISIG;
        ASSERT_TRUE(!Solver(s, whichType, solutions));

        // TX_MULTISIG n = 1 with 0 pubkeys
        s.clear();
        s << OP_1 << OP_1 << OP_CHECKMULTISIG;
        ASSERT_TRUE(!Solver(s, whichType, solutions));

        // TX_NULL_DATA with other opcodes
        s.clear();
        s << OP_RETURN << std::vector<unsigned char>({75}) << OP_ADD;
        ASSERT_TRUE(!Solver(s, whichType, solutions));

        /* witness tests are absent, bcz Komodo doesn't support witness */
    }

    TEST(TestScriptStandardTests, script_standard_ExtractDestination) {

        CKey key;
        CPubKey pubkey;
        key.MakeNewKey(true);
        pubkey = key.GetPubKey();

        CScript s;
        CTxDestination address;

        // TX_PUBKEY
        s.clear();
        s << ToByteVector(pubkey) << OP_CHECKSIG;
        ASSERT_TRUE(ExtractDestination(s, address));
        ASSERT_TRUE(boost::get<CKeyID>(&address) &&
                    *boost::get<CKeyID>(&address) == pubkey.GetID());

        // TX_PUBKEYHASH
        s.clear();
        s << OP_DUP << OP_HASH160 << ToByteVector(pubkey.GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;
        ASSERT_TRUE(ExtractDestination(s, address));
        ASSERT_TRUE(boost::get<CKeyID>(&address) &&
                    *boost::get<CKeyID>(&address) == pubkey.GetID());

        // TX_SCRIPTHASH
        CScript redeemScript(s); // initialize with leftover P2PKH script
        s.clear();
        s << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;
        ASSERT_TRUE(ExtractDestination(s, address));
        ASSERT_TRUE(boost::get<CScriptID>(&address) &&
                    *boost::get<CScriptID>(&address) == CScriptID(redeemScript));

        // TX_MULTISIG
        s.clear();
        s << OP_1 << ToByteVector(pubkey) << OP_1 << OP_CHECKMULTISIG;
        ASSERT_TRUE(!ExtractDestination(s, address));

        // TX_NULL_DATA
        s.clear();
        s << OP_RETURN << std::vector<unsigned char>({75});
        ASSERT_TRUE(!ExtractDestination(s, address));
    }

    TEST(TestScriptStandardTests, script_standard_ExtractDestinations) {

        CKey keys[3];
        CPubKey pubkeys[3];
        for (int i = 0; i < 3; i++) {
            keys[i].MakeNewKey(true);
            pubkeys[i] = keys[i].GetPubKey();
        }

        CScript s;
        txnouttype whichType;
        std::vector<CTxDestination> addresses;
        int nRequired;

        // TX_PUBKEY
        s.clear();
        s << ToByteVector(pubkeys[0]) << OP_CHECKSIG;
        ASSERT_TRUE(ExtractDestinations(s, whichType, addresses, nRequired));
        ASSERT_EQ(whichType, TX_PUBKEY);
        ASSERT_EQ(addresses.size(), 1);
        ASSERT_EQ(nRequired, 1);
        ASSERT_TRUE(boost::get<CKeyID>(&addresses[0]) &&
                    *boost::get<CKeyID>(&addresses[0]) == pubkeys[0].GetID());

        // TX_PUBKEYHASH
        s.clear();
        s << OP_DUP << OP_HASH160 << ToByteVector(pubkeys[0].GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;
        ASSERT_TRUE(ExtractDestinations(s, whichType, addresses, nRequired));
        ASSERT_EQ(whichType, TX_PUBKEYHASH);
        ASSERT_EQ(addresses.size(), 1);
        ASSERT_EQ(nRequired, 1);
        ASSERT_TRUE(boost::get<CKeyID>(&addresses[0]) &&
                    *boost::get<CKeyID>(&addresses[0]) == pubkeys[0].GetID());

        // TX_SCRIPTHASH
        CScript redeemScript(s); // initialize with leftover P2PKH script
        s.clear();
        s << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;
        ASSERT_TRUE(ExtractDestinations(s, whichType, addresses, nRequired));
        ASSERT_EQ(whichType, TX_SCRIPTHASH);
        ASSERT_EQ(addresses.size(), 1);
        ASSERT_EQ(nRequired, 1);
        ASSERT_TRUE(boost::get<CScriptID>(&addresses[0]) &&
                    *boost::get<CScriptID>(&addresses[0]) == CScriptID(redeemScript));

        // TX_MULTISIG
        s.clear();
        s << OP_2 <<
            ToByteVector(pubkeys[0]) <<
            ToByteVector(pubkeys[1]) <<
            OP_2 << OP_CHECKMULTISIG;
        ASSERT_TRUE(ExtractDestinations(s, whichType, addresses, nRequired));
        ASSERT_EQ(whichType, TX_MULTISIG);
        ASSERT_EQ(addresses.size(), 2);
        ASSERT_EQ(nRequired, 2);
        ASSERT_TRUE(boost::get<CKeyID>(&addresses[0]) &&
                    *boost::get<CKeyID>(&addresses[0]) == pubkeys[0].GetID());
        ASSERT_TRUE(boost::get<CKeyID>(&addresses[1]) &&
                    *boost::get<CKeyID>(&addresses[1]) == pubkeys[1].GetID());

        // TX_NULL_DATA
        s.clear();
        s << OP_RETURN << std::vector<unsigned char>({75});
        ASSERT_TRUE(!ExtractDestinations(s, whichType, addresses, nRequired));

    }

    TEST(TestScriptStandardTests, script_standard_GetScriptFor_) {

        CKey keys[3];
        CPubKey pubkeys[3];
        for (int i = 0; i < 3; i++) {
            keys[i].MakeNewKey(true);
            pubkeys[i] = keys[i].GetPubKey();
        }

        CScript expected, result;

        // CKeyID
        expected.clear();
        expected << OP_DUP << OP_HASH160 << ToByteVector(pubkeys[0].GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;
        result = GetScriptForDestination(pubkeys[0].GetID());
        ASSERT_TRUE(result == expected);

        // CScriptID
        CScript redeemScript(result);
        expected.clear();
        expected << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;
        result = GetScriptForDestination(CScriptID(redeemScript));
        ASSERT_TRUE(result == expected);

        // CNoDestination
        expected.clear();
        result = GetScriptForDestination(CNoDestination());
        ASSERT_TRUE(result == expected);

        // GetScriptForRawPubKey
        // expected.clear();
        // expected << ToByteVector(pubkeys[0]) << OP_CHECKSIG;
        // result = GetScriptForRawPubKey(pubkeys[0]);
        // ASSERT_TRUE(result == expected);

        // GetScriptForMultisig
        expected.clear();
        expected << OP_2 <<
            ToByteVector(pubkeys[0]) <<
            ToByteVector(pubkeys[1]) <<
            ToByteVector(pubkeys[2]) <<
            OP_3 << OP_CHECKMULTISIG;
        result = GetScriptForMultisig(2, std::vector<CPubKey>(pubkeys, pubkeys + 3));
        ASSERT_TRUE(result == expected);
    }

    TEST(TestScriptStandardTests, script_standard_IsMine) {

        CKey keys[2];
        CPubKey pubkeys[2];
        for (int i = 0; i < 2; i++) {
            keys[i].MakeNewKey(true);
            pubkeys[i] = keys[i].GetPubKey();
        }

        CKey uncompressedKey;
        uncompressedKey.MakeNewKey(false);
        CPubKey uncompressedPubkey = uncompressedKey.GetPubKey();

        CScript scriptPubKey;
        isminetype result;

        // P2PK compressed
        {
            CBasicKeyStore keystore;
            scriptPubKey.clear();
            scriptPubKey << ToByteVector(pubkeys[0]) << OP_CHECKSIG;

            // Keystore does not have key
            result = IsMine(keystore, scriptPubKey);
            ASSERT_EQ(result, ISMINE_NO);

            // Keystore has key
            keystore.AddKey(keys[0]);
            result = IsMine(keystore, scriptPubKey);
            ASSERT_EQ(result, ISMINE_SPENDABLE);
        }

        // P2PK uncompressed
        {
            CBasicKeyStore keystore;
            scriptPubKey.clear();
            scriptPubKey << ToByteVector(uncompressedPubkey) << OP_CHECKSIG;

            // Keystore does not have key
            result = IsMine(keystore, scriptPubKey);
            ASSERT_EQ(result, ISMINE_NO);

            // Keystore has key
            keystore.AddKey(uncompressedKey);
            result = IsMine(keystore, scriptPubKey);
            ASSERT_EQ(result, ISMINE_SPENDABLE);
        }

        // P2PKH compressed
        {
            CBasicKeyStore keystore;
            scriptPubKey.clear();
            scriptPubKey << OP_DUP << OP_HASH160 << ToByteVector(pubkeys[0].GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;

            // Keystore does not have key
            result = IsMine(keystore, scriptPubKey);
            ASSERT_EQ(result, ISMINE_NO);

            // Keystore has key
            keystore.AddKey(keys[0]);
            result = IsMine(keystore, scriptPubKey);
            ASSERT_EQ(result, ISMINE_SPENDABLE);
        }

        // P2PKH uncompressed
        {
            CBasicKeyStore keystore;
            scriptPubKey.clear();
            scriptPubKey << OP_DUP << OP_HASH160 << ToByteVector(uncompressedPubkey.GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;

            // Keystore does not have key
            result = IsMine(keystore, scriptPubKey);
            ASSERT_EQ(result, ISMINE_NO);

            // Keystore has key
            keystore.AddKey(uncompressedKey);
            result = IsMine(keystore, scriptPubKey);
            ASSERT_EQ(result, ISMINE_SPENDABLE);
        }

        // P2SH
        {
            CBasicKeyStore keystore;

            CScript redeemScript;
            redeemScript << OP_DUP << OP_HASH160 << ToByteVector(pubkeys[0].GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;

            scriptPubKey.clear();
            scriptPubKey << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;

            // Keystore does not have redeemScript or key
            result = IsMine(keystore, scriptPubKey);
            ASSERT_EQ(result, ISMINE_NO);

            // Keystore has redeemScript but no key
            keystore.AddCScript(redeemScript);
            result = IsMine(keystore, scriptPubKey);
            ASSERT_EQ(result, ISMINE_NO);

            // Keystore has redeemScript and key
            keystore.AddKey(keys[0]);
            result = IsMine(keystore, scriptPubKey);
            ASSERT_EQ(result, ISMINE_SPENDABLE);
        }

        // scriptPubKey multisig
        {
            CBasicKeyStore keystore;

            scriptPubKey.clear();
            scriptPubKey << OP_2 <<
                ToByteVector(uncompressedPubkey) <<
                ToByteVector(pubkeys[1]) <<
                OP_2 << OP_CHECKMULTISIG;

            // Keystore does not have any keys
            result = IsMine(keystore, scriptPubKey);
            ASSERT_EQ(result, ISMINE_NO);

            // Keystore has 1/2 keys
            keystore.AddKey(uncompressedKey);

            result = IsMine(keystore, scriptPubKey);
            ASSERT_EQ(result, ISMINE_NO);

            // Keystore has 2/2 keys
            keystore.AddKey(keys[1]);

            result = IsMine(keystore, scriptPubKey);
            ASSERT_EQ(result, ISMINE_NO);

            // Keystore has 2/2 keys and the script
            keystore.AddCScript(scriptPubKey);

            result = IsMine(keystore, scriptPubKey);
            ASSERT_EQ(result, ISMINE_NO);
        }

        // P2SH multisig
        {
            CBasicKeyStore keystore;
            keystore.AddKey(uncompressedKey);
            keystore.AddKey(keys[1]);

            CScript redeemScript;
            redeemScript << OP_2 <<
                ToByteVector(uncompressedPubkey) <<
                ToByteVector(pubkeys[1]) <<
                OP_2 << OP_CHECKMULTISIG;

            scriptPubKey.clear();
            scriptPubKey << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;

            // Keystore has no redeemScript
            result = IsMine(keystore, scriptPubKey);
            ASSERT_EQ(result, ISMINE_NO);

            // Keystore has redeemScript
            keystore.AddCScript(redeemScript);
            result = IsMine(keystore, scriptPubKey);
            ASSERT_EQ(result, ISMINE_SPENDABLE);
        }

        // OP_RETURN
        {
            CBasicKeyStore keystore;
            keystore.AddKey(keys[0]);

            scriptPubKey.clear();
            scriptPubKey << OP_RETURN << ToByteVector(pubkeys[0]);

            result = IsMine(keystore, scriptPubKey);
            ASSERT_EQ(result, ISMINE_NO);
        }

        // Nonstandard
        {
            CBasicKeyStore keystore;
            keystore.AddKey(keys[0]);

            scriptPubKey.clear();
            scriptPubKey << OP_9 << OP_ADD << OP_11 << OP_EQUAL;

            result = IsMine(keystore, scriptPubKey);
            ASSERT_EQ(result, ISMINE_NO);
        }

    }

    TEST(TestScriptStandardTests, script_standard_Malpill) {

        static const std::string log_tab = "             ";

        /* testing against non-minimal forms of PUSHDATA in P2PK/P2PKH (by Decker) */

        CKey key; CPubKey pubkey;
        txnouttype whichType;
        std::vector<std::vector<unsigned char> > solutions;

        key.MakeNewKey(true); // true - compressed pubkey, false - uncompressed
        pubkey = key.GetPubKey();

        ASSERT_TRUE(pubkey.size() == 0x21 || pubkey.size() == 0x41);

        std::vector<CScript> vScriptPubKeys = {
            CScript() << ToByteVector(pubkey) << OP_CHECKSIG, // 0x21 pubkey OP_CHECKSIG (the shortest form, typical)
            CScript() << OP_DUP << OP_HASH160 << ToByteVector(pubkey.GetID()) << OP_EQUALVERIFY << OP_CHECKSIG,
        };

        CScript modifiedScript; std::vector<unsigned char> modData;

        // PUSHDATA1(0x21) pubkey OP_CHECKSIG (one byte longer)
        modifiedScript = vScriptPubKeys[0];
        modData = {OP_PUSHDATA1, (unsigned char )pubkey.size()};
        modifiedScript.erase(modifiedScript.begin()); modifiedScript.insert(modifiedScript.begin(), modData.begin(), modData.end());
        vScriptPubKeys.push_back(modifiedScript);

        // PUSHDATA2(0x21) pubkey OP_CHECKSIG (two bytes longer)
        modifiedScript = vScriptPubKeys[0];
        modData = {OP_PUSHDATA2, (unsigned char )pubkey.size(), 0x00};
        modifiedScript.erase(modifiedScript.begin()); modifiedScript.insert(modifiedScript.begin(), modData.begin(), modData.end());
        vScriptPubKeys.push_back(modifiedScript);

        // PUSHDATA4(0x21) pubkey OP_CHECKSIG (four bytes longer)
        modifiedScript = vScriptPubKeys[0];
        modData = {OP_PUSHDATA4, (unsigned char )pubkey.size(), 0x00, 0x00, 0x00};
        modifiedScript.erase(modifiedScript.begin()); modifiedScript.insert(modifiedScript.begin(), modData.begin(), modData.end());
        vScriptPubKeys.push_back(modifiedScript);

        // same forms for p2kh

        modifiedScript = vScriptPubKeys[1];
        modData = {OP_PUSHDATA1, 0x14};
        modifiedScript.erase(modifiedScript.begin() + 2); modifiedScript.insert(modifiedScript.begin() + 2, modData.begin(), modData.end());
        vScriptPubKeys.push_back(modifiedScript);

        modifiedScript = vScriptPubKeys[1];
        modData = {OP_PUSHDATA2, 0x14, 0x00};
        modifiedScript.erase(modifiedScript.begin() + 2); modifiedScript.insert(modifiedScript.begin() + 2, modData.begin(), modData.end());
        vScriptPubKeys.push_back(modifiedScript);

        modifiedScript = vScriptPubKeys[1];
        modData = {OP_PUSHDATA4, 0x14, 0x00, 0x00, 0x00};
        modifiedScript.erase(modifiedScript.begin() + 2); modifiedScript.insert(modifiedScript.begin() + 2, modData.begin(), modData.end());
        vScriptPubKeys.push_back(modifiedScript);

        int test_count = 0;
        for(const auto &s : vScriptPubKeys) {
            solutions.clear();
            if (test_count < 2)
                EXPECT_TRUE(Solver(s, whichType, solutions)) << "Failed on Test #" << test_count;
            else
                EXPECT_FALSE(Solver(s, whichType, solutions)) << "Failed on Test #" << test_count;

            /* std::cerr << log_tab << "Test #" << test_count << ":" << std::endl;
            std::cerr << log_tab << "scriptPubKey [asm]: " << s.ToString() << std::endl;
            std::cerr << log_tab << "scriptPubKey [hex]: " << HexStr(s.begin(), s.end()) << std::endl;
            std::cerr << log_tab << "solutions.size(): " << solutions.size() << std::endl;
            std::cerr << log_tab << "whichType: " << txnouttypeToString(whichType) << std::endl; */

            switch (test_count)
            {
                case 0:
                    ASSERT_EQ(whichType, TX_PUBKEY);
                    ASSERT_EQ(solutions.size(), 1);
                    ASSERT_TRUE(solutions[0] == ToByteVector(pubkey));
                    break;
                case 1:
                    ASSERT_EQ(whichType, TX_PUBKEYHASH);
                    ASSERT_EQ(solutions.size(), 1);
                    ASSERT_TRUE(solutions[0] == ToByteVector(pubkey.GetID()));
                    break;
                default:
                    EXPECT_EQ(solutions.size(), 0);
                    EXPECT_EQ(whichType, TX_NONSTANDARD);
                    break;
            }

            test_count++;
        }
    }

    TEST(TestScriptStandardTests, p2sh_simple)
    {
        // test P2SH by adding 2 numbers that total 99
        CScript redeemScript;
        redeemScript << OP_ADD << 99 << OP_EQUAL;

        // standard P2SH (see BIP16)
        CScript scriptPubKey;
        scriptPubKey << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;

        CScript scriptSig;
        scriptSig << 1 << 98; // add 1 byte (decimal 98) to stack
        scriptSig.push_back(redeemScript.size()); // add next n bytes to stack;
        scriptSig += redeemScript;

        ScriptError err;
        EXPECT_TRUE(VerifyScript(scriptSig, scriptPubKey, SCRIPT_VERIFY_P2SH, BaseSignatureChecker(), 1, &err));
        EXPECT_EQ(err, SCRIPT_ERR_OK);
    }

    TEST(TestScriptStandardTests, IncludesMinerFee)
    {
        // turn on CryptoConditions
        ASSETCHAINS_CC = 1;
        // force validation although not synced
        KOMODO_CONNECTING = 0;
        
        CKey key;
        key.MakeNewKey(true);

        // make a CryptoCondition
        // Synopsis: threshold of 2 fulfillments that are:
        // - IncludesMinerFee validates
        // - Transaction signed by the private key of the passed-in public key
        CC* cc = MakeCCcond1(EVAL_INCLMINERFEE, key.GetPubKey());

        {
            // create a transaction with something for the miner
            CMutableTransaction tx;
            tx.vin.resize(1);
            tx.valueBalance = 100;
            tx.vout.resize(1);
            tx.vout[0].nValue = 99;
            PrecomputedTransactionData mutable_txdata(tx);

            // sign it
            uint256 sighash = SignatureHash(CCPubKey(cc), tx, 0, SIGHASH_ALL, 0, 0, &mutable_txdata);
            int out = cc_signTreeSecp256k1Msg32(cc, key.begin(), sighash.begin());

            // see if it works
            CAmount amount;
            CTransaction txTo(tx);
            PrecomputedTransactionData txdata(txTo);
            auto checker = ServerTransactionSignatureChecker(&txTo, 0, amount, false, txdata);
            ScriptError error;
            EXPECT_TRUE(VerifyScript(CCSig(cc), CCPubKey(cc), 0, checker, 0, &error));
            EXPECT_EQ(error, SCRIPT_ERR_OK);
        }
        {
            // create a transaction with nothing for the miner
            CMutableTransaction tx;
            tx.vin.resize(1);
            tx.valueBalance = 100;
            tx.vout.resize(1);
            tx.vout[0].nValue = 100;
            PrecomputedTransactionData mutable_txdata(tx);

            // sign it
            uint256 sighash = SignatureHash(CCPubKey(cc), tx, 0, SIGHASH_ALL, 0, 0, &mutable_txdata);
            int out = cc_signTreeSecp256k1Msg32(cc, key.begin(), sighash.begin());

            // see if it fails
            CAmount amount;
            CTransaction txTo(tx);
            PrecomputedTransactionData txdata(txTo);
            auto checker = ServerTransactionSignatureChecker(&txTo, 0, amount, false, txdata);
            ScriptError error;
            EXPECT_FALSE(VerifyScript(CCSig(cc), CCPubKey(cc), 0, checker, 0, &error));
            EXPECT_EQ(error, SCRIPT_ERR_EVAL_FALSE);
        }
    }

    TEST(TestScriptStandardTests, IncludesMinerFeeFullProcess)
    {
        TestChain chain; // start a new chain
        CBlock currentBlock = chain.generateBlock(); // Genesis block
        CBlock aliceFundingBlock;
        CBlock bobFundingBlock;

        // turn on CryptoConditions
        ASSETCHAINS_CC = 1;
        // force validation although not synced
        KOMODO_CONNECTING = 0;

        // users
        CKey notaryKey = chain.getNotaryKey();
        CKey aliceKey;
        aliceKey.MakeNewKey(true);
        CKey bobKey;
        bobKey.MakeNewKey(true);

        {
            // create a transaction that gives some coin to Alice
            CTransaction coinbaseTx = currentBlock.vtx[0];
            CMutableTransaction tx;
            CTxIn incoming;
            incoming.prevout.hash = coinbaseTx.GetHash();
            incoming.prevout.n = 0;
            tx.vin.push_back(incoming);
            // give some to alice
            CTxOut out1;
            out1.scriptPubKey = GetScriptForDestination(aliceKey.GetPubKey());
            out1.nValue = 100000;
            tx.vout.push_back(out1);
            // give the rest back to the notary
            CTxOut out2;
            out2.scriptPubKey = GetScriptForDestination(notaryKey.GetPubKey());
            out2.nValue = coinbaseTx.vout[0].nValue - 100000;
            tx.vout.push_back(out2);

            uint256 hash = SignatureHash(coinbaseTx.vout[0].scriptPubKey, tx, 0, SIGHASH_ALL, 0, 0);
            std::vector<unsigned char> vchSig;
            notaryKey.Sign(hash, vchSig);
            vchSig.push_back((unsigned char)SIGHASH_ALL);
            tx.vin[0].scriptSig << vchSig;

            CTransaction fundAlice(tx);
            CValidationState returnState = chain.acceptTx(fundAlice);
            // verify everything worked
            EXPECT_TRUE( returnState.IsValid() );

            // mine a block
            currentBlock = chain.generateBlock(); // vtx[0] is unrelated, vtx[1] is ours
            aliceFundingBlock = currentBlock;
        }
        {
            // notary gives some coin to Bob, with the stipulation that he pay some kind of miner fee
            CTransaction notaryPrevOut = currentBlock.vtx[1];
            CMutableTransaction tx;
            CTxIn notaryIn;
            notaryIn.prevout.hash = notaryPrevOut.GetHash();
            notaryIn.prevout.n = 1;
            tx.vin.push_back(notaryIn);
            // build the cryptocondition
            CC* bobMustPayFee = MakeCCcond1(EVAL_INCLMINERFEE, bobKey.GetPubKey());
            // give some coin to bob, but he must pay something to the miner
            CTxOut bobOut;
            bobOut.scriptPubKey = CCPubKey(bobMustPayFee);
            bobOut.nValue = 100000;
            tx.vout.push_back(bobOut);
            // give the rest back to the notary
            CTxOut notaryOut;
            notaryOut.scriptPubKey = GetScriptForDestination(notaryKey.GetPubKey());
            notaryOut.nValue = notaryPrevOut.vout[1].nValue - 100000;
            tx.vout.push_back(notaryOut);

            uint256 hash = SignatureHash(notaryPrevOut.vout[1].scriptPubKey, tx, 0, SIGHASH_ALL, 0, 0);
            std::vector<unsigned char> vchSig;
            notaryKey.Sign(hash, vchSig);
            vchSig.push_back((unsigned char)SIGHASH_ALL);
            tx.vin[0].scriptSig << vchSig;

            CTransaction fundBob(tx);
            CValidationState returnState = chain.acceptTx(fundBob);

            // verify everything worked
            EXPECT_TRUE( returnState.IsValid() );

            // mine a block
            currentBlock = chain.generateBlock();
            bobFundingBlock = currentBlock;
            cc_free(bobMustPayFee);
        }
        {
            // alice gives everything to bob without paying a fee
            CTransaction alicePrevOut = aliceFundingBlock.vtx[1];
            CMutableTransaction tx;
            CTxIn aliceFunds;
            aliceFunds.prevout.hash = alicePrevOut.GetHash();
            aliceFunds.prevout.n = 0;
            tx.vin.push_back(aliceFunds);
            CTxOut bobOut;
            bobOut.scriptPubKey = GetScriptForDestination(bobKey.GetPubKey());
            bobOut.nValue = alicePrevOut.vout[0].nValue;
            tx.vout.push_back(bobOut);

            uint256 hash = SignatureHash(alicePrevOut.vout[0].scriptPubKey, tx, 0, SIGHASH_ALL, 0, 0);
            std::vector<unsigned char> vchSig;
            aliceKey.Sign(hash, vchSig);
            vchSig.push_back((unsigned char)SIGHASH_ALL);
            tx.vin[0].scriptSig << vchSig;

            CTransaction fundBobAgain(tx);
            CValidationState returnState = chain.acceptTx(fundBobAgain);

            // verify everything worked
            EXPECT_TRUE( returnState.IsValid() );

            // mine a block
            currentBlock = chain.generateBlock();
        }
        {
            // bob can't give anything to alice without paying a mining fee
            CTransaction bobPrevOut = bobFundingBlock.vtx[1];
            CMutableTransaction tx;
            CTxIn bobFunds;
            bobFunds.prevout.hash = bobPrevOut.GetHash();
            bobFunds.prevout.n = 0;
            tx.vin.push_back(bobFunds);
            CTxOut aliceOut;
            aliceOut.scriptPubKey = GetScriptForDestination(aliceKey.GetPubKey());
            aliceOut.nValue = bobPrevOut.vout[0].nValue;
            tx.vout.push_back(aliceOut);

            CC* cc = MakeCCcond1(EVAL_INCLMINERFEE, bobKey.GetPubKey());
            uint256 hash = SignatureHash(bobPrevOut.vout[0].scriptPubKey, tx, 0, SIGHASH_ALL, 0, 0);
            int out = cc_signTreeSecp256k1Msg32(cc, bobKey.begin(), hash.begin());            
            tx.vin[0].scriptSig = CCSig(cc);

            CTransaction fundAliceAgain(tx);
            CValidationState returnState = chain.acceptTx(fundAliceAgain);

            // this should not work
            EXPECT_FALSE( returnState.IsValid() );
            cc_free(cc);
        }
        {
            // bob can give something to alice if he pays a mining fee
            CTransaction bobPrevOut = bobFundingBlock.vtx[1];
            CMutableTransaction tx;
            CTxIn bobFunds;
            bobFunds.prevout.hash = bobPrevOut.GetHash();
            bobFunds.prevout.n = 0;
            tx.vin.push_back(bobFunds);
            CTxOut aliceOut;
            aliceOut.scriptPubKey = GetScriptForDestination(aliceKey.GetPubKey());
            aliceOut.nValue = bobPrevOut.vout[0].nValue - 1;
            tx.vout.push_back(aliceOut);

            CC* cc = MakeCCcond1(EVAL_INCLMINERFEE, bobKey.GetPubKey());
            uint256 hash = SignatureHash(bobPrevOut.vout[0].scriptPubKey, tx, 0, SIGHASH_ALL, 0, 0);
            int out = cc_signTreeSecp256k1Msg32(cc, bobKey.begin(), hash.begin());            
            tx.vin[0].scriptSig = CCSig(cc);

            CTransaction fundAliceAgain(tx);
            CValidationState returnState = chain.acceptTx(fundAliceAgain);

            // this should now work
            EXPECT_TRUE( returnState.IsValid() );

            // mine a block
            currentBlock = chain.generateBlock();
            cc_free(cc);
        }
    }

    TEST(TestScriptStandardTests, p2sh_htlc)
    {
        // turn on CryptoConditions
        ASSETCHAINS_CC = 1;
        // force validation although not synced
        KOMODO_CONNECTING = 0;

        CKey key;
        key.MakeNewKey(true);
        CPubKey pubkey = key.GetPubKey();

        // make a CryptoCondition
        // Synopsis: threshold of 2 fulfillments that are:
        // - HTLC validates
        // - Transaction signed by the passed-in public key
        CC* cc = MakeCCcond1(EVAL_HTLC, pubkey);
        std::cout << cc_conditionToJSONString(cc) << std::endl;

        // sign it
        CMutableTransaction tx;
        tx.vin.resize(1);
        PrecomputedTransactionData mutable_txdata(tx);
        uint256 sighash = SignatureHash(CCPubKey(cc), tx, 0, SIGHASH_ALL, 0, 0, &mutable_txdata);
        int out = cc_signTreeSecp256k1Msg32(cc, key.begin(), sighash.begin());
        tx.vin[0].scriptSig = CCSig(cc);

        // see if it works as a regular transaction
        CAmount amount;
        ScriptError error;
        CTransaction txTo(tx);
        PrecomputedTransactionData txdata(txTo);
        auto checker = ServerTransactionSignatureChecker(&txTo, 0, amount, false, txdata);
        EXPECT_TRUE(VerifyScript(CCSig(cc), CCPubKey(cc), 0, checker, 0, &error));

        // attempt to turn it into a P2SH
        CScript redeemScript = CCPubKey(cc);

        CScript scriptSig = CCSig(cc);
        // push the redeemScript onto the stack instead of evaluating it
        size_t redeemScriptSize = redeemScript.size();
        if (redeemScriptSize < 76)
            scriptSig.push_back( (uint8_t)redeemScriptSize );
        else
        {
            redeemScript.push_back(OP_PUSHDATA1);
            redeemScript.push_back( redeemScriptSize );
        }
        scriptSig += redeemScript;

        CScript scriptPubKey;
        scriptPubKey << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;       
        EXPECT_TRUE(VerifyScript(scriptSig, scriptPubKey, SCRIPT_VERIFY_P2SH, checker, 1, &error));

        EXPECT_EQ(error, SCRIPT_ERR_OK);
        cc_free(cc);
    }


} // namespace TestScriptStandardTests