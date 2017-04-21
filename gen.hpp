//one big file to try to resolve compiling problems
#ifndef GEN_HPP
#define GEN_HPP

#define DEBUG 1

extern "C"{
#include <gaul.h>
}

int sim_speed = 0.001; /* instructions / us : by setting to 0.001 it's 1 per 100us*/

struct time_chunk{
  double real_time; /* always 100 us right now - value stored in us*/
  byte ** inst; /* array of instruction's bitstrings */
  int inst_len;
  double voltage; /* avg measurement in this time chunk */
  double dv; /* change in voltage since last measurement */
};

struct inst{
  char * inst_name;
  int inst_name_len;
  byte * inst;
  int inst_len;
  int exec_cycles; /* number of cycles it takes to execute this instruction */
};

int num_supported_inst;
struct inst* supported_inst_list;
int instruction_length;

struct time_chunk* time_chunk_list;
int v_len; /* also the length of the time_chunk_list */
int sim_inst_len;


int perceptron(byte * inst);
boolean struggle_score(population *pop, entity *entity);

#ifdef __cplusplus
#include <string>
void e(std::string string);
#else
void e(char * string);
#endif


int how_long_is_file(FILE *fp);
int populate_time_chunk_list(FILE *sim_inst, FILE *v, struct time_chunk* time_chunk_list);
int read_inst(FILE *sim_inst, FILE * supported_inst, struct time_chunk* time_chunk_list, FILE* v);

#endif
