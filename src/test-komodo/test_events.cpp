#include <gtest/gtest.h>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <boost/filesystem.hpp>
#include <komodo_structs.h>
#include <komodo_utils.h>

int32_t komodo_faststateinit(struct komodo_state *sp,char *fname,char *symbol,char *dest);
extern int32_t KOMODO_EXTERNAL_NOTARIES;
extern komodo_state KOMODO_STATES[34];

namespace TestEvents {

void write_p_record(std::FILE* fp)
{
    // a pubkey record with 2 keys
    char data[5+1+(33*2)] = {'P', 1, 0, 0, 0, 2}; // public key record identifier with height of 1 plus number of keys plus keys themselves
    memset(&data[6], 1, 33); // 1st key is all 1s
    memset(&data[39], 2, 33); // 2nd key is all 2s
    std::fwrite(data, sizeof(data), 1, fp);
}
void write_n_record(std::FILE* fp)
{
    // a notarized record has
    // 5 byte header 
    // 4 bytes notarized_height
    // 32 bytes notarized hash( blockhash)
    // 32 bytes desttxid
    char data[73] = {'N', 1, 0, 0, 0, 2, 0, 0, 0};
    memset(&data[9], 1, 32); // notarized hash
    memset(&data[41], 2, 32); // desttxid
    std::fwrite(data, sizeof(data), 1, fp);
}
void write_m_record(std::FILE* fp)
{
    // a notarized M record has
    // 5 byte header 
    // 4 bytes notarized_height
    // 32 bytes notarized hash( blockhash)
    // 32 bytes desttxid
    // 32 bytes MoM
    // 4 bytes MoMdepth
    char data[109] = {'M', 1, 0, 0, 0, 3, 0, 0, 0};
    memset(&data[9], 1, 32); // notarized hash
    memset(&data[41], 2, 32); // desttxid
    memset(&data[73], 3, 32); // MoM
    memset(&data[105], 4, 4); // MoMdepth
    std::fwrite(data, sizeof(data), 1, fp);
}
void write_u_record(std::FILE* fp)
{
    // a U record has
    // 5 byte header 
    // 1 byte n and 1 byte nid
    // 8 bytes mask
    // 32 bytes hash
    char data[47] = {'U', 1, 0, 0, 0, 'N', 'I'};
    memset(&data[7], 1, 8); // mask
    memset(&data[15], 2, 32); // hash
    std::fwrite(data, sizeof(data), 1, fp);
}
void write_k_record(std::FILE* fp)
{
    // a K record has
    // 5 byte header 
    // 4 bytes height
    char data[9] = {'K', 1, 0, 0, 0 };
    memset(&data[5], 1, 4); // height
    std::fwrite(data, sizeof(data), 1, fp);
}
void write_t_record(std::FILE* fp)
{
    // a T record has
    // 5 byte header 
    // 4 bytes height
    // 4 bytes timestamp
    char data[13] = {'T', 1, 0, 0, 0 };
    memset(&data[5], 1, 4); // height
    memset(&data[9], 2, 4); // timestamp
    std::fwrite(data, sizeof(data), 1, fp);
}
void write_r_record(std::FILE* fp)
{
    // an R record has
    // 5 byte header 
    // 32 byte txid
    // 2 byte v
    // 8 byte ovalue
    // 2 byte olen
    // olen bytes of data
    char data[65583] = {'R', 1, 0, 0, 0 };
    memset(&data[5], 1, 32); // txid
    memset(&data[37], 2, 2); // v
    memset(&data[39], 3, 8); // ovalue
    unsigned char olen[2] = {254, 255}; // 65534
    memcpy(&data[47], olen, 2);
    memset(&data[49], 4, 65534);
    std::fwrite(data, sizeof(data), 1, fp);
}
void write_v_record(std::FILE* fp)
{
    // a V record has
    // 5 byte header 
    // 1 byte counter
    // counter * 4 bytes of data
    char data[146] = {'V', 1, 0, 0, 0};
    memset(&data[5], 35, 1); // counter
    memset(&data[6], 1, 140); // data
    std::fwrite(data, sizeof(data), 1, fp);
}
template<class T>
bool compare_serialization(const std::string& filename, std::shared_ptr<T> in)
{
    // read contents of file
    std::ifstream s(filename, std::ios::binary);
    std::vector<char> file_contents((std::istreambuf_iterator<char>(s)), std::istreambuf_iterator<char>());
    // get contents of in
    std::stringstream ss;
    ss << *(in.get());
    std::vector<char> in_contents( (std::istreambuf_iterator<char>(ss)), std::istreambuf_iterator<char>()); 
    bool retval = file_contents == in_contents;
    if (!retval)
    {
        if (file_contents.size() != in_contents.size())
        {
            std::cout << "File has " << std::to_string( file_contents.size() ) << " bytes but serialized data has " << in_contents.size() << "\n";
        }
        else
        {
            for(size_t i = 0; i < file_contents.size(); ++i)
            {
                if (file_contents[i] != in_contents[i])
                {
                    std::cout << "Difference at position " << std::to_string(i) 
                            << " " << std::hex << std::setfill('0') << std::setw(2) << (int)file_contents[i] 
                            << " " << std::hex << std::setfill('0') << std::setw(2) << (int)in_contents[i] << "\n";
                    break;
                }
            }
        }
    }
    return retval;
}

/****
 * The main purpose of this test is to verify that
 * state files created continue to be readable despite logic
 * changes. Files need to be readable. The record format should 
 * not change without hardfork protection
 */
TEST(TestEvents, komodo_faststateinit_test)
{
    strcpy(ASSETCHAINS_SYMBOL, "TST");
    KOMODO_EXTERNAL_NOTARIES = 1;

    boost::filesystem::path temp = boost::filesystem::unique_path();
    boost::filesystem::create_directories(temp);
    try
    {
        // NOTE: header contains a 5 byte header that is make up of
        // an 8 bit identifier (i.e. 'P', 'N', etc.)
        // plus a 32 bit height. Everything else is record specific
        // pubkey record
        {
            // create a binary file that should be readable by komodo
            const std::string temp_filename = temp.native() + "/kstate.tmp";
            char full_filename[temp_filename.size()+1];
            strcpy(full_filename, temp_filename.c_str());
            std::FILE* fp = std::fopen(full_filename, "wb+");
            ASSERT_NE(fp, nullptr);
            write_p_record(fp);
            std::fclose(fp);
            // verify file still exists
            ASSERT_TRUE(boost::filesystem::exists(full_filename));
            // attempt to read the file
            char symbol[] = "TST";
            komodo_state* state = komodo_stateptrget((char*)symbol);
            ASSERT_NE(state, nullptr);
            char* dest = nullptr;
            // attempt to read the file
            int32_t result = komodo_faststateinit( state, full_filename, symbol, dest);
            // compare results
            ASSERT_EQ(result, 1);
            // The first and second event should be pub keys
            ASSERT_EQ(state->Komodo_numevents, 1);
            komodo_event* ev1 = state->Komodo_events[0];
            ASSERT_EQ(ev1->height, 1);
            ASSERT_EQ(ev1->type, 'P');
            // check that the new way is the same
            ASSERT_EQ(state->events.size(), 1);
            std::shared_ptr<komodo::event_pubkeys> ev2 = std::dynamic_pointer_cast<komodo::event_pubkeys>(state->events.front());
            ASSERT_EQ(ev2->height, 1);
            ASSERT_EQ(ev2->type, komodo::komodo_event_type::EVENT_PUBKEYS);
            // the serialized version should match the input
            ASSERT_TRUE(compare_serialization(full_filename, ev2));
        }
        // notarized record
        {
            const std::string temp_filename = temp.native() + "/notarized.tmp";
            char full_filename[temp_filename.size()+1];
            strcpy(full_filename, temp_filename.c_str());
            std::FILE* fp = std::fopen(full_filename, "wb+");
            ASSERT_NE(fp, nullptr);
            write_n_record(fp);
            std::fclose(fp);
            // verify file still exists
            ASSERT_TRUE(boost::filesystem::exists(full_filename));
            // attempt to read the file
            char symbol[] = "TST";
            komodo_state* state = komodo_stateptrget((char*)symbol);
            ASSERT_NE(state, nullptr);
            char* dest = (char*)"123456789012345";
            // attempt to read the file
            int32_t result = komodo_faststateinit( state, full_filename, symbol, dest);
            // compare results
            ASSERT_EQ(result, 1);
            ASSERT_EQ(state->Komodo_numevents, 2);
            komodo_event* ev = state->Komodo_events[1];
            ASSERT_EQ(ev->height, 1);
            ASSERT_EQ(ev->type, 'N');
            // check that the new way is the same
            ASSERT_EQ(state->events.size(), 2);
            std::shared_ptr<komodo::event_notarized> ev2 = std::dynamic_pointer_cast<komodo::event_notarized>( *(++state->events.begin()) );
            ASSERT_EQ(ev2->height, 1);
            ASSERT_EQ(ev2->type, komodo::komodo_event_type::EVENT_NOTARIZED);
            // the serialized version should match the input
            ASSERT_TRUE(compare_serialization(full_filename, ev2));
        }
        // notarized M record
        {
            const std::string temp_filename = temp.native() + "/notarized.tmp";
            char full_filename[temp_filename.size()+1];
            strcpy(full_filename, temp_filename.c_str());
            std::FILE* fp = std::fopen(full_filename, "wb+");
            ASSERT_NE(fp, nullptr);
            write_m_record(fp);
            std::fclose(fp);
            // verify file still exists
            ASSERT_TRUE(boost::filesystem::exists(full_filename));
            // attempt to read the file
            char symbol[] = "TST";
            komodo_state* state = komodo_stateptrget((char*)symbol);
            ASSERT_NE(state, nullptr);
            char* dest = (char*)"123456789012345";
            // attempt to read the file
            int32_t result = komodo_faststateinit(state, full_filename, symbol, dest);
            // compare results
            ASSERT_EQ(result, 1);
            ASSERT_EQ(state->Komodo_numevents, 3);
            komodo_event* ev = state->Komodo_events[2];
            ASSERT_EQ(ev->height, 1);
            ASSERT_EQ(ev->type, 'N'); // code converts "M" to "N"
            // check that the new way is the same
            ASSERT_EQ(state->events.size(), 3);
            auto itr = state->events.begin();
            std::advance(itr, 2);
            std::shared_ptr<komodo::event_notarized> ev2 = std::dynamic_pointer_cast<komodo::event_notarized>( *(itr) );
            ASSERT_EQ(ev2->height, 1);
            ASSERT_EQ(ev2->type, komodo::komodo_event_type::EVENT_NOTARIZED);
            // the serialized version should match the input
            ASSERT_TRUE(compare_serialization(full_filename, ev2));
        }
        // record type "U" (deprecated)
        {
            const std::string temp_filename = temp.native() + "/type_u.tmp";
            char full_filename[temp_filename.size()+1];
            strcpy(full_filename, temp_filename.c_str());
            std::FILE* fp = std::fopen(full_filename, "wb+");
            ASSERT_NE(fp, nullptr);
            write_u_record(fp);
            std::fclose(fp);
            // verify file still exists
            ASSERT_TRUE(boost::filesystem::exists(full_filename));
            // attempt to read the file
            char symbol[] = "TST";
            komodo_state* state = komodo_stateptrget((char*)symbol);
            ASSERT_NE(state, nullptr);
            char* dest = (char*)"123456789012345";
            // attempt to read the file
            int32_t result = komodo_faststateinit(state, full_filename, symbol, dest);
            // compare results
            ASSERT_EQ(result, 1);
            ASSERT_EQ(state->Komodo_numevents, 3); // does not get added to state
            // check that the new way is the same
            ASSERT_EQ(state->events.size(), 3);
            auto itr = state->events.begin();
            // this does not get added to state, so we need to serialize the object just
            // to verify serialization works as expected
            std::shared_ptr<komodo::event_u> ev2 = std::make_shared<komodo::event_u>();
            ev2->height = 1;
            ev2->n = 'N';
            ev2->nid = 'I';
            memset(ev2->mask, 1, 8);
            memset(ev2->hash, 2, 32);
            ASSERT_TRUE(compare_serialization(full_filename, ev2));
        }
        // record type K (KMD height)
        {
            const std::string temp_filename = temp.native() + "/kmdtype.tmp";
            char full_filename[temp_filename.size()+1];
            strcpy(full_filename, temp_filename.c_str());
            std::FILE* fp = std::fopen(full_filename, "wb+");
            ASSERT_NE(fp, nullptr);
            write_k_record(fp);
            std::fclose(fp);
            // verify file still exists
            ASSERT_TRUE(boost::filesystem::exists(full_filename));
            // attempt to read the file
            char symbol[] = "TST";
            komodo_state* state = komodo_stateptrget((char*)symbol);
            ASSERT_NE(state, nullptr);
            char* dest = (char*)"123456789012345";
            // attempt to read the file
            int32_t result = komodo_faststateinit(state, full_filename, symbol, dest);
            // compare results
            ASSERT_EQ(result, 1);
            ASSERT_EQ(state->Komodo_numevents, 4);
            komodo_event* ev = state->Komodo_events[3];
            ASSERT_EQ(ev->height, 1);
            ASSERT_EQ(ev->type, 'K');            
            // check that the new way is the same
            ASSERT_EQ(state->events.size(), 4);
            auto itr = state->events.begin();
            std::advance(itr, 3);
            std::shared_ptr<komodo::event_kmdheight> ev2 = std::dynamic_pointer_cast<komodo::event_kmdheight>( *(itr) );
            ASSERT_EQ(ev2->height, 1);
            ASSERT_EQ(ev2->type, komodo::komodo_event_type::EVENT_KMDHEIGHT);
            // the serialized version should match the input
            ASSERT_TRUE(compare_serialization(full_filename, ev2));
        }
        // record type T (KMD height with timestamp)
        {
            const std::string temp_filename = temp.native() + "/kmdtypet.tmp";
            char full_filename[temp_filename.size()+1];
            strcpy(full_filename, temp_filename.c_str());
            std::FILE* fp = std::fopen(full_filename, "wb+");
            ASSERT_NE(fp, nullptr);
            write_t_record(fp);
            std::fclose(fp);
            // verify file still exists
            ASSERT_TRUE(boost::filesystem::exists(full_filename));
            // attempt to read the file
            char symbol[] = "TST";
            komodo_state* state = komodo_stateptrget((char*)symbol);
            ASSERT_NE(state, nullptr);
            char* dest = (char*)"123456789012345";
            // attempt to read the file
            int32_t result = komodo_faststateinit(state, full_filename, symbol, dest);
            // compare results
            ASSERT_EQ(result, 1);
            ASSERT_EQ(state->Komodo_numevents, 5);
            komodo_event* ev = state->Komodo_events[4];
            ASSERT_EQ(ev->height, 1);
            ASSERT_EQ(ev->type, 'K'); // changed from T to K
            // check that the new way is the same
            ASSERT_EQ(state->events.size(), 5);
            auto itr = state->events.begin();
            std::advance(itr, 4);
            std::shared_ptr<komodo::event_kmdheight> ev2 = std::dynamic_pointer_cast<komodo::event_kmdheight>( *(itr) );
            ASSERT_EQ(ev2->height, 1);
            ASSERT_EQ(ev2->type, komodo::komodo_event_type::EVENT_KMDHEIGHT);
            // the serialized version should match the input
            ASSERT_TRUE(compare_serialization(full_filename, ev2));
        }
        // record type R (opreturn)
        {
            const std::string temp_filename = temp.native() + "/kmdtypet.tmp";
            char full_filename[temp_filename.size()+1];
            strcpy(full_filename, temp_filename.c_str());
            std::FILE* fp = std::fopen(full_filename, "wb+");
            ASSERT_NE(fp, nullptr);
            write_r_record(fp);
            std::fclose(fp);
            // verify file still exists
            ASSERT_TRUE(boost::filesystem::exists(full_filename));
            // attempt to read the file
            char symbol[] = "TST";
            komodo_state* state = komodo_stateptrget((char*)symbol);
            ASSERT_NE(state, nullptr);
            char* dest = (char*)"123456789012345";
            // attempt to read the file
            int32_t result = komodo_faststateinit(state, full_filename, symbol, dest);
            // compare results
            ASSERT_EQ(result, 1);
            ASSERT_EQ(state->Komodo_numevents, 6);
            komodo_event* ev = state->Komodo_events[5];
            ASSERT_EQ(ev->height, 1);
            ASSERT_EQ(ev->type, 'R');
            // check that the new way is the same
            ASSERT_EQ(state->events.size(), 6);
            auto itr = state->events.begin();
            std::advance(itr, 5);
            std::shared_ptr<komodo::event_opreturn> ev2 = std::dynamic_pointer_cast<komodo::event_opreturn>( *(itr) );
            ASSERT_EQ(ev2->height, 1);
            ASSERT_EQ(ev2->type, komodo::komodo_event_type::EVENT_OPRETURN);
            // the serialized version should match the input
            ASSERT_TRUE(compare_serialization(full_filename, ev2));
        }
        // record type V
        {
            const std::string temp_filename = temp.native() + "/kmdtypet.tmp";
            char full_filename[temp_filename.size()+1];
            strcpy(full_filename, temp_filename.c_str());
            std::FILE* fp = std::fopen(full_filename, "wb+");
            ASSERT_NE(fp, nullptr);
            write_v_record(fp);
            std::fclose(fp);
            // verify file still exists
            ASSERT_TRUE(boost::filesystem::exists(full_filename));
            // attempt to read the file
            char symbol[] = "TST";
            komodo_state* state = komodo_stateptrget((char*)symbol);
            ASSERT_NE(state, nullptr);
            char* dest = (char*)"123456789012345";
            // attempt to read the file
            int32_t result = komodo_faststateinit(state, full_filename, symbol, dest);
            // compare results
            ASSERT_EQ(result, 1);
            ASSERT_EQ(state->Komodo_numevents, 7);
            komodo_event* ev = state->Komodo_events[6];
            ASSERT_EQ(ev->height, 1);
            ASSERT_EQ(ev->type, 'V');
            // check that the new way is the same
            ASSERT_EQ(state->events.size(), 7);
            auto itr = state->events.begin();
            std::advance(itr, 6);
            std::shared_ptr<komodo::event_pricefeed> ev2 = std::dynamic_pointer_cast<komodo::event_pricefeed>( *(itr) );
            ASSERT_EQ(ev2->height, 1);
            ASSERT_EQ(ev2->type, komodo::komodo_event_type::EVENT_PRICEFEED);
            // the serialized version should match the input
            ASSERT_TRUE(compare_serialization(full_filename, ev2));
        }
        // all together in 1 file
        {
            const std::string temp_filename = temp.native() + "/combined_state.tmp";
            char full_filename[temp_filename.size()+1];
            strcpy(full_filename, temp_filename.c_str());
            std::FILE* fp = std::fopen(full_filename, "wb+");
            ASSERT_NE(fp, nullptr);
            write_p_record(fp);
            write_n_record(fp);
            write_m_record(fp);
            write_u_record(fp);
            write_k_record(fp);
            write_t_record(fp);
            write_r_record(fp);
            write_v_record(fp);
            std::fclose(fp);
            // attempt to read the file
            char symbol[] = "TST";
            komodo_state* state = komodo_stateptrget((char*)symbol);
            ASSERT_NE(state, nullptr);
            char* dest = (char*)"123456789012345";
            // attempt to read the file
            int32_t result = komodo_faststateinit( state, full_filename, symbol, dest);
            // compare results
            ASSERT_EQ(result, 1);
            ASSERT_EQ(state->Komodo_numevents, 14);
            komodo_event* ev1 = state->Komodo_events[7];
            ASSERT_EQ(ev1->height, 1);
            ASSERT_EQ(ev1->type, 'P');
            // check that the new way is the same
            ASSERT_EQ(state->events.size(), 14);
            auto itr = state->events.begin();
            std::advance(itr, 7);
            {
                ASSERT_EQ( (*itr)->height, 1);
                ASSERT_EQ( (*itr)->type, komodo::komodo_event_type::EVENT_PUBKEYS);
                itr++;
            }
            {
                ASSERT_EQ( (*itr)->height, 1);
                ASSERT_EQ( (*itr)->type, komodo::komodo_event_type::EVENT_NOTARIZED);
                itr++;
            }
            {
                ASSERT_EQ( (*itr)->height, 1);
                ASSERT_EQ( (*itr)->type, komodo::komodo_event_type::EVENT_NOTARIZED);
                itr++;
            }
            {
                ASSERT_EQ( (*itr)->height, 1);
                ASSERT_EQ( (*itr)->type, komodo::komodo_event_type::EVENT_KMDHEIGHT);
                itr++;
            }
            {
                ASSERT_EQ( (*itr)->height, 1);
                ASSERT_EQ( (*itr)->type, komodo::komodo_event_type::EVENT_KMDHEIGHT);
                itr++;
            }
            {
                ASSERT_EQ( (*itr)->height, 1);
                ASSERT_EQ( (*itr)->type, komodo::komodo_event_type::EVENT_OPRETURN);
                itr++;
            }
            {
                ASSERT_EQ( (*itr)->height, 1);
                ASSERT_EQ( (*itr)->type, komodo::komodo_event_type::EVENT_PRICEFEED);
                itr++;
            }
        }
    } 
    catch(...)
    {
        FAIL() << "Exception thrown";
    }
    boost::filesystem::remove_all(temp);
}

TEST(TestEvents, komodo_stateptrget_test)
{
    strcpy(ASSETCHAINS_SYMBOL, "JMJ");
    komodo_state* kmd = komodo_stateptrget(nullptr); // this gets what's in position 33
    komodo_state* hrk = komodo_stateptrget("HRK"); // this should get HRK
    ASSERT_NE(kmd, hrk);
    komodo_state* jmj = komodo_stateptrget("JMJ"); // ASSETCHAINS_SYMBOL seems to get put at index 0
    ASSERT_EQ(jmj, &KOMODO_STATES[0]);
    komodo_state* blah = komodo_stateptrget("BLAH"); // if we give an invalid symbol, we get index 0 as well
    ASSERT_NE(jmj, blah);
}

} // namespace TestEvents