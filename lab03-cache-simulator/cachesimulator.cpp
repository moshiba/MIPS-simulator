/* Cache Simulator
Level one L1 and level two L2 cache parameters are read from file:
- block size
- line per set
- set per cache

The 32 bit address is divided into
- tag bits (t)
    - t = 32-s-b
- set index bits (s)
    - s = log2(#sets)
- block offset bits (b)
    - b = log2(block size in bytes)

32 bit address (MSB -> LSB): TAG || SET || OFFSET

- Tag Bits:
    - the tag field along with the valid bit is used to determine whether the
      block in the cache is valid or not.
- Index Bits:
    - the set index field is used to determine which set in the cache the block
      is stored in.
- Offset Bits:
    - the offset field is used to determine which byte in the block is being
      accessed.
*/

#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#ifdef DEBUG
#include "../debug.h"
#else
namespace debug {
inline namespace bg {
using ost = std::ostream&;
ost reset(ost s) { return s; }
ost black(ost s) { return s; }
ost red(ost s) { return s; }
ost green(ost s) { return s; }
ost yellow(ost s) { return s; }
ost blue(ost s) { return s; }
ost magenta(ost s) { return s; }
ost cyan(ost s) { return s; }
ost white(ost s) { return s; }
}  // namespace bg
}  // namespace debug
#endif

inline namespace logging {

struct debug_cout {};
debug_cout dout;

bool is_debug() { return nullptr != std::getenv("DEBUG"); }

template < typename T >
debug_cout& operator<<(debug_cout& s, const T& x) {
#ifdef DEBUG
    std::cout << x;
#endif
    return s;
}

debug_cout& operator<<(debug_cout& s, std::ostream& (*f)(std::ostream&)) {
#ifdef DEBUG
    f(std::cout);
#endif
    return s;
}

debug_cout& operator<<(debug_cout& s, std::ostream& (*f)(std::ios&)) {
#ifdef DEBUG
    f(std::cout);
#endif
    return s;
}

debug_cout& operator<<(debug_cout& s, std::ostream& (*f)(std::ios_base&)) {
#ifdef DEBUG
    f(std::cout);
#endif
    return s;
}

}  // namespace logging

std::ios oldCoutState(nullptr);

using namespace std;
// access state:
#define NA 0          // no action
#define RH 1          // read hit
#define RM 2          // read miss
#define WH 3          // Write hit
#define WM 4          // write miss
#define NOWRITEMEM 5  // no write to memory
#define WRITEMEM 6    // write to memory

enum class read_request { hit = RH, miss = RM };
enum class write_request { hit = WH, miss = WM };

struct Config {
    int L1blocksize;
    int L1setsize;
    int L1size;
    int L2blocksize;
    int L2setsize;
    int L2size;
};

struct CacheBlock {
    /*
     * a single cache block:
     *   - valid bit (is the data in the block valid?)
     *   - dirty bit (has the data in the block been modified by means of a
     * write?)
     *   - tag (the tag bits of the address)
     *   - data (the actual data stored in the block, in our case, we don't need
     * to store the data)
     *
     * we don't actually need to allocate space for data, because we only need
     * to simulate the cache action or else it would have looked something like
     * this: array<number of bytes> Data;
     */
    CacheBlock(unsigned tag_, bool valid_, bool dirty_)
        : tag(tag_), valid(valid_), dirty(dirty_) {}

    CacheBlock() : CacheBlock(0, 0, 0) {}

    unsigned tag;
    bool valid;
    bool dirty;
};

struct CacheSet {
    /*
     * a cache set:
     *   - a vector of CacheBlocks
     *   - a counter to keep track of which block to evict next
     */
    CacheSet(int size_) : size(size_) { blocks.resize(size, CacheBlock()); }

    CacheBlock& operator[](int index) { return blocks[index]; }

    const CacheBlock& operator[](int index) const { return blocks[index]; }

    auto search(unsigned tag) {
        return std::any_of(blocks.cbegin(), blocks.cend(),
                           [&tag](CacheBlock block) {
                               return block.valid && block.tag == tag;
                           });
    }

    auto evict() {
        const auto current_ptr = eviction_ptr;
        auto copied_current_block = blocks[current_ptr];

        blocks[current_ptr].valid = false;
        eviction_ptr = (eviction_ptr + 1) % size;

        return copied_current_block;
    }

    std::vector< CacheBlock > blocks = {CacheBlock()};
    int eviction_ptr = 0;

   private:
    int size;  // number of ways
};

class Cache {
    std::vector< CacheSet > cache_set;

   public:
    Cache(int num_sets_, int ways_) : sets({CacheSet(ways_)}) {
        sets.resize(num_sets_, CacheSet(ways_));
    }

    read_request read(unsigned addr){
        // TODO

    };
    write_request write(unsigned addr){
        // TODO
    };
    bool check_exist(unsigned addr){
        // TODO
    };
    auto evict(unsigned addr){
        // TODO
    };

    std::vector< CacheSet > sets;
};

constexpr auto bitmask(unsigned n) { return (1UL << (n + 1)) - 1; }

template < int level_ >
class CacheAddress {
    /*
     * |------------------------|
     * |         32-bit         |
     * | <tag> <index> <offset> |
     * |------------------------|
     *
     * Fields
     * - tag bits (t)
     *      - t = 32-s-b
     * - set index bits (s)
     *      - s = log2(#sets)
     * - block offset bits (b)
     *      - b = log2(block size in bytes)
     */
   public:
    CacheAddress(int block_size, int set_size_, int total_size_)
        : index_size(std::lround(
              std::ceil(std::log2(total_size_ * 1024 / set_size_)))),
          offset_size(std::lround(std::ceil(std::log2(block_size)))),
          tag_size(32 - index_size - offset_size) {
        dout << "L" << level << " cfg:  <size " << total_size_
             << " KiB> / <associativity " << set_size_ << "> / <block size "
             << block_size << ">" << endl;
        dout << "L" << level << " addr: <tag " << tag_size << "> / <index "
             << index_size << "> / <offset " << offset_size << ">" << endl;
    }

    auto parse(unsigned address) {
        return std::make_tuple(
            // tag bits
            (address >> (offset_size + index_size)) & bitmask(tag_size),
            // index bits
            (address >> offset_size) & bitmask(index_size),
            // offset bits
            address & bitmask(offset_size));
    }

    const decltype(level_) level = level_;

    int index_size;
    int offset_size;
    int tag_size;
};
using L1Address_t = CacheAddress< 1 >;
using L2Address_t = CacheAddress< 2 >;

class CacheSystem {
   public:
    CacheSystem(Config cfg)
        : l1_cache(cfg.L1size / cfg.L1blocksize / cfg.L1setsize, cfg.L1setsize),
          l2_cache(cfg.L2size / cfg.L2blocksize / cfg.L2setsize, cfg.L2setsize),
          l1_address(cfg.L1blocksize, cfg.L1setsize, cfg.L1size),
          l2_address(cfg.L2blocksize, cfg.L2setsize, cfg.L2size) {}

    auto read(unsigned addr) {
        if (l1_cache.read(addr) == read_request::hit) {
            return make_tuple(RH, NA, NOWRITEMEM);
        } else {
            if (l2_cache.read(addr) == read_request::hit) {
                bool evict_to_mem;  // TODO: evict L2 to memory
                return make_tuple(RM, RH, evict_to_mem ? WRITEMEM : NOWRITEMEM);
            } else {
                bool evict_to_mem;  // TODO: evict L2 to memory
                return make_tuple(RM, RM, evict_to_mem ? WRITEMEM : NOWRITEMEM);
            }
        }
    };

    auto write(unsigned addr) {
        if (l1_cache.write(addr) == write_request::hit) {
            return make_tuple(WH, NA, NOWRITEMEM);
        } else {
            if (l2_cache.write(addr) == write_request::hit) {
                // TODO: evict ?
                return make_tuple(WM, WH, NOWRITEMEM);
            } else {
                // TODO: evict ?
                return make_tuple(WM, WM, WRITEMEM);
            }
        }
    };

    Cache l1_cache;
    Cache l2_cache;
    L1Address_t l1_address;
    L2Address_t l2_address;
};

int main(int argc, char* argv[]) {
    Config cacheconfig;
    {
        ifstream cache_params;
        string dummyLine;
        cache_params.open(argv[1]);
        while (!cache_params.eof()) {
            // read config file
            cache_params >> dummyLine;                // L1:
            cache_params >> cacheconfig.L1blocksize;  // L1 Block size
            cache_params >> cacheconfig.L1setsize;    // L1 Associativity
            cache_params >> cacheconfig.L1size;       // L1 Cache Size
            cache_params >> dummyLine;                // L2:
            cache_params >> cacheconfig.L2blocksize;  // L2 Block size
            cache_params >> cacheconfig.L2setsize;    // L2 Associativity
            cache_params >> cacheconfig.L2size;       // L2 Cache Size
        }
    }

    ifstream traces;
    ofstream tracesout;
    const auto outname = string(argv[2]) + ".out";
    traces.open(argv[2]);
    tracesout.open(outname.c_str());

    if (cacheconfig.L1blocksize != cacheconfig.L2blocksize) {
        printf("please test with the same block size\n");
        return 1;
    }

    CacheSystem cache_sys(cacheconfig);

    if (traces.is_open() && tracesout.is_open()) {
        string line;
        while (getline(traces,
                       line)) {  // read mem access file and access Cache

            string accesstype;
            unsigned int addr;
            bitset< 32 > access_addr;
            string hex_addr;

            istringstream iss(line);
            if (!(iss >> accesstype >> hex_addr)) {
                break;
            }
            stringstream saddr(hex_addr);
            saddr >> std::hex >> addr;
            access_addr = bitset< 32 >(addr);

            // access the L1 and L2 Cache according to the trace;
            if (accesstype.compare("R") == 0) {
                const auto& [l1_ret, l2_ret, mem_ret] = cache_sys.read(addr);
                // Output hit/miss results for L1 and L2 to the output file;
                tracesout << l1_ret << " " << l2_ret << " " << mem_ret << endl;
            } else {
                const auto& [l1_ret, l2_ret, mem_ret] = cache_sys.write(addr);
                // Output hit/miss results for L1 and L2 to the output file;
                tracesout << l1_ret << " " << l2_ret << " " << mem_ret << endl;
            }
        }
        traces.close();
        tracesout.close();
    } else
        cout << "Unable to open trace or traceout file ";

    return 0;
}
