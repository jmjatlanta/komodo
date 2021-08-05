#include "solver.h"
#include "pow/tromp/equi_miner.h"
#include <crypto/equihash.h> // eh_index and GetMinimalFromIndices
#include "metrics.h" // extern AtomicCounter ehSolverRuns
#include "util.h" // LogPrint

BlockSolverResult TrompSolver::Run(crypto_generichash_blake2b_state &curr_state, 
        std::function<bool(std::vector<unsigned char>)> validBlock, unsigned int n, unsigned int k,
        std::mutex &m_cs, bool &cancelSolver)
{
    // Create solver and initialize it.
    equi eq(1);
    eq.setstate(&curr_state);

    // Initialization done, start algo driver.
    eq.digit0(0);
    eq.xfull = eq.bfull = eq.hfull = 0;
    eq.showbsizes(0);
    for (u32 r = 1; r < WK; r++) {
        (r&1) ? eq.digitodd(r, 0) : eq.digiteven(r, 0);
        eq.xfull = eq.bfull = eq.hfull = 0;
        eq.showbsizes(r);
    }
    eq.digitK(0);
    ehSolverRuns.increment();

    // Convert solution indices to byte array (decompress) and pass it to validBlock method.
    for (size_t s = 0; s < eq.nsols; s++) {
        LogPrint("pow", "Checking solution %d\n", s+1);
        std::vector<eh_index> index_vector(PROOFSIZE);
        for (size_t i = 0; i < PROOFSIZE; i++) {
            index_vector[i] = eq.sols[s][i];
        }
        std::vector<unsigned char> sol_char = GetMinimalFromIndices(index_vector, DIGITBITS);

        if (validBlock(sol_char)) {
            // If we find a POW solution, do not try other solutions
            // because they become invalid as we created a new block in blockchain.
            return BlockSolverResult::BSR_LOOP_BREAK;
        }
    }
    return BlockSolverResult::BSR_NORMAL;
}
