#include <gtest/gtest.h>
#include "hex.h"

namespace TestHex {

TEST(TestHex, testIsHexStr)
{
    {
        // happy path
        char* in = (char*) "01";
        EXPECT_EQ( is_hexstr(in, 2), 1);
    }
    {
        // longer happy path
        char* in = (char*) "0102";
        EXPECT_EQ( is_hexstr(in, 2), 1);
    }
    {
        // go until null
        char* in = (char*) "01";
        EXPECT_EQ( is_hexstr(in, 0), 2);
        in = (char*) "0102";
        EXPECT_EQ( is_hexstr(in, 0), 4);
    }
    {
        // invalid hex
        char* in = (char*) "0G";
        EXPECT_EQ( is_hexstr(in, 2), 0);
    }
}

TEST(TestHex, decodehex)
{
    {
        // n = 0
        char* in = (char*) "01";
        uint8_t bytes[2] = {0,0};
        // Original returned 1 instead of 0 which doesn't matter much
        // as return values are never used
        EXPECT_EQ(decode_hex(bytes, 0, in), 0);
        EXPECT_EQ(bytes[0], 0x00);
    }
    {
        // happy path
        char* in = (char*) "01";
        uint8_t bytes[1] = {0};
        EXPECT_EQ(decode_hex(bytes, 1, in), 1);
        EXPECT_EQ(bytes[0], 0x01);
    }
    {
        // cr/lf
        char* in = (char*) "01\r\n";
        uint8_t bytes[1] = {0};
        EXPECT_EQ(decode_hex(bytes, 1, in), 1);
        EXPECT_EQ(bytes[0], 0x01);
    }
    {
        // string longer than what we say by 1 
        // evidently a special case that we handle by treating
        // the 1st char as a complete byte
        char* in = (char*) "010";
        uint8_t bytes[2] = {0, 0};
        EXPECT_EQ(decode_hex(bytes, 1, in), 2);
        EXPECT_EQ(bytes[0], 0);
        EXPECT_EQ(bytes[1], 16);
    }
    {
        // string longer than what we say by 2 
        char* in = (char*) "0101";
        uint8_t bytes[2] = {0, 0};
        EXPECT_EQ(decode_hex(bytes, 1, in), 1);
        EXPECT_EQ(bytes[0], 1);
        EXPECT_EQ(bytes[1], 0);
    }
    {
        // string shorter than what we expect by 2
        char* in = (char*) "01";
        uint8_t bytes[2] = {0, 0};
        EXPECT_EQ(decode_hex(bytes, 2, in), 2);
        EXPECT_EQ(bytes[0], 1);
        EXPECT_EQ(bytes[1], 0); // has a random value here.
    }
    {
        // string shorter than what we expect by 1
        char* in = (char*) "010";
        uint8_t bytes[2] = {0, 0};
        EXPECT_EQ(decode_hex(bytes, 2, in), 3);
        EXPECT_EQ(bytes[0], 0);
        EXPECT_EQ(bytes[1], 16);
    }
    {
        // buffer overrun
        // this is not prevented, as the interface does not know sizeof(bytes)
        // left here to remind us that we need to do something about this.
        /*
        char* in = (char*) "0101";
        uint8_t bytes[1] = {0};
        EXPECT_EQ(decode_hex(bytes, 2, in), 2);
        EXPECT_EQ(bytes[0], 1);
        EXPECT_EQ(bytes[1], 1);
        */
    }
}

} // namespace TestHex
