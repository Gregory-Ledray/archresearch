#ifndef _RUN_VOLTSPOT
#define _RUN_VOLTSPOT

#include <stdio.h>
#include <fstream>

int populate_LUT(char* LUT_filename, char* sim_inst_filename);

int append_LUT(char* LUT_filename, int num_nops, int num_vectors);

int create_ptrace(int num_nops, int num_vectors, int num_loops);

#endif
