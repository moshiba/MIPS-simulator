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
#include <stdexcept>
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
    auto cbegin() { return blocks.cbegin(); }
    auto cend() { return blocks.cend(); }

    auto search(unsigned tag) {
        return std::find_if(blocks.begin(), blocks.end(),
                            [&tag](CacheBlock block) {
                                return block.valid && block.tag == tag;
                            });
    }

    bool has_space() {
        return std::any_of(blocks.cbegin(), blocks.cend(),
                           [](CacheBlock b) { return !(b.valid); });
    }

    bool is_full() { return !(this->has_space()); }

    auto find_space() {
        auto ptr = std::find_if(blocks.begin(), blocks.end(),
                                [](CacheBlock b) { return !(b.valid); });
        if (ptr == blocks.cend()) {
            // didn't find empty spot
            throw std::runtime_error("no space left");
        } else {
            return ptr;
        }
    }

    auto evict_who() {
        const auto copied_eviction_ptr = eviction_ptr;
        eviction_ptr = (eviction_ptr + 1) % size;
        return copied_eviction_ptr;
    }

    std::vector< CacheBlock > blocks = {CacheBlock()};
    int eviction_ptr = 0;

   private:
    int size;  // number of ways
};

constexpr long bitmask(unsigned n) { return (1UL << n) - 1; }

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
    CacheAddress(int level_, int block_size, int set_size_, int total_size_)
        : index_size(std::lround(std::ceil(
              std::log2(total_size_ * 1024 / block_size / set_size_)))),
          offset_size(std::lround(std::ceil(std::log2(block_size)))),
          tag_size(32 - index_size - offset_size) {
        dout << "L" << level_ << " cfg:  <size " << total_size_
             << " KiB> / <associativity " << set_size_ << "> / <block size "
             << block_size << ">" << endl;
        dout << "L" << level_ << " addr: <tag " << tag_size << "> / <index "
             << index_size << "> / <offset " << offset_size << ">" << endl;
    }

    auto parse(unsigned address) {
        return std::make_tuple(
            // tag bits
            ((address >> (offset_size + index_size)) & bitmask(tag_size)),
            // index bits
            ((address >> offset_size) & bitmask(index_size)),
            // offset bits
            (address & bitmask(offset_size)));
    }

    int index_size;
    int offset_size;
    int tag_size;
};

class Cache {
   public:
    Cache(int block_size_, int num_ways_, int total_size_,
          CacheAddress addr_sys_)
        : sets({CacheSet(num_ways_)}), addr_sys(addr_sys_) {
        const auto num_sets = total_size_ * 1024 / block_size_ / num_ways_;
        sets.resize(num_sets, CacheSet(num_ways_));
    }

    read_request read(unsigned addr) {
        const auto& [tag, index, offset] = addr_sys.parse(addr);
        auto& set = sets[index];
        const auto found = set.search(tag);

        if (found != set.cend()) {
            return read_request::hit;
        } else {
            return read_request::miss;
        }
    };

    write_request write(unsigned addr) {
        const auto& [tag, index, offset] = addr_sys.parse(addr);
        auto& set = sets[index];
        const auto found = set.search(tag);

        if (found != set.cend()) {
            found->dirty = true;
            return write_request::hit;
        } else {
            return write_request::miss;
        }
    };

    std::vector< CacheSet > sets;
    CacheAddress addr_sys;
};

class CacheSystem {
   public:
    CacheSystem(Config cfg)
        : l1_cache(cfg.L1blocksize, cfg.L1setsize, cfg.L1size,
                   CacheAddress(1, cfg.L1blocksize, cfg.L1setsize, cfg.L1size)),
          l2_cache(
              cfg.L2blocksize, cfg.L2setsize, cfg.L2size,
              CacheAddress(2, cfg.L2blocksize, cfg.L2setsize, cfg.L2size)) {}

    bool l2_evict(unsigned addr) {
        // return value: <bool> did_write_to_mem ?

        const auto& [tag, index, offset] = l2_cache.addr_sys.parse(addr);
        auto& set = l2_cache.sets[index];

        {
            // assert addr is cached in L2
            const auto found = set.search(tag);
            if (found == set.cend()) {
                dout << debug::bg::red << "evicting L2 addr(" << addr
                     << ") when there's space" << debug::reset << endl;
                exit(3);
            }
        }

        auto evict_idx = set.evict_who();
        set[evict_idx].valid = false;

        // pseudo-op: write to mem
        const bool did_write_to_mem = set[evict_idx].dirty;
        return did_write_to_mem;
    }

    bool l1_evict(unsigned addr) {
        // return value: <bool> did_write_to_mem ?

        const auto& [l1_tag, l1_index, l1_offset] =
            l1_cache.addr_sys.parse(addr);
        auto& l1_set = l1_cache.sets[l1_index];

        {
            // assert addr is cached in L1
            const auto found = l1_set.search(l1_tag);
            if (found == l1_set.cend()) {
                dout << debug::bg::red << "evicting L1 addr(" << addr
                     << ") when there's space" << debug::reset << endl;
                exit(2);
            }
        }

        auto evict_idx = l1_set.evict_who();
        auto& evicted_block = l1_set[evict_idx];

        // reconstruct addr of the evicted L1 block
        unsigned evicted_l1_block_addr =
            // tag
            ((evicted_block.tag & bitmask(l1_cache.addr_sys.tag_size))
             << (l1_cache.addr_sys.offset_size +
                 l1_cache.addr_sys.index_size)) |
            // index
            ((l1_index & bitmask(l1_cache.addr_sys.index_size))
             << l1_cache.addr_sys.offset_size) |
            // offset
            (0 & bitmask(l1_cache.addr_sys.offset_size));

        bool did_write_to_mem = false;

        // move L1_evicted to L2
        // search empty spot in L2 with evicted_L1_block_addr
        const auto& [l2_tag, l2_index, l2_offset] =
            l2_cache.addr_sys.parse(evicted_l1_block_addr);
        auto& l2_set = l2_cache.sets[l2_index];
        const auto l2_full_or_not =
            std::find_if(l2_set.blocks.begin(), l2_set.blocks.end(),
                         [](CacheBlock b) { return !(b.valid); });
        if (l2_full_or_not == l2_set.cend()) {  // L2 is full
            // evict L2
            auto did_write_mem = this->l2_evict(evicted_l1_block_addr);
            did_write_to_mem = did_write_mem;
        }

        // insert L1_evicted to L2
        const auto l2_empty_spot =
            std::find_if(l2_set.blocks.begin(), l2_set.blocks.end(),
                         [](CacheBlock b) { return !(b.valid); });
        if (l2_empty_spot == l2_set.cend()) {
            dout << debug::bg::red
                 << "cannot find empty spot right after eviction"
                 << debug::reset << endl;
            exit(4);
        }
        *l2_empty_spot = evicted_block;

        evicted_block.valid = false;

        return did_write_to_mem;
    }

    auto read(unsigned addr) {
        if (l1_cache.read(addr) == read_request::hit) {
            dout << debug::green << "L1 hit" << debug::reset << endl;
            return make_tuple(RH, NA, NOWRITEMEM);
        } else {
            dout << debug::red << "L1 miss" << debug::reset << endl;
            if (l2_cache.read(addr) == read_request::hit) {
                dout << debug::green << "L2 hit" << debug::reset << endl;
                /*
                 * 1. move "block" to L1
                 * 2. evict something from L1 to L2 if L1 is full
                 * 3. evict something from L2 to mem if L2 is full
                 */
                bool did_write_to_mem = false;

                // Move from L2 to L1
                // - copy then mark the L2 block as invalid
                const auto& [l2_tag, l2_index, l2_offset] =
                    l2_cache.addr_sys.parse(addr);
                const auto l2_found = l2_cache.sets[l2_index].search(l2_tag);
                CacheBlock copied_block = *l2_found;
                l2_found->valid = false;

                // - find empty spot in L1
                const auto& [l1_tag, l1_index, l1_offset] =
                    l1_cache.addr_sys.parse(addr);
                auto& l1_set = l1_cache.sets[l1_index];
                const auto empty_spot_in_l1 =
                    std::find_if(l1_set.blocks.begin(), l1_set.blocks.end(),
                                 [](CacheBlock b) { return !(b.valid); });
                if (empty_spot_in_l1 != l1_set.cend()) {
                    // found empty spot
                    *empty_spot_in_l1 = copied_block;
                } else {
                    // did not find empty spot, need to evict someone from L1
                    did_write_to_mem = this->l1_evict(addr) || did_write_to_mem;
                    // place "evicted L1 block" into L2
                    // search empty spot in L1 again
                    const auto l1_empty_spot_after_eviction =
                        std::find_if(l1_set.blocks.begin(), l1_set.blocks.end(),
                                     [](CacheBlock b) { return !(b.valid); });
                    if (l1_empty_spot_after_eviction == l1_set.cend()) {
                        dout << debug::bg::red
                             << "cannot find empty spot right after eviction"
                             << debug::reset << endl;
                        exit(5);
                    }
                    *l1_empty_spot_after_eviction = copied_block;
                }
                return make_tuple(RM, RM,
                                  did_write_to_mem ? WRITEMEM : NOWRITEMEM);
            } else {  // L2 miss
                dout << debug::red << "L2 miss" << debug::reset << endl;
                bool did_write_to_mem = false;

                // search for empty spot in L1
                const auto& [tag, index, offset] =
                    l1_cache.addr_sys.parse(addr);
                auto& set = l1_cache.sets[index];
                const auto l1_is_full =
                    std::find_if(set.blocks.begin(), set.blocks.end(),
                                 [](CacheBlock b) { return !(b.valid); });
                // TODO: check all "find_if" and try to replace with "any_of"
                // if L1 full, evict
                // TODO:
                if (l1_is_full == set.cend()) {
                    did_write_to_mem = this->l1_evict(addr) || did_write_to_mem;
                    // try inserting to L1 again
                    const auto l1_empty_spot =
                        std::find_if(set.blocks.begin(), set.blocks.end(),
                                     [](CacheBlock b) { return !(b.valid); });
                    // TODO: assert can find not-end element
                    l1_empty_spot->tag = tag;
                    l1_empty_spot->dirty = false;
                    l1_empty_spot->valid = true;
                }

                return make_tuple(RM, RM,
                                  did_write_to_mem ? WRITEMEM : NOWRITEMEM);
            }
        }
    };

    auto write(unsigned addr) {
        if (l1_cache.write(addr) == write_request::hit) {
            dout << debug::green << "L1 hit" << debug::reset << endl;
            return make_tuple(WH, NA, NOWRITEMEM);
        } else {
            dout << debug::red << "L1 miss" << debug::reset << endl;
            if (l2_cache.write(addr) == write_request::hit) {
                dout << debug::green << "L2 hit" << debug::reset << endl;
                return make_tuple(WM, WH, NOWRITEMEM);
            } else {
                dout << debug::red << "L2 miss" << debug::reset << endl;
                return make_tuple(WM, WM, WRITEMEM);
            }
        }
    };

    Cache l1_cache;
    Cache l2_cache;
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
                dout << debug::bg::blue << "R" << debug::reset << " "
                     << hex_addr << " " << access_addr << endl;

                const auto& [l1_ret, l2_ret, mem_ret] = cache_sys.read(addr);
                // Output hit/miss results for L1 and L2 to the output file;
                tracesout << l1_ret << " " << l2_ret << " " << mem_ret << endl;
            } else {
                dout << debug::bg::red << "W" << debug::reset << " " << hex_addr
                     << " " << access_addr << endl;

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
