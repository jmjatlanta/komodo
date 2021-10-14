// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/******************************************************************************
 * Copyright © 2014-2019 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

#include "key_io.h"
#include "main.h"
#include "crypto/equihash.h"

#include "util.h"
#include "utilstrencodings.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

#include "chainparamsseeds.h"

/****
 * @brief Create the genesis block. 
 * @param pszTimestamp
 * @param genesisOutputScript
 * @param nTime
 * @param nNonce
 * @param nSolution
 * @param nBits
 * @param nVersion
 * @param genesisReward
 * @returns the new block
 */
static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, 
        uint32_t nTime, const uint256& nNonce, const std::vector<unsigned char>& nSolution, 
        uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    // To create a genesis block for a new chain which is Overwintered:
    //   txNew.nVersion = OVERWINTER_TX_VERSION
    //   txNew.fOverwintered = true
    //   txNew.nVersionGroupId = OVERWINTER_VERSION_GROUP_ID
    //   txNew.nExpiryHeight = <default value>
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 520617983 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nSolution = nSolution;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(txNew);
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = genesis.BuildMerkleTree();
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database (and is in any case of zero value).
 *
 * >>> from pyblake2 import blake2s
 * >>> 'Zcash' + blake2s(b'The Economist 2016-10-29 Known unknown: Another crypto-currency is born. BTC#436254 0000000000000000044f321997f336d2908cf8c8d6893e88dbf067e2d949487d ETH#2521903 483039a6b6bd8bd05f0584f9a078d075e454925eb71c1f13eaff59b405a721bb DJIA close on 27 Oct 2016: 18,169.68').hexdigest()
 *
 * CBlock(hash=00040fe8, ver=4, hashPrevBlock=00000000000000, hashMerkleRoot=c4eaa5, nTime=1477641360, nBits=1f07ffff, nNonce=4695, vtx=1)
 *   CTransaction(hash=c4eaa5, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff071f0104455a6361736830623963346565663862376363343137656535303031653335303039383462366665613335363833613763616331343161303433633432303634383335643334)
 *     CTxOut(nValue=0.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
 *   vMerkleTree: c4eaa5
 */
static CBlock CreateGenesisBlock(uint32_t nTime, const uint256& nNonce, const std::vector<unsigned char>& nSolution, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "Zcash0b9c4eef8b7cc417ee5001e3500984b6fea35683a7cac141a043c42064835d34";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nSolution, nBits, nVersion, genesisReward);
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */
#include "komodo_defs.h"
int32_t ASSETCHAINS_BLOCKTIME = 60;
uint64_t ASSETCHAINS_NK[2];

const arith_uint256 maxUint = UintToArith256(uint256S("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));


void CChainParams::SetCheckpointData(CChainParams::CCheckpointData checkpointData)
{
    CChainParams::checkpointData = checkpointData;
}

/*
 To change the max block size, all that needs to be updated is the #define _MAX_BLOCK_SIZE in utils.h
 
 However, doing that without any other changes will allow forking non-updated nodes by creating a larger block. So, make sure to height activate the new blocksize properly.
 
 Assuming it is 8MB, then:
 #define _OLD_MAX_BLOCK_SIZE (4096 * 1024)
 #define _MAX_BLOCK_SIZE (2 * 4096 * 1024)
 
 change the body of if:
 {
    if ( height < saplinght+1000000 ) // activates 8MB blocks 1 million blocks after saplinght
        return(_OLD_MAX_BLOCK_SIZE);
    else return(_MAX_BLOCK_SIZE);
 }

*/

// Block height must be >0 and <=last founders reward block height
// Index variable i ranges from 0 - (vFoundersRewardAddress.size()-1)
std::string CChainParams::GetFoundersRewardAddressAtHeight(int nHeight) const {
    int maxHeight = consensus.GetLastFoundersRewardBlockHeight();
    assert(nHeight > 0 && nHeight <= maxHeight);

    size_t addressChangeInterval = (maxHeight + vFoundersRewardAddress.size()) / vFoundersRewardAddress.size();
    size_t i = nHeight / addressChangeInterval;
    return vFoundersRewardAddress[i];
}

// Block height must be >0 and <=last founders reward block height
// The founders reward address is expected to be a multisig (P2SH) address
CScript CChainParams::GetFoundersRewardScriptAtHeight(int nHeight) const {
    assert(nHeight > 0 && nHeight <= consensus.GetLastFoundersRewardBlockHeight());

    CTxDestination address = DecodeDestination(GetFoundersRewardAddressAtHeight(nHeight).c_str());
    assert(IsValidDestination(address));
    assert(boost::get<CScriptID>(&address) != nullptr);
    CScriptID scriptID = boost::get<CScriptID>(address); // address is a boost variant
    CScript script = CScript() << OP_HASH160 << ToByteVector(scriptID) << OP_EQUAL;
    return script;
}

std::string CChainParams::GetFoundersRewardAddressAtIndex(int i) const {
    assert(i >= 0 && i < vFoundersRewardAddress.size());
    return vFoundersRewardAddress[i];
}

int32_t CChainParams::MaxBlockSize(int32_t height) const
{
    int32_t saplinght = consensus.GetSaplingHeight();
    if ( height <= 0 || (saplinght > 0 && height >= saplinght) )
    {
        return(_MAX_BLOCK_SIZE);
    }
    else return(2000000);
}

void CChainParams::SetSaplingHeight(int32_t height)
{
    consensus.vUpgrades[Consensus::UPGRADE_SAPLING].nActivationHeight = height;
}

void CChainParams::SetOverwinterHeight(int32_t height)
{
    consensus.vUpgrades[Consensus::UPGRADE_OVERWINTER].nActivationHeight = height;
}

CMainParams::CMainParams()
{

    strNetworkID = "main";
    strCurrencyUnits = "KMD";
    bip44CoinType = 141; // As registered in https://github.com/satoshilabs/slips/blob/master/slip-0044.md 
    consensus.fCoinbaseMustBeProtected = false; // true this is only true wuth Verus and enforced after block 12800
    consensus.nSubsidySlowStartInterval = 20000;
    consensus.nSubsidyHalvingInterval = 840000;
    consensus.nMajorityEnforceBlockUpgrade = 750;
    consensus.nMajorityRejectBlockOutdated = 950;
    consensus.nMajorityWindow = 4000;
    consensus.powLimit = uint256S("0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f");
    consensus.powAlternate = uint256S("0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f");
    consensus.nPowAveragingWindow = 17;
    consensus.nMaxFutureBlockTime = 7 * 60; // 7 mins

    assert(maxUint/UintToArith256(consensus.powLimit) >= consensus.nPowAveragingWindow);
    consensus.nPowMaxAdjustDown = 32; // 32% adjustment down
    consensus.nPowMaxAdjustUp = 16; // 16% adjustment up
    consensus.nPowTargetSpacing = 1 * 60;
    consensus.nPowAllowMinDifficultyBlocksAfterHeight = boost::none;
    consensus.vUpgrades[Consensus::BASE_SPROUT].nProtocolVersion = 170002;
    consensus.vUpgrades[Consensus::BASE_SPROUT].nActivationHeight =
        Consensus::NetworkUpgrade::ALWAYS_ACTIVE;
    consensus.vUpgrades[Consensus::UPGRADE_TESTDUMMY].nProtocolVersion = 170002;
    consensus.vUpgrades[Consensus::UPGRADE_TESTDUMMY].nActivationHeight =
        Consensus::NetworkUpgrade::NO_ACTIVATION_HEIGHT;
    consensus.vUpgrades[Consensus::UPGRADE_OVERWINTER].nProtocolVersion = 170005;
    consensus.SetOverwinterHeight(Consensus::NetworkUpgrade::NO_ACTIVATION_HEIGHT);
    consensus.vUpgrades[Consensus::UPGRADE_SAPLING].nProtocolVersion = 170007;
    consensus.SetSaplingHeight(Consensus::NetworkUpgrade::NO_ACTIVATION_HEIGHT);

    // The best chain should have at least this much work.
    consensus.nMinimumChainWork = uint256S("0x00000000000000000000000000000000000000000000000000281b32ff3198a1");

    /**
     * The message start string is designed to be unlikely to occur in normal data.
     * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
     * a large 32-bit integer with any alignment.
     */
    pchMessageStart[0] = 0xf9;
    pchMessageStart[1] = 0xee;
    pchMessageStart[2] = 0xe4;
    pchMessageStart[3] = 0x8d;
    vAlertPubKey = ParseHex("020e46e79a2a8d12b9b5d12c7a91adb4e454edfae43c0a0cb805427d2ac7613fd9");
    // (Zcash) vAlertPubKey = ParseHex("04b7ecf0baa90495ceb4e4090f6b2fd37eec1e9c85fac68a487f3ce11589692e4a317479316ee814e066638e1db54e37a10689b70286e6315b1087b6615d179264");
    nDefaultPort = 7770;
    nMinerThreads = 0;
    nMaxTipAge = 24 * 60 * 60;
    nPruneAfterHeight = 100000;
    const size_t N = 200, K = 9;
    BOOST_STATIC_ASSERT(equihash_parameters_acceptable(N, K));
    nEquihashN = N;
    nEquihashK = K;

    const char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
    CMutableTransaction txNew;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = 50 * COIN;
    txNew.vout[0].scriptPubKey = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    genesis.vtx.push_back(txNew);
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = genesis.BuildMerkleTree();
    genesis.nVersion = 1;
    genesis.nTime    = 1231006505;
    genesis.nBits    = KOMODO_MINDIFF_NBITS;
    genesis.nNonce   = uint256S("0x000000000000000000000000000000000000000000000000000000000000000b");
    genesis.nSolution = ParseHex("000d5ba7cda5d473947263bf194285317179d2b0d307119c2e7cc4bd8ac456f0774bd52b0cd9249be9d40718b6397a4c7bbd8f2b3272fed2823cd2af4bd1632200ba4bf796727d6347b225f670f292343274cc35099466f5fb5f0cd1c105121b28213d15db2ed7bdba490b4cedc69742a57b7c25af24485e523aadbb77a0144fc76f79ef73bd8530d42b9f3b9bed1c135ad1fe152923fafe98f95f76f1615e64c4abb1137f4c31b218ba2782bc15534788dda2cc08a0ee2987c8b27ff41bd4e31cd5fb5643dfe862c9a02ca9f90c8c51a6671d681d04ad47e4b53b1518d4befafefe8cadfb912f3d03051b1efbf1dfe37b56e93a741d8dfd80d576ca250bee55fab1311fc7b3255977558cdda6f7d6f875306e43a14413facdaed2f46093e0ef1e8f8a963e1632dcbeebd8e49fd16b57d49b08f9762de89157c65233f60c8e38a1f503a48c555f8ec45dedecd574a37601323c27be597b956343107f8bd80f3a925afaf30811df83c402116bb9c1e5231c70fff899a7c82f73c902ba54da53cc459b7bf1113db65cc8f6914d3618560ea69abd13658fa7b6af92d374d6eca9529f8bd565166e4fcbf2a8dfb3c9b69539d4d2ee2e9321b85b331925df195915f2757637c2805e1d4131e1ad9ef9bc1bb1c732d8dba4738716d351ab30c996c8657bab39567ee3b29c6d054b711495c0d52e1cd5d8e55b4f0f0325b97369280755b46a02afd54be4ddd9f77c22272b8bbb17ff5118fedbae2564524e797bd28b5f74f7079d532ccc059807989f94d267f47e724b3f1ecfe00ec9e6541c961080d8891251b84b4480bc292f6a180bea089fef5bbda56e1e41390d7c0e85ba0ef530f7177413481a226465a36ef6afe1e2bca69d2078712b3912bba1a99b1fbff0d355d6ffe726d2bb6fbc103c4ac5756e5bee6e47e17424ebcbf1b63d8cb90ce2e40198b4f4198689daea254307e52a25562f4c1455340f0ffeb10f9d8e914775e37d0edca019fb1b9c6ef81255ed86bc51c5391e0591480f66e2d88c5f4fd7277697968656a9b113ab97f874fdd5f2465e5559533e01ba13ef4a8f7a21d02c30c8ded68e8c54603ab9c8084ef6d9eb4e92c75b078539e2ae786ebab6dab73a09e0aa9ac575bcefb29e930ae656e58bcb513f7e3c17e079dce4f05b5dbc18c2a872b22509740ebe6a3903e00ad1abc55076441862643f93606e3dc35e8d9f2caef3ee6be14d513b2e062b21d0061de3bd56881713a1a5c17f5ace05e1ec09da53f99442df175a49bd154aa96e4949decd52fed79ccf7ccbce32941419c314e374e4a396ac553e17b5340336a1a25c22f9e42a243ba5404450b650acfc826a6e432971ace776e15719515e1634ceb9a4a35061b668c74998d3dfb5827f6238ec015377e6f9c94f38108768cf6e5c8b132e0303fb5a200368f845ad9d46343035a6ff94031df8d8309415bb3f6cd5ede9c135fdabcc030599858d803c0f85be7661c88984d88faa3d26fb0e9aac0056a53f1b5d0baed713c853c4a2726869a0a124a8a5bbc0fc0ef80c8ae4cb53636aa02503b86a1eb9836fcc259823e2692d921d88e1ffc1e6cb2bde43939ceb3f32a611686f539f8f7c9f0bf00381f743607d40960f06d347d1cd8ac8a51969c25e37150efdf7aa4c2037a2fd0516fb444525ab157a0ed0a7412b2fa69b217fe397263153782c0f64351fbdf2678fa0dc8569912dcd8e3ccad38f34f23bbbce14c6a26ac24911b308b82c7e43062d180baeac4ba7153858365c72c63dcf5f6a5b08070b730adb017aeae925b7d0439979e2679f45ed2f25a7edcfd2fb77a8794630285ccb0a071f5cce410b46dbf9750b0354aae8b65574501cc69efb5b6a43444074fee116641bb29da56c2b4a7f456991fc92b2");


    /*genesis = CreateGenesisBlock(
        1477641360,
        uint256S("0x0000000000000000000000000000000000000000000000000000000000001257"),
        ParseHex("000a889f00854b8665cd555f4656f68179d31ccadc1b1f7fb0952726313b16941da348284d67add4686121d4e3d930160c1348d8191c25f12b267a6a9c131b5031cbf8af1f79c9d513076a216ec87ed045fa966e01214ed83ca02dc1797270a454720d3206ac7d931a0a680c5c5e099057592570ca9bdf6058343958b31901fce1a15a4f38fd347750912e14004c73dfe588b903b6c03166582eeaf30529b14072a7b3079e3a684601b9b3024054201f7440b0ee9eb1a7120ff43f713735494aa27b1f8bab60d7f398bca14f6abb2adbf29b04099121438a7974b078a11635b594e9170f1086140b4173822dd697894483e1c6b4e8b8dcd5cb12ca4903bc61e108871d4d915a9093c18ac9b02b6716ce1013ca2c1174e319c1a570215bc9ab5f7564765f7be20524dc3fdf8aa356fd94d445e05ab165ad8bb4a0db096c097618c81098f91443c719416d39837af6de85015dca0de89462b1d8386758b2cf8a99e00953b308032ae44c35e05eb71842922eb69797f68813b59caf266cb6c213569ae3280505421a7e3a0a37fdf8e2ea354fc5422816655394a9454bac542a9298f176e211020d63dee6852c40de02267e2fc9d5e1ff2ad9309506f02a1a71a0501b16d0d36f70cdfd8de78116c0c506ee0b8ddfdeb561acadf31746b5a9dd32c21930884397fb1682164cb565cc14e089d66635a32618f7eb05fe05082b8a3fae620571660a6b89886eac53dec109d7cbb6930ca698a168f301a950be152da1be2b9e07516995e20baceebecb5579d7cdbc16d09f3a50cb3c7dffe33f26686d4ff3f8946ee6475e98cf7b3cf9062b6966e838f865ff3de5fb064a37a21da7bb8dfd2501a29e184f207caaba364f36f2329a77515dcb710e29ffbf73e2bbd773fab1f9a6b005567affff605c132e4e4dd69f36bd201005458cfbd2c658701eb2a700251cefd886b1e674ae816d3f719bac64be649c172ba27a4fd55947d95d53ba4cbc73de97b8af5ed4840b659370c556e7376457f51e5ebb66018849923db82c1c9a819f173cccdb8f3324b239609a300018d0fb094adf5bd7cbb3834c69e6d0b3798065c525b20f040e965e1a161af78ff7561cd874f5f1b75aa0bc77f720589e1b810f831eac5073e6dd46d00a2793f70f7427f0f798f2f53a67e615e65d356e66fe40609a958a05edb4c175bcc383ea0530e67ddbe479a898943c6e3074c6fcc252d6014de3a3d292b03f0d88d312fe221be7be7e3c59d07fa0f2f4029e364f1f355c5d01fa53770d0cd76d82bf7e60f6903bc1beb772e6fde4a70be51d9c7e03c8d6d8dfb361a234ba47c470fe630820bbd920715621b9fbedb49fcee165ead0875e6c2b1af16f50b5d6140cc981122fcbcf7c5a4e3772b3661b628e08380abc545957e59f634705b1bbde2f0b4e055a5ec5676d859be77e20962b645e051a880fddb0180b4555789e1f9344a436a84dc5579e2553f1e5fb0a599c137be36cabbed0319831fea3fddf94ddc7971e4bcf02cdc93294a9aab3e3b13e3b058235b4f4ec06ba4ceaa49d675b4ba80716f3bc6976b1fbf9c8bf1f3e3a4dc1cd83ef9cf816667fb94f1e923ff63fef072e6a19321e4812f96cb0ffa864da50ad74deb76917a336f31dce03ed5f0303aad5e6a83634f9fcc371096f8288b8f02ddded5ff1bb9d49331e4a84dbe1543164438fde9ad71dab024779dcdde0b6602b5ae0a6265c14b94edd83b37403f4b78fcd2ed555b596402c28ee81d87a909c4e8722b30c71ecdd861b05f61f8b1231795c76adba2fdefa451b283a5d527955b9f3de1b9828e7b2e74123dd47062ddcc09b05e7fa13cb2212a6fdbc65d7e852cec463ec6fd929f5b8483cf3052113b13dac91b69f49d1b7d1aec01c4a68e41ce157"),
        0x1f07ffff, 4, 0);*/

    consensus.hashGenesisBlock = genesis.GetHash();
    assert(consensus.hashGenesisBlock == uint256S("0x027e3758c3a65b12aa1046462b486d0a63bfa1beae327897f56c5cfb7daaae71"));
    assert(genesis.hashMerkleRoot == uint256S("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));
    vFixedSeeds.clear();
    vSeeds.clear();

    vSeeds.push_back(CDNSSeedData("komodoseeds.org", "kmd.komodoseeds.org")); // decker
    vSeeds.push_back(CDNSSeedData("kmd.sh", "seeds1.kmd.sh")); // decker
    vSeeds.push_back(CDNSSeedData("cipig.net", "kmdseed.cipig.net")); // cipig
    vSeeds.push_back(CDNSSeedData("lordofthechains.com", "kmdseeds.lordofthechains.com")); // gcharang

    // TODO: we need more seed crawlers from other community members
    base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,60);
    base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,85);
    base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,188);
    base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
    base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();
    // guarantees the first two characters, when base58 encoded, are "zc"
    base58Prefixes[ZCPAYMENT_ADDRRESS] = {22,154};
    // guarantees the first 4 characters, when base58 encoded, are "ZiVK"
    base58Prefixes[ZCVIEWING_KEY]      = {0xA8,0xAB,0xD3};
    // guarantees the first two characters, when base58 encoded, are "SK"
    base58Prefixes[ZCSPENDING_KEY] = {171,54};

    bech32HRPs[SAPLING_PAYMENT_ADDRESS]      = "zs";
    bech32HRPs[SAPLING_FULL_VIEWING_KEY]     = "zviews";
    bech32HRPs[SAPLING_INCOMING_VIEWING_KEY] = "zivks";
    bech32HRPs[SAPLING_EXTENDED_SPEND_KEY]   = "secret-extended-key-main";

    vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

    fMiningRequiresPeers = true;
    fDefaultConsistencyChecks = false;
    fRequireStandard = true;
    fMineBlocksOnDemand = false;
    fTestnetToBeDeprecatedFieldRPC = false;
}

CTestNetParams::CTestNetParams() 
{
    strNetworkID = "test";
    strCurrencyUnits = "TAZ";
    bip44CoinType = 1;
    consensus.fCoinbaseMustBeProtected = true;
    consensus.nSubsidySlowStartInterval = 20000;
    consensus.nSubsidyHalvingInterval = 840000;
    consensus.nMajorityEnforceBlockUpgrade = 51;
    consensus.nMajorityRejectBlockOutdated = 75;
    consensus.nMajorityWindow = 400;
    consensus.powLimit = uint256S("07ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    consensus.powAlternate = uint256S("07ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    consensus.nPowAveragingWindow = 17;
    assert(maxUint/UintToArith256(consensus.powLimit) >= consensus.nPowAveragingWindow);
    consensus.nMaxFutureBlockTime = 7 * 60;

    vAlertPubKey = ParseHex("00");
    nDefaultPort = 17770;
    nMinerThreads = 0;
    consensus.nPowMaxAdjustDown = 32; // 32% adjustment down
    consensus.nPowMaxAdjustUp = 16; // 16% adjustment up
    consensus.nPowTargetSpacing = 2.5 * 60;
    consensus.nPowAllowMinDifficultyBlocksAfterHeight = 299187;
    consensus.vUpgrades[Consensus::BASE_SPROUT].nProtocolVersion = 170002;
    consensus.vUpgrades[Consensus::BASE_SPROUT].nActivationHeight =
        Consensus::NetworkUpgrade::ALWAYS_ACTIVE;
    consensus.vUpgrades[Consensus::UPGRADE_TESTDUMMY].nProtocolVersion = 170002;
    consensus.vUpgrades[Consensus::UPGRADE_TESTDUMMY].nActivationHeight =
        Consensus::NetworkUpgrade::NO_ACTIVATION_HEIGHT;
    consensus.vUpgrades[Consensus::UPGRADE_OVERWINTER].nProtocolVersion = 170003;
    consensus.SetOverwinterHeight(207500);
    consensus.vUpgrades[Consensus::UPGRADE_SAPLING].nProtocolVersion = 170007;
    consensus.SetSaplingHeight(280000);

    // The best chain should have at least this much work.
    consensus.nMinimumChainWork = uint256S("0x00000000000000000000000000000000000000000000000000000001d0c4d9cd");

    pchMessageStart[0] = 0x5A;
    pchMessageStart[1] = 0x1F;
    pchMessageStart[2] = 0x7E;
    pchMessageStart[3] = 0x62;
    vAlertPubKey = ParseHex("020e46e79a2a8d12b9b5d12c7a91adb4e454edfae43c0a0cb805427d2ac7613fd9");
    nMaxTipAge = 24 * 60 * 60;

    nPruneAfterHeight = 1000;
    const size_t N = 200, K = 9;
    BOOST_STATIC_ASSERT(equihash_parameters_acceptable(N, K));
    nEquihashN = N;
    nEquihashK = K;

    //! Modify the testnet genesis block so the timestamp is valid for a later start.
    genesis.nTime = 1296688602;
    genesis.nBits = KOMODO_MINDIFF_NBITS;
    genesis.nNonce = uint256S("0x0000000000000000000000000000000000000000000000000000000000000009");
    genesis.nSolution = ParseHex("003423da3e41f916bf3ff0ee770eb844a240361abe08a8c9d46bd30226e2ad411a4047b6ddc230d173c60537e470e24f764120f5a2778b2a1285b0727bf79a0b085ad67e6266fb38fd72ef17f827315c42f921720248c983d4100e6ebd1c4b5e8762a973bac3bec7f7153b93752ebbb465f0fc9520bcfc30f9abfe303627338fed6ede9cf1b9173a736cf270cf4d9c6999ff4c3a301a78fd50dab6ccca67a0c5c2e41f216a1f3efd049a74bbe6252f9773bc309d3f9e554d996913ce8e1cec672a1fa4ea59726b61ea9e75d5ce9aa5dbfa96179a293810e02787f26de324fe7c88376ff57e29574a55faff7c2946f3e40e451861c32bf67da7377de3136858a18f34fab1bc8da37726ca2c25fc7b312a5427554ec944da81c7e27255d6c94ade9987ff7daedc2d1cc63d7d4cf93e691d13326fb1c7ee72ccdc0b134eb665fc6a9821e6fef6a6d45e4aac6dca6b505a0100ad56ea4f6fa4cdc2f0d1b65f730104a515172e34163bdb422f99d083e6eb860cf6b3f66642c4dbaf0d0fa1dca1b6166f1d1ffaa55a9d6d6df628afbdd14f1622c1c8303259299521a253bc28fcc93676723158067270fc710a09155a1e50c533e9b79ed5edba4ab70a08a9a2fc0eef0ddae050d75776a9804f8d6ad7e30ccb66c6a98d86710ca7a4dfb4feb159484796b9a015c5764aa3509051c87f729b9877ea41f8b470898c01388ed9098b1e006d3c30fc6e7c781072fa3f75d918505ee8ca75840fc62f67c57060666aa42578a2dd022eda62e3f1e447d7364074d34fd60ad9b138f60422afa6cfcb913fd6c213b496144dbfda7bfc7c24540cfe40ad0c0fd5a8c0902127f53d3178ba1b2a87bf1224d53d3a15e49ccdf121ae872a011c996d1b9793153cdcd4c0a7e99f8a35669788551cca2b62769eda24b6b55e2f4e0ac0d30aa50ecf33c6cdb24adfc922006a7bf434ced800fefe814c94c6fc8caa37b372d5088bb31d2f6b11a7a67ad3f70abbac0d5c256b637828de6cc525978cf151a2e50798e0c591787639a030291272c9ced3ab7d682e03f8c7db51f60163baa85315789666ea8c5cd6f789a7f4a5de4f8a9dfefce20f353cec606492fde8eab3e3b487b3a3a57434f8cf252a4b643fc125c8a5948b06744f5dc306aa587bdc85364c7488235c6edddd78763675e50a9637181519be06dd30c4ba0d845f9ba320d01706fd6dd64d1aa3cd4211a4a7d1d3f2c1ef2766d27d5d2cdf8e7f5e3ea309d4f149bb737305df1373a7f5313abe5986f4aa620bec4b0065d48aafac3631de3771f5c4d2f6eec67b09d9c70a3c1969fecdb014cb3c69832b63cc9d6efa378bff0ef95ffacdeb1675bb326e698f022c1a3a2e1c2b0f05e1492a6d2b7552388eca7ee8a2467ef5d4207f65d4e2ae7e33f13eb473954f249d7c20158ae703e1accddd4ea899f026618695ed2949715678a32a153df32c08922fafad68b1895e3b10e143e712940104b3b352369f4fe79bd1f1dbe03ea9909dbcf5862d1f15b3d1557a6191f54c891513cdb3c729bb9ab08c0d4c35a3ed67d517ffe1e2b7a798521aed15ff9822169c0ec860d7b897340bc2ef4c37f7eb73bd7dafef12c4fd4e6f5dd3690305257ae14ed03df5e3327b68467775a90993e613173fa6650ffa2a26e84b3ce79606bf234eda9f4053307f344099e3b10308d3785b8726fd02d8e94c2759bebd05748c3fe7d5fe087dc63608fb77f29708ab167a13f32da251e249a544124ed50c270cfc6986d9d1814273d2f0510d0d2ea335817207db6a4a23ae9b079967b63b25cb3ceea7001b65b879263f5009ac84ab89738a5b8b71fd032beb9f297326f1f5afa630a5198d684514e242f315a4d95fa6802e82799a525bb653b80b4518ec610a5996403b1391");
    consensus.hashGenesisBlock = genesis.GetHash();
    //assert(consensus.hashGenesisBlock == uint256S("0x05a60a92d99d85997cce3b87616c089f6124d7342af37106edc76126334a2c38"));

    vFixedSeeds.clear();
    vSeeds.clear();
    //vSeeds.push_back(CDNSSeedData("z.cash", "dns.testnet.z.cash")); // Komodo

    base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,0);
    base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,5);
    base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
    base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
    base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();
    base58Prefixes[ZCPAYMENT_ADDRRESS] = {20,81};
    // guarantees the first 4 characters, when base58 encoded, are "ZiVt"
    base58Prefixes[ZCVIEWING_KEY]  = {0xA8,0xAC,0x0C};
    base58Prefixes[ZCSPENDING_KEY] = {177,235};

    bech32HRPs[SAPLING_PAYMENT_ADDRESS]      = "ztestsapling";
    bech32HRPs[SAPLING_FULL_VIEWING_KEY]     = "zviewtestsapling";
    bech32HRPs[SAPLING_INCOMING_VIEWING_KEY] = "zivktestsapling";
    bech32HRPs[SAPLING_EXTENDED_SPEND_KEY]   = "secret-extended-key-test";

    vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

    //fRequireRPCPassword = true;
    fMiningRequiresPeers = false;//true;
    fDefaultConsistencyChecks = false;
    fRequireStandard = true;
    fMineBlocksOnDemand = false;
    fTestnetToBeDeprecatedFieldRPC = true;


    checkpointData = (CCheckpointData) {
        boost::assign::map_list_of
        (0, consensus.hashGenesisBlock)
        (38000, uint256S("0x001e9a2d2e2892b88e9998cf7b079b41d59dd085423a921fe8386cecc42287b8")),
        1486897419,  // * UNIX timestamp of last checkpoint block
        47163,       // * total number of transactions between genesis and last checkpoint
                        //   (the tx=... number in the SetBestChain debug.log lines)
        715          //   total number of tx / (checkpoint block height / (24 * 24))
    };
}

CRegTestParams::CRegTestParams() 
{
    strNetworkID = "regtest";
    strCurrencyUnits = "REG";
    bip44CoinType = 1;
    consensus.fCoinbaseMustBeProtected = false;
    consensus.nSubsidySlowStartInterval = 0;
    consensus.nSubsidyHalvingInterval = 150;
    consensus.nMajorityEnforceBlockUpgrade = 750;
    consensus.nMajorityRejectBlockOutdated = 950;
    consensus.nMajorityWindow = 1000;
    consensus.powLimit = uint256S("0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f");
    consensus.powAlternate = uint256S("0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f");
    consensus.nPowAveragingWindow = 17;
    consensus.nMaxFutureBlockTime = 7 * 60; // 7 mins
    assert(maxUint/UintToArith256(consensus.powLimit) >= consensus.nPowAveragingWindow);
    consensus.nPowMaxAdjustDown = 0; // Turn off adjustment down
    consensus.nPowMaxAdjustUp = 0; // Turn off adjustment up
    consensus.nPowTargetSpacing = 2.5 * 60;
    consensus.nPowAllowMinDifficultyBlocksAfterHeight = 0;
    consensus.vUpgrades[Consensus::BASE_SPROUT].nProtocolVersion = 170002;
    consensus.vUpgrades[Consensus::BASE_SPROUT].nActivationHeight =
        Consensus::NetworkUpgrade::ALWAYS_ACTIVE;
    consensus.vUpgrades[Consensus::UPGRADE_TESTDUMMY].nProtocolVersion = 170002;
    consensus.vUpgrades[Consensus::UPGRADE_TESTDUMMY].nActivationHeight =
        Consensus::NetworkUpgrade::NO_ACTIVATION_HEIGHT;
    consensus.vUpgrades[Consensus::UPGRADE_OVERWINTER].nProtocolVersion = 170003;
    consensus.SetOverwinterHeight(Consensus::NetworkUpgrade::NO_ACTIVATION_HEIGHT);
    consensus.vUpgrades[Consensus::UPGRADE_SAPLING].nProtocolVersion = 170006;
    consensus.SetSaplingHeight(Consensus::NetworkUpgrade::NO_ACTIVATION_HEIGHT);

    // The best chain should have at least this much work.
    consensus.nMinimumChainWork = uint256S("0x00");

    pchMessageStart[0] = 0xaa;
    pchMessageStart[1] = 0x8e;
    pchMessageStart[2] = 0xf3;
    pchMessageStart[3] = 0xf5;
    nMinerThreads = 1;
    nMaxTipAge = 24 * 60 * 60;
    nPruneAfterHeight = 1000;
    const size_t N = 48, K = 5;
    BOOST_STATIC_ASSERT(equihash_parameters_acceptable(N, K));
    nEquihashN = N;
    nEquihashK = K;

    genesis = CreateGenesisBlock(
        1296688602,
        uint256S("0x0000000000000000000000000000000000000000000000000000000000000009"),
        ParseHex("01936b7db1eb4ac39f151b8704642d0a8bda13ec547d54cd5e43ba142fc6d8877cab07b3"),
        KOMODO_MINDIFF_NBITS, 4, 0);

    consensus.hashGenesisBlock = genesis.GetHash();
    assert(consensus.hashGenesisBlock == uint256S("0x029f11d80ef9765602235e1bc9727e3eb6ba20839319f761fee920d63401e327"));
    assert(genesis.hashMerkleRoot == uint256S("0xc4eaa58879081de3c24a7b117ed2b28300e7ec4c4c1dff1d3f1268b7857a4ddb"));

    nDefaultPort = 17779;
    nPruneAfterHeight = 1000;

    vFixedSeeds.clear(); //! Regtest mode doesn't have any fixed seeds.
    vSeeds.clear();  //! Regtest mode doesn't have any DNS seeds.

    fMiningRequiresPeers = false;
    fDefaultConsistencyChecks = true;
    fRequireStandard = false;
    fMineBlocksOnDemand = true;
    fTestnetToBeDeprecatedFieldRPC = false;

    checkpointData = (CCheckpointData){
        boost::assign::map_list_of
        ( 0, uint256S("0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206")),
        0,
        0,
        0
    };
    // These prefixes are the same as the testnet prefixes
    base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,60);
    base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,85);
    base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,188);
    //base58Prefixes[PUBKEY_ADDRESS]     = {0x1D,0x25};
    //base58Prefixes[SCRIPT_ADDRESS]     = {0x1C,0xBA};
    //base58Prefixes[SECRET_KEY]         = {0xEF};
    // do not rely on these BIP32 prefixes; they are not specified and may change
    base58Prefixes[EXT_PUBLIC_KEY]     = {0x04,0x35,0x87,0xCF};
    base58Prefixes[EXT_SECRET_KEY]     = {0x04,0x35,0x83,0x94};
    base58Prefixes[ZCPAYMENT_ADDRRESS] = {0x16,0xB6};
    base58Prefixes[ZCVIEWING_KEY]      = {0xA8,0xAC,0x0C};
    base58Prefixes[ZCSPENDING_KEY]     = {0xAC,0x08};

    bech32HRPs[SAPLING_PAYMENT_ADDRESS]      = "zregtestsapling";
    bech32HRPs[SAPLING_FULL_VIEWING_KEY]     = "zviewregtestsapling";
    bech32HRPs[SAPLING_INCOMING_VIEWING_KEY] = "zivkregtestsapling";
    bech32HRPs[SAPLING_EXTENDED_SPEND_KEY]   = "secret-extended-key-regtest";

    // Founders reward script expects a vector of 2-of-3 multisig addresses
    vFoundersRewardAddress = { "t2FwcEhFdNXuFMv1tcYwaBJtYVtMj8b1uTg" };
    assert(vFoundersRewardAddress.size() <= consensus.GetLastFoundersRewardBlockHeight());
}

void CRegTestParams::UpdateNetworkUpgradeParameters(Consensus::UpgradeIndex idx, int nActivationHeight)
{
    assert(idx > Consensus::BASE_SPROUT && idx < Consensus::MAX_NETWORK_UPGRADES);
    consensus.vUpgrades[idx].nActivationHeight = nActivationHeight;
}
