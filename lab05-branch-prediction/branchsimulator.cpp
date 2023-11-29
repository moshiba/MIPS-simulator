#include <math.h>

#include <bitset>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

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

    // TODO: Implement a two-level branch predictor
    while (!trace.eof()) {
        out << 0 << endl;  // predict not taken
    }

    trace.close();
    out.close();
}

// Path: branchsimulator_skeleton_23.cpp
