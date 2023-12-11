#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <ostream>
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

constexpr long bitmask(unsigned n) { return (1UL << n) - 1; }

string inputtracename = "trace.txt";
string outputtracename = "trace.out.txt";
string hardwareconfigname = "config.txt";

struct HardwareConfig {
    int LoadRSsize;   // number of load reservation stations
    int StoreRSsize;  // number of store reservation stations
    int AddRSsize;    // number of add reservation stations
    int MultRSsize;   // number of multiply reservation stations
    int FRegSize;     // number of fp registers

    friend ostream& operator<<(ostream& os, const HardwareConfig& cfg) {
        os << "@@@@@@@@@@\n  Config\n@@@@@@@@@@\n";
        os << "Load : " << cfg.LoadRSsize << " RS\n";
        os << "Store: " << cfg.StoreRSsize << " RS\n";
        os << "Add  : " << cfg.AddRSsize << " RS\n";
        os << "Mult : " << cfg.MultRSsize << " RS\n";
        os << "Float: " << cfg.FRegSize << " Reg\n";
        os << "@@@@@@@@@@" << endl;
        return os;
    }
};

struct InstructionStatus {
    int cycleIssued;
    int cycleCompleted;
    int cycleWriteResult;
};

enum class Instruction_t { Add, Sub, Load, Store, Mult, Div };

enum class FunctionalUnit_t { Load, Store, AddSub, MultDiv };

class Instruction {
   public:
    Instruction(string line_str) : line(line_str) {
        auto line = istringstream(line_str);
        string buf_instr, buf_dest, buf_src1, buf_src2;
        line >> buf_instr >> buf_dest >> buf_src1 >> buf_src2;
        dest = stoi(buf_dest.substr(1));
        status = {-1, -1, -1};

        switch (tolower(buf_instr[2])) {
            case 'd': {
                type = Instruction_t::Add;
                fu_cat = FunctionalUnit_t::AddSub;
                latency = 2;
                src1 = stoi(buf_src1.substr(1));
                src2 = stoi(buf_src2.substr(1));
                break;
            }
            case 'b': {
                type = Instruction_t::Sub;
                fu_cat = FunctionalUnit_t::AddSub;
                latency = 2;
                src1 = stoi(buf_src1.substr(1));
                src2 = stoi(buf_src2.substr(1));
                break;
            }
            case 'a': {
                type = Instruction_t::Load;
                fu_cat = FunctionalUnit_t::Load;
                latency = 2;
                src1 = stoi(buf_src1);
                src2 = stoi(buf_src2);
                break;
            }
            case 'o': {
                type = Instruction_t::Store;
                fu_cat = FunctionalUnit_t::Store;
                latency = 2;
                src1 = stoi(buf_src1);
                src2 = stoi(buf_src2);
                break;
            }
            case 'l': {
                type = Instruction_t::Mult;
                fu_cat = FunctionalUnit_t::MultDiv;
                latency = 10;
                src1 = stoi(buf_src1.substr(1));
                src2 = stoi(buf_src2.substr(1));
                break;
            }
            case 'v': {
                type = Instruction_t::Div;
                fu_cat = FunctionalUnit_t::MultDiv;
                latency = 40;
                src1 = stoi(buf_src1.substr(1));
                src2 = stoi(buf_src2.substr(1));
                break;
            }
            default: {
                cerr << "instruction: " << line_str << endl;
                throw runtime_error("invalid instruction");
            }
        }
    }

    string line;
    Instruction_t type;
    FunctionalUnit_t fu_cat;
    unsigned latency;
    unsigned dest;
    unsigned src1;
    unsigned src2;
    InstructionStatus status;
};

struct RegisterResultStatus {
    string ReservationStationName;
    bool dataReady;
};

class RegisterResultStatuses {
   public:
    // ...

    /*********************************** ↑↑↑ Todo: Implement by you ↑↑↑
     * ******************************************/
    /*
    Print all register result status. It is called by
    PrintRegisterResultStatus4Grade() for grading. If you don't want to write
    such a class, then make sure you call the same function and print the
    register result status in the same format.
    */
    string _printRegisterResultStatus() const {
        std::ostringstream result;
        for (int idx = 0; idx < _registers.size(); idx++) {
            result << "F" + std::to_string(idx) << ": ";
            result << _registers[idx].ReservationStationName << ", ";
            result << "dataRdy: " << (_registers[idx].dataReady ? "Y" : "N")
                   << ", ";
            result << "\n";
        }
        return result.str();
    }
    /*********************************** ↓↓↓ Todo: Implement by you ↓↓↓
     * ******************************************/
   private:
    vector< RegisterResultStatus > _registers;
};

struct ReservationStationId {
    FunctionalUnit_t rs_type;
    unsigned index;
};

class ReservationStation {
   public:
    ReservationStation(ReservationStationId rs_idx)
        : reservation_station_index(rs_idx) {
        this->clear();
    }

    bool ready() {
        return (!(Qj) && !(Qk)) && (Vj.has_value() && Vk.has_value());
    }

    bool complete() { return remain_cycle.value() == 0; }

    void clear() {
        busy = false;
        Op.reset();
        Vj.reset();
        Vk.reset();
        Qj.reset();
        Qk.reset();
        remain_cycle.reset();
    }

    bool free() { return !busy; }

    ReservationStationId reservation_station_index;

   private:
    bool busy;
    optional< Instruction_t > Op;         // Operation
    optional< unsigned > Vj;              // Value j
    optional< unsigned > Vk;              // Value k
    optional< ReservationStationId > Qj;  // Dependee source j
    optional< ReservationStationId > Qk;  // Dependee source k
    optional< int > remain_cycle;
};

class ReservationStations {
   public:
    auto& operator[](FunctionalUnit_t&& key) { return stations[key]; }

    auto& operator[](ReservationStationId&& key) {
        return stations[key.rs_type][key.index];
    }

    map< FunctionalUnit_t, vector< ReservationStation > > stations;
};

struct CommonDataBus {
   public:
    FunctionalUnit_t source;
    unsigned data;
};

/*
print the register result status each 5 cycles
@param filename: output file name
@param registerResultStatus: register result status
@param thiscycle: current cycle
*/
void PrintRegisterResultStatus4Grade(
    const string& filename, const RegisterResultStatuses& registerResultStatus,
    const int thiscycle) {
    if (thiscycle % 5 != 0) return;
    std::ofstream outfile(
        filename, std::ios_base::app);  // append result to the end of file
    outfile << "Cycle " << thiscycle << ":\n";
    outfile << registerResultStatus._printRegisterResultStatus() << "\n";
    outfile.close();
}

// Function to simulate the Tomasulo algorithm
void simulateTomasulo() {
    int thiscycle = 1;  // start cycle: 1
    RegisterResultStatuses registerResultStatus;

    while (thiscycle < 100000000) {
        // Reservation Stations should be updated every cycle, and broadcast to
        // Common Data Bus
        // ...

        // Issue new instruction in each cycle
        // ...

        // At the end of this cycle, we need this function to print all
        // registers status for grading
        PrintRegisterResultStatus4Grade(outputtracename, registerResultStatus,
                                        thiscycle);

        ++thiscycle;

        // The simulator should stop when all instructions are finished.
        // ...
    }
};

/*********************************** ↑↑↑ Todo: Implement by you ↑↑↑
 * ******************************************/

/*
print the instruction status, the reservation stations and the register result
status
@param filename: output file name
@param instructionStatus: instruction status
*/
void PrintResult4Grade(const string& filename,
                       const vector< InstructionStatus >& instructionStatus) {
    std::ofstream outfile(
        filename, std::ios_base::app);  // append result to the end of file
    outfile << "Instruction Status:\n";
    for (int idx = 0; idx < instructionStatus.size(); idx++) {
        outfile << "Instr" << idx << ": ";
        outfile << "Issued: " << instructionStatus[idx].cycleIssued << ", ";
        outfile << "Completed: " << instructionStatus[idx].cycleCompleted
                << ", ";
        outfile << "Write Result: " << instructionStatus[idx].cycleWriteResult
                << ", ";
        outfile << "\n";
    }
    outfile.close();
}

class InstructionTrace {
   public:
    InstructionTrace(string trace_file_name) {
        ifstream trace_file(trace_file_name);
        for (string trace_line; getline(trace_file, trace_line);) {
            instr_trace.emplace_back(trace_line);
        }
    }

    auto instruction() { return instr_trace[instr_idx]; }
    void advance_to_next_instruction() { instr_idx += 1; }

   private:
    vector< Instruction > instr_trace;
    unsigned instr_idx = 0;
};

int main(int argc, char** argv) {
    if (argc > 3) {
        hardwareconfigname = argv[1];
        inputtracename = argv[2];
    }

    HardwareConfig hardwareConfig;
    std::ifstream config;
    config.open(hardwareconfigname);
    config >> hardwareConfig.LoadRSsize;  // number of load reservation stations
    config >>
        hardwareConfig.StoreRSsize;      // number of store reservation stations
    config >> hardwareConfig.AddRSsize;  // number of add reservation stations
    config >>
        hardwareConfig.MultRSsize;  // number of multiply reservation stations
    config >> hardwareConfig.FRegSize;  // number of fp registers
    config.close();

    /*********************************** ↓↓↓ Todo: Implement by you ↓↓↓
     * ******************************************/

    // Read instructions from a file (replace 'instructions.txt' with your file
    // name)
    // ...

    // Initialize the register result status
    // RegisterResultStatuses registerResultStatus();
    // ...

    // Initialize the instruction status table
    vector< InstructionStatus > instructionStatus;
    // ...

    // Simulate Tomasulo:
    // simulateTomasulo(registerResultStatus, instructionStatus, ...);

    /*********************************** ↑↑↑ Todo: Implement by you ↑↑↑
     * ******************************************/

    // At the end of the program, print Instruction Status Table for grading
    PrintResult4Grade(outputtracename, instructionStatus);

    return 0;
}
