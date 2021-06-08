#include "komodo_algorithms.h"

hash_algorithm hash_algorithm::get_algorithm(const std::string& in)
{
    if (in == "equihash")
        return get_algorithm(HASH_ALGO_EQUIHASH);
    if (in == "verushash")
        return get_algorithm(HASH_ALGO_VERUSHASH);
    if (in == "verushash11")
        return get_algorithm(HASH_ALGO_VERUSHASHV1_1);
    return hash_algorithm();
}

hash_algorithm hash_algorithm::get_algorithm(hash_algo in)
{
    if (in == HASH_ALGO_EQUIHASH)
        return equihash();
    if (in == HASH_ALGO_VERUSHASH)
        return verushash();
    if (in == HASH_ALGO_VERUSHASHV1_1)
        return verushash11();
    return hash_algorithm();
}
