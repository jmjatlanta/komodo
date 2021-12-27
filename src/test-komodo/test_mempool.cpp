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
    bool rewind(size_t offset) { if (!is_open()) return false; fseek(filePtr, offset, 0); return true; }
private:
    const std::string filename;
    FILE* filePtr;
};

TEST(TestMempool, FeeEstimateReadWrite)
{
    /****
     * CLIENT_VERSION went down instead of up, causing
     * problems reading the mempool file
     */
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
        bool ReadFeeEstimates(CAutoFile& filein) const
        {
            try {
                int nVersionRequired, nVersionThatWrote;
                filein >> nVersionRequired >> nVersionThatWrote;
                if (nVersionRequired > 109900)
                    return error("CTxMemPool::ReadFeeEstimates(): up-version (%d) fee estimate file", nVersionRequired);

                LOCK(cs);
                minerPolicyEstimator->Read(filein);
            }
            catch (const std::exception&) {
                LogPrintf("CTxMemPool::ReadFeeEstimates(): unable to read policy estimator data (non-fatal)\n");
                return false;
            }
            return true;
        }
    };
    class NewTxMempool : public CTxMemPool
    {
        public:
        NewTxMempool( const CFeeRate& in ) : CTxMemPool(in) {};
        bool WriteFeeEstimates(CAutoFile& fileout) const 
        {
            try {
                LOCK(cs);
                fileout << 70100; // version required to read: 0.10.99 or later
                fileout << 70100; // version that wrote the file
                minerPolicyEstimator->Write(fileout);
            }
            catch (const std::exception&) {
                LogPrintf("CTxMemPool::WriteFeeEstimates(): unable to write policy estimator data (non-fatal)\n");
                return false;
            }
            return true;
        }
    };

    int type = 1;
    int version = 2;
    // the way it used to work
    {
        OldTxMempool pool{CFeeRate(100)};
        // create temp file
        TempFile tempFile("test.tmp");
        EXPECT_TRUE(tempFile.is_open());
        CAutoFile autoFile(tempFile.pointer(), type, version);

        EXPECT_TRUE(pool.WriteFeeEstimates(autoFile));
        tempFile.rewind(0);
        EXPECT_TRUE(pool.ReadFeeEstimates(autoFile));
    }
    // Now older files cannot be read due to "newer" CLIENT_VERSION (although a lower number)
    {
        TempFile tempFile("test.tmp");
        CAutoFile autoFile(tempFile.pointer(), type, version);
        EXPECT_TRUE(tempFile.is_open());
        {
            OldTxMempool pool{CFeeRate(100)};
            EXPECT_TRUE(pool.WriteFeeEstimates(autoFile));
        }
        tempFile.rewind(0);
        {
            NewTxMempool pool{CFeeRate(100)};
            EXPECT_FALSE(pool.ReadFeeEstimates(autoFile));
            // write with the new code
            tempFile.rewind(0);
            EXPECT_TRUE(pool.WriteFeeEstimates(autoFile));
            // subsequent reads are okay
            tempFile.rewind(0);
            EXPECT_TRUE(pool.ReadFeeEstimates(autoFile));
        }
    }
}

} // namespace TestMempool
