#include "assetchain.h"
#include "komodo_defs.h"

#include <boost/assign.hpp>

CChainParams &assetchain::Params(CBaseChainParams::Network network) {
    switch (network) {
        case CBaseChainParams::MAIN:
            return mainParams;
        case CBaseChainParams::TESTNET:
            return testNetParams;
        case CBaseChainParams::REGTEST:
            return regTestParams;
        default:
            assert(false && "Unimplemented network");
            return mainParams;
    }
}

void assetchain::SelectParams(CBaseChainParams::Network network) {
    SelectBaseParams(network);
    pCurrentParams = &Params(network);

    // Some python qa rpc tests need to enforce the coinbase consensus rule
    if (network == CBaseChainParams::REGTEST && mapArgs.count("-regtestprotectcoinbase")) {
        regTestParams.SetRegTestCoinbaseMustBeProtected();
    }
}

bool assetchain::SelectParamsFromCommandLine()
{
    CBaseChainParams::Network network = NetworkIdFromCommandLine();
    if (network == CBaseChainParams::MAX_NETWORK_TYPES)
        return false;

    SelectParams(network);

    return true;
}


void assetchain::InitChainParams(int64_t saplingActivationHeight, int64_t overwinterActivationHeight)
{
    CChainParams::CCheckpointData checkpointData;
    if ( !this->isKMD() )
    {
        if ( ASSETCHAINS_BLOCKTIME != 60 )
        {
            pCurrentParams->consensus.nMaxFutureBlockTime = 7 * ASSETCHAINS_BLOCKTIME; // 7 blocks
            pCurrentParams->consensus.nPowTargetSpacing = ASSETCHAINS_BLOCKTIME;
        }
        pCurrentParams->SetDefaultPort(ASSETCHAINS_P2PPORT);
        if ( ASSETCHAINS_NK[0] != 0 && ASSETCHAINS_NK[1] != 0 )
        {
            //BOOST_STATIC_ASSERT(equihash_parameters_acceptable(ASSETCHAINS_NK[0], ASSETCHAINS_NK[1]));
            pCurrentParams->SetNValue(ASSETCHAINS_NK[0]);
            pCurrentParams->SetKValue(ASSETCHAINS_NK[1]);
        }
        if ( IS_KOMODO_TESTNODE )
            pCurrentParams->SetMiningRequiresPeers(false);
        if ( ASSETCHAINS_RPCPORT == 0 )
            ASSETCHAINS_RPCPORT = ASSETCHAINS_P2PPORT + 1;
        pCurrentParams->pchMessageStart[0] = ASSETCHAINS_MAGIC & 0xff;
        pCurrentParams->pchMessageStart[1] = (ASSETCHAINS_MAGIC >> 8) & 0xff;
        pCurrentParams->pchMessageStart[2] = (ASSETCHAINS_MAGIC >> 16) & 0xff;
        pCurrentParams->pchMessageStart[3] = (ASSETCHAINS_MAGIC >> 24) & 0xff;
        fprintf(stderr,">>>>>>>>>> %s: p2p.%u rpc.%u magic.%08x %u %u coins\n",chain.symbol().c_str(),ASSETCHAINS_P2PPORT,ASSETCHAINS_RPCPORT,ASSETCHAINS_MAGIC,ASSETCHAINS_MAGIC,(uint32_t)ASSETCHAINS_SUPPLY);
        if (ASSETCHAINS_ALGO == ASSETCHAINS_VERUSHASH)
        {
            // this is only good for 60 second blocks with an averaging window of 45. for other parameters, use:
            // nLwmaAjustedWeight = (N+1)/2 * (0.9989^(500/nPowAveragingWindow)) * nPowTargetSpacing
            pCurrentParams->consensus.nLwmaAjustedWeight = 1350;
            pCurrentParams->consensus.nPowAveragingWindow = 45;
            pCurrentParams->consensus.powAlternate = uint256S("00000f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f");
        }
        else if (ASSETCHAINS_ALGO == ASSETCHAINS_VERUSHASHV1_1)
        {
            // this is only good for 60 second blocks with an averaging window of 45. for other parameters, use:
            // nLwmaAjustedWeight = (N+1)/2 * (0.9989^(500/nPowAveragingWindow)) * nPowTargetSpacing
            pCurrentParams->consensus.nLwmaAjustedWeight = 1350;
            pCurrentParams->consensus.nPowAveragingWindow = 45;
            pCurrentParams->consensus.powAlternate = uint256S("0000000f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f");
        }

        if (ASSETCHAINS_LWMAPOS != 0)
        {
            pCurrentParams->consensus.posLimit = uint256S("000000000f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f");
            pCurrentParams->consensus.nPOSAveragingWindow = 45;
            // spacing is 1000 units per block to get better resolution, POS is 50% hard coded for now, we can vary it later
            // when we get reliable integer math on nLwmaPOSAjustedWeight
            pCurrentParams->consensus.nPOSTargetSpacing = VERUS_BLOCK_POSUNITS * 2;
            // nLwmaPOSAjustedWeight = (N+1)/2 * (0.9989^(500/nPOSAveragingWindow)) * nPOSTargetSpacing
            // this needs to be recalculated if VERUS_BLOCK_POSUNITS is changed
            pCurrentParams->consensus.nLwmaPOSAjustedWeight = 46531;
        }

        // only require coinbase protection on Verus from the Komodo family of coins
        if ( chain.isSymbol("VRSC") )
        {
            pCurrentParams->SetSaplingHeight(227520);
            pCurrentParams->SetOverwinterHeight(227520);
            pCurrentParams->consensus.fCoinbaseMustBeProtected = true;
            checkpointData = //(Checkpoints::CCheckpointData)
                    {
                            boost::assign::map_list_of
                                    (0, pCurrentParams->consensus.hashGenesisBlock)
                                    (10000, uint256S("0xac2cd7d37177140ea4991cf630c0b9c7f94d707b84fb0351bf3a44856d2ae5dc"))
                                    (20000, uint256S("0xb0e8cb9f77aaa7ff5bd90d6c08d06f4c4bf03e00c2b8a35a042e760845590c8a"))
                                    (30000, uint256S("0xf2112ca577338ad7104bf905fa6a63d36b17a86f914c97b73cd31d43fcd7557c"))
                                    (40000, uint256S("0x00000000008f83378dab727864b763ce91a4ea5f75d55939c0c1390cfb8c38f1"))
                                    (49170, uint256S("0x2add646c0089871ec2379f02f7cd60b3af6efd9c152a6f16fc10925458c270cc")),
                            (int64_t)1529910234,    // * UNIX timestamp of last checkpoint block
                            (int64_t)63661,         // * total number of transactions between genesis and last checkpoint
                            //   (the tx=... number in the SetBestChain debug.log lines)
                            (double)2777            // * estimated number of transactions per day after checkpoint
                            //   total number of tx / (checkpoint block height / (24 * 24))
                    };

            pCurrentParams->consensus.nMinimumChainWork = uint256S("0x000000000000000000000000000000000000000000000001a8f4f23f8b2d1f7e");
        }
        else
        {
            if ( chain.symbol() == "VRSCTEST" || chain.symbol() == "VERUSTEST" )
            {
                pCurrentParams->consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000000000001f7e");
            }
            pCurrentParams->SetSaplingHeight(saplingActivationHeight);
            pCurrentParams->SetOverwinterHeight(overwinterActivationHeight);
            checkpointData = //(Checkpoints::CCheckpointData)
                    {
                            boost::assign::map_list_of
                                    (0, pCurrentParams->consensus.hashGenesisBlock),
                            (int64_t)1231006505,
                            (int64_t)1,
                            (double)2777            // * estimated number of transactions per day after checkpoint
                            //   total number of tx / (checkpoint block height / (24 * 24))
                    };
        }
    }
    else
    {
        checkpointData = // (Checkpoints::CCheckpointData)
            {
                boost::assign::map_list_of

                (0, pCurrentParams->consensus.hashGenesisBlock)
                (	50000,	uint256S("0x00076e16d3fa5194da559c17cf9cf285e21d1f13154ae4f7c7b87919549345aa"))
                (	100000,	uint256S("0x0f02eb1f3a4b89df9909fec81a4bd7d023e32e24e1f5262d9fc2cc36a715be6f"))
                (	150000,	uint256S("0x0a817f15b9da636f453a7a01835cfc534ed1a55ce7f08c566471d167678bedce"))
                (	200000,	uint256S("0x000001763a9337328651ca57ac487cc0507087be5838fb74ca4165ff19f0e84f"))
                (	250000,	uint256S("0x0dd54ef5f816c7fde9d2b1c8c1a26412b3c761cc5dd3901fa5c4cd1900892fba"))
                (	300000,	uint256S("0x000000fa5efd1998959926047727519ed7de06dcf9f2cd92a4f71e907e1312dc"))
                (	350000,	uint256S("0x0000000228ef321323f81dae00c98d7960fc7486fb2d881007fee60d1e34653f"))
                (	400000,	uint256S("0x036d294c5be96f4c0efb28e652eb3968231e87204a823991a85c5fdab3c43ae6"))
                (	450000,	uint256S("0x0906ef1e8dc194f1f03bd4ce1ac8c6992fd721ef2c5ccbf4871ec8cdbb456c18"))
                (	500000,	uint256S("0x0bebdb417f7a51fe0c36fcf94e2ed29895a9a862eaa61601272866a7ecd6391b"))
                (	550000,	uint256S("0x06df52fc5f9ba03ccc3a7673b01ab47990bd5c4947f6e1bc0ba14d21cd5bcccd"))
                (	600000,	uint256S("0x00000005080d5689c3b4466e551cd1986e5d2024a62a79b1335afe12c42779e4"))
                (	650000,	uint256S("0x039a3cb760cc6e564974caf69e8ae621c14567f3a36e4991f77fd869294b1d52"))
                (	700000,	uint256S("0x00002285be912b2b887a5bb42d2f1aa011428c565b0ffc908129c47b5ce87585"))
                (	750000,	uint256S("0x04cff4c26d185d591bed3613ce15e1d15d9c91dd8b98a6729f89c58ce4bd1fd6"))
                (	800000,	uint256S("0x0000000617574d402fca8e6570f0845bd5fe449398b318b4e1f65bc69cdd6606"))
                (	850000,	uint256S("0x044199301f37194f20ba7b498fc72ed742f6c0ba6e476f28d6c81d225e58d5ce"))
                (	900000,	uint256S("0x08bdbe4de2a65ac89fd2913192d05362c900e3af476a0c99d9f311875067451e"))
                (	950000,	uint256S("0x0000000aa9a44b593e6138f247bfae75bd43b9396ef9ff0a6a3ebd852f131806"))
                (	1000000,	uint256S("0x0cb1d2457eaa58af5028e86e27ac54578fa09558206e7b868ebd35e7005ed8bb"))
                (	1050000,	uint256S("0x044d49bbc3bd9d32b6288b768d4f7e0afe3cbeda606f3ac3579a076e4bddf6ae"))
                (	1100000,	uint256S("0x000000050cad04887e170059dd2556d85bbd20390b04afb9b07fb62cafd647b4"))
                (	1150000,	uint256S("0x0c85501c759d957dd1ccc5f7fdfcc415c89c7f2a26471fffc75b75f79e63c16a"))
                (	1200000,	uint256S("0x0763cbf43ed7227988081c29d9e9fc7ab2450216e6d0354cc4596c86689702d4"))
                (	1250000,	uint256S("0x0489640207f8c343a56a10e45d987516059ea82a3c6859a771b3a9cf94f5c3bb"))
                (	1300000,	uint256S("0x000000012a01709b254b4f75e2b9ed772d8fe558655c8c859892ca8c9d625e87"))
                (	1350000,	uint256S("0x075a1a5c66a68b47d9848ca6986687ed2665b1852457051bf142208e62f98a60"))
                (	1400000,	uint256S("0x055f73dd9b20650c3d6e6dbb606af8d9479e4c81d89430867abff5329f167bb2"))
                (	1450000,	uint256S("0x014c2926e07e9712211c5e82f05df1b802c59cc8bc24e3cc9b09942017080f2d"))
                (	1500000,	uint256S("0x0791f892210ce3c513ab607d689cd1e8907a27f3dfeb58dec21ae299b7981cb7"))
                (	1550000,	uint256S("0x08fcbaffb7164b161a25efc6dd5c70b679498ee637d663fe201a55c7decc37a3"))
                (	1600000,	uint256S("0x0e577dcd49319a67fe2acbb39ae6d46afccd3009d3ba9d1bcf6c624708e12bac"))
                (	1650000,	uint256S("0x091ac57a0f786a9526b2224a10b62f1f464b9ffc0afc2240d86264439e6ad3d0"))
                (	1700000,	uint256S("0x0d0be6ab4a5083ce9d2a7ea2549b03cfc9770427b7d51c0bf0c603399a60d037"))
                (	1750000,	uint256S("0x0a019d830157db596eeb678787279908093fd273a4e022b5e052f3a9f95714ca"))
                (	1800000,	uint256S("0x0390779f6c615620391f9dd7df7f3f4947523bd6350b26625c0315571c616076"))
                (	1850000,	uint256S("0x000000007ca2de1bd9cb7b52fe0accca4874143822521d955e58c73e304279e0"))
                (	1900000,	uint256S("0x04c6589d5703f8237bf215c4e3e881c1c77063ef079cea5dc132a0e7f7a0cbd9"))
                (	1950000,	uint256S("0x00000000386795b9fa21f14782ee1b9176763d6a544d7e0511d1860c62d540aa"))
                (	2000000,	uint256S("0x0b0403fbe3c5742bbd0e3fc643049534e50c4a49bbc4a3b284fc0ecedf15c044"))
                (	2050000,	uint256S("0x0c7923957469864d49a0be56b5ffbee7f21c1b6d00acc7374f60f1c1c7b87e14"))
                (	2100000,	uint256S("0x05725ed166ae796529096ac2a42e85a3bdd0d69dbb2f69e620c08219fda1130a"))
                (	2150000,	uint256S("0x0edb94f5a5501fc8dd72a455cdaccf0af0215b914dd3d8d4ae5d644e27ef562c"))
                (	2200000,	uint256S("0x08b92203aa4a3b09001a75e8eebe8e64434259ae7ed7a31384203924d1ab03b8"))
                (	2250000,	uint256S("0x0127d1ed5cd6f261631423275b6b17728c392583177e1151a6e638a4b0dea985"))
                (	2300000,	uint256S("0x07df8af646bc30c71d068b146d9ea2c8b25b27a180e9537d5aef859efcfc41f7"))
                (	2350000,	uint256S("0x0b8028dbfcd92fe34496953872cba2d256923e3e52b4abbdcbe9911071e929e5"))
                (	2395555,	uint256S("0x0a09f16d886ed8152aaa2e2fcdf6ab4bb142ff8ce5abac131c5eda385a5d712f")),
                1621188001,     // * UNIX timestamp of last checkpoint block
                13903562,         // * total number of transactions between genesis and last checkpoint
                                //   (the tx=... number in the SetBestChain debug.log lines)
                2777            // * estimated number of transactions per day after checkpoint
                                //   total number of tx / (checkpoint block height / (24 * 24))
            };
    }

    pCurrentParams->SetCheckpointData(checkpointData);

    initialized = true;
    return;
}
