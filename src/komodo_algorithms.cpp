#include "komodo_algorithms.h"

std::shared_ptr<hash_algorithm> hash_algorithm::get_algorithm(const std::string& in)
{
    if (in == "equihash")
        return get_algorithm(HASH_ALGO_EQUIHASH);
    if (in == "verushash")
        return get_algorithm(HASH_ALGO_VERUSHASH);
    if (in == "verushash11")
        return get_algorithm(HASH_ALGO_VERUSHASHV1_1);
    return nullptr;
}

std::shared_ptr<hash_algorithm> hash_algorithm::get_algorithm(hash_algo in)
{
    if (in == HASH_ALGO_EQUIHASH)
        return std::make_shared<equihash>();
    if (in == HASH_ALGO_VERUSHASH)
        return std::make_shared<verushash>();
    if (in == HASH_ALGO_VERUSHASHV1_1)
        return std::make_shared<verushash11>();
    return nullptr;
}
