#include <bitset>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
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
debug_cout& operator<<(debug_cout& s, [[maybe_unused]] const T& x) {
#ifdef DEBUG
    std::cout << x;
#endif
    return s;
}

debug_cout& operator<<(debug_cout& s,
                       [[maybe_unused]] std::ostream& (*f)(std::ostream&)) {
#ifdef DEBUG
    f(std::cout);
#endif
    return s;
}

debug_cout& operator<<(debug_cout& s,
                       [[maybe_unused]] std::ostream& (*f)(std::ios&)) {
#ifdef DEBUG
    f(std::cout);
#endif
    return s;
}

debug_cout& operator<<(debug_cout& s,
                       [[maybe_unused]] std::ostream& (*f)(std::ios_base&)) {
#ifdef DEBUG
    f(std::cout);
#endif
    return s;
}

}  // namespace logging

std::ios oldCoutState(nullptr);

using namespace std;

constexpr const int MemSize = 65536;

constexpr long bitmask(unsigned n) { return (1UL << n) - 1; }

class PhysicalMemory {
   public:
    PhysicalMemory() {
        DMem.resize(MemSize);
        ifstream dmem;
        string line;
        int i = 0;
        dmem.open("pt_initialize.txt");
        if (dmem.is_open()) {
            while (getline(dmem, line)) {
                DMem[i] = bitset< 8 >(line);
                i++;
            }
        } else
            cout << "Unable to open page table init file";
        dmem.close();
    }

    unsigned operator[](int index) {
        if (index >= (1 << 12)) {
            throw std::out_of_range("Memory access out of range");
        }
        return DMem[index + 0].to_ulong() << 24 |
               DMem[index + 1].to_ulong() << 16 |
               DMem[index + 2].to_ulong() << 8 |
               DMem[index + 3].to_ulong() << 0;
    }

    bitset< 32 > outputMemValue(bitset< 12 > address_bits) {
        return bitset< 32 >((*this)[address_bits.to_ulong()]);
    }

   private:
    vector< bitset< 8 > > DMem;
};

class VirtualAddress {
   public:
    static constexpr int n_bits_outer = 4;
    static constexpr int n_bits_inner = 4;
    static constexpr int n_bits_offset = 6;
    static_assert(n_bits_outer + n_bits_inner + n_bits_offset == 14);

    VirtualAddress(unsigned address)
        : value(address),
          outer_page_number((address >> (n_bits_inner + n_bits_offset)) &
                            bitmask(n_bits_outer)),
          inner_page_number((address >> n_bits_offset) & bitmask(n_bits_inner)),
          offset(address & bitmask(n_bits_offset)) {}

    unsigned value;
    unsigned outer_page_number;
    unsigned inner_page_number;
    unsigned offset;
};

constexpr const int page_size = 1 << VirtualAddress::n_bits_offset;

class OuterPageTableEntry {
   public:
    static constexpr int n_bits_pa = 12;
    static constexpr int n_bits_unused = 19;
    static constexpr int n_bits_valid = 1;
    static_assert(n_bits_pa + n_bits_unused + n_bits_valid == 32);

    OuterPageTableEntry(unsigned address)
        : value(address),
          inner_table_addr((address >> (n_bits_unused + n_bits_valid)) &
                           bitmask(n_bits_pa)),
          unused((address >> n_bits_valid) & bitmask(n_bits_unused)),
          valid(address & bitmask(n_bits_valid)) {}

    unsigned value;
    unsigned inner_table_addr;
    unsigned unused;
    unsigned valid;
};

class InnerPageTableEntry {
   public:
    static constexpr int n_bits_frame = 6;
    static constexpr int n_bits_unused = 25;
    static constexpr int n_bits_valid = 1;
    static_assert(n_bits_frame + n_bits_unused + n_bits_valid == 32);

    InnerPageTableEntry(unsigned address)
        : value(address),
          frame_num((address >> (n_bits_unused + n_bits_valid)) &
                    bitmask(n_bits_frame)),
          unused((address >> n_bits_valid) & bitmask(n_bits_unused)),
          valid(address & bitmask(n_bits_valid)) {}

    unsigned value;
    unsigned frame_num;
    unsigned unused;
    unsigned valid;
};

class PhysicalAddress {
   public:
    PhysicalAddress(unsigned frame_number_, unsigned frame_offset_)
        : frame_number(frame_number_), frame_offset(frame_offset_) {}

    operator unsigned() { return frame_number << 6 | frame_offset; }

    unsigned frame_number;
    unsigned frame_offset;
};

class OuterPageTable {
   public:
    OuterPageTable(PhysicalMemory& memory_, unsigned pt_addr_)
        : memory(memory_), pt_addr(pt_addr_) {}

    OuterPageTableEntry operator[](int index) {
        const unsigned addr_offset = index << 2;
        if (addr_offset >= page_size) {
            throw std::out_of_range("Outer-PT access out of range");
        }
        if (addr_offset % 4 != 0) {
            throw std::invalid_argument("Unaligned outer-PT memory access");
        }
        return memory[pt_addr + addr_offset];
    }

    PhysicalMemory& memory;
    unsigned pt_addr;
};

class InnerPageTable {
   public:
    InnerPageTable(PhysicalMemory& memory_, unsigned pt_addr_)
        : memory(memory_), pt_addr(pt_addr_) {}

    InnerPageTableEntry operator[](int index) {
        const unsigned addr_offset = index << 2;
        if (addr_offset >= page_size) {
            throw std::out_of_range("Inner-PT access out of range");
        }
        if (addr_offset % 4 != 0) {
            throw std::invalid_argument("Unaligned inner-PT memory access");
        }
        return memory[pt_addr + addr_offset];
    }

    PhysicalMemory& memory;
    unsigned pt_addr;
};

int main([[maybe_unused]] int argc, char* argv[]) {
    PhysicalMemory myPhyMem;

    ifstream traces;
    ifstream PTB_file;
    ofstream tracesout;

    string outname;
    outname = "pt_results.txt";

    traces.open(argv[1]);
    PTB_file.open(argv[2]);
    tracesout.open(outname.c_str());

    // Initialize the PTBR
    bitset< 12 > PTBR;
    PTB_file >> PTBR;

    auto outer_page_table = OuterPageTable(myPhyMem, PTBR.to_ulong());

    // Read a virtual address form the PageTable and convert it to the
    // physical address
    if (traces.is_open() && tracesout.is_open()) {
        string line;
        while (getline(traces, line)) {
            const auto virtual_addr_bits = bitset< 14 >(line);
            const auto virtual_addr =
                VirtualAddress(virtual_addr_bits.to_ulong());

            auto outer_pte = outer_page_table[virtual_addr.outer_page_number];

            if (outer_pte.valid) {
                auto inner_page_table =
                    InnerPageTable(myPhyMem, outer_pte.inner_table_addr);
                auto inner_pte =
                    inner_page_table[virtual_addr.inner_page_number];

                if (inner_pte.valid) {
                    auto phy_addr = PhysicalAddress(inner_pte.frame_num,
                                                    virtual_addr.offset);
                    tracesout << std::hex << "1, 1, 0x" << std::setfill('0')
                              << std::setw(3) << phy_addr << ", 0x"
                              << std::setw(8) << myPhyMem[phy_addr] << std::dec
                              << endl;
                } else {
                    tracesout << "1, 0, 0x000, 0x00000000" << endl;
                }
            } else {
                tracesout << "0, 0, 0x000, 0x00000000" << endl;
            }
        }
        traces.close();
        tracesout.close();
    }

    else
        cout << "Unable to open trace or traceout file ";
}
