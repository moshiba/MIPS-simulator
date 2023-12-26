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

template <typename T>
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
    InstructionStatus()
        : cycleIssued(-1), cycleCompleted(-1), cycleWriteResult(-1) {}

    bool operator==(const InstructionStatus& other) const {
        return cycleIssued == other.cycleIssued &&
               cycleCompleted == other.cycleCompleted &&
               cycleWriteResult == other.cycleWriteResult;
    }

    int cycleIssued;
    int cycleCompleted;
    int cycleWriteResult;
};

enum class Instruction_t { Add, Sub, Load, Store, Mult, Div };

enum class FunctionalUnit_t { Load, Store, AddSub, MultDiv };
string FU_to_string(optional<FunctionalUnit_t> fu) {
    if (fu.has_value()) {
        FunctionalUnit_t fu_val = fu.value();
        switch (fu_val) {
            case FunctionalUnit_t::Load:
                return "Load";
            case FunctionalUnit_t::Store:
                return "Store";
            case FunctionalUnit_t::AddSub:
                return "Add";
            case FunctionalUnit_t::MultDiv:
                return "Mult";
            default:
                throw runtime_error("invalid functional unit type");
        }
    } else {
        return "";
    }
}

class Instruction {
   public:
    Instruction(string line_str, unsigned index_) : index(index_) {
        auto line = istringstream(line_str);
        string buf_instr, buf_dest, buf_src1, buf_src2;
        line >> buf_instr >> buf_dest >> buf_src1 >> buf_src2;
        dest = stoi(buf_dest.substr(1));

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

    bool operator==(const Instruction& other) const {
        return index == other.index && type == other.type &&
               fu_cat == other.fu_cat && latency == other.latency &&
               dest == other.dest && src1 == other.src1 && src2 == other.src2 &&
               status == other.status;
    }

    unsigned index;
    Instruction_t type;
    FunctionalUnit_t fu_cat;
    unsigned latency;
    unsigned dest;
    unsigned src1;
    unsigned src2;
    InstructionStatus status;
};

struct ReservationStationId {
    ReservationStationId(FunctionalUnit_t rs_type_, unsigned index_)
        : rs_type(rs_type_), index(index_) {}

    bool operator==(const ReservationStationId& other) const {
        return rs_type == other.rs_type && index == other.index;
    }

    friend ostream& operator<<(ostream& os, const ReservationStationId& rs_id) {
        os << FU_to_string(rs_id.rs_type) + to_string(rs_id.index);
        return os;
    }

    FunctionalUnit_t rs_type;
    unsigned index;
};

class ReservationStation {
   public:
    ReservationStation(ReservationStationId rs_idx) : rs_id(rs_idx) {
        this->clear();
    }

    bool ready() const {
        return (!(Qj) && !(Qk)) && (Vj.has_value() && Vk.has_value());
    }

    bool completed() const {
        return remain_cycle.has_value() && remain_cycle.value() <= 0;
        //return !remain_cycle.has_value() && remain_cycle.value() <= 0
    }

    void clear() {
        busy = false;
        Op.reset();
        Vj.reset();
        Vk.reset();
        Qj.reset();
        Qk.reset();
        remain_cycle.reset();
    }

    bool free() const { return !busy; }

    bool operator==(const ReservationStation& other) const {
        return rs_id == other.rs_id && busy == other.busy && Op == other.Op &&
               Vj == other.Vj && Vk == other.Vk && Qj == other.Qj &&
               Qk == other.Qk && remain_cycle == other.remain_cycle;
    }

    ReservationStationId rs_id;
    bool busy;
    optional<Instruction> Op;           // Operation
    optional<unsigned> Vj;              // Value j
    optional<unsigned> Vk;              // Value k
    optional<ReservationStationId> Qj;  // Dependee source j
    optional<ReservationStationId> Qk;  // Dependee source k
    optional<int> remain_cycle;
};

class ReservationStations {
   public:
    ReservationStations(HardwareConfig cfg) {
        // Initialize each type of reservation stations
        for (unsigned i = 0; i < cfg.LoadRSsize; i++) {
            stations[FunctionalUnit_t::Load].emplace_back(
                ReservationStationId{FunctionalUnit_t::Load, i});
        }
        for (unsigned i = 0; i < cfg.StoreRSsize; i++) {
            stations[FunctionalUnit_t::Store].emplace_back(
                ReservationStationId{FunctionalUnit_t::Store, i});
        }
        for (unsigned i = 0; i < cfg.AddRSsize; i++) {
            stations[FunctionalUnit_t::AddSub].emplace_back(
                ReservationStationId{FunctionalUnit_t::AddSub, i});
        }
        for (unsigned i = 0; i < cfg.MultRSsize; i++) {
            stations[FunctionalUnit_t::MultDiv].emplace_back(
                ReservationStationId{FunctionalUnit_t::MultDiv, i});
        }
    }

    // auto& operator[](FunctionalUnit_t&& key) { return stations[key]; }

    // auto& operator[](ReservationStationId&& key) { return
    // stations[key.rs_type][key.index]; }

    map<FunctionalUnit_t, vector<ReservationStation>> stations;
};

struct RegisterResultStatus {
    RegisterResultStatus() : data_ready(false) {}
    RegisterResultStatus(ReservationStationId rs_id_, bool data_ready_)
        : rs_id(rs_id_), data_ready(data_ready_) {}

    optional<ReservationStationId> rs_id;
    bool data_ready;
};

class RegisterResultStatuses {
   public:
    RegisterResultStatuses(HardwareConfig cfg) : registers(cfg.FRegSize) {}

    /*
    Print all register result status. It is called by
    PrintRegisterResultStatus4Grade() for grading. If you don't want to write
    such a class, then make sure you call the same function and print the
    register result status in the same format.
    */
    string _printRegisterResultStatus() const {
        std::ostringstream result;
        for (int idx = 0; idx < registers.size(); idx++) {
            result << "F" + std::to_string(idx) << ": ";

            if (registers[idx].rs_id.has_value()) {
                result << registers[idx].rs_id.value();
            }
            result << ", ";

            result << "dataRdy: " << (registers[idx].data_ready ? "Y" : "N")
                   << ", ";
            result << "\n";
        }
        return result.str();
    }

    vector<RegisterResultStatus> registers;
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
    if (thiscycle % 5 != 0) return; //Uncomment this before submission
    std::ofstream outfile(
        filename, std::ios_base::app);  // append result to the end of file
    outfile << "Cycle " << thiscycle << ":\n";
    outfile << registerResultStatus._printRegisterResultStatus() << "\n";
    outfile.close();
}

/*
print the instruction status, the reservation stations and the register result
status
@param filename: output file name
@param instructionStatus: instruction status
*/
void PrintResult4Grade(const string& filename,
                       const vector<InstructionStatus>& instructionStatus) {
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
        unsigned index = 0;
        ifstream trace_file(trace_file_name);
        for (string trace_line; getline(trace_file, trace_line); index++) {
            instr_trace.emplace_back(trace_line, index);
        }
    }

    auto&& instruction() { return instr_trace.at(instr_idx); }
    void advance_to_next_instruction() { instr_idx += 1; }

    vector<Instruction> instr_trace;
    unsigned instr_idx = 0;
};

int main(int argc, char** argv) {
    if (argc > 3) {
        hardwareconfigname = argv[1];
        inputtracename = argv[2];
    }

    HardwareConfig hardwareConfig;
    {
        std::ifstream config(hardwareconfigname);
        config >> hardwareConfig.LoadRSsize;
        config >> hardwareConfig.StoreRSsize;
        config >> hardwareConfig.AddRSsize;
        config >> hardwareConfig.MultRSsize;
        config >> hardwareConfig.FRegSize;
        config.close();
    }
    dout << hardwareConfig;

    ReservationStations rss = {hardwareConfig};
    RegisterResultStatuses reg_stats = {hardwareConfig};
    optional<ReservationStationId> cdb;
    auto instr_trace = InstructionTrace(inputtracename);

    unsigned cycle = 1;
    while (1) {
        // All RS advance 1 cycle
        dout << "\n\n========\ncycle " << cycle << "\n========" << endl;
        cout << "\nCycle: " << cycle << endl;
        {
            for (auto&& [fu, rs_vec] : rss.stations) {  // for every RS type
                for (auto&& rs : rs_vec) {  // for every RS of that type
                    if (rs.ready()) {
                        dout << debug::bg::magenta << rs.rs_id << " is running"
                             << debug::reset << endl;
                        *rs.remain_cycle -= 1;
                    }
                }
            }

            // Broadcast what's in CDB to all registers and all RS
            {
                if (cdb.has_value()) {
                    const ReservationStationId& cdb_val = cdb.value();
                    dout << debug::bg::blue << "CDB broadcasting (" << cdb_val
                         << ") to:" << debug::reset << endl;
                    cout << "CDB broadcasting (" << cdb_val << ") to: ";
                    for (auto&& [fu, rs_vec] :
                         rss.stations) {            // for every RS type
                        for (auto&& rs : rs_vec) {  // for every RS of that type
                            // if CDB has something Qj is waiting for
                            if (rs.Qj == cdb_val) {
                                dout << "  " << rs.rs_id << " Qj" << endl;
                                cout << rs.rs_id << " Qj\t";
                                rs.Qj.reset();
                                rs.Vj = 1;
                            }
                            // if CDB has something Qk is waiting for
                            if (rs.Qk == cdb_val) {
                                dout << "  " << rs.rs_id << " Qk" << endl;
                                cout << rs.rs_id << " Qk\t";
                                rs.Qk.reset();
                                rs.Vk = 1;
                            }
                        }
                    }
                    for (auto&& reg : reg_stats.registers) {
                        // if CDB has something that any register is waiting for
                        if (reg.rs_id == cdb_val) {
                            dout << "  " << reg.rs_id.value() << " Reg" << endl;
                            cout << reg.rs_id.value() << " Reg\t" << endl;
                            reg.data_ready = true;
                        }
                    }
                    cdb.reset();
                }
            }
        }

        dout << "--------" << endl;

        // See if we can issue the next instruction
        {
            if (instr_trace.instr_idx < instr_trace.instr_trace.size()) {
                Instruction& this_instr = instr_trace.instruction();

                // get the RS of the type of the instruction
                const auto instr_fu_type = this_instr.fu_cat;
                auto& rs_vec = rss.stations[instr_fu_type];
                if (auto free_rs =
                        find_if(begin(rs_vec), end(rs_vec),
                                [](ReservationStation o) { return o.free(); });
                    free_rs != end(rs_vec)) {
                    // Found free RS for this FU type, issue instruction!
                    dout << "Issuing instruction[" << this_instr.index << "]"
                         << endl;
                    this_instr.status.cycleIssued = cycle;
                    const unsigned this_op1 = this_instr.src1;
                    const unsigned this_op2 = this_instr.src2;
                    const unsigned this_dst = this_instr.dest;

                    free_rs->busy = true;
                    free_rs->Op = this_instr;
                    // Logic for few functional units: Now that we have an entry in the
                    // CDB, we need to make sure the register status doesn't have multiple 
                    // reservation station entries of the same type
                    cout << "Id: " << free_rs->rs_id << endl;
                    for (size_t i=0; i<reg_stats.registers.size(); ++i) {
                        // if the register status already has an entry for that free_rs
                        if ((reg_stats.registers[i].rs_id == free_rs->rs_id) && 
                        (i != free_rs->Op->dest)) {
                            cout << "Reg F" << i << " is being reset" << endl;
                            reg_stats.registers[i].rs_id.reset();
                        }
                    }

                    switch (instr_fu_type) {
                        case FunctionalUnit_t::Store: {
                            if (reg_stats.registers[this_dst].data_ready) {
                                free_rs->Vj = this_op1 + this_op2;
                                free_rs->Vk = this_op1 + this_op2;
                            } else {
                                free_rs->Qj =
                                    reg_stats.registers[this_dst].rs_id;
                                free_rs->Qk =
                                    reg_stats.registers[this_dst].rs_id;
                            }
                            break;
                        }
                        case FunctionalUnit_t::Load: {
                            free_rs->Vj = this_op1 + this_op2;
                            free_rs->Vk = this_op1 + this_op2;
                            reg_stats.registers[this_dst] = {free_rs->rs_id,
                                                             false};
                            break;
                        }
                        default: {
                            free_rs->Qj = reg_stats.registers[this_op1].rs_id;
                            if (reg_stats.registers[this_op1].data_ready) {
                                free_rs->Vj = this_op1;
                            }
                            free_rs->Qk = reg_stats.registers[this_op2].rs_id;
                            if (reg_stats.registers[this_op2].data_ready) {
                                free_rs->Vk = this_op2;
                            }
                            reg_stats.registers[this_dst] = {free_rs->rs_id,
                                                             false};
                            break;
                        }
                    }
                    free_rs->remain_cycle = this_instr.latency;
                    // Logic for few functional units: Now that we have an entry in the
                    // CDB, we need to make sure the register status doesn't have multiple 
                    // reservation station entries of the same type
                    // for (size_t i=0; i<reg_stats.registers.size(); ++i) {
                    //     // if the register status already has an entry for that free_rs
                    //     if ((reg_stats.registers[i].rs_id == free_rs->rs_id) && 
                    //     (i != free_rs->Op->dest)) {
                    //         cout << "Reg F" << i << " is being reset" << endl;
                    //         reg_stats.registers[i].rs_id.reset();
                    //     }
                    // }

                    // reg_stats.registers[this_dst] = {free_rs->rs_id, false};

                    instr_trace.advance_to_next_instruction();
                } else {
                    // No free RS for this FU type
                    // Do nothing
                }
            } else {
                dout << "no more instructions to issue" << endl;
            }
        }

        // Run each RS if their operands are ready
        {
            // Check if any RS is completed
            for (auto&& [fu, rs_vec] : rss.stations) {  // for every RS type
                for (auto&& rs : rs_vec) {  // for every RS of that type
                    if (rs.completed()) {
                        if(instr_trace.instr_trace[rs.Op->index].status.cycleCompleted == -1){
                            // Only write in completion cycle if it hasn't been written before
                            // Otherwise you will overwrite the previous completion cycle with 
                            // the current cycle
                            instr_trace.instr_trace[rs.Op->index]
                            .status.cycleCompleted = cycle;
                            dout << debug::bg::green << rs.rs_id << " completed"
                             << debug::reset << endl;
                            cout << rs.rs_id << " completed" << endl;
                        }
                    }
                }
            }

            // Retire the oldest instruction first: bus arbitration
            {
                vector<ReservationStation> completed_rs;
                for (auto&& [fu, rs_vec] : rss.stations) {  // for every RS type
                    copy_if(
                        rs_vec.begin(), rs_vec.end(),
                        back_inserter(completed_rs),
                        [](ReservationStation& rs) { return rs.completed(); });
                }
                if (completed_rs.empty()) {
                    dout << debug::bg::red
                         << "nothing is completed during this round"
                         << debug::reset << endl;
                } else {
                    // Find the oldest completed instruction
                    const auto& min_completed_rs = *min_element(
                        completed_rs.begin(), completed_rs.end(),
                        [](ReservationStation& rs_A, ReservationStation& rs_B) {
                            return rs_A.Op->index < rs_B.Op->index;
                        });

                    // Broadcast this RS to CDB
                    // - Retrace ReservationStationId from instruction
                    const auto& rs_idx = min_completed_rs.rs_id;
                    dout << debug::bg::cyan << rs_idx << " commit (broadcast)"
                         << debug::reset << endl;
                    cdb = rs_idx;

                    // Retire this instruction
                    // - Mark broadcast timestamp
                    instr_trace.instr_trace[min_completed_rs.Op->index]
                        .status.cycleWriteResult = cycle + 1;
                    // - Clear the original RS in RSS
                    find(begin(rss.stations[rs_idx.rs_type]),
                         end(rss.stations[rs_idx.rs_type]), min_completed_rs)
                        ->clear();
                
                    // Logic for few functional units: Now that we have an entry in the
                    // CDB, we need to make sure the register status doesn't have multiple 
                    // reservation station entries of the same type
                    // cout << "Dest: " << min_completed_rs.Op->dest << endl;
                    // for (size_t i=0; i<reg_stats.registers.size(); ++i) {
                    //     // if the register status already has an entry for that cdb
                    //     if ((reg_stats.registers[i].rs_id == min_completed_rs.rs_id) && 
                    //     (i != min_completed_rs.Op->dest)) {
                    //         cout << "Reg F" << i << " is being reset" << endl;
                    //         reg_stats.registers[i].rs_id.reset();
                    //     }
                    // }
                }
            }
        }

        dout << "--------" << endl;

        //Check the register result status to see if any values are available
        for(auto&& [fu, rs_vec] : rss.stations){
            for(auto&& rs : rs_vec){
                //cout << "Looping through reservation station: " << rs.rs_id << endl;
                for(size_t i=0; i<reg_stats.registers.size(); ++i){
                    // FunctionalUnit_t fu = FunctionalUnit_t::AddSub;
                    // ReservationStationId my_rs_id(fu, 0);
                    // if(rs.rs_id == my_rs_id){
                    //     if(reg_stats.registers[i].rs_id.has_value()){
                    //         cout << "For Add0, Reg "  << "F" << i << 
                    //         " has value: " << reg_stats.registers[i].rs_id.value();
                    //         cout << "  Is it ready? " << reg_stats.registers[i].data_ready << endl;
                    //     }
                    //     else{
                    //         cout << "For Add0, Reg " << "F" << i << " has no value" << endl;
                    //     }
                    // }
                    if((rs.Qj == reg_stats.registers[i].rs_id) && reg_stats.registers[i].data_ready){
                        if(rs.Qj.has_value()){
                            cout << "Found Qj " << rs.Qj.value() << " in F" << i << 
                            " for " << rs.rs_id << endl;
                        }
                        // cout << "Found Qj " << rs.Qj.value() << " in F" << i << 
                        // " for " << rs.rs_id << endl;
                        rs.Qj.reset();
                        rs.Vj = 1;
                    }
                    if((rs.Qk == reg_stats.registers[i].rs_id) && reg_stats.registers[i].data_ready){
                        if(rs.Qk.has_value()){
                            cout << "Found Qk " << rs.Qk.value() << " in F" << i << 
                            " for " << rs.rs_id << endl;
                        }
                        // cout << "Found Qk " << rs.Qk.value() << " in F" << i << 
                        // " for " << rs.rs_id << endl;
                        rs.Qk.reset();
                        rs.Vk = 1;
                    }
                }
            }
        }

        FunctionalUnit_t fu = FunctionalUnit_t::AddSub;
        if(rss.stations.at(fu)[0].remain_cycle.has_value()){
            cout << "RS Add0: cycle val: " << rss.stations.at(fu)[0].remain_cycle.value() << endl;
        }
        else{
            cout << "RS Add0: not busy" << endl;
        }
        if(rss.stations.at(fu)[0].Vj.has_value()){
            cout << "RS Add0: Vj has value " << rss.stations.at(fu)[0].Vj.value() << endl;
        }
        else{
            cout << "RS Add0: Vj has no value" << endl;
        }
        if(rss.stations.at(fu)[0].Vk.has_value()){
            cout << "RS Add0: Vk has value " << rss.stations.at(fu)[0].Vk.value() << endl;
        }
        else{
            cout << "RS Add0: Vk has no value" << endl;
        }
        if(rss.stations.at(fu)[0].Qj.has_value()){
            cout << "RS Add0: Qj has value " << rss.stations.at(fu)[0].Qj.value() << endl;
        }
        else{
            cout << "RS Add0: Qj has no value" << endl;
        }
        if(rss.stations.at(fu)[0].Qk.has_value()){
            cout << "RS Add0: Qk has value " << rss.stations.at(fu)[0].Qk.value() << endl;
        }
        else{
            cout << "RS Add0: Qk has no value" << endl;
        }
        cout << "RS Add0: ready: " << rss.stations.at(fu)[0].ready() << endl;


        // Print everything
        {
            dout << "@@@@@@@@" << endl;
            // Debug RSs
            {
                dout << debug::black << debug::bg::white
                     << "RSs\tName\tBusy\tOp\tVj\tVk\tQj\tQk\tRemainCycles"
                     << debug::reset << endl;
                for (auto&& [fu, rs_vec] : rss.stations) {  // for every RS type
                    for (auto&& rs : rs_vec) {  // for every RS of that type
                        dout
                            << "\t" << rs.rs_id << "\t"
                            << (rs.busy ? "Yes" : "No") << "\t"
                            << (!rs.Op ? "-" : FU_to_string(rs.Op->fu_cat))
                            << "\t" << (!rs.Vj ? "-" : to_string(rs.Vj.value()))
                            << "\t" << (!rs.Vk ? "-" : to_string(rs.Vk.value()))
                            << "\t"
                            << (!rs.Qj ? "-"
                                       : FU_to_string(rs.Qj->rs_type) +
                                             to_string(rs.Qj->index)

                                    )
                            << "\t"
                            << (!rs.Qk ? "-"
                                       : FU_to_string(rs.Qk->rs_type) +
                                             to_string(rs.Qk->index)

                                    )
                            << "\t"
                            << (!rs.remain_cycle
                                    ? "-"
                                    : to_string(rs.remain_cycle.value()))
                            << endl;
                    }
                }
            }

            // Debug RegStats
            {
                dout << debug::black << debug::bg::white << "Reg";
                for (int i = 0; i != hardwareConfig.FRegSize; ++i) {
                    dout << "\tF" << i;
                }
                dout << debug::reset << endl << "FU";
                for (int i = 0; i != hardwareConfig.FRegSize; ++i) {
                    auto&& reg = reg_stats.registers[i];
                    dout << "\t";
                    if (reg.rs_id.has_value()) {
                        dout << reg.rs_id.value();
                    }
                }
                dout << debug::reset << endl << "Ready";
                for (int i = 0; i != hardwareConfig.FRegSize; ++i) {
                    auto&& reg = reg_stats.registers[i];
                    dout << "\t" << (reg.data_ready ? "1" : "-");
                }
                dout << debug::reset << endl;
            }

            // Debug Instructions
            {
                dout << debug::black << debug::bg::white
                     << "Instr\tIssue\tComp\tWrite" << debug::reset << endl;
                for (auto&& instr : instr_trace.instr_trace) {
                    dout << "\t"
                         << (instr.status.cycleIssued == -1
                                 ? "-"
                                 : to_string(instr.status.cycleIssued))
                         << "\t"
                         << (instr.status.cycleCompleted == -1
                                 ? "-"
                                 : to_string(instr.status.cycleCompleted))
                         << "\t"
                         << (instr.status.cycleWriteResult == -1
                                 ? "-"
                                 : to_string(instr.status.cycleWriteResult))
                         << endl;
                }
            }
            dout << "@@@@@@@@" << endl;
        }

        // At the end of this cycle, we need this function to print all
        // registers status for grading
        PrintRegisterResultStatus4Grade(outputtracename, reg_stats, cycle);

        {
            // Stop when all instructions are committed
            const auto& instr_vec = instr_trace.instr_trace;

            if (instr_vec.size() ==
                count_if(cbegin(instr_vec), cend(instr_vec),
                         [](Instruction instr) {
                             return instr.status.cycleWriteResult != -1;
                         })) {
                dout << "All instructions are committed" << endl;
                break;
            }
        }

        cycle += 1;
    }

    {  // At the end of the program, print Instruction Status Table for
       // grading
        vector<InstructionStatus> instruction_status_vec;
        generate_n(back_inserter(instruction_status_vec),
                   size(instr_trace.instr_trace), [&instr_trace]() {
                       static auto iter = begin(instr_trace.instr_trace);
                       return iter++->status;
                   });
        PrintResult4Grade(outputtracename, instruction_status_vec);
    }

    return 0;
}
