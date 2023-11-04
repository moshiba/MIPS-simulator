/*
Cache Simulator
Level one L1 and level two L2 cache parameters are read from file (block size, line per set and set per cache).
The 32 bit address is divided into tag bits (t), set index bits (s) and block offset bits (b)
s = log2(#sets)   b = log2(block size in bytes)  t=32-s-b
32 bit address (MSB -> LSB): TAG || SET || OFFSET

Tag Bits   : the tag field along with the valid bit is used to determine whether the block in the cache is valid or not.
Index Bits : the set index field is used to determine which set in the cache the block is stored in.
Offset Bits: the offset field is used to determine which byte in the block is being accessed.
*/

#include <algorithm>
#include <bitset>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <vector>
using namespace std;
//access state:
#define NA 0 // no action
#define RH 1 // read hit
#define RM 2 // read miss
#define WH 3 // Write hit
#define WM 4 // write miss
#define NOWRITEMEM 5 // no write to memory
#define WRITEMEM 6   // write to memory

#define RH_AND_NOWRITEMEM 7
#define RH_AND_WRITEMEM 8
#define RM_AND_NOWRITEMEM 9
#define RM_AND_WRITEMEM 10

#define WH_AND_NOWRITEMEM 11
#define WH_AND_WRITEMEM 12
#define WM_AND_NOWRITEMEM 13
#define WM_AND_WRITEMEM 14

struct config
{
    int L1blocksize;
    int L1setsize;
    int L1size;
    int L2blocksize;
    int L2setsize;
    int L2size;
};

/*********************************** ↓↓↓ Todo: Implement by you ↓↓↓ ******************************************/
// You need to define your cache class here, or design your own data structure for L1 and L2 cache

/*
A single cache block:
    - valid bit (is the data in the block valid?)
    - dirty bit (has the data in the block been modified by means of a write?)
    - tag (the tag bits of the address)
    - data (the actual data stored in the block, in our case, we don't need to store the data)
*/
struct CacheBlock
{
    // we don't actually need to allocate space for data, because we only need to simulate the cache action
    // or else it would have looked something like this: vector<number of bytes> Data; 
    bool valid = 0;
    bool dirty = 0;
    unsigned int tag;
};

/*
A CacheSet:
    - a vector of CacheBlocks
    - a counter to keep track of which block to evict next
*/
struct Set
{
    // tips: 
    // Associativity: eg. resize to 4-ways set associative cache
    vector<CacheBlock> set_blocks;
    int associativity;
    int counter;    
};

// You can design your own data structure for L1 and L2 cache; just an example here
class CacheSystem
{
    // some cache configuration parameters.
    // cache L1 or L2
public:
    CacheSystem(int L1_block_size, int L1_associativity, int L1_cache_size, 
          int L2_block_size, int L2_associativity, int L2_cache_size) : 
    L1_block_size(L1_block_size), L1_associativity(L1_associativity), L1_cache_size(L1_cache_size), 
    L2_block_size(L2_block_size), L2_associativity(L2_associativity), L2_cache_size(L2_cache_size) {
        // initialize the L1 cache according to cache parameters
        L1_num_sets = (L1_cache_size * 1024)/(L1_block_size * L1_associativity);
        for(int i=0; i<L1_num_sets; ++i){
            L1_cache.push_back(new Set());
            for(int j=0; j<L1_associativity; ++j){
                L1_cache[L1_cache.size()-1]->set_blocks.push_back(CacheBlock());
            }
        }
        //initialize the L2 cache according to cache parameters
        L2_num_sets = (L2_cache_size * 1024)/(L2_block_size * L2_associativity);
        for(int i=0; i<L2_num_sets; ++i){
            L2_cache.push_back(new Set());
            for(int j=0; j<L2_associativity; ++j){
                L2_cache[L2_cache.size()-1]->set_blocks.push_back(CacheBlock());
            }
        }
    }

    ~CacheSystem(){
        for(int i=0; i<L1_num_sets; ++i){
            delete L1_cache[i];
        }
        L1_cache.clear();

        for(int i=0; i<L2_num_sets; ++i){
            delete L2_cache[i];
        }
        L2_cache.clear();
    }

    int writeL1(uint32_t addr){
        /*
        step 1: select the set in our L1 cache using set index bits
        step 2: iterate through each way in the current set
            - If Matching tag and Valid Bit High -> WriteHit!
                                                    -> Dirty Bit High
        step 3: Otherwise? -> WriteMiss!

        return WH or WM
        */
        uint32_t L1_block_size_bits = log2(L1_block_size);
        uint32_t L1_set_bits = log2(L1_num_sets);
        uint32_t L1_tag = addr >> (L1_block_size_bits + L1_set_bits);
        uint32_t L1_row = (addr >> L1_block_size_bits)%(L1_num_sets);
        for(int i=0; i<L1_associativity; ++i){
            if((L1_tag == L1_cache[L1_row]->set_blocks[i].tag) && (L1_cache[L1_row]->set_blocks[i].valid == 1)){
                //We have a write hit; set dirty bit to 1
                L1_cache[L1_row]->set_blocks[i].dirty = 1;
                return WH;
            }
        }
        //We have a write miss
        return WM;
    }

    int writeL2(uint32_t addr){
        /*
        step 1: select the set in our L2 cache using set index bits
        step 2: iterate through each way in the current set
            - If Matching tag and Valid Bit High -> WriteHit!
                                                 -> Dirty Bit High
        step 3: Otherwise? -> WriteMiss!

        return {WM or WH, WRITEMEM or NOWRITEMEM}
        */
        uint32_t L2_block_size_bits = log2(L2_block_size);
        uint32_t L2_row_bits = log2(L2_num_sets);
        uint32_t L2_tag = addr >> (L2_block_size_bits + L2_row_bits);
        uint32_t L2_row = (addr >> L2_block_size_bits)%(L2_num_sets);

        //Debugging only

        for(int i=0; i<L2_associativity; ++i){
            if((L2_tag == L2_cache[L2_row]->set_blocks[i].tag) && (L2_cache[L2_row]->set_blocks[i].valid == 1)){
                //We have an L2 cache hit, set dirty bit to 1
                L2_cache[L2_row]->set_blocks[i].dirty = 1;
                //for write hit, we do not write to memory
                return WH;
            }
        }
        //We have a write miss, we write to memory
        return WM;
    }

    int readL1(uint32_t addr){
        /*
        step 1: select the set in our L1 cache using set index bits
        step 2: iterate through each way in the current set
            - If Matching tag and Valid Bit High -> ReadHit!
        step 3: Otherwise? -> ReadMiss!

        return RH or RM
        */
        uint32_t L1_block_size_bits = log2(L1_block_size);
        uint32_t L1_set_bits = log2(L1_num_sets);
        uint32_t L1_tag = addr >> (L1_block_size_bits + L1_set_bits);
        uint32_t L1_row = (addr >> L1_block_size_bits)%(L1_num_sets);

        //Debugging only
        cout << "L1 currently has tag: " << L1_cache[0]->set_blocks[0].tag << " at row 0" << endl;

        for(int i=0; i<L1_associativity; ++i){
            if((L1_tag == L1_cache[L1_row]->set_blocks[i].tag) && (L1_cache[L1_row]->set_blocks[i].valid == 1)){
                //We have a cache hit; do nothing
                return RH;
            }
        }
        //We have a cache miss
        return RM;
    }

    int readL2(uint32_t addr){
        /*
        step 1: select the set in our L2 cache using set index bits
        step 2: iterate through each way in the current set
            - If Matching tag and Valid Bit High -> ReadHit!
                                                 -> copy dirty bit
        step 3: otherwise? -> ReadMiss! -> need to pull data from Main Memory
        step 4: find a place in L1 for our requested data
            - case 1: empty way in L1 -> place requested data
            - case 2: no empty way in L1 -> evict from L1 to L2
                    - case 2.1: empty way in L2 -> place evicted L1 data there
                    - case 2.2: no empty way in L2 -> evict from L2 to memory

        return {RM or RH, WRITEMEM or NOWRITEMEM}
        */
        uint32_t L2_block_size_bits = log2(L2_block_size);
        uint32_t L2_row_bits = log2(L2_num_sets);
        uint32_t L2_tag = addr >> (L2_block_size_bits + L2_row_bits);
        uint32_t L2_row = (addr >> L2_block_size_bits)%(L2_num_sets);


        //Debugging only
        cout << "L2 currently has tag: " << L2_cache[0]->set_blocks[0].tag << " at row 0" << endl;

        for(int i=0; i<L2_associativity; ++i){
            if((L2_tag == L2_cache[L2_row]->set_blocks[i].tag) && (L2_cache[L2_row]->set_blocks[i].valid == 1)){
                //We have a cache hit; move L2 data to L1
                L2_cache[L2_row]->set_blocks[i].valid = 0;
                bool write_to_mem = move_to_L1(L2_cache[L2_row]->set_blocks[i], addr);
                if(write_to_mem){
                    return RH_AND_WRITEMEM;
                }
                else{
                    return RH_AND_NOWRITEMEM;
                }
            }
        }
        //We have a cache miss, take data from mem and put it into memory
        CacheBlock data_from_mem;
        bool write_to_mem = move_to_L1(data_from_mem, addr);
        if(write_to_mem){
            return RM_AND_WRITEMEM;
        }
        else{
            return RM_AND_NOWRITEMEM;
        }

    }

    bool move_to_L1(const CacheBlock cache_block, const uint32_t addr){
        bool write_to_mem = 0;
        uint32_t L1_block_size_bits = log2(L1_block_size);
        uint32_t L1_row_bits = log2(L1_num_sets);
        uint32_t L1_new_tag = addr >> (L1_block_size_bits + L1_row_bits);
        uint32_t L1_row = (addr >> L1_block_size_bits)%(L1_num_sets);
        int counter = L1_cache[L1_row]->counter;
        //if full, evict to L2 first then move new block
        if(is_L1_row_full(L1_row)){
            //We need to calculate the address of the current L1 tag at the specified row
            //in order to evict it
            //uint32_t L1_tag_bits = 32 - (L1_block_size_bits + L1_row_bits);
            uint32_t L1_old_tag = L1_cache[L1_row]->set_blocks[counter].tag;
            uint32_t L1_old_addr = (L1_old_tag << (L1_row_bits + L1_block_size_bits)) | 
            (L1_row << L1_block_size_bits) | (power(2, L1_block_size_bits)-1);
            cout << "The address to be passed to L2 is: " << L1_old_addr << endl;
            write_to_mem = move_to_L2(L1_cache[L1_row]->set_blocks[counter], L1_old_addr);
        }
        L1_cache[L1_row]->set_blocks[counter] = cache_block;
        L1_cache[L1_row]->set_blocks[counter].valid = 1;
        L1_cache[L1_row]->set_blocks[counter].tag = L1_new_tag;
        L1_cache[L1_row]->counter = (counter + 1) % L1_associativity;
        return write_to_mem;
    }

    bool move_to_L2(const CacheBlock cache_block, const uint32_t addr){
        bool write_to_mem = 0;
        uint32_t L2_block_size_bits = log2(L2_block_size);
        uint32_t L2_row_bits = log2(L2_num_sets);
        uint32_t L2_tag = addr >> (L2_block_size_bits + L2_row_bits);
        uint32_t L2_row = (addr >> L2_block_size_bits)%(L2_num_sets);
        int counter = L2_cache[L2_row]->counter;
        //if full, evict to memory first then move new block
        if(is_L2_row_full(L2_row)){
            //eviction is just changing the valid bit
            L2_cache[L2_row]->set_blocks[counter].valid = 0;
            if(L2_cache[L2_row]->set_blocks[counter].dirty){
                //if dirty bit is 1, then we need to write the data into memory
                write_to_mem = 1;
            }
            L2_cache[L2_row]->counter = (counter + 1) % L2_associativity;
        }
        L2_cache[L2_row]->set_blocks[counter] = cache_block;
        L2_cache[L2_row]->set_blocks[counter].valid = 1;
        L2_cache[L2_row]->set_blocks[counter].tag = L2_tag;
        return write_to_mem;
    }

private:
    vector<Set*> L1_cache;
    int L1_block_size;
    int L1_associativity;
    int L1_cache_size;
    int L1_num_sets;

    vector<Set*> L2_cache;
    int L2_block_size;
    int L2_associativity;
    int L2_cache_size;
    int L2_num_sets;

    //Function for detrmining whether a cache row is full or not, used for eviction
    bool is_L1_row_full(int row){
        for(int i=0; i<L1_associativity; ++i){
            if(L1_cache[row]->set_blocks[i].valid == 0){
                return 0;
            }
        }
        return 1;
    }

    bool is_L2_row_full(int row){
        for(int i=0; i<L2_associativity; ++i){
            if(L2_cache[row]->set_blocks[i].valid == 0){
                return 0;
            }
        }
        return 1;
    }

    //Function for do exponent operator; used for calculating number of sets
    //in cache
    int power(int base, int exponent){
        int result = 1;
        for(int i=0; i<exponent; ++i){
            result = result * base;
        }
        return result;
    }

    //Function for taking log2(); used for calculating offset bits due to block size
    int log2(int number){
        int result = 0;
        while(number>1){
            number = number/2;
            ++result;
        }
        return result;
    }
};

/*********************************** ↑↑↑ Todo: Implement by you ↑↑↑ ******************************************/





int main(int argc, char *argv[])
{

    config cacheconfig;
    ifstream cache_params;
    string dummyLine;
    cache_params.open(argv[1]);
    while (!cache_params.eof())                   // read config file
    {
        cache_params >> dummyLine;                // L1:
        cache_params >> cacheconfig.L1blocksize;  // L1 Block size
        cache_params >> cacheconfig.L1setsize;    // L1 Associativity
        cache_params >> cacheconfig.L1size;       // L1 Cache Size
        cache_params >> dummyLine;                // L2:
        cache_params >> cacheconfig.L2blocksize;  // L2 Block size
        cache_params >> cacheconfig.L2setsize;    // L2 Associativity
        cache_params >> cacheconfig.L2size;       // L2 Cache Size
    }
    ifstream traces;
    ofstream tracesout;
    string outname;
    outname = string(argv[2]) + ".out";
    traces.open(argv[2]);
    tracesout.open(outname.c_str());
    string line;
    string accesstype;     // the Read/Write access type from the memory trace;
    string xaddr;          // the address from the memory trace store in hex;
    unsigned int addr;     // the address from the memory trace store in unsigned int;
    bitset<32> accessaddr; // the address from the memory trace store in the bitset;




/*********************************** ↓↓↓ Todo: Implement by you ↓↓↓ ******************************************/
    // Implement by you:
    // initialize the hirearch cache system with those configs
    // probably you may define a Cache class for L1 and L2, or any data structure you like
    if (cacheconfig.L1blocksize!=cacheconfig.L2blocksize){
        printf("please test with the same block size\n");
        return 1;
    }
    // cache c1(cacheconfig.L1blocksize, cacheconfig.L1setsize, cacheconfig.L1size,
    //          cacheconfig.L2blocksize, cacheconfig.L2setsize, cacheconfig.L2size);

    CacheSystem cache(cacheconfig.L1blocksize, cacheconfig.L1setsize, cacheconfig.L1size,
                   cacheconfig.L2blocksize, cacheconfig.L2setsize, cacheconfig.L2size);

    int L1AcceState = 0; // L1 access state variable, can be one of NA, RH, RM, WH, WM;
    int L2AcceState = 0; // L2 access state variable, can be one of NA, RH, RM, WH, WM;
    int MemAcceState = 0; // Main Memory access state variable, can be either NA or WH;

    if (traces.is_open() && tracesout.is_open())
    {
        while (getline(traces, line))
        { // read mem access file and access Cache

            istringstream iss(line);
            if (!(iss >> accesstype >> xaddr)){
                break;
            }
            stringstream saddr(xaddr);
            saddr >> std::hex >> addr;
            accessaddr = bitset<32>(addr);

            // access the L1 and L2 Cache according to the trace;
            if (accesstype.compare("R") == 0)  // a Read request
            {
                // Implement by you:
                //   read access to the L1 Cache,
                //   and then L2 (if required),
                //   update the access state variable;
                //   return: L1AcceState L2AcceState MemAcceState
                
                // For example:
                // L1AcceState = cache.readL1(addr); // read L1
                // if(L1AcceState == RM){
                //     L2AcceState, MemAcceState = cache.readL2(addr); // if L1 read miss, read L2
                // }
                // else{ ... }
                cout << xaddr << ": " << endl;
                L1AcceState = cache.readL1(addr);
                if(L1AcceState == RH){
                    L2AcceState = NA;
                    MemAcceState = NOWRITEMEM;
                }

                else{
                    int state = cache.readL2(addr);
                    if(state == RH_AND_NOWRITEMEM){
                        L2AcceState = RH;
                        MemAcceState = NOWRITEMEM;
                    }
                    else if(state == RH_AND_WRITEMEM){
                        L2AcceState = RH;
                        MemAcceState = WRITEMEM;
                    }
                    else if(state == RM_AND_NOWRITEMEM){
                        L2AcceState = RM;
                        MemAcceState = NOWRITEMEM;
                    }
                    else{
                        L2AcceState = RM;
                        MemAcceState = WRITEMEM;
                    }
                }
            }
            else{ // a Write request
                // Implement by you:
                //   write access to the L1 Cache, or L2 / main MEM,
                //   update the access state variable;
                //   return: L1AcceState L2AcceState

                // For example:
                // L1AcceState = cache.writeL1(addr);
                // if (L1AcceState == WM){
                //     L2AcceState, MemAcceState = cache.writeL2(addr);
                // }
                // else if(){...}
                L1AcceState = cache.writeL1(addr);
                if(L1AcceState == WH){
                    L2AcceState = NA;
                    MemAcceState = NOWRITEMEM;
                }
                else{
                    L2AcceState = cache.writeL2(addr);
                    if(L2AcceState == WH){
                        MemAcceState = NOWRITEMEM;
                    }
                    else{
                        MemAcceState = WRITEMEM;
                    }
                }
            }

/*********************************** ↑↑↑ Todo: Implement by you ↑↑↑ ******************************************/




            // Grading: don't change the code below.
            // We will print your access state of each cycle to see if your simulator gives the same result as ours.
            tracesout << L1AcceState << " " << L2AcceState << " " << MemAcceState << endl; // Output hit/miss results for L1 and L2 to the output file;
        }
        traces.close();
        tracesout.close();
    }
    else
        cout << "Unable to open trace or traceout file ";

    return 0;
}
