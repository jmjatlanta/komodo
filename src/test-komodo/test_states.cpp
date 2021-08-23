#include <gtest/gtest.h>

#include "komodo_structs.h"

#define KOMODO_ASSETCHAIN_MAXLEN 65
extern char ASSETCHAINS_SYMBOL[KOMODO_ASSETCHAIN_MAXLEN];
void komodo_nameset(char *symbol, char *dest, const char *source);
komodo_state *komodo_stateptr();
komodo_state *komodo_stateptr(const char *in);

namespace TestStates {


TEST(TestStates, test_nameset)
{
    char symbol[KOMODO_ASSETCHAIN_MAXLEN];
    char dest[KOMODO_ASSETCHAIN_MAXLEN];
    char zero[1];
    zero[0] = 0;

    // nullptr is invalid
    //komodo_nameset(symbol, dest, nullptr);
    //EXPECT_EQ(symbol, "KMD");
    //EXPECT_EQ(dest, "BTC");

    //empty
    komodo_nameset(symbol, dest, zero);
    EXPECT_EQ(strcmp(symbol, "KMD"), 0);
    EXPECT_EQ(strcmp(dest, "BTC"), 0);

    // KMD (not expected, but how the old code works)
    komodo_nameset(symbol, dest, "KMD");
    EXPECT_EQ(strcmp(symbol, "KMD"), 0);
    EXPECT_EQ(strcmp(symbol, "KMD"), 0);

    // child chain
    komodo_nameset(symbol, dest, "ABC");
    EXPECT_EQ(strcmp(symbol, "ABC"), 0);
    EXPECT_EQ(strcmp(dest, "KMD"), 0);
}

TEST(TestStates, test_stateptr)
{
    ASSETCHAINS_SYMBOL[0] = 0; // KMD
    komodo_state *kmd = komodo_stateptr();
    strcpy(ASSETCHAINS_SYMBOL, "ABC");
    komodo_state *child = komodo_stateptr();
    // the addresses should be different
    EXPECT_NE(kmd, child);
    // change something in the struct, to verify the address doesn't change
    kmd->CURRENT_HEIGHT = 42;
    child->CURRENT_HEIGHT = 43;
    // the addresses should be the same
    komodo_state *child2 = komodo_stateptr();
    EXPECT_EQ(child, child2);
    EXPECT_EQ(child2->CURRENT_HEIGHT, 43);
    ASSETCHAINS_SYMBOL[0] = 0;
    komodo_state *kmd2 = komodo_stateptr();
    EXPECT_EQ(kmd, kmd2);
    EXPECT_EQ(kmd2->CURRENT_HEIGHT, 42);

    // now pass in what we're looking for
    strcpy(ASSETCHAINS_SYMBOL, "something else");
    child2 = komodo_stateptr("ABC");
    kmd2 = komodo_stateptr("KMD");
    EXPECT_EQ(child, child2);
    EXPECT_EQ(child2->CURRENT_HEIGHT, 43);
    EXPECT_EQ(kmd, kmd2);
    EXPECT_EQ(kmd2->CURRENT_HEIGHT, 42);
}

} // namespce TestStates