//one big file to try to resolve compiling problems
#ifndef GEN_HPP
#define GEN_HPP

#define DEBUG 1

extern "C"{
#include <gaul.h>
}
#define uint unsigned int
double euclidean_distance(uint x, uint y);

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
