#include <bitset>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
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

using B32 = std::bitset< 32 >;
using B8 = std::bitset< 8 >;

using namespace std;

#define ADDU (1)
#define SUBU (3)
#define AND (4)
#define OR (5)
#define NOR (7)

// Memory size.
// In reality, the memory size should be 2^32, but for this lab and space
// reasons, we keep it as this large number, but the memory is still 32-bit
// addressable.
#define MemSize (65536)

class RF {
   public:
    bitset< 32 > ReadData1, ReadData2;
    RF() {
        Registers.resize(32);
        Registers[0] = bitset< 32 >(0);
    }

    void ReadWrite(bitset< 5 > RdReg1, bitset< 5 > RdReg2, bitset< 5 > WrtReg,
                   bitset< 32 > WrtData, bitset< 1 > WrtEnable) {
        /**
         * @brief Reads or writes data from/to the Register.
         *
         * This function is used to read or write data from/to the register,
         * depending on the value of WrtEnable. Put the read results to the
         * ReadData1 and ReadData2.
         */
        dout << debug::bg::magenta << " REG  ";

        if (WrtData == 1) {
            auto reg_idx = WrtReg.to_ulong();
            Registers[reg_idx] = WrtData;

            dout << debug::bg::red << " WRITE " << debug::reset << "R"
                 << reg_idx << "=" << WrtData << endl;

        } else {
            auto reg1_idx = RdReg1.to_ulong();
            auto reg2_idx = RdReg2.to_ulong();
            ReadData1 = Registers[reg1_idx];
            ReadData2 = Registers[reg2_idx];

            dout << debug::bg::blue << " READ  " << debug::reset << "R"
                 << reg1_idx << "=0x" << hex << uppercase << setfill('0')
                 << setw(8) << ReadData1.to_ulong() << ",R" << dec << reg2_idx
                 << "=0x" << hex << setw(8) << ReadData2.to_ulong() << endl;
        }
        std::cout.copyfmt(oldCoutState);
    }

    void OutputRF() {
        ofstream rfout;
        rfout.open("RFresult.txt", std::ios_base::app);
        if (rfout.is_open()) {
            rfout << "A state of RF:" << endl;
            for (int j = 0; j < 32; j++) {
                rfout << Registers[j] << endl;
            }

        } else
            cout << "Unable to open file";
        rfout.close();
    }

   private:
    vector< bitset< 32 > > Registers;
};

class ALU {
   public:
    bitset< 32 > ALUresult;
    bitset< 32 > ALUOperation(bitset< 3 > ALUOP, bitset< 32 > oprand1,
                              bitset< 32 > oprand2) {
        /**
         * @brief Implement the ALU operation here.
         *
         * ALU operation depends on the ALUOP, which are definded as ADDU, SUBU,
         * etc.
         */
        dout << " ALU  " << debug::reset << "ctrl=" << ALUOP.to_ulong()
             << " op1=" << oprand1.to_ulong() << " op2=" << oprand2.to_ulong();
        switch (ALUOP.to_ulong()) {
            case 1: {  // addu
                ALUresult = B32(oprand1.to_ulong() + oprand2.to_ulong());
                break;
            }
            case 3: {  // subu
                ALUresult = B32(oprand1.to_ulong() - oprand2.to_ulong());
                break;
            }
            case 4: {  // and
                ALUresult = oprand1 & oprand2;
                break;
            }
            case 5: {  // or
                ALUresult = oprand1 | oprand2;
                break;
            }
            case 7: {  // nor
                ALUresult = oprand1 ^ oprand2;
                break;
            }
            default: {
                throw runtime_error("ALU: unknown op");
            }
        }

        dout << " result=" << ALUresult.to_ulong() << endl;
        return ALUresult;
    }
};

class INSMem {
   public:
    bitset< 32 > Instruction;

    INSMem() {
        IMem.resize(MemSize);
        ifstream imem;
        string line;
        int i = 0;
        imem.open("imem.txt");
        if (imem.is_open()) {
            while (getline(imem, line)) {
                IMem[i] = bitset< 8 >(line);
                i++;
            }

        } else
            cout << "Unable to open file";
        imem.close();
    }

    bitset< 32 > ReadMemory(bitset< 32 > ReadAddress) {
        /**
         * @brief Read Instruction Memory (IMem).
         *
         * Read the byte at the ReadAddress and the following three byte,
         * and return the read result.
         */
        unsigned address = ReadAddress.to_ulong();
        Instruction = IMem[address + 0].to_ulong() << 24 |
                      IMem[address + 1].to_ulong() << 16 |
                      IMem[address + 2].to_ulong() << 8 |
                      IMem[address + 3].to_ulong() << 0;

        dout << debug::bg::yellow << " IMEM " << debug::bg::blue << " READ  "
             << debug::reset << "[" << setfill('0') << setw(5) << right
             << address << "]"
             << "=" << Instruction << endl;
        std::cout.copyfmt(oldCoutState);

        return Instruction;
    }

   private:
    vector< bitset< 8 > > IMem;
};

class DataMem {
   public:
    bitset< 32 > readdata;
    DataMem() {
        DMem.resize(MemSize);
        ifstream dmem;
        string line;
        int i = 0;
        dmem.open("dmem.txt");
        if (dmem.is_open()) {
            while (getline(dmem, line)) {
                DMem[i] = bitset< 8 >(line);
                i++;
            }
        } else
            cout << "Unable to open file";
        dmem.close();
    }
    bitset< 32 > MemoryAccess(bitset< 32 > Address, bitset< 32 > WriteData,
                              bitset< 1 > readmem, bitset< 1 > writemem) {
        /**
         * @brief Reads/writes data from/to the Data Memory.
         *
         * This function is used to read/write data from/to the DataMem,
         * depending on the readmem and writemem. First, if writemem enabled,
         * WriteData should be written to DMem, clear or ignore the return value
         * readdata, and note that 32-bit WriteData will occupy 4 continious
         * Bytes in DMem. If readmem enabled, return the DMem read result as
         * readdata.
         */
        if ((readmem & writemem) == 1) {
            throw logic_error("data mem op conflict");
        }
        unsigned address = Address.to_ulong();

        if (readmem == 1) {
            readdata = B32(DMem[address + 0].to_ulong() << 24 |
                           DMem[address + 1].to_ulong() << 16 |
                           DMem[address + 2].to_ulong() << 8 |
                           DMem[address + 3].to_ulong() << 0);

            dout << debug::bg::green << " DMEM " << debug::bg::blue << " READ  "
                 << debug::reset << "[" << setfill('0') << setw(5) << right
                 << address << "]"
                 << "=" << readdata << endl;
            std::cout.copyfmt(oldCoutState);
        }

        if (writemem == 1) {
            DMem[address + 0] = B8((WriteData >> 24).to_ulong());
            DMem[address + 1] = B8((WriteData >> 16).to_ulong());
            DMem[address + 2] = B8((WriteData >> 8).to_ulong());
            DMem[address + 3] = B8((WriteData >> 0).to_ulong());

            dout << debug::bg::green << " DMEM " << debug::bg::red << " WRITE "
                 << debug::reset << "[" << setfill('0') << setw(5) << right
                 << address << "]"
                 << "=" << WriteData << endl;
            std::cout.copyfmt(oldCoutState);
        }

        return readdata;
    }

    void OutputDataMem() {
        ofstream dmemout;
        dmemout.open("dmemresult.txt");
        if (dmemout.is_open()) {
            for (int j = 0; j < 1000; j++) {
                dmemout << DMem[j] << endl;
            }

        } else
            cout << "Unable to open file";
        dmemout.close();
    }

   private:
    vector< bitset< 8 > > DMem;
};

int main() {
    oldCoutState.copyfmt(std::cout);

    RF myRF;
    ALU myALU;
    INSMem myInsMem;
    DataMem myDataMem;

    unsigned PC = 0;
    while (PC < MemSize)  // TODO: implement!
    {
        // Fetch: fetch an instruction from myInsMem.
        auto instruction = myInsMem.ReadMemory(PC);
        PC += 4;

        // If current instruction is "11111111111111111111111111111111",
        // then break; (exit the while loop)

        // decode(Read RF): get opcode and other signals from instruction,
        // decode instruction

        // Execute: after decoding, ALU may run and return result

        // Read/Write Mem: access data memory (myDataMem)

        // Write back to RF: some operations may write things to RF

        /**** You don't need to modify the following lines. ****/
        myRF.OutputRF();  // dump RF;
    }
    myDataMem.OutputDataMem();  // dump data mem

    return 0;
}
