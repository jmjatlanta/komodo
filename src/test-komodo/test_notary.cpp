#include "komodo_notary.h"

#include <gtest/gtest.h>

namespace TestNotary
{

void convertToCharArray(unsigned char* buffer, const std::string& in)
{
    for(int i = 0; i < 33; ++i)
    {
        const std::string& piece = in.substr(i * 2, 2);
        buffer[i] = strtol(piece.c_str(), nullptr, 16);
    }
}

bool equal(const unsigned char* lhs, const unsigned char* rhs, size_t length)
{
    for(size_t i = 0; i < length; ++i)
        if (lhs[i] != rhs[i])
            return false;
    return true;
}

TEST(TestNotary, init)
{
    unsigned char expected[33];
    convertToCharArray(expected, "02209073bc0943451498de57f802650311b1f12aa6deffcd893da198a544c04f36");
    int32_t height = 0;
    uint32_t timestamp = 0;

    komodo_init(0);
    uint8_t pubkeys[64][33];
    komodo_notaries(pubkeys, height, timestamp);
    EXPECT_TRUE( equal(pubkeys[2], expected, 33));
}

TEST(TestNotary, ChosenNotary)
{
    unsigned char search[33];
    convertToCharArray(search, "02209073bc0943451498de57f802650311b1f12aa6deffcd893da198a544c04f36");

    int32_t notaryid;
    int32_t height = 0;
    uint32_t timestamp = 0;

    int32_t retVal = komodo_chosennotary(&notaryid, height, search, timestamp);
    EXPECT_EQ(retVal, 0);
    EXPECT_EQ(notaryid, 2);
}

} // namespace TestNotary