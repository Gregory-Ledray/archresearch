#include "run_voltspot.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <string>
#include <sstream>

#include <stdexcept>

template <typename T>
std::string to_string(T const& value) {
    std::stringstream sstr;
    sstr << value;
    return sstr.str();
}

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
  std::ofstream LUT;
  LUT.open(LUT_filename, std::fstream::app);
  std::ifstream sim_inst;
  sim_inst.open(sim_inst_filename);
  if (LUT.is_open() && sim_inst.is_open())
  {
    //may wish to generate the ptrace from the sim_inst file here!
    //the output should be LUT.ptrace
    int num_nops = 5;
    int num_vectors = 4;

    if (append_LUT_table(LUT) < 0)
    {
      sim_inst.close();
      return -1;
    };
  }
  else
  {
    fprintf(stderr, "LUT file and sim_inst file could not be opened");
    return -1;
  }

  LUT.close();
  sim_inst.close();

  return 1;
}

// after a trace has been generated from voltspot, this function
append_LUT_table(std::ofstream LUT)
{
  exec("cd /home/grego/gem5/VoltSpot-2.0 2>&1");

  // this will run the program with the power traces from ptrace
  exec("./voltspot -c pdn.config -f example.flp -p LUT.ptrace -v trans.vtrace 2>&1");

  exec("cd /home/grego/gem5 2>&1");

  // to get the LUT table generated, look in trans.vtrace
  std::ifstream trace_output;
  trace_output.open("/home/grego/gem5/VoltSpot-2.0/trans.vtrace");

  if (!trace_output.is_open())
  {
    fprintf(stderr, "failed to write new trace output");
    return -1;
  }

  std::string line;
  int getit = 0;
  while( getline(trace_output, line) )
  {
    if (line == "END_OF_CONFIGS" || line == "END_OF_CONFIGS\n"){
      getline(trace_output, line); // get the next header line
      getit = 1;
      break;
    }
  }

  double max_max_onchip_drop = -50.0;
  double max_onchip_drop;
  double pkgDrop;
  while(trace_output >> pkgDrop >> max_onchip_drop)
  {
    if (max_onchip_drop > max_max_onchip_drop){
      max_max_onchip_drop = max_onchip_drop;
    }
  }

  LUT << (to_string(num_nops) + " " + to_string(num_vectors) + " " + to_string(max_max_onchip_drop) );

  trace_output.close();

}
