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
    fprintf(stderr, "result: %s", result.c_str());
    return result;
}

int populate_LUT(char* LUT_filename, char* sim_inst_filename){
  std::ifstream sim_inst;
  sim_inst.open(sim_inst_filename);
  srand(1000);
  if (sim_inst.is_open())
  {
    //the output should be LUT.ptrace
    for (int num_nops = 0; num_nops < 1001; num_nops+=100)
    {
      if (num_nops > 0 && num_nops < 135) num_nops -= 99; //go in steps until reach 30 for greater granularity
      if (num_nops > 30) num_nops += 30;
      for (int num_vectors = 0; num_vectors < 1001; num_vectors += 100)
      {
        if (num_vectors > 0 && num_vectors < 135) num_vectors -= 99; //go in steps until reach 30 for greater granularity
        if (num_vectors > 30) num_vectors += 30;
        // this function creates a ptrace called LUT.ptrace
        fprintf(stderr, "num_nops: %d; num_vectors: %d\n", num_nops, num_vectors);
        if (create_ptrace(num_nops, num_vectors, 4) < 0)
        {
          sim_inst.close();
          return -1;
        }//worked in testing

        // actually append the LUT given voltspot input files exist
        if (append_LUT(LUT_filename, num_nops, num_vectors) < 0)
        {
          sim_inst.close();
          return -1;
        }
      }
    }
  }
  else
  {
    fprintf(stderr, "sim_inst file could not be opened\n");
    return -1;
  }

  sim_inst.close();

  return 1;
}

// create a ptrace from the num_nops and num_vectors inputs
// STATUS: this works
// added feature: now each cycle of nops and vectors runs num_loops times
int create_ptrace(int num_nops, int num_vectors, int num_loops)
{
  std::ofstream LUT_ptrace;
  LUT_ptrace.open("/home/grego/gem5/VoltSpot-2.0/LUT.ptrace");
  if (!LUT_ptrace.is_open())
  {
    fprintf(stderr, "failed to open LUT.ptrace\n");
    return -1;
  }

  LUT_ptrace << "ICache1	BTB1	BrP1	InstBuf1	InstDec1	IntRAT1	FlpRAT1	FL1	DCache1	LdQ1	StQ1	Itlb1	Dtlb1	IntRF1	FlpRF1	IntIW1	FlpIW1	ROB1	ALU1	FPU1	CplALU1	ICache2	BTB2	BrP2	InstBuf2	InstDec2	IntRAT2	FlpRAT2	FL2	DCache2	LdQ2	StQ2	Itlb2	Dtlb2	IntRF2	FlpRF2	IntIW2	FlpIW2	ROB2	ALU2	FPU2	CplALU2	NoC1	NoC2	L2_1	L2_2	MC1	\n";
  for (int it=0;it<num_loops; it++)
  {
    for (int i=0;i<num_nops;i++)
    {
      LUT_ptrace << "0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000\n";
    }

    for (int i=0;i<num_vectors;i++)
    {
      LUT_ptrace << "0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 1.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000\n";
    }
  }

  return 1;
}

// generates a trace from voltspot,
// will append the LUT table with the largest percentage droop
// STATUS: command line arguments fail
int append_LUT(char* LUT_filename, int num_nops, int num_vectors)
{
  std::ofstream LUT_out;
  LUT_out.open(LUT_filename, std::fstream::app);
  if (!LUT_out.is_open()) return -1;

  // this will run the program with the power traces from ptrace
  fprintf(stderr, "starting exec\n");
  exec("(cd /home/grego/gem5/VoltSpot-2.0 && ./voltspot -c pdn.config -f example.flp -p LUT.ptrace -v trans2.vtrace && cwd && cd /home/grego/gem5 2>&1)");
  fprintf(stderr, "exec finished\n");

  // VoltSpot output is in trans.vtrace
  std::ifstream trace_output;
  trace_output.open("/home/grego/gem5/VoltSpot-2.0/trans2.vtrace");

  if (!trace_output.is_open())
  {
    fprintf(stderr, "failed to write new trace output");
    return -1;
  }

  // disregard garbage config information
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

  // pull in the interesting data
  double max_max_onchip_drop = -50.0;
  double max_onchip_drop;
  double pkgDrop;
  while(trace_output >> pkgDrop >> max_onchip_drop)
  {
    if (max_onchip_drop > max_max_onchip_drop){
      max_max_onchip_drop = max_onchip_drop;
    }
  }

  // append the LUT with the largest percentage droop
  LUT_out << (to_string(num_nops) + " " + to_string(num_vectors) + " " + to_string(max_max_onchip_drop) + "\n" );

  trace_output.close();
  LUT_out.close();

  return 1;
}
