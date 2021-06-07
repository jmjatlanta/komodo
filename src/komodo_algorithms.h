#pragma once
#include <cstdint>
#include <string>
#include <memory>

enum hash_algo
{
    HASH_ALGO_UNKNOWN,
    HASH_ALGO_EQUIHASH,
    HASH_ALGO_VERUSHASH,
    HASH_ALGO_VERUSHASHV1_1
};

class hash_algorithm
{
public:
    uint64_t nonce_mask;
    uint32_t nonce_shift;
    uint32_t hashes_per_round;
    uint32_t mindiff;
    const std::string name;
    hash_algo algo = hash_algo::HASH_ALGO_UNKNOWN;
    static std::shared_ptr<hash_algorithm> get_algorithm(const std::string& in);
    static std::shared_ptr<hash_algorithm> get_algorithm(hash_algo algo);
protected:
    hash_algorithm(uint64_t mask, uint32_t shift, uint32_t hashesPerRound, uint32_t mindiff, const std::string& name, hash_algo algo)
            : nonce_mask(mask), nonce_shift(shift), hashes_per_round(hashesPerRound), mindiff(mindiff), name(name), algo(algo)
    {}

};

class equihash : public hash_algorithm
{
public:
    equihash() : hash_algorithm(0xffff, 32, 1, 537857807, "equihash", HASH_ALGO_EQUIHASH) {}
};

class verushash : public hash_algorithm
{
public:
    verushash() : hash_algorithm(0xfffffff, 16, 4096, 504303375, "verushash", HASH_ALGO_VERUSHASH) {}
};

class verushash11 : public hash_algorithm
{
public:
    verushash11() : hash_algorithm(0xfffffff, 16, 4096, 487526159, "verushash11", HASH_ALGO_VERUSHASHV1_1) {}
};
