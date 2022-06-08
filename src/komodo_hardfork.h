#pragma once

#include <cstdint>

#include "komodo_defs.h"
#include "chainparams.h"

template<class T>
class ChainParamsWithHardfork : public ChainParams
{
public:
    constexpr uint32_t KomodoNotariesHardcoded() const override { return T::komodoNotariesHardcoded; }
    constexpr int32_t S1HardforkHeight() const override { return T::s1HardforkHeight; }
    constexpr int32_t S2HardforkHeight() const override { return T::s2HardforkHeight; }
    constexpr int32_t DecemberHardforkHeight() const override { return T::nDecemberHardforkHeight; }
    constexpr uint32_t StakedDecemberHardforkTimestamp() const override { return T::nStakedDecemberHardforkTimestamp; }
    constexpr uint32_t S4Timestamp() const override { return T::nS4Timestamp; }
    constexpr int32_t S4HardforkHeight() const override { return T::nS4HardforkHeight; }
    constexpr uint32_t S5Timestamp() const override { return T::nS5Timestamp; }
    constexpr int32_t S5HardforkHeight() const { return T::nS5HardforkHeight; }
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
};

class MainnetHardfork
{
public:
    static const uint32_t komodoNotariesHardcoded = 180000;
    static const int32_t s1HardforkHeight = 814000;
    static const int32_t s2HardforkHeight = 2588672;
    static const uint32_t nStakedDecemberHardforkTimestamp = 1576840000; //December 2019 hardfork 12/20/2019 @ 11:06am (UTC)
    static const int32_t nDecemberHardforkHeight = 1670000;   //December 2019 hardfork
    static const uint32_t nS4Timestamp = 1592146800; //dPoW Season 4 2020 hardfork Sunday, June 14th, 2020 03:00:00 PM UTC
    static const int32_t nS4HardforkHeight = 1922000;   //dPoW Season 4 2020 hardfork Sunday, June 14th, 2020 
    static const uint32_t nS5Timestamp = 1623682800;  //dPoW Season 5 Monday, June 14th, 2021 (03:00:00 PM UTC)
    static const int32_t nS5HardforkHeight = 2437300;  //dPoW Season 5 Monday, June 14th, 2021
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
};

class TestnetHardfork
{
public:
    static const uint32_t komodoNotariesHardcoded = 2;
    static const int32_t s1HardforkHeight = 4;
    static const int32_t s2HardforkHeight = 6;
    static const uint32_t nStakedDecemberHardforkTimestamp = 1576840000; //December 2019 hardfork 12/20/2019 @ 11:06am (UTC)
    static const int32_t nDecemberHardforkHeight = 6;   //December 2019 hardfork
    static const uint32_t nS4Timestamp = 1592146800; //dPoW Season 4 2020 hardfork Sunday, June 14th, 2020 03:00:00 PM UTC
    static const int32_t nS4HardforkHeight = 7;   //dPoW Season 4 2020 hardfork Sunday, June 14th, 2020 
    static const uint32_t nS5Timestamp = 1623682800;  //dPoW Season 5 Monday, June 14th, 2021 (03:00:00 PM UTC)
    static const int32_t nS5HardforkHeight = 9;  //dPoW Season 5 Monday, June 14th, 2021
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
};

class RegtestHardfork
{
public:
    static const uint32_t komodoNotariesHardcoded = 2;
    static const int32_t s1HardforkHeight = 4;
    static const int32_t s2HardforkHeight = 6;
    static const uint32_t nStakedDecemberHardforkTimestamp = 1576840000; //December 2019 hardfork 12/20/2019 @ 11:06am (UTC)
    static const int32_t nDecemberHardforkHeight = 3;   //December 2019 hardfork
    static const uint32_t nS4Timestamp = 1592146800; //dPoW Season 4 2020 hardfork Sunday, June 14th, 2020 03:00:00 PM UTC
    static const int32_t nS4HardforkHeight = 6;   //dPoW Season 4 2020 hardfork Sunday, June 14th, 2020 
    static const uint32_t nS5Timestamp = 1623682800;  //dPoW Season 5 Monday, June 14th, 2021 (03:00:00 PM UTC)
    static const int32_t nS5HardforkHeight = 9;  //dPoW Season 5 Monday, June 14th, 2021
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
};

extern const uint32_t KMD_SEASON_TIMESTAMPS[NUM_KMD_SEASONS];
extern const int32_t KMD_SEASON_HEIGHTS[NUM_KMD_SEASONS];

// Era array of pubkeys. Add extra seasons to bottom as requried, after adding appropriate info above. 
extern const char *notaries_elected[NUM_KMD_SEASONS][NUM_KMD_NOTARIES][2];
extern char NOTARYADDRS[64][64];
extern char NOTARY_ADDRESSES[NUM_KMD_SEASONS][64][64];
