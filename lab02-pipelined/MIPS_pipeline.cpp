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

        {
            dout << debug::bg::magenta << " REG  " << debug::bg::blue
                 << " READ  " << debug::reset << "R" << Reg_addr.to_ulong()
                 << "=0x" << hex << uppercase << setfill('0') << setw(8)
                 << Reg_data.to_ulong() << endl;
            std::cout.copyfmt(oldCoutState);
        }

        return Reg_data;
    }

    void writeRF(bitset< 5 > Reg_addr, bitset< 32 > Wrt_reg_data) {
        Registers[Reg_addr.to_ulong()] = Wrt_reg_data;

        {
            dout << debug::bg::magenta << " REG  " << debug::bg::red
                 << " WRITE " << debug::reset << "R" << Reg_addr.to_ulong()
                 << "=" << Wrt_reg_data.to_ulong() << endl;
            std::cout.copyfmt(oldCoutState);
        }
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

        {
            dout << debug::bg::yellow << " IMEM " << debug::bg::blue
                 << " READ  " << debug::reset << "[" << setfill('0') << setw(5)
                 << right << ReadAddress.to_ulong() << "]"
                 << "=" << Instruction << endl;
            std::cout.copyfmt(oldCoutState);
        }

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

        {
            dout << debug::bg::green << " DMEM " << debug::bg::blue << " READ  "
                 << debug::reset << "[" << setfill('0') << setw(5) << right
                 << Address.to_ulong() << "]"
                 << "=" << ReadData << endl;
            std::cout.copyfmt(oldCoutState);
        }

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

        {
            dout << debug::bg::green << " DMEM " << debug::bg::red << " WRITE "
                 << debug::reset << "[" << setfill('0') << setw(5) << right
                 << Address.to_ulong() << "]"
                 << "=" << WriteData << endl;
            std::cout.copyfmt(oldCoutState);
        }
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
    {  // IF
        state.IF.PC = 0;
        state.IF.nop = 0;
    }
    {  // ID
        state.ID.nop = 1;
        // init rest to 0
        state.ID.Instr = 0;
    }
    {  // EX
        state.EX.nop = 1;
        state.EX.alu_op = 1;
        // init rest to 0
        state.EX.Read_data1 = 0;
        state.EX.Read_data2 = 0;
        state.EX.Imm = 0;
        state.EX.Rs = 0;
        state.EX.Rt = 0;
        state.EX.Wrt_reg_addr = 0;
        state.EX.is_I_type = 0;
        state.EX.rd_mem = 0;
        state.EX.wrt_mem = 0;
        state.EX.wrt_enable = 0;
    }
    {  // MEM
        state.MEM.nop = 1;
        // init rest to 0
        state.MEM.ALUresult = 0;
        state.MEM.Store_data = 0;
        state.MEM.Rs = 0;
        state.MEM.Rt = 0;
        state.MEM.Wrt_reg_addr = 0;
        state.MEM.rd_mem = 0;
        state.MEM.wrt_mem = 0;
        state.MEM.wrt_enable = 0;
    }
    {  // WB
        state.WB.nop = 1;
        // init rest to 0
        state.WB.Wrt_data = 0;
        state.WB.Rs = 0;
        state.WB.Rt = 0;
        state.WB.Wrt_reg_addr = 0;
        state.WB.wrt_enable = 0;
    }
    stateStruct newState = state;
    int cycle = 0;

    while (1) {
        dout << "\n"
                "================================"
                "================================"
             << debug::bg::white << debug::black << endl
             << "cycle " << cycle << debug::reset << endl;

        bool freeze_if = 0;
        bool freeze_id = 0;
        bool bubble = 0;

        /* --------------------- WB stage --------------------- */
        {
            dout << "----------------\nWB\n";

            if (!state.WB.nop) {
                if (state.WB.wrt_enable) {
                    myRF.writeRF(state.WB.Wrt_reg_addr, state.WB.Wrt_data);
                }
            }
        }

        /* --------------------- MEM stage --------------------- */
        {
            dout << "----------------\nMEM\n";

            if (!state.MEM.nop) {
                if (state.MEM.rd_mem || state.MEM.wrt_mem) {
                    if (state.MEM.wrt_mem) {  // sw
                        myDataMem.writeDataMem(state.MEM.ALUresult,
                                               state.MEM.Store_data);
                    }
                    // lw
                    newState.WB.Wrt_data =
                        myDataMem.readDataMem(state.MEM.ALUresult);
                } else {
                    newState.WB.Wrt_data = state.MEM.ALUresult;
                }
                newState.WB.Rs = state.MEM.Rs;
                newState.WB.Rt = state.MEM.Rt;
                newState.WB.Wrt_reg_addr = state.MEM.Wrt_reg_addr;
                newState.WB.wrt_enable = state.MEM.wrt_enable;
            }

            newState.WB.nop = state.MEM.nop;
        }

        /* --------------------- EX stage --------------------- */
        {
            dout << "----------------\nEX\n";

            unsigned operand1;
            unsigned operand2;

            if (!state.EX.nop) {
                // Mem-Ex hazard control
                const bool mem_ex_forward_rs =
                    state.WB.wrt_enable &&
                    (state.WB.Wrt_reg_addr == state.EX.Rs);
                const bool mem_ex_forward_rt =
                    state.WB.wrt_enable &&
                    (state.WB.Wrt_reg_addr == state.EX.Rt);

                // Insert bubble between lw-add
                // if (state.EX.rd_mem &&
                //     (mem_ex_forward_rs || mem_ex_forward_rt)) {
                //     freeze_if = 1;  // TODO: fix false-halt
                //     freeze_id = 1;
                //     // create bubble
                //     {
                //         newState.EX.Read_data1 = 0;
                //         newState.EX.Read_data2 = 0;
                //         newState.EX.Imm = 0;
                //         newState.EX.Rs = 0;
                //         newState.EX.Rt = 0;
                //         newState.EX.Wrt_reg_addr = 0;
                //         newState.EX.is_I_type = 0;
                //         newState.EX.rd_mem = 0;
                //         newState.EX.wrt_enable = 0;
                //         newState.EX.alu_op = 0;
                //         newState.EX.wrt_enable = 0;
                //         newState.EX.nop = 0;
                //     }
                // }

                // Ex-Ex hazard control
                const bool ex_ex_forward_rs =
                    state.MEM.wrt_enable &&
                    (state.MEM.Wrt_reg_addr == state.EX.Rs);
                const bool ex_ex_forward_rt =
                    state.MEM.wrt_enable &&
                    (state.MEM.Wrt_reg_addr == state.EX.Rt);

                // const unsigned operand1 =
                //     mem_ex_forward_rs
                //         ? state.WB.Wrt_data.to_ulong()
                //         : (ex_ex_forward_rs ? state.MEM.ALUresult.to_ulong()
                //                             :
                //                             state.EX.Read_data1.to_ulong());
                // const unsigned operand2 =
                //     state.EX.is_I_type
                //         ? state.EX.Imm.to_ulong()
                //         : (mem_ex_forward_rt
                //                ? state.WB.Wrt_data.to_ulong()
                //                : (ex_ex_forward_rt
                //                       ? state.MEM.ALUresult.to_ulong()
                //                       : state.EX.Read_data2.to_ulong()));

                if (ex_ex_forward_rs) {
                    dout << "EX-EX RS" << endl;
                    // we might have some forwarding, but first we need to make
                    // sure previous instruction was add or sub; recall result
                    // doesn't become available for lw until after the MEM stage
                    // exexutes
                    if (state.MEM.wrt_enable &&
                        !state.MEM.rd_mem) {  // add and sub
                        operand1 = state.MEM.ALUresult.to_ulong();
                    }

                    else if (state.MEM.rd_mem) {
                        dout << "This was a lw --stalling-- at cycle: " << cycle
                             << endl;
                        // FIXME:
                        // we have a lw
                        // we need to stall one cycle
                        bubble = 1;
                        freeze_if = 1;
                        freeze_id = 1;
                        // state.IF.nop = 1;
                        // state.ID.nop = 1;
                        // state.EX.nop = 1;
                        // bubble is handled later in the code
                    }
                } else if (mem_ex_forward_rs) {  // else if here because EX-EX
                                                 // takes priority over MEM-EX
                    dout << "MEM-EX RS" << endl;
                    if (state.WB.wrt_enable) {
                        operand1 = state.WB.Wrt_data.to_ulong();
                    }
                } else {  // no dependencies detected for operand 1
                    operand1 = state.EX.Read_data1.to_ulong();
                }

                if (state.EX.is_I_type) {
                    operand2 = state.EX.Imm.to_ulong();
                } else if (ex_ex_forward_rt) {
                    dout << "EX-EX Rt" << endl;
                    if (state.MEM.wrt_enable && !state.MEM.rd_mem) {
                        operand2 = state.MEM.ALUresult.to_ulong();
                    } else if (state.MEM.rd_mem) {
                        // we need to stall one cycle
                        dout << "This was a lw --stalling-- at cycle: " << cycle
                             << endl;
                        // FIXME:
                        bubble = 1;
                        freeze_if = 1;
                        freeze_id = 1;
                        // state.IF.nop = 1;
                        // state.ID.nop = 1;
                        // state.EX.nop = 1;
                        // bubble is handled later in the code
                    }
                } else if (mem_ex_forward_rt) {  // else if here because EX-EX
                                                 // takes priority over MEM-EX
                    dout << "MEM-EX Rt" << endl;
                    if (state.WB.wrt_enable) {
                        operand2 = state.WB.Wrt_data.to_ulong();
                    }
                } else {  // no dependencies detected for operand 2
                    operand2 = state.EX.Read_data2.to_ulong();
                }

                if (state.EX.alu_op) {
                    newState.MEM.ALUresult = operand1 + operand2;
                } else {
                    newState.MEM.ALUresult = operand1 - operand2;
                }

                // if (state.EX.wrt_mem) { DON'T CARE
                newState.MEM.Store_data = state.EX.Read_data2;
                newState.MEM.Rs = state.EX.Rs;
                newState.MEM.Rt = state.EX.Rt;
                newState.MEM.Wrt_reg_addr = state.EX.Wrt_reg_addr;
                newState.MEM.wrt_enable = state.EX.wrt_enable;
                newState.MEM.rd_mem = state.EX.rd_mem;
                newState.MEM.wrt_mem = state.EX.wrt_mem;
            }

            if (bubble) {
                newState.MEM.nop = 1;
                newState.EX.Read_data1 = operand1;
                newState.EX.Read_data2 = operand2;
            } else {
                newState.MEM.nop = state.EX.nop;
            }
        }

        /* --------------------- ID stage --------------------- */
        bool branch_pc_flag = 0;
        {
            dout << "----------------\nID\n";
            const unsigned instruction = state.ID.Instr.to_ulong();
            const unsigned opcode = instruction >> 26;
            const unsigned rs = (instruction >> 21) & 0x1F;
            const unsigned rt = (instruction >> 16) & 0x1F;
            const unsigned rd = (instruction >> 11) & 0x1F;
            const unsigned funct = instruction & 0x3F;
            const unsigned imm = instruction & 0xFFFF;
            const uint32_t sign_extended_imm = (imm ^ 0x8000) - 0x8000;
            const bool is_bubble = state.ID.Instr.none();
            const bool is_r_type = opcode == 0x00;
            // The only J-type here is HALT
            const bool is_j_type = opcode == 0x3F;
            const bool is_i_type = !(is_r_type || is_j_type);
            const bool is_load = opcode == 0x23;
            const bool is_store = opcode == 0x2B;
            const bool is_branch = opcode == 0x05;

            if (!state.ID.nop && !freeze_id) {
                {
                    const unsigned shamt = (instruction >> 6) & 0x1F;
                    const unsigned jmp_addr = instruction & 0x3FFFFFF;

                    if (is_r_type) {
                        dout << debug::bg::white << debug::black << "R-type"
                             << debug::reset << uppercase << " opcode=0x" << hex
                             << opcode << " rs=" << dec << rs << " rt=" << rt
                             << " rd=" << rd << " shamt=" << shamt
                             << " funct=0x" << hex << funct << endl;
                    }
                    if (is_i_type) {
                        dout << debug::bg::white << debug::black << "I-type"
                             << debug::reset << uppercase << " opcode=0x" << hex
                             << opcode << " rs=" << dec << rs << " rt=" << rt
                             << " imm=" << imm << endl;
                    }
                    if (is_j_type) {
                        dout << debug::bg::white << debug::black << "I-type"
                             << debug::reset << uppercase << " opcode=0x"
                             << opcode << " addr=" << jmp_addr << endl;
                    }
                    std::cout.copyfmt(oldCoutState);
                }

                if (is_r_type || is_i_type) {
                    newState.EX.Rs = rs;
                    newState.EX.Rt = rt;
                }
                newState.EX.Read_data1 = myRF.readRF(newState.EX.Rs);
                newState.EX.Read_data2 = myRF.readRF(newState.EX.Rt);
                newState.EX.wrt_enable = !is_bubble && (is_r_type || is_load);
                // if (newState.EX.wrt_enable) { DON'T CARE
                newState.EX.Wrt_reg_addr = is_r_type ? rd : rt;
                newState.EX.alu_op =
                    !(is_r_type && funct == 0x23);  // statement: (not 'subu')
                newState.EX.is_I_type = is_i_type;
                newState.EX.rd_mem = is_load;
                newState.EX.wrt_mem = is_store;
                newState.EX.Imm = sign_extended_imm;

                if (is_branch) {  // BNE
                    const unsigned relative_addr = sign_extended_imm << 2;
                    dout << "BNE relative_addr: " << relative_addr << endl;
                    const bool branch_taken =
                        newState.EX.Read_data1 != newState.EX.Read_data2;

                    if (branch_taken) {
                        dout << debug::bg::cyan << "branch " << debug::red
                             << "taken" << debug::reset
                             << " PC: " << state.IF.PC.to_ulong();

                        state.IF.PC = state.IF.PC.to_ulong() + relative_addr;
                        newState.ID.nop = 1;
                        branch_pc_flag = 1;

                        dout << " -> " << state.IF.PC.to_ulong() << endl;
                    } else {
                        dout << debug::bg::cyan << "branch " << debug::red
                             << "NOT taken" << debug::reset << endl;
                    }
                }
            }

            // newState.EX.nop = state.ID.nop || freeze_id;
            newState.EX.nop = state.ID.nop;
        }

        /* --------------------- IF stage --------------------- */
        {
            dout << "----------------\nIF\n";
            dout << " PC: " << state.IF.PC.to_ulong() << endl;

            if (!state.IF.nop && !freeze_if) {
                newState.ID.Instr =
                    myInsMem.readInstr(state.IF.PC);  // read from imem

                if (newState.ID.Instr == 0xFFFFFFFF) {  // check for halt
                    dout << debug::bg::red << "             " << debug::reset
                         << endl
                         << debug::bg::red << "    HALT     " << debug::reset
                         << endl
                         << debug::bg::red << "             " << debug::reset
                         << endl;

                    newState.IF.nop = 1;
                    newState.ID.nop = 1;
                    freeze_if = 1;  // newState.ID.nop = 1; will get overwritten
                    state.IF.nop = 1;  // HACK: modify the value that's going to
                                       // overwrite newState.ID.nop
                } else if (branch_pc_flag) {
                    newState.IF.PC = state.IF.PC.to_ulong();
                } else {
                    newState.IF.PC = state.IF.PC.to_ulong() + 4;  // PC = PC + 4
                }
            }
            dout << endl
                 << "(old) " << state.IF.PC.to_ulong() << " -> "
                 << newState.IF.PC.to_ulong() << " (new)" << endl;

            // newState.ID.nop = state.IF.nop || freeze_if;
            newState.ID.nop = state.IF.nop;
        }

        //////////////////////////////////////////////////////////
        {
            dout << "----------------\n";
            dout << "NOP old: ";
            dout << ((state.IF.nop || freeze_if) ? debug::red : debug::reset)
                 << "IF" << debug::reset << " ";
            dout << ((state.ID.nop || freeze_id) ? debug::red : debug::reset)
                 << "ID" << debug::reset << " ";
            dout << (state.EX.nop ? debug::red : debug::reset) << "EX"
                 << debug::reset << " ";
            dout << (state.MEM.nop ? debug::red : debug::reset) << "MEM"
                 << debug::reset << " ";
            dout << (state.WB.nop ? debug::red : debug::reset) << "WB"
                 << debug::reset << endl;

            dout << "    new: ";
            dout << (newState.IF.nop ? debug::red : debug::reset) << "IF"
                 << debug::reset << " ";
            dout << (newState.ID.nop ? debug::red : debug::reset) << "ID"
                 << debug::reset << " ";
            dout << (newState.EX.nop ? debug::red : debug::reset) << "EX"
                 << debug::reset << " ";
            dout << (newState.MEM.nop ? debug::red : debug::reset) << "MEM"
                 << debug::reset << " ";
            dout << (newState.WB.nop ? debug::red : debug::reset) << "WB"
                 << debug::reset << endl;
        }

        // Full stop
        if (state.IF.nop && state.ID.nop && state.EX.nop && state.MEM.nop &&
            state.WB.nop)
            break;

        printState(newState, cycle);  // print states after executing cycle 0,
                                      // cycle 1, cycle 2 ...

        state = newState;
        /* The end of the cycle
         * updates the current state with the values calculated in this cycle.
         */
        ++cycle;
    }

    myRF.outputRF();            // dump RF;
    myDataMem.outputDataMem();  // dump data mem

    return 0;
}
