#include "solver.h"
#include "metrics.h" // extern AtomicCounter ehSolverRuns
#include "util.h" // LogPrint
#include "crypto/equihash.h" // EhOptimisedSolve

BlockSolverResult DefaultSolver::Run(crypto_generichash_blake2b_state &curr_state, 
        std::function<bool(std::vector<unsigned char>)> validBlock, unsigned int n, unsigned int k,
        std::mutex &m_cs, bool &cancelSolver)
{
    try {
        // If we find a valid block, we rebuild
        bool found = EhOptimisedSolve(n, k, curr_state, validBlock, 
                [&m_cs, &cancelSolver](EhSolverCancelCheck pos) {
                    std::lock_guard<std::mutex> lock{m_cs};
                    return cancelSolver;
                });
        ehSolverRuns.increment();
        if (found) {
            return BlockSolverResult::BSR_LOOP_BREAK;
        }
    } catch (EhSolverCancelledException&) {
        LogPrint("pow", "Equihash solver cancelled\n");
        std::lock_guard<std::mutex> lock{m_cs};
        cancelSolver = false;
    }
    return BlockSolverResult::BSR_NORMAL;
}
