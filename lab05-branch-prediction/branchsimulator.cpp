#include <math.h>

#include <bitset>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

class BHT {
    friend ostream& operator<<(ostream& os, const BHT& bht);

   public:
    BHT(int w, int h) : entries(pow(2, h), 0), max_entry_val(pow(2, w) - 1) {}
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
        if ((index >= 0) && ((size_t)index < entries.size())) {
            return entries[index];
        } else {
            return -1;
        }
    }

   private:
    vector< int > entries;
    int max_entry_val;
};

ostream& operator<<(ostream& os, const BHT& bht) {
    for (size_t i = 0; i < bht.entries.size(); ++i) {
        cout << bht.entries[i] << endl;
    }
    return os;
}

class PHT {
    friend ostream& operator<<(ostream& os, const PHT& pht);

   public:
    PHT(int m) : entries(pow(2, m), bitset< 2 >(2)) {}
    void update_state(bool branch_outcome, int pht_address) {
        if ((size_t)pht_address > entries.size() - 1) {
            cerr << "Invalid PHT address" << endl;
            return;
        }
        if (entries[pht_address] == 0) {  // strongly not taken
            if (branch_outcome == 0) {
                // no change
            } else {
                entries[pht_address] = 1;  // weakly not taken
            }
        } else if (entries[pht_address] == 1) {  // weakly not taken
            if (branch_outcome == 0) {
                entries[pht_address] = 0;  // strongly not taken
            } else {
                entries[pht_address] = 2;  // weakly taken
            }
        } else if (entries[pht_address] == 2) {  // weakly taken
            if (branch_outcome == 0) {
                entries[pht_address] = 1;  // weakly not taken
            } else {
                entries[pht_address] = 3;  // strongly taken
            }
        } else {  // strongly taken
            if (branch_outcome == 0) {
                entries[pht_address] = 2;  // weakly taken
            } else {
                // no change
            }
        }
    }
    bool make_prediction(int pht_address) {
        if ((entries[pht_address] == 0) ||
            (entries[pht_address] == 1)) {  // not taken
            return 0;
        } else {
            return 1;
        }
    }

   private:
    vector< bitset< 2 > > entries;
};

ostream& operator<<(ostream& os, const PHT& pht) {
    for (size_t i = 0; i < pht.entries.size(); ++i) {
        cout << pht.entries[i] << endl;
    }
    return os;
}

int main(int argc, char** argv) {
    ifstream config;
    config.open(argv[1]);

    int m, w, h;
    config >> m;
    config >> h;
    config >> w;

    config.close();

    ofstream out;
    string out_file_name = string(argv[2]) + ".out";
    out.open(out_file_name.c_str());

    ifstream trace;
    trace.open(argv[2]);

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

    int address;
    int branch_outcome;
    string address_line;
    string branch_line;
    int count = 0;
    // TODO: Implement a two-level branch predictor
    while (!trace.eof()) {
        branch_line = "";
        bool space_flag = 0;
        getline(trace, address_line);
        // cout << address_line << endl;
        for (size_t i = 0; i < address_line.size(); ++i) {
            if (space_flag == 1) {
                branch_line.push_back(address_line[i]);
            }
            if (address_line[i] == ' ') {
                space_flag = 1;
            }
        }
        address = stoi(address_line, nullptr, 16);
        branch_outcome = stoi(branch_line, nullptr, 2);
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

// Path: branchsimulator_skeleton_23.cpp
