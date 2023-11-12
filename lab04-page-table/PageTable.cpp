#include <bitset>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
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

#define MemSize (65536)

class PhyMem {
   public:
    bitset< 32 > readdata;
    PhyMem() {
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
    bitset< 32 > outputMemValue(bitset< 12 > Address) {
        bitset< 32 > readdata;
        /**TODO: implement!
         * Returns the value stored in the physical address
         */

        return readdata;
    }

   private:
    vector< bitset< 8 > > DMem;
};

int main(int argc, char *argv[]) {
    PhyMem myPhyMem;

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

    string line;
    bitset< 14 > virtualAddr;

    /*********************************** ↓↓↓ Todo: Implement by you ↓↓↓
     * ******************************************/

    // Read a virtual address form the PageTable and convert it to the physical
    // address
    if (traces.is_open() && tracesout.is_open()) {
        while (getline(traces, line)) {
            // TODO: Implement!
            //  Access the outer page table

            // If outer page table valid bit is 1, access the inner page table

            // Return valid bit in outer and inner page table, physical address,
            // and value stored in the physical memory.
            //  Each line in the output file for example should be: 1, 0, 0x000,
            //  0x00000000
        }
        traces.close();
        tracesout.close();
    }

    else
        cout << "Unable to open trace or traceout file ";

    return 0;
}
