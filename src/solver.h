#pragma once
#include "sodium/crypto_generichash_blake2b.h"

#include <string>
#include <vector>
#include <mutex>
#include <functional>

enum BlockSolverResult
{
    BSR_LOOP_BREAK, BSR_LOOP_CONTINUE, BSR_NORMAL
};

class BlockSolver
{
public:
    BlockSolver() : valid(false) {}
    BlockSolver(const std::string& name) : name(name), valid(true) {}
    ~BlockSolver() {}
    virtual BlockSolverResult Run(crypto_generichash_blake2b_state &curr_state, 
            std::function<bool(std::vector<unsigned char>)> validBlock, unsigned int n, unsigned int k,
            std::mutex &m_cs, bool &cancelSolver) = 0;
    std::string GetName() const { return name; }
    bool IsValid() const { return valid; }
private:
    std::string name;
    bool valid;
};

class TrompSolver : public BlockSolver
{
public:
    TrompSolver() : BlockSolver("tromp")
    {
    }
    virtual BlockSolverResult Run(crypto_generichash_blake2b_state &curr_state, 
            std::function<bool(std::vector<unsigned char>)> validBlock, unsigned int n, unsigned int k,
            std::mutex &m_cs, bool &cancelSolver);
};

class DefaultSolver : public BlockSolver
{
public:
    DefaultSolver() : BlockSolver("default")
    {
    }
    virtual BlockSolverResult Run(crypto_generichash_blake2b_state &curr_state, 
            std::function<bool(std::vector<unsigned char>)> validBlock, unsigned int n, unsigned int k,
            std::mutex &m_cs, bool &cancelSolver);
};