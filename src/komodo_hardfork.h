#pragma once

#include "chainparams.h"

#include <cstdint>

template<class T>
class ChainParamsWithHardfork : public ChainParams
{
public:
    constexpr int32_t DecemberHardforkHeight() const { return T::nDecemberHardforkHeight; }
    constexpr uint32_t StakedDecemberHardforkTimestamp() const { return T::nStakedDecemberHardforkTimestamp; }
    constexpr uint32_t S4Timestamp() const { return T::nS4Timestamp; }
    constexpr int32_t S4HardforkHeight() const { return T::nS4HardforkHeight; }
    constexpr uint32_t S5Timestamp() const { return T::nS5Timestamp; }
    constexpr int32_t S5HardforkHeight() const { return T::nS5HardforkHeight; }
};

class MainnetHardfork
{
public:
    static const uint32_t nStakedDecemberHardforkTimestamp = 1576840000; //December 2019 hardfork 12/20/2019 @ 11:06am (UTC)
    static const int32_t nDecemberHardforkHeight = 1670000;   //December 2019 hardfork
    static const uint32_t nS4Timestamp = 1592146800; //dPoW Season 4 2020 hardfork Sunday, June 14th, 2020 03:00:00 PM UTC
    static const int32_t nS4HardforkHeight = 1922000;   //dPoW Season 4 2020 hardfork Sunday, June 14th, 2020 
    static const uint32_t nS5Timestamp = 1623682800;  //dPoW Season 5 Monday, June 14th, 2021 (03:00:00 PM UTC)
    static const int32_t nS5HardforkHeight = 2437300;  //dPoW Season 5 Monday, June 14th, 2021
};

class TestnetHardfork
{
public:
    static const uint32_t nStakedDecemberHardforkTimestamp = 1576840000; //December 2019 hardfork 12/20/2019 @ 11:06am (UTC)
    static const int32_t nDecemberHardforkHeight = 3;   //December 2019 hardfork
    static const uint32_t nS4Timestamp = 1592146800; //dPoW Season 4 2020 hardfork Sunday, June 14th, 2020 03:00:00 PM UTC
    static const int32_t nS4HardforkHeight = 6;   //dPoW Season 4 2020 hardfork Sunday, June 14th, 2020 
    static const uint32_t nS5Timestamp = 1623682800;  //dPoW Season 5 Monday, June 14th, 2021 (03:00:00 PM UTC)
    static const int32_t nS5HardforkHeight = 9;  //dPoW Season 5 Monday, June 14th, 2021
};

class RegtestHardfork
{
public:
    static const uint32_t nStakedDecemberHardforkTimestamp = 1576840000; //December 2019 hardfork 12/20/2019 @ 11:06am (UTC)
    static const int32_t nDecemberHardforkHeight = 3;   //December 2019 hardfork
    static const uint32_t nS4Timestamp = 1592146800; //dPoW Season 4 2020 hardfork Sunday, June 14th, 2020 03:00:00 PM UTC
    static const int32_t nS4HardforkHeight = 6;   //dPoW Season 4 2020 hardfork Sunday, June 14th, 2020 
    static const uint32_t nS5Timestamp = 1623682800;  //dPoW Season 5 Monday, June 14th, 2021 (03:00:00 PM UTC)
    static const int32_t nS5HardforkHeight = 9;  //dPoW Season 5 Monday, June 14th, 2021
};
