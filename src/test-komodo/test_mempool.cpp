#include "txmempool.h"
#include "streams.h"
#include "policy/fees.h"

#include <gtest/gtest.h>

namespace TestMempool
{

/****
 * Helps clean up temp file
 */
class TempFile
{
public:
    TempFile(const std::string& filename) : filename(filename) 
    {
        filePtr = fopen(filename.c_str(), "w+");
    }
    ~TempFile() { unlink(filename.c_str()); }
    FILE* pointer() { return filePtr; }
    bool is_open() { return filePtr != nullptr; }
private:
    const std::string filename;
    FILE* filePtr;
};

TEST(TestMempool, FeeEstimateReadWrite)
{
    int type = 1;
    int version = 2;
    // typical case
    {
        CTxMemPool pool{CFeeRate(100)};
        // create temp file
        TempFile tempFile("test.tmp");
        EXPECT_TRUE(tempFile.is_open());
        CAutoFile autoFile(tempFile.pointer(), type, version);

        EXPECT_TRUE(pool.WriteFeeEstimates(autoFile));
        fseek(tempFile.pointer(), 0, 0);
        EXPECT_TRUE(pool.ReadFeeEstimates(autoFile));
    }
    // read an "older" mempool file. Note that CLIENT_VERSION used to be numerically
    // higher (109900) but is now 70100
    {
        class OldTxMempool : public CTxMemPool
        {
            public:
            OldTxMempool( const CFeeRate& in ) : CTxMemPool(in) {};
            bool WriteFeeEstimates(CAutoFile& fileout) const 
            {
                try {
                    LOCK(cs);
                    fileout << 109900; // version required to read: 0.10.99 or later
                    fileout << 109900; // version that wrote the file
                    minerPolicyEstimator->Write(fileout);
                }
                catch (const std::exception&) {
                    LogPrintf("CTxMemPool::WriteFeeEstimates(): unable to write policy estimator data (non-fatal)\n");
                    return false;
                }
                return true;
            }
        };
        TempFile tempFile("test.tmp");
        CAutoFile autoFile(tempFile.pointer(), type, version);
        EXPECT_TRUE(tempFile.is_open());
        {
            OldTxMempool oldPool{CFeeRate(100)};
            EXPECT_TRUE(oldPool.WriteFeeEstimates(autoFile));
        }
        // move file back to beginning
        fseek(tempFile.pointer(), 0, 0);
        CTxMemPool newPool{CFeeRate(100)};
        EXPECT_TRUE(newPool.ReadFeeEstimates(autoFile));
    }

}

} // namespace TestMempool