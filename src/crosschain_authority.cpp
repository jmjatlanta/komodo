#include "cc/eval.h"
#include "crosschain.h"
#include "notarisationdb.h"
#include "notaries_staked.h"

int GetSymbolAuthority(const char* symbol)
{
    if (strncmp(symbol, "TXSCL", 5) == 0)
        return CROSSCHAIN_TXSCL;
    if (is_STAKED(symbol) != 0) {
        return CROSSCHAIN_STAKED;
    }
    return CROSSCHAIN_KOMODO;
}

