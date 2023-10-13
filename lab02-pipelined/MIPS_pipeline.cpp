#include <bitset>
#include <fstream>
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

using namespace std;

#define MemSize \
    1000  // memory size, in reality, the memory size should be 2^32, but for
          // this lab csa23, for the space resaon, we keep it as this large
          // number, but the memory is still 32-bit addressable.

struct IFStruct {
    bitset< 32 > PC;
    bool nop;
};

struct IDStruct {
    bitset< 32 > Instr;
    bool nop;
};

struct EXStruct {
    bitset< 32 > Read_data1;
    bitset< 32 > Read_data2;
    bitset< 16 > Imm;
    bitset< 5 > Rs;
    bitset< 5 > Rt;
    bitset< 5 > Wrt_reg_addr;
    bool is_I_type;
    bool rd_mem;
    bool wrt_mem;
    bool alu_op;  // 1 for addu, lw, sw, 0 for subu
    bool wrt_enable;
    bool nop;
};

struct MEMStruct {
    bitset< 32 > ALUresult;
    bitset< 32 > Store_data;
    bitset< 5 > Rs;
    bitset< 5 > Rt;
    bitset< 5 > Wrt_reg_addr;
    bool rd_mem;
    bool wrt_mem;
    bool wrt_enable;
    bool nop;
};

struct WBStruct {
    bitset< 32 > Wrt_data;
    bitset< 5 > Rs;
    bitset< 5 > Rt;
    bitset< 5 > Wrt_reg_addr;
    bool wrt_enable;
    bool nop;
};

struct stateStruct {
    IFStruct IF;
    IDStruct ID;
    EXStruct EX;
    MEMStruct MEM;
    WBStruct WB;
};

class RF {
   public:
    bitset< 32 > Reg_data;
    RF() {
        Registers.resize(32);
        Registers[0] = bitset< 32 >(0);
    }

    bitset< 32 > readRF(bitset< 5 > Reg_addr) {
        Reg_data = Registers[Reg_addr.to_ulong()];
        return Reg_data;
    }

    void writeRF(bitset< 5 > Reg_addr, bitset< 32 > Wrt_reg_data) {
        Registers[Reg_addr.to_ulong()] = Wrt_reg_data;
    }

    void outputRF() {
        ofstream rfout;
        rfout.open("RFresult.txt", std::ios_base::app);
        if (rfout.is_open()) {
            rfout << "State of RF:\t" << endl;
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

    bitset< 32 > readInstr(bitset< 32 > ReadAddress) {
        string insmem;
        insmem.append(IMem[ReadAddress.to_ulong()].to_string());
        insmem.append(IMem[ReadAddress.to_ulong() + 1].to_string());
        insmem.append(IMem[ReadAddress.to_ulong() + 2].to_string());
        insmem.append(IMem[ReadAddress.to_ulong() + 3].to_string());
        Instruction = bitset< 32 >(insmem);  // read instruction memory
        return Instruction;
    }

   private:
    vector< bitset< 8 > > IMem;
};

class DataMem {
   public:
    bitset< 32 > ReadData;
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

    bitset< 32 > readDataMem(bitset< 32 > Address) {
        string datamem;
        datamem.append(DMem[Address.to_ulong()].to_string());
        datamem.append(DMem[Address.to_ulong() + 1].to_string());
        datamem.append(DMem[Address.to_ulong() + 2].to_string());
        datamem.append(DMem[Address.to_ulong() + 3].to_string());
        ReadData = bitset< 32 >(datamem);  // read data memory
        return ReadData;
    }

    void writeDataMem(bitset< 32 > Address, bitset< 32 > WriteData) {
        DMem[Address.to_ulong()] =
            bitset< 8 >(WriteData.to_string().substr(0, 8));
        DMem[Address.to_ulong() + 1] =
            bitset< 8 >(WriteData.to_string().substr(8, 8));
        DMem[Address.to_ulong() + 2] =
            bitset< 8 >(WriteData.to_string().substr(16, 8));
        DMem[Address.to_ulong() + 3] =
            bitset< 8 >(WriteData.to_string().substr(24, 8));
    }

    void outputDataMem() {
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

void printState(stateStruct state, int cycle) {
    ofstream printstate;
    printstate.open("stateresult.txt", std::ios_base::app);
    if (printstate.is_open()) {
        printstate << "State after executing cycle:\t" << cycle << endl;

        printstate << "IF.PC:\t" << state.IF.PC.to_ulong() << endl;
        printstate << "IF.nop:\t" << state.IF.nop << endl;

        printstate << "ID.Instr:\t" << state.ID.Instr << endl;
        printstate << "ID.nop:\t" << state.ID.nop << endl;

        printstate << "EX.Read_data1:\t" << state.EX.Read_data1 << endl;
        printstate << "EX.Read_data2:\t" << state.EX.Read_data2 << endl;
        printstate << "EX.Imm:\t" << state.EX.Imm << endl;
        printstate << "EX.Rs:\t" << state.EX.Rs << endl;
        printstate << "EX.Rt:\t" << state.EX.Rt << endl;
        printstate << "EX.Wrt_reg_addr:\t" << state.EX.Wrt_reg_addr << endl;
        printstate << "EX.is_I_type:\t" << state.EX.is_I_type << endl;
        printstate << "EX.rd_mem:\t" << state.EX.rd_mem << endl;
        printstate << "EX.wrt_mem:\t" << state.EX.wrt_mem << endl;
        printstate << "EX.alu_op:\t" << state.EX.alu_op << endl;
        printstate << "EX.wrt_enable:\t" << state.EX.wrt_enable << endl;
        printstate << "EX.nop:\t" << state.EX.nop << endl;

        printstate << "MEM.ALUresult:\t" << state.MEM.ALUresult << endl;
        printstate << "MEM.Store_data:\t" << state.MEM.Store_data << endl;
        printstate << "MEM.Rs:\t" << state.MEM.Rs << endl;
        printstate << "MEM.Rt:\t" << state.MEM.Rt << endl;
        printstate << "MEM.Wrt_reg_addr:\t" << state.MEM.Wrt_reg_addr << endl;
        printstate << "MEM.rd_mem:\t" << state.MEM.rd_mem << endl;
        printstate << "MEM.wrt_mem:\t" << state.MEM.wrt_mem << endl;
        printstate << "MEM.wrt_enable:\t" << state.MEM.wrt_enable << endl;
        printstate << "MEM.nop:\t" << state.MEM.nop << endl;

        printstate << "WB.Wrt_data:\t" << state.WB.Wrt_data << endl;
        printstate << "WB.Rs:\t" << state.WB.Rs << endl;
        printstate << "WB.Rt:\t" << state.WB.Rt << endl;
        printstate << "WB.Wrt_reg_addr:\t" << state.WB.Wrt_reg_addr << endl;
        printstate << "WB.wrt_enable:\t" << state.WB.wrt_enable << endl;
        printstate << "WB.nop:\t" << state.WB.nop << endl;
    } else
        cout << "Unable to open file";
    printstate.close();
}

int main() {
    RF myRF;
    INSMem myInsMem;
    DataMem myDataMem;

    stateStruct state;
    state.IF.PC = 0;
    state.IF.nop = 0;
    state.ID.nop = 1;
    state.EX.nop = 1;
    state.MEM.nop = 1;
    state.WB.nop = 1;
    stateStruct newState = state;
    int cycle = 0;

    bool control_hazard_flag = 0;

    while (1) {
        dout << "cycle " << cycle << std::endl;

        /* --------------------- WB stage --------------------- */
        {
            if (state.WB.nop == 0) {
                if (state.WB.wrt_enable) {
                    myRF.writeRF(state.WB.Wrt_reg_addr, state.WB.Wrt_data);
                }
            }
        }

        /* --------------------- MEM stage --------------------- */
        {
            newState.WB.nop = state.MEM.nop;
            if (newState.WB.nop == 0) {
                if (state.MEM.rd_mem) {  // lw
                    newState.WB.Wrt_data =
                        myDataMem.readDataMem(state.MEM.ALUresult);
                }
                if (state.MEM.wrt_mem) {  // sw
                    myDataMem.writeDataMem(state.MEM.ALUresult,
                                           state.MEM.Store_data);
                }
                newState.WB.Rs = state.MEM.Rs;
                newState.WB.Rt = state.MEM.Rt;
                newState.WB.Wrt_reg_addr = state.MEM.Wrt_reg_addr;
                newState.WB.wrt_enable = state.MEM.wrt_enable;
            }
        }

        /* --------------------- EX stage --------------------- */
        {
            newState.MEM.nop = state.EX.nop;
            if (newState.MEM.nop == 0) {
                if (state.EX.alu_op) {
                    // addition
                    if (state.EX.is_I_type) {
                        // I type
                        newState.MEM.ALUresult =
                            bitset< 32 >(state.EX.Read_data1.to_ullong() +
                                         state.EX.Imm.to_ullong());
                    } else {
                        // R type
                        newState.MEM.ALUresult =
                            bitset< 32 >(state.EX.Read_data1.to_ullong() +
                                         state.EX.Read_data2.to_ullong());
                    }
                } else {
                    // subtraction
                    newState.MEM.ALUresult =
                        bitset< 32 >(state.EX.Read_data1.to_ullong() -
                                     state.EX.Read_data2.to_ullong());
                }
                if (state.EX.wrt_mem) {
                    newState.MEM.Store_data = state.EX.Read_data2;
                }
                newState.MEM.Rs = state.EX.Rs;
                newState.MEM.Rt = state.EX.Rt;
                newState.MEM.Wrt_reg_addr = state.EX.Wrt_reg_addr;
                newState.MEM.wrt_enable = state.EX.wrt_enable;
                newState.MEM.rd_mem = state.EX.rd_mem;
                newState.MEM.wrt_mem = state.EX.wrt_mem;
            }
        }

        /* --------------------- ID stage --------------------- */
        {
            const auto instruction = state.ID.Instr.to_ulong();
            const auto opcode = instruction >> 26;
            newState.EX.nop = state.ID.nop;
            if (newState.EX.nop == 0) {
                const bitset< 6 > funct =
                    bitset< 6 >(state.ID.Instr.to_ullong() & 0x3F);

                if (opcode == 0x0) {
                    newState.EX.is_I_type = 0;
                    newState.EX.rd_mem = 0;
                    newState.EX.wrt_mem = 0;
                    newState.EX.wrt_enable = 1;
                    if (funct == 0x23) {
                        newState.EX.alu_op = 0;  // subu
                    } else {
                        newState.EX.alu_op = 1;  // addu
                    }
                } else if (opcode != 0x4) {  // not bne
                    newState.EX.is_I_type = 1;
                    newState.EX.alu_op = 1;
                    if (opcode == 0x23) {  // lw
                        newState.EX.rd_mem = 1;
                        newState.EX.wrt_mem = 0;
                        newState.EX.wrt_enable = 1;
                    } else if (opcode == 0x2B) {  // sw
                        newState.EX.rd_mem = 0;
                        newState.EX.wrt_mem = 1;
                        newState.EX.wrt_enable = 0;
                    }
                } else if (opcode == 0x5) {  // bne
                    newState.EX.is_I_type = 1;
                    newState.EX.alu_op = 0;
                    newState.EX.rd_mem = 0;
                    newState.EX.wrt_mem = 0;
                    newState.EX.wrt_enable = 0;
                } else {
                    cout << "Opcode not accounted for" << endl;
                }

                if ((opcode != 0x2) && (opcode != 0x3)) {  // not a j-type
                    newState.EX.Rs =
                        bitset< 5 >((state.ID.Instr.to_ullong() >> 21) & 0x1F);
                    newState.EX.Rt =
                        bitset< 5 >((state.ID.Instr.to_ullong() >> 16) & 0x1F);
                    newState.EX.Read_data1 = myRF.readRF(newState.EX.Rs);
                    newState.EX.Read_data2 = myRF.readRF(newState.EX.Rt);
                    newState.EX.Imm =
                        bitset< 16 >((state.ID.Instr.to_ullong() & 0xFFFF));
                    if (newState.EX.is_I_type == 1) {
                        newState.EX.Wrt_reg_addr = newState.EX.Rt;
                        // Rt is the destination register for I-type
                    } else {
                        newState.EX.Wrt_reg_addr = bitset< 5 >(
                            (state.ID.Instr.to_ullong() >> 11) & 0x1F);
                        // Rd is the destination register for R-type
                    }
                }
                if (opcode == 0x4) {  // bne
                    if (newState.EX.Read_data1 != newState.EX.Read_data2) {
                        newState.IF.PC = state.IF.PC.to_ullong() +
                                         (newState.EX.Imm.to_ullong() << 2);
                        newState.ID.nop = 1;
                        control_hazard_flag = 1;
                    }
                    // else do nothing
                }
            }
        }

        /* --------------------- IF stage --------------------- */
        {
            if (control_hazard_flag) {
                newState.ID.nop = 1;
            } else {
                newState.ID.nop = state.IF.nop;
            }
            if (newState.ID.nop == 0) {
                newState.IF.PC =
                    bitset< 32 >(state.IF.PC.to_ullong() + 4);  // PC = PC + 4
                newState.ID.Instr =
                    myInsMem.readInstr(state.IF.PC);  // read from imem

                if (newState.ID.Instr == 0xFFFFFFFF) {  // check for halt
                    dout << debug::bg::red << "HALT" << debug::reset << endl;
                    newState.IF.nop = 1;
                    newState.ID.nop = 1;
                }
            }
        }

        //////////////////////////////////////////////////////////
        dout << "NOP" << endl
             << "if " << newState.IF.nop << endl
             << "id " << newState.ID.nop << endl
             << "ex " << newState.EX.nop << endl
             << "mm " << newState.MEM.nop << endl
             << "wb " << newState.WB.nop << endl;
        if (state.IF.nop && state.ID.nop && state.EX.nop && state.MEM.nop &&
            state.WB.nop)
            break;

        printState(newState, cycle);  // print states after executing cycle 0,
                                      // cycle 1, cycle 2 ...

        state = newState;
        /*** The end of the cycle and updates the current state
                         with the values calculated in this cycle. csa23 ***/
        control_hazard_flag = 0;
        ++cycle;
    }

    myRF.outputRF();            // dump RF;
    myDataMem.outputDataMem();  // dump data mem

    return 0;
}
