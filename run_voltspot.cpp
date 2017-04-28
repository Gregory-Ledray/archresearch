#include "run_voltspot.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <string>

#include <stdexcept>

std::string exec(const char* cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (!feof(pipe)) {
            if (fgets(buffer, 128, pipe) != NULL)
                result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}

int populate_LUT_table(char* LUT_filename, char* sim_inst_filename){
  std::ifstream LUT;
  LUT.open(LUT_filename);
  std::ifstream sim_inst;
  sim_inst.open(sim_inst_filename);
  if (LUT.is_open() && sim_inst.is_open())
  {
    exec("cd /home/grego/gem5/VoltSpot-2.0 2>&1");

    // this will run the program with the power traces from ptrace
    exec("./voltspot -c pdn.config -f example.flp -p example.ptrace -v trans.vtrace 2>&1");

    exec("cd /home/grego/gem5 2>&1");



  }
  else
  {
    fprintf(stderr, "LUT file and sim_inst file could not be opened");
    return -1;
  }


  LUT.close();
  sim_inst.close();
}
