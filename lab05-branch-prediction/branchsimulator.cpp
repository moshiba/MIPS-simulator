#include <algorithm>
#include <bitset>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

constexpr long bitmask(unsigned n) { return (1UL << n) - 1; }

struct Config {
    int m;
    int h;
    int w;

    friend ostream& operator<<(ostream& os, const Config& cfg) {
        os << "@@@@@\n cfg\n@@@@@\nm: ";
        os << cfg.m << "\nw: " << cfg.w << "\nh: " << cfg.h;
        os << "\n@@@@@" << endl;
        return os;
    };
};

class BhtIndex {
   public:
    BhtIndex(unsigned addr_, Config cfg)
        : addr((addr_ >> 2) && bitmask(cfg.h)){};

    operator unsigned() { return addr; }

    unsigned addr;
};

class BHT {
   public:
    BHT(int w, int h)
        : n_entries(pow(2, h)),
          entries(n_entries, 0),
          max_entry_val(pow(2, w) - 1) {}

    void update_state(bool branch_outcome, int bht_address) {
        if ((size_t)bht_address > entries.size() - 1) {
            cerr << "Invalid BHT address" << endl;
            return;
        }
        int new_entry = entries[bht_address] << 1;
        if (branch_outcome) {
            new_entry |= 0b1;
        }
        new_entry &= max_entry_val;  // ensure entries don't exceed max val
        entries[bht_address] = new_entry;
    }

    int operator[](int index) {
        if ((index < 0) || (index >= n_entries)) {
            throw out_of_range("BHT [] index out of range");
        }
        return entries[index];
    }

    friend ostream& operator<<(ostream& os, const BHT& bht) {
        for (size_t i = 0; i < bht.entries.size(); ++i) {
            cout << bht.entries[i] << endl;
        }
        return os;
    }

   private:
    int n_entries;
    vector< int > entries;
    int max_entry_val;
};

class PhtIndex {
   public:
    PhtIndex(unsigned pc, unsigned bht_entry, Config cfg)
        : addr((pc >> 2) && bitmask(cfg.m - cfg.w)){};

    operator unsigned() { return addr; }

    unsigned addr;
};

class SaturatingCounter {
   public:
    SaturatingCounter() = default;

    void taken() { state = min(3, state + 1); }

    void not_taken() { state = max(0, state - 1); }

    bool predict() { return state >= 2; }

    int state = 2;
};

class PHT {
   public:
    PHT(int m)
        : n_entries(pow(2, m)), entries(n_entries, SaturatingCounter()) {}

    void update_state(bool branch_outcome, int pht_address) {
        if ((size_t)pht_address > entries.size() - 1) {
            cerr << "Invalid PHT address" << endl;
            return;
        }

        auto& counter = entries[pht_address];
        if (branch_outcome == 1) {
            counter.taken();
        } else {
            counter.not_taken();
        }
    }

    bool make_prediction(int pht_address) {
        if (entries[pht_address].state < 2) {  // not taken
            return 0;
        } else {
            return 1;
        }
    }

    auto operator[](int index) {
        if ((index < 0) || (index >= n_entries)) {
            throw out_of_range("PHT [] index out of range");
        }
        return entries[index];
    }

    friend ostream& operator<<(ostream& os, const PHT& pht) {
        for (size_t i = 0; i < pht.entries.size(); ++i) {
            cout << pht.entries[i].state << endl;
        }
        return os;
    }

   private:
    int n_entries;
    vector<SaturatingCounter> entries;
};

int main([[maybe_unused]] int argc, char** argv) {
    Config cfg;
    {
        ifstream config_f(argv[1]);
        config_f >> cfg.m >> cfg.h >> cfg.w;
        cout << cfg;
    }

    PHT my_pht(m);
    // cout << "The PHT looks like this when initialized: \n" << my_pht << endl;
    // my_pht.update_state(1, 0);
    // my_pht.update_state(1, 1);
    // my_pht.update_state(1, 1);
    // my_pht.update_state(0, 2);
    // my_pht.update_state(0, 3);
    // my_pht.update_state(0, 3);
    // my_pht.update_state(0, 4);
    // my_pht.update_state(1, 4);
    // cout << "PHT state machine test: \n" << my_pht << endl;

    BHT my_bht(w, h);
    // cout << "The BHT looks like this when initialized: \n" << my_bht << endl;
    // my_bht.update_state(1, 0);
    // my_bht.update_state(0, 0);
    // my_bht.update_state(1, 0);
    // my_bht.update_state(0, 0);
    // my_bht.update_state(1, 3);
    // cout << "BHT update test: \n" << my_bht << endl;

    ofstream out((string(argv[2]) + ".out").c_str());
    ifstream trace(argv[2]);
    int address;
    int branch_outcome;
    int count = 0;
    while (trace >> hex >> address >> dec >> branch_outcome) {
        cout << "addr: 0x" << hex << address;
        cout << " - branch: " << dec << branch_outcome << endl;

        int bht_address = (address >> 2) & (int)(pow(2, h) - 1);

        int pht_addr_upper = (address >> 2) & (int)(pow(2, m - w) - 1);
        int pht_addr_lower = my_bht[bht_address];
        int pht_address = (pht_addr_upper << w) | pht_addr_lower;
        out << my_pht.make_prediction(pht_address) << endl;  // make prediction

        my_pht.update_state(branch_outcome, pht_address);
        my_bht.update_state(branch_outcome, bht_address);

        ++count;
        cout << "BHT after update: " << count << '\n' << my_bht << endl;
        cout << "PHT after update: " << count << '\n' << my_pht << endl;

        // cout << "Address: " << address << ' ';
        // cout << "Branch Outcome: " << branch_outcome << endl;
        // out << 0 << endl; // predict not taken
    }

    trace.close();
    out.close();
}
