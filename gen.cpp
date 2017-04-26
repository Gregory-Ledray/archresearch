//one big file to try to resolve compiling problems

//input processing first
/* cmd line: ./genetic sim_inst.txt supported_inst.txt v.csv */
//sim_inst.txt
// this file has lines of run instruction bitstrings, one per line. Every line
// corresponds to one instruction. its runtime will be matched to real time later
/* file format - lines of: inst_bitstring */

// supported_inst.txt
/* file format - lines of: inst_name instruction_bitstring exec_cycles*/
//v.csv
// every line has the voltage in 100 us (here assumed to be one instruction cycle) increments
/* file format - lines of: double_string */

#include <stdio.h>
extern "C"{
#include <gaul.h>
}
#include <stdlib.h>
#include <fstream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <string>
#include "gen.hpp"

int how_long_is_file(FILE *fp){
  int size = -1;
  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  return size;
}
/* takes a char* and makes it one bigger and adds in the new char */
/* assumes the current array has an extra space at the end for a char */
/* this assumption exists since the array will need a \0 anyway */
#define ADD_CHAR(char_ptr, current_size, new_char) \
char_ptr[current_size-1] = new_char; \
current_size++; \
char_ptr = (char *) realloc(char_ptr, current_size)

#define ADD_INST_TO_TIME_CHUNK(ptr_byte_ptr, current_size,new_byte_ptr) \
ptr_byte_ptr = (byte **) realloc(ptr_byte_ptr, current_size+1); \
ptr_byte_ptr[current_size] = new_byte_ptr; \
current_size++

/* requires the file to already be open for reading
 */
int read_inst(FILE *sim_inst, FILE * supported_inst, struct time_chunk* time_chunk_list, FILE* LUT){
	int line_count=0;
	string name, bytes;
	int cyc;
      /* this bit will resolve all char arrays into a meaningful struct inst */
	while(supported_inst >> name >> bytes >> cyc)
    {
		struct inst* new_entry = (struct inst*) calloc(1, sizeof(inst));
		new_entry->inst_name = name;
		new_entry->exec_cycles = cyc;
		
		byte * inst = ga_bit_new(bytes.length());
		int ia;
        for (ia =0;ia<bytes.length();ia++){
          if (bytes[ia] == '0')ga_bit_clear(inst, ia);
          else if (bytes[ia] == '1')ga_bit_set(inst, ia);
          else {
            fprintf(stderr, "invalid input for instruction bitstring - not 0s and 1s\n");
            return -1;
          }
        }
		new_entry->inst = inst;
		new_entry->inst_len = bytes.length();
		
		instruction_length = bytes.length();
		
		supported_inst_list.push_back(new_entry);
		
		line_count++;
    }
  num_supported_inst = line_ct;
  #if DEBUG
  for (int i=0;i<2;i++){
    fprintf(stderr, "%s\n",supported_inst_list[i].inst_name);
    fprintf(stderr, "%d\n", supported_inst_list[i].inst_len);
    fprintf(stderr, "%d\n", supported_inst_list[i].exec_cycles);
    fprintf(stderr, "%d\n", ga_bit_decode_binary_uint(supported_inst_list[i].inst,0,instruction_length) );
  }
  #endif
  
  // at this point I have all of the supported instructions in the supported_inst_list
  
  // the next step is to read in the LUT
  if (populate_LUT_vector(LUT) < 0) return -1;

  /* populating a time_chunk_list */
  //if (populate_time_chunk_list(sim_inst, LUT, time_chunk_list) < 0) return -1;
}

bool lutentrysorter(struct LUT_entry* a, struct LUT_entry* b){return a->vector_instructions < b->vector_instructions};

// populate std::vector<struct LUT_entry*> LUT_entry_list
int populate_LUT_vector(FILE* LUT)
{
	int a, b;
	double c;

	while(LUT >> a >> b >> c){
		struct LUT_entry* new_entry = (struct LUT_entry*) calloc(1, sizeof(LUT_entry));
		new_entry->nop_instructions = a;
		new_entry->vector_instructions = b;
		new_entry->power_consumption = c;
		LUT_entry_list.push_back(new_entry);
	}
	// sort the array by vector_instructions
	std::sort(LUT_entry_list.begin(), LUT_entry_list.end(), lutentrysorter);
}

int populate_time_chunk_list(FILE *sim_inst, FILE *v, struct time_chunk* time_chunk_list)
{
  char line[256];
  int line_ct = 0;
  sim_inst_len = how_long_is_file(sim_inst);
  v_len = how_long_is_file(v);
  if (v_len < 1 || sim_inst_len < 1){
    fprintf(stderr, "size of files sim_instructions or voltage info was less than 1\n");
    return -1;
  }
  time_chunk_list = (struct time_chunk *) calloc(v_len, sizeof(struct time_chunk) );
  int i, a;

  /* pupulating a time_chunk_list */
  while (fgets(line, sizeof(line), v)){
    fprintf(stderr, "%s", line); /* debug */
    time_chunk_list[i].real_time = 100.0;
    char * double_string = (char*) calloc(1, sizeof(char));
    int double_string_len = 1;
    for (i=0;i<256;i++)
    {
      if (line[i] == '\n' || line[i] == '\0')
      {
        time_chunk_list[line_ct].voltage = atof(double_string);
        free(double_string);
        if (i==0) time_chunk_list[line_ct].dv = 0;
        else time_chunk_list[line_ct].dv = time_chunk_list[line_ct].voltage - time_chunk_list[i-1].voltage;
        break;
      }
      else if (line[i] == '.' || (line[i] <= '9' && line[i] >= '0') )
      {
        ADD_CHAR(double_string, double_string_len, line[i]);
      }
      else{
        fprintf(stderr, "invalid format for input file with voltages; line[i]: %c\n", line[i]);
        return -1;
      }
    }
    line_ct++;
  }
  line_ct = 0;

  instruction_length = -1;
  byte ** input_instructions = (byte **) calloc( sim_inst_len, sizeof(byte *) );

/* populate input_instructions */
  while (fgets(line, sizeof(line), sim_inst)){
    fprintf(stderr, "%s", line); /* debug */
    char * bitstring_string = (char*) calloc(1, sizeof(char));
    int bitstring_string_len = 1;
    for (i=0;i<256;i++)
    {
      if (line[i] == '\n') {
        byte * ja = ga_bit_new(bitstring_string_len-1);
        if (instruction_length == -1) instruction_length = bitstring_string_len-1;
        else if (instruction_length != bitstring_string_len-1){
          fprintf(stderr, "Error: Bitstring lengths differ\n");
          return -1;
        }
        for (a=0;a<bitstring_string_len-1;a++)
        {
          if (bitstring_string[a] == '0') ga_bit_clear(ja, a);
          else if (bitstring_string[a] == '1') ga_bit_set(ja, a);
          else {
            fprintf(stderr, "Error: Bitstring not string in sim_inst\n");
            return -1;
          }
        }
        input_instructions[line_ct] = ja;
        free(bitstring_string);
        break;
      }
      else if (line[i] <= '1' && line[i] >= '0'){
        ADD_CHAR(bitstring_string, bitstring_string_len, line[i]);
      }
      else{
        fprintf(stderr, "invalid format for input file with simulator instructions\n");
        return -1;
      }
    }
    line_ct++;
  }
  int current_time_chunk;
  double max_exec_cycles = 0.0; // tracks the cycle we're on. round down to get correct time_chunk
  //double current_exec_cycles = 0.0;
  int cur_time_chunk_len = 0;
/* transfer members of input_instructions (bitstrings) into the correct time_chunk */
/* to do so, go through every input/sim_inst and keep track of the current time chunk
current time chunk is floor of max_exec_cycles.
@ end of processing an instruction, max_exec_cycles += time of inst just processed / sim_speed
with correct time_chunk known by converting max_exec_cycles into an int,
add the instruction to that time chunk
*/
  for (a=0;a<sim_inst_len;a++)
  {
    current_time_chunk = (int) max_exec_cycles;
    fprintf(stderr, "cur_time_chunk: %d\n", current_time_chunk );
    unsigned int sim_inst_being_tested = ga_bit_decode_binary_uint(input_instructions[a], 0, instruction_length);
    fprintf(stderr, "ga_bit_decode_binary_uint worked: %d\n", sim_inst_being_tested);
    #if DEBUG
    fprintf(stderr, "a\n");
    for (int i=0;i<2;i++){
      fprintf(stderr, "%s\n",supported_inst_list[i].inst_name);
      fprintf(stderr, "%d\n", supported_inst_list[i].inst_name_len);
      fprintf(stderr, "%d\n", supported_inst_list[i].inst_len);
      fprintf(stderr, "%d\n", supported_inst_list[i].exec_cycles);
    }
    #endif
    for (i=0;i<num_supported_inst;i++)
    {
      fprintf(stderr, "%p", supported_inst_list[i].inst);
      unsigned int to_compare = ga_bit_decode_binary_uint(supported_inst_list[i].inst,0,instruction_length);
      fprintf(stderr, "%d\n", to_compare);
      if ( sim_inst_being_tested == to_compare )
      {
        fprintf(stderr, "ga\n");
        ADD_INST_TO_TIME_CHUNK(time_chunk_list[current_time_chunk].inst, cur_time_chunk_len, input_instructions[a]);
        break;
      }
    }
    // update max_exec_cycles and if new one will be greater, update cur_time_chunk_len
    max_exec_cycles += time_chunk_list[current_time_chunk].real_time / sim_speed;
    if ( ((int) max_exec_cycles) > current_time_chunk) cur_time_chunk_len = 0;

  }
  fprintf(stderr, "a\n");
  free(input_instructions);
}

// the genetic algorithm second
#include <gaul.h>
#include <string>
#include <stdio.h>

int target_fitness = 50;//?

/*this is the objective function - must return TRUE upon evaluation; false otherwise */
/* option 1: examine all past data, run genetic to optimize instructions,
run instructions and get more data */
/* option 2: examine only the most recent data,
/* algorithm 2 */
/* for every chromosome, examine the voltage droop before and after */
/* weigh on-time chunk with .4, adjacent with .2 each and edge with .1 */

/* from algo. 2: for a time span 3, weigh each spot more for fitness if closer
 to the given time chunk */
 /* current version is dumber: just look at the time_chunk of the inst itself */
 /* this means i'm only looking at the effects of an instruction on its own time */
 /* cumulative effects are lost */
int perceptron(byte * inst)
{
  unsigned int inst_uint = ga_bit_decode_binary_uint(inst,0,instruction_length);
  /* verify that this is a legitimate instruction */
  int i=0;
  int found = 0;
  for (i=0;i<num_supported_inst;i++)
  {
    if (ga_bit_decode_binary_uint(supported_inst_list[i].inst, 0, instruction_length)==inst_uint )
     {found = 1; break;}
  }
  if (!found) {return -1;} /* no such instruction */

  double fit=0;

  /* for every time chunk, try to find this instruction */
  for (i=0;i<v_len;i++) {
    int x=0;
    for (x=0;x<time_chunk_list[i].inst_len;x++){
      if (ga_bit_decode_binary_uint(time_chunk_list[i].inst[x], 0, instruction_length) == inst_uint)
      {/* FITNESS - BASED ON THIS time_chunk_list VALUE */
        fit+=time_chunk_list[i].dv;
      }
    }
  }

  return (int) (1000*fit);
}

double euclid(struct LUT_entry* t, int nop_instructions, int vector_instructions){
	int a = t->nop_instructions - nop_instructions;
	int b = t->vector_instructions - vector_instructions;
	double aa = (double) a*a;
	double bb = (double) b*b;
	return sqrt(aa+bb);
}

double find_in_LUT(int nop_instructions, int vector_instructions)
{
	// check for invalid chromosomes
	if (nop_instructions < 0 || vector_instructions < 0) return -10000.0;
	
	// find the nearest neighbor and return its power
	double low_dist = 1000000.0;
	double power = -1.0;
	if ( 1 ) { // currently there's no need for a more complex algorithm with only a few LUT entries
		for (int i=0;i<LUT_entry_list.size(); i++){
			double d;
			if (d = euclid(LUT_entry_list[i], nop_instructions, vector_instructions) < low_dist){
				low_dist = d;
				power = LUT_entry_list[i].power_consumption;
			}
		}
	}
	if (power == -1.0) e("couldn't find power");
	return power;
}

boolean struggle_score(population *pop, entity *entity)
  {
  int           k;              /* Loop variable over all alleles. */

  entity->fitness = 0.0; /* entity stores details about individual solutions */
  int c; /* chromosome number aka instruction number */
  int fit=0;
  
  entity->fitness = find_in_LUT(entity->chromosome[0], entity->chromosome[1]);

  return TRUE;
}

void e(std::string string){
  fprintf(stderr, "%s", string.c_str());
  exit(0);
}

/* cmd line: ./gen_exec LUT_Test.txt supported_inst.txt sim_inst.txt */
/* this version uses integer chromosomes */
/* solve with integer chromosomes where the chromosome length is three, one for each type. no allele processing */
int main(int argc, char **argv)
  {
    if (argc != 4) {
      fprintf(stderr, "%s", "wrong number of cmd line args\n");
      return -1;
    }

    
    /* open files */
    FILE* LUT = fopen(argv[1], "r");
    if (LUT==NULL)
    {
      e("failed to open v.csv file");
    }
    FILE* supported_inst = fopen(argv[2], "r");
    if (supported_inst == NULL)
    {
      e("failed to open supported_inst file");
    }
	FILE* sim_inst = fopen(argv[3], "r");
    if (sim_inst == NULL){
      e("failed to open sim_inst file");
    }
	
	/* process files for length */
    int supported_inst_file_length = how_long_is_file(supported_inst);
    if (supported_inst_file_length < 1) {
      fclose(sim_inst);
      fclose(supported_inst);
      fclose(LUT);
      e("failed to process supported_inst");
    }

    supported_inst_list = (struct inst *) calloc(supported_inst_file_length, sizeof(struct inst));
    if (supported_inst_list == NULL){
      fclose(sim_inst);
      fclose(supported_inst);
      fclose(LUT);
      e("createing supported instruction list failed");
    }

	// do all of the file I/O
    if (read_inst(sim_inst, supported_inst, time_chunk_list, LUT) < 0){
      fclose(sim_inst);
      fclose(supported_inst);
      fclose(LUT);
      e("failed to process supported_inst or sim_inst_len - 2");

    }
    fclose(sim_inst);
    fclose(supported_inst);
    fclose(LUT);

  population *pop=NULL;	/* The population of solutions. */

  random_seed(2003);	/* Random seed requires any integer parameter. */

/*input parameters */
  pop = ga_genesis_bitstring(
       500,                      /* const int              population_size large due to improper instrucion losses*/
       1,                        /* const int              num_chromosome */
       sizeof(int)*3,      /* const int              len_chromo  - not sure if this is 3 or sizeof(int)*3 */
       NULL,                     /* GAgeneration_hook      generation_hook */
       NULL,                     /* GAiteration_hook       iteration_hook */
       NULL,                     /* GAdata_destructor      data_destructor */
       NULL,                     /* GAdata_ref_incrementor data_ref_incrementor */
       struggle_score,           /* GAevaluate             evaluate */
       ga_seed_integer_random, /* GAseed                 seed */
       NULL,                     /* GAadapt                adapt */
       ga_select_one_sus,        /* GAselect_one           select_one */
       ga_select_two_sus,        /* GAselect_two           select_two */
       ga_mutate_integer_singlepoint_drift, /* GAmutate  mutate */
       ga_crossover_integer_singlepoints, /* GAcrossover     crossover */
       NULL,                     /* GAreplace              replace */
       NULL                      /* void *                 userdata */
            );

    ga_population_set_parameters(
       pop,                     /* population              *pop */
       GA_SCHEME_DARWIN,        /* const ga_class_type     class */
       GA_ELITISM_PARENTS_DIE,  /* const ga_elitism_type   elitism */
       0.9,                     /* double                  crossover */
       0.2,                     /* double                  mutation */
       0.0                      /* double                  migration */
                              );

                              ga_evolution(
                                     pop,                     /* population              *pop */
                                     500                      /* const int               max_generations */
                                   );
  // the thing doing the actual evolution
  ga_evolution(
         pop,                     /* population              *pop */
         100                      /* const int               max_generations */
                );
  //char* tex = (char*) calloc(1, sizeof(int));
  //long unsigned int texnum = sizeof(tex);

  printf( "The final solution found was:\n");
  
  size_t len = 3;
  char[len] tex;
  printf("%s\n", ga_chromosome_integer_to_string(pop, ga_get_entity_from_rank(pop,0), tex, &len));
  
  printf( "Fitness score = %f\n", ga_get_entity_from_rank(pop,0)->fitness);


  ga_extinction(pop);	/* Deallocates all memory associated with
  		 * the population and it's entities.
  		 */

  exit(EXIT_SUCCESS);
}
