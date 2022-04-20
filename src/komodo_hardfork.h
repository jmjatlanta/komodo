#pragma once
/******************************************************************************
 * Copyright © 2022 The Komodo Project Developers.                            *
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
/**************
 * Place hardfork triggers in this file. This centralizes them for easier documentation and 
 * reference. It also provides a mechanism to have different triggers for different
 * environments.
 * 
 * TODO: Setting this up in chainparams.h and again in this file is a pain. Find a more
 * efficient way. Optimizing this with -O1 normally results in the value being a literal,
 * so finding a way that does not cause a performance hit would be great.
 *************/
#include "chainparams.h"

#include <cstdint>

template<class T>
class ChainParamsWithHardfork : public ChainParams
{
public:
    constexpr uint32_t Season1StartHeight() const override { return T::nSeason1StartHeight; }
    constexpr uint32_t Season1StartTimestamp() const override { return T::nSeason1StartTimestamp; }
    constexpr uint32_t Season2StartHeight() const override { return T::nSeason2StartHeight; }
    constexpr uint32_t Season2StartTimestamp() const override { return T::nSeason2StartTimestamp; }
    constexpr uint32_t Season3StartHeight() const override { return T::nSeason3StartHeight; }
    constexpr uint32_t Season3StartTimestamp() const override { return T::nSeason3StartTimestamp; }
    constexpr uint32_t Season4StartHeight() const override { return T::nSeason4StartHeight; }
    constexpr uint32_t Season4StartTimestamp() const override { return T::nSeason4StartTimestamp; }
    constexpr uint32_t Season5StartHeight() const override { return T::nSeason5StartHeight; }
    constexpr uint32_t Season5StartTimestamp() const override { return T::nSeason5StartTimestamp; }
    constexpr uint32_t Season6StartHeight() const override { return T::nSeason6StartHeight; }
    constexpr uint32_t Season6StartTimestamp() const override { return T::nSeason6StartTimestamp; }
    constexpr int32_t NotaryLowerDifficultyStartHeight() const override { return T::nNotaryLowerDifficultyStartHeight; }
    constexpr int32_t NotaryLimitRepeatHeight() const override { return T::nNotaryLimitRepeatHeight; }
    constexpr int32_t NotarySpecialStartHeight() const override { return T::nNotarySpecialStartHeight; }
    constexpr int32_t NotarySpecialTimeTooShortHeight() const override { return T::nNotarySpecialTimeTooShortHeight; }
    constexpr int32_t NotaryMovedTo66() const override { return T::nNotaryMovedTo66; }
    constexpr int32_t NotaryOncePerCycle() const override { return T::nNotaryOncePerCycle; }
    constexpr int32_t NotarySAndS2StartHeight() const override { return T::nNotarySAndS2StartHeight; }
    constexpr int32_t NotaryS2StartHeight() const override { return T::nNotaryS2StartHeight; }
    constexpr int32_t NotaryS2IncludeElectionGapHeight() const override { return T::nNotaryS2ElectionGap; }
    constexpr int32_t NotaryElectionGapOverrideHeight() const override { return T::nNotaryElectionGapOverride; }
    constexpr int32_t NotarySpecialFlagHeight() const override { return T::nNotarySpecialFlagHeight; }
    constexpr uint32_t NotaryEligibleEvenIfMinedRecently() const override { return T::nNotaryEligibleEvenIfMinedRecentlyHeight; }
    constexpr uint32_t EnableValidateInterestHeight() const override { return T::nEnableValidateInterestHeight; }
    constexpr uint32_t ValidateInterestTrueTimeHeight() const override { return T::nValidateInterestTrueTimeHeight; }
    constexpr uint32_t AdaptivePoWMinHeight() const override { return T::nAdaptivePoWMinHeight; }
    constexpr uint32_t KomodoEarlyTXIDHeight() const override { return T::nKomodoEarlyTXIDHeight; }
    constexpr uint32_t EnableCheckPoWHeight() const  override { return T::nEnableCheckPoWHeight; }
    constexpr uint32_t MinPoWHeight() const override { return T::nMinPoWHeight; }
    constexpr uint32_t KomodoNotaryLowerLimitHeight() const override { return T::nKomodoNotaryLowerLimitHeight; }
    constexpr uint32_t KomodoNotaryUpperLimitHeight() const override { return T::nKomodoNotaryUpperLimitHeight; }
    constexpr uint32_t KomodoPaxLowerLimitHeight() const override { return T::nKomodoPaxLowerLimitHeight; }
    constexpr uint32_t KomodoPrintNonZMessageHeight() const override { return T::nKomodoPrintNonZMessage; }
    constexpr uint32_t KomodoGatewayRejectStrangeoutHeight() const override { return T::nKomodoGatewayRejectStrangeoutHeight; }
    constexpr uint32_t KomodoGatewayPaxHeight() const override { return T::nKomodoGatewayPaxHeight; }
    constexpr uint32_t KomodoGatewayPaxMessageLowerHeight() const override { return T::nKomodoGatewayPaxMessageLowerHeight; }
    constexpr uint32_t KomodoGatewayPaxMessageUpperHeight() const override { return T::nKomodoGatewayPaxMessageUpperHeight; }
    constexpr uint32_t KomodoNotariesHardcodedHeight() const override { return T::nKomodoNotariesHardcodedHeight; }
    constexpr uint32_t KomodoOpReturnSwapHeight() const override { return T::nKomodoOpReturnSwapHeight; }
    constexpr uint32_t KomodoSignedMaskChangeHeight() const override { return T::nKomodoSignedMaskChangeHeight; }
    constexpr uint32_t KomodoMinRatifyHeight() const override { return T::nKomodoMinRatifyHeight; }
    constexpr uint32_t KomodoInterestMinSpendHeight() const override { return T::nKomodoInterestMinSpendHeight; }
};

class MainnetHardfork
{
public:
    static const uint32_t nSeason1StartHeight = 814000;
    static const uint32_t nSeason1StartTimestamp = 1525132800;
    static const uint32_t nSeason2StartHeight = 1444000;
    static const uint32_t nSeason2StartTimestamp = 1563148800;
    static const uint32_t nSeason3StartHeight = 1670000;   //December 2019 hardfork
    static const uint32_t nSeason3StartTimestamp = 1576840000; //December 2019 hardfork 12/20/2019 @ 11:06am (UTC)
    static const uint32_t nSeason4StartHeight = 1922000;   //dPoW Season 4 2020 hardfork Sunday, June 14th, 2020 
    static const uint32_t nSeason4StartTimestamp = 1592146800; //dPoW Season 4 2020 hardfork Sunday, June 14th, 2020 03:00:00 PM UTC
    static const uint32_t nSeason5StartHeight = 2437300;  //dPoW Season 5 Monday, June 14th, 2021
    static const uint32_t nSeason5StartTimestamp = 1623682800;  //dPoW Season 5 Monday, June 14th, 2021 (03:00:00 PM UTC)
    static const uint32_t nSeason6StartHeight = 7113400; // unknown, set way into the future
    static const uint32_t nSeason6StartTimestamp = 1751328000; // unknown, set way into the future
    static const int32_t nNotaryLowerDifficultyStartHeight = 82000;
    static const int32_t nNotaryLimitRepeatHeight = 792000;
    static const int32_t nNotarySpecialStartHeight = 34000;
    static const int32_t nNotarySpecialTimeTooShortHeight = 807000;
    static const int32_t nNotaryMovedTo66 = 79693;
    static const int32_t nNotaryOncePerCycle = 225000;
    static const int32_t nNotarySAndS2StartHeight = 10000;
    static const int32_t nNotaryS2StartHeight = 80000;
    static const int32_t nNotaryS2ElectionGap = 108000;
    static const int32_t nNotaryElectionGapOverride = 1000000;
    static const int32_t nNotarySpecialFlagHeight = 790833;
    static const uint32_t nNotaryEligibleEvenIfMinedRecentlyHeight = 186233;
    static const uint32_t nEnableValidateInterestHeight = 246748;
    static const uint32_t nValidateInterestTrueTimeHeight = 247205;
    static const uint32_t nAdaptivePoWMinHeight = 10;
    static const uint32_t nKomodoEarlyTXIDHeight = 100;
    static const uint32_t nEnableCheckPoWHeight = 100;
    static const uint32_t nMinPoWHeight = 2;
    static const uint32_t nKomodoNotaryLowerLimitHeight = 235300;
    static const uint32_t nKomodoNotaryUpperLimitHeight = 236000;
    static const uint32_t nKomodoPaxLowerLimitHeight = 165000;
    static const uint32_t nKomodoPrintNonZMessage = 800000;
    static const uint32_t nKomodoGatewayRejectStrangeoutHeight = 1000000;
    static const uint32_t nKomodoGatewayPaxHeight = 195000;
    static const uint32_t nKomodoGatewayPaxMessageLowerHeight = 214700;
    static const uint32_t nKomodoGatewayPaxMessageUpperHeight = 238000;
    static const uint32_t nKomodoNotariesHardcodedHeight = 180000;
    static const uint32_t nKomodoOpReturnSwapHeight = 121842;
    static const uint32_t nKomodoSignedMaskChangeHeight = 91400;
    static const uint32_t nKomodoMinRatifyHeight = 90000;
    static const uint32_t nKomodoInterestMinSpendHeight = 60000;
};

class TestnetHardfork
{
public:
    static const uint32_t nSeason1StartHeight = 10;
    static const uint32_t nSeason1StartTimestamp = 1525132800;
    static const uint32_t nSeason2StartHeight = 20;
    static const uint32_t nSeason2StartTimestamp = 1563148800;
    static const int32_t nSeason3StartHeight = 3;   //December 2019 hardfork
    static const uint32_t nSeason3StartTimestamp = 1576840000; //December 2019 hardfork 12/20/2019 @ 11:06am (UTC)
    static const int32_t nSeason4StartHeight = 6;   //dPoW Season 4 2020 hardfork Sunday, June 14th, 2020 
    static const uint32_t nSeason4StartTimestamp = 1592146800; //dPoW Season 4 2020 hardfork Sunday, June 14th, 2020 03:00:00 PM UTC
    static const int32_t nSeason5StartHeight = 9;  //dPoW Season 5 Monday, June 14th, 2021
    static const uint32_t nSeason5StartTimestamp = 1623682800;  //dPoW Season 5 Monday, June 14th, 2021 (03:00:00 PM UTC)
    static const uint32_t nSeason6StartHeight = 1000; // unknown, set way into the future
    static const uint32_t nSeason6StartTimestamp = 1751328000; // unknown, set way into the future
    static const int32_t nNotaryLowerDifficultyStartHeight = 30;
    static const int32_t nNotaryLimitRepeatHeight = 40;
    static const int32_t nNotarySpecialStartHeight = 20;
    static const int32_t nNotarySpecialTimeTooShortHeight = 50;
    static const int32_t nNotaryMovedTo66 = 10;
    static const int32_t nNotaryOncePerCycle = 10;
    static const int32_t nNotarySAndS2StartHeight = 10;
    static const int32_t nNotaryS2StartHeight = 20;
    static const int32_t nNotaryS2ElectionGap = 30;
    static const int32_t nNotaryElectionGapOverride = 40;
    static const int32_t nNotarySpecialFlagHeight = 38;
    static const uint32_t nNotaryEligibleEvenIfMinedRecentlyHeight = 3;
    static const uint32_t nEnableValidateInterestHeight = 3;
    static const uint32_t nValidateInterestTrueTimeHeight = 4;
    static const uint32_t nAdaptivePoWMinHeight = 10;
    static const uint32_t nKomodoEarlyTXIDHeight = 100;
    static const uint32_t nEnableCheckPoWHeight = 100;
    static const uint32_t nMinPoWHeight = 2;
    static const uint32_t nKomodoNotaryLowerLimitHeight = 150;
    static const uint32_t nKomodoNotaryUpperLimitHeight = 175;
    static const uint32_t nKomodoPaxLowerLimitHeight = 160;
    static const uint32_t nKomodoPrintNonZMessage = 150;
    static const uint32_t nKomodoGatewayRejectStrangeoutHeight = 200;
    static const uint32_t nKomodoGatewayPaxHeight = 195000;
    static const uint32_t nKomodoGatewayPaxMessageLowerHeight = 214700;
    static const uint32_t nKomodoGatewayPaxMessageUpperHeight = 238000;
    static const uint32_t nKomodoNotariesHardcodedHeight = 5;
    static const uint32_t nKomodoOpReturnSwapHeight = 121842;
    static const uint32_t nKomodoSignedMaskChangeHeight = 91400;
    static const uint32_t nKomodoMinRatifyHeight = 90000;
    static const uint32_t nKomodoInterestMinSpendHeight = 60000;
};

class RegtestHardfork
{
public:
    static const uint32_t nSeason1StartHeight = 10;
    static const uint32_t nSeason1StartTimestamp = 1525132800;
    static const uint32_t nSeason2StartHeight = 20;
    static const uint32_t nSeason2StartTimestamp = 1563148800;
    static const int32_t nSeason3StartHeight = 3;   //December 2019 hardfork
    static const uint32_t nSeason3StartTimestamp = 1576840000; //December 2019 hardfork 12/20/2019 @ 11:06am (UTC)
    static const int32_t nSeason4StartHeight = 6;   //dPoW Season 4 2020 hardfork Sunday, June 14th, 2020 
    static const uint32_t nSeason4StartTimestamp = 1592146800; //dPoW Season 4 2020 hardfork Sunday, June 14th, 2020 03:00:00 PM UTC
    static const uint32_t nSeason5StartTimestamp = 1623682800;  //dPoW Season 5 Monday, June 14th, 2021 (03:00:00 PM UTC)
    static const int32_t nSeason5StartHeight = 9;  //dPoW Season 5 Monday, June 14th, 2021
    static const uint32_t nSeason6StartHeight = 1000; // unknown, set way into the future
    static const uint32_t nSeason6StartTimestamp = 1751328000; // unknown, set way into the future
    static const int32_t nNotaryLowerDifficultyStartHeight = 30;
    static const int32_t nNotaryLimitRepeatHeight = 40;
    static const int32_t nNotarySpecialStartHeight = 20;
    static const int32_t nNotarySpecialTimeTooShortHeight = 50;
    static const int32_t nNotaryMovedTo66 = 10;
    static const int32_t nNotaryOncePerCycle = 10;
    static const int32_t nNotarySAndS2StartHeight = 10;
    static const int32_t nNotaryS2StartHeight = 20;
    static const int32_t nNotaryS2ElectionGap = 30;
    static const int32_t nNotaryElectionGapOverride = 40;
    static const int32_t nNotarySpecialFlagHeight = 38;
    static const uint32_t nNotaryEligibleEvenIfMinedRecentlyHeight = 3;
    static const uint32_t nEnableValidateInterestHeight = 3;
    static const uint32_t nValidateInterestTrueTimeHeight = 4;
    static const uint32_t nAdaptivePoWMinHeight = 10;
    static const uint32_t nKomodoEarlyTXIDHeight = 100;
    static const uint32_t nEnableCheckPoWHeight = 100;
    static const uint32_t nMinPoWHeight = 2;
    static const uint32_t nKomodoNotaryLowerLimitHeight = 150;
    static const uint32_t nKomodoNotaryUpperLimitHeight = 175;
    static const uint32_t nKomodoPaxLowerLimitHeight = 160;
    static const uint32_t nKomodoPrintNonZMessage = 150;
    static const uint32_t nKomodoGatewayRejectStrangeoutHeight = 200;
    static const uint32_t nKomodoGatewayPaxHeight = 195000;
    static const uint32_t nKomodoGatewayPaxMessageLowerHeight = 214700;
    static const uint32_t nKomodoGatewayPaxMessageUpperHeight = 238000;
    static const uint32_t nKomodoNotariesHardcodedHeight = 5;
    static const uint32_t nKomodoOpReturnSwapHeight = 121842;
    static const uint32_t nKomodoSignedMaskChangeHeight = 91400;
    static const uint32_t nKomodoMinRatifyHeight = 90000;
    static const uint32_t nKomodoInterestMinSpendHeight = 50;
};
