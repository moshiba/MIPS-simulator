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

class BHT {
   public:
    BHT(Config cfg_)
        : cfg(cfg_),
          n_entries(pow(2, cfg.h)),
          entries(n_entries, 0),
          max_entry_val(pow(2, cfg.w) - 1) {}

    void update_state(unsigned bht_index, bool branch_outcome) {
        unsigned new_entry = ((((*this)[bht_index] << 1) |
                               static_cast< unsigned >(branch_outcome)) &
                              bitmask(cfg.w));
        entries[bht_index] = new_entry;
    }

    unsigned operator[](int index) {
        if ((index < 0) || (index >= n_entries)) {
            cout << "index: " << index << ", max_n: " << n_entries << endl;
            throw out_of_range("BHT [] index out of range");
        }
        return entries[index];
    }

    friend ostream& operator<<(ostream& os, const BHT& bht) {
        for (int i = 0; i < bht.n_entries; ++i) {
            cout << bht.entries[i] << endl;
        }
        return os;
    }

   private:
    Config cfg;
    int n_entries;
    vector< unsigned > entries;
    int max_entry_val;
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
    PHT(Config cfg)
        : n_entries(pow(2, cfg.m)), entries(n_entries, SaturatingCounter()) {}

    void update_state(unsigned pht_index, bool branch_outcome) {
        auto& counter = (*this)[pht_index];
        if (branch_outcome == 1) {
            counter.taken();
        } else {
            counter.not_taken();
        }
    }

    SaturatingCounter& operator[](int index) {
        if ((index < 0) || (index >= n_entries)) {
            throw out_of_range("PHT [] index out of range");
        }
        return entries[index];
    }

    friend ostream& operator<<(ostream& os, const PHT& pht) {
        for (int i = 0; i < pht.n_entries; ++i) {
            cout << pht.entries[i].state << endl;
        }
        return os;
    }

   private:
    int n_entries;
    vector< SaturatingCounter > entries;
};

int main([[maybe_unused]] int argc, char** argv) {
    Config cfg;
    {
        ifstream config_f(argv[1]);
        config_f >> cfg.m >> cfg.h >> cfg.w;
        cout << cfg;
    }

    auto BhtIndex = [cfg](unsigned pc) -> unsigned {
        return pc >> 2 & bitmask(cfg.h);
    };

    auto PhtIndex = [cfg](unsigned pc, unsigned bht_entry) -> unsigned {
        return (pc >> 2 & bitmask(cfg.m - cfg.w)) << cfg.w |
               (bht_entry & bitmask(cfg.w));
    };

    PHT pht(cfg);
    BHT bht(cfg);

    ofstream out((string(argv[2]) + ".out").c_str());
    ifstream trace(argv[2]);

    int count = 0;
    unsigned address, branch_outcome;
    while (trace >> hex >> address >> dec >> branch_outcome) {
        cout << "--------------------------------" << endl;
        cout << "  " << ++count << endl;
        cout << "--------------------------------" << endl;
        cout << "addr: 0x" << hex << address << dec << endl;

        const unsigned bht_index = BhtIndex(address);
        const unsigned pht_index = PhtIndex(address, bht[bht_index]);
        cout << "bht_index: " << bht_index << endl;
        cout << "pht_index: " << pht_index << endl;

        const bool prediction = pht[pht_index].predict();
        out << prediction << endl;

        cout << "BHT entry (before): " << bitset< 32 >(bht[bht_index]) << endl;
        cout << "PHT entry (before): " << pht[pht_index].state << endl;

        cout << "predict: " << (prediction ? "taken" : "not taken") << endl;
        cout << "branch:  " << (branch_outcome ? "TAKEN" : "NOT TAKEN") << endl;

        pht.update_state(pht_index, branch_outcome);
        bht.update_state(bht_index, branch_outcome);

        cout << "BHT entry (after):  " << bitset< 32 >(bht[bht_index]) << endl;
        cout << "PHT entry (after):  " << pht[pht_index].state << endl;
    }

    trace.close();
    out.close();
}
