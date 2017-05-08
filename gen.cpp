//one big file to try to resolve compiling problems

//input processing with a LUT
/*
// lookup table format: nop_instructions vector_instructions power
lookup table data structure:
to look up the data or its nearest neighbor in O(1):
*/

#include "gen.hpp"
#include "run_voltspot.hpp"

// the genetic algorithm second
extern "C"{
#include <gaul.h>
}
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <string>


//LUT code
double euclidean_distance(uint x, uint y, uint x2, uint y2){
  double op1, op2;
  if (x>x2) op1 = (double) (x-x2)*(x-x2);
  else op1 = (double) (x2-x)*(x2-x);
  if (y>y2) op2 = (double) (y-y2)*(y-y2);
  else op2 = (double) (y2-y)*(y2-y);

  return sqrt( op1+op2 );
}



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


int target_fitness = 50;
/* requires the file to already be open for reading
 */
int read_inst(char* sim_inst_filename, char* supported_inst, struct time_chunk* time_chunk_list, char* LUT){
  std::ifstream supported_inst_;
  supported_inst_.open(supported_inst);
  if (!supported_inst_.is_open()) return -1;
  int line_count=0;
	std::string name, bytes;
	int cyc;
      /* this bit will resolve all char arrays into a meaningful struct inst */
	while(supported_inst_ >> name >> bytes >> cyc)
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
  num_supported_inst = line_count;
  #if DEBUG
  for (int i=0;i<2;i++){
    fprintf(stderr, "%s\n", supported_inst_list[i]->inst_name.c_str());
    fprintf(stderr, "%d\n", supported_inst_list[i]->inst_len);
    fprintf(stderr, "%d\n", supported_inst_list[i]->exec_cycles);
    fprintf(stderr, "%d\n", ga_bit_decode_binary_uint(supported_inst_list[i]->inst,0,instruction_length) );
  }
  #endif

  // at this point I have all of the supported instructions in the supported_inst_list

  // the next step is to read in the LUT
  if (populate_LUT(LUT, sim_inst_filename) < 0) return -1;

  /* populating a time_chunk_list */
  //if (populate_time_chunk_list(sim_inst, LUT, time_chunk_list) < 0) return -1;
}

bool lutentrysorter(struct LUT_entry* a, struct LUT_entry* b){
  return (a->vector_instructions < b->vector_instructions);
}

// populate std::vector<struct LUT_entry*> LUT_entry_list
int populate_LUT_vector(char* LUT_filename)
{
  std::ifstream LUT_filestream;
  LUT_filestream.open(LUT_filename);
  if (!LUT_filestream.is_open()) return -1;

	int a, b;
	double c;

	while(LUT_filestream >> a >> b >> c){
		struct LUT_entry* new_entry = (struct LUT_entry*) calloc(1, sizeof(LUT_entry));
		new_entry->nop_instructions = a;
		new_entry->vector_instructions = b;
		new_entry->power_consumption = c;
		LUT_entry_list.push_back(new_entry);
	}
	// sort the array by vector_instructions
	std::sort(LUT_entry_list.begin(), LUT_entry_list.end(), lutentrysorter);

  for (int i=0;i<LUT_entry_list.size(); i++){
    fprintf(stderr, "%d\n", LUT_entry_list[i]->nop_instructions);
    fprintf(stderr, "%d\n", LUT_entry_list[i]->vector_instructions);
    fprintf(stderr, "%f\n", LUT_entry_list[i]->power_consumption);
  }
  exit(1);
}

//genetic algorithm code

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

double euclid(struct LUT_entry* t, int nop_instructions, int vector_instructions){
  fprintf(stderr, "euc\n");
  int a = t->nop_instructions - nop_instructions;
	int b = t->vector_instructions - vector_instructions;
	double aa = (double) a*a;
	double bb = (double) b*b;
	return sqrt(aa+bb);
}

double find_in_LUT(unsigned int nop_instructions, unsigned int vector_instructions)
{
  fprintf(stderr, "find_in_LUT: nop: %d, vect: %d\n", nop_instructions, vector_instructions);
	// check for invalid chromosomes
	if (nop_instructions < 0 || vector_instructions < 0) return -10000.0;

	// find the nearest neighbor and return its power
	double low_dist = 5000000000.0;
	double power = -1.0;
	if ( 1 ) { // currently there's no need for a more complex algorithm with only a few LUT entries
		for (int i=0;i<LUT_entry_list.size(); i++){
			double d;
			if (d = euclid(LUT_entry_list[i], nop_instructions, vector_instructions) < low_dist){
				low_dist = d;
				power = LUT_entry_list[i]->power_consumption;
			}
      fprintf(stderr, "d: %f\n", d);
		}
	}
	if (power == -1.0) e("couldn't find power");
	return power;
}

boolean struggle_score(population *pop, entity *entity)
  {

  entity->fitness = 0.0; /* entity stores details about individual solutions */
  int* chromos = (int*) entity->chromosome;
  int a = chromos[0];
  int b  = chromos[1];
  fprintf(stderr, "a: %d\n", a);
  fprintf(stderr, "b: %d\n", b);
  entity->fitness = find_in_LUT(a, b);

  return TRUE;
}

void e(std::string string){
  fprintf(stderr, "%s", string.c_str());
  exit(0);
}

/* cmd line: ./genetic LUT_Test.txt supported_inst.txt sim_inst.txt */
/* this version uses integer chromosomes */
/* solve with integer chromosomes where the chromosome length is three, one for each type. no allele processing */
int main(int argc, char **argv)
  {
    if (argc != 4) {
      fprintf(stderr, "%s", "wrong number of cmd line args\n");
      return -1;
    }

    fprintf(stderr, "Populating the LUT\n");

    /*
    // this has already been populated
    if (populate_LUT(argv[1], argv[3]) < 0){
      fprintf(stderr, "LUT table generation failed\n");
      return -1;
    }
    */

    if (populate_LUT_vector(argv[1]) < 0){
      fprintf(stderr, "populating the LUT vector failed\n");
      return -1;
    }

    fprintf(stderr, "LUT processing for the genetic algorithm\n");

	// do all of the file I/O
    if (read_inst(argv[3], argv[2], time_chunk_list, argv[1]) < 0){
      e("failed to process supported_inst or sim_inst_len - 2");
    }

    fprintf(stderr, "Beginning the genetic algorithm\n");

  population *pop=NULL;	/* The population of solutions. */

  random_seed(2003);	/* Random seed requires any integer parameter. */
  fprintf(stderr, "rando\n");
/*input parameters */
  pop = ga_genesis_integer(
       500,                      /* const int              population_size large due to improper instrucion losses*/
       2,                        /* const int              num_chromosome */
       sizeof(int),                        /* const int              len_chromo */
       NULL,                     /* GAgeneration_hook      generation_hook */
       NULL,                     /* GAiteration_hook       iteration_hook */
       NULL,                     /* GAdata_destructor      data_destructor */
       NULL,                     /* GAdata_ref_incrementor data_ref_incrementor */
       struggle_score,           /* GAevaluate             evaluate */
       ga_seed_integer_zero, /* GAseed                 seed */
       NULL,                     /* GAadapt                adapt */
       ga_select_one_sus,        /* GAselect_one           select_one */
       ga_select_two_sus,        /* GAselect_two           select_two */
       ga_mutate_integer_singlepoint_drift, /* GAmutate  mutate */
       ga_crossover_integer_singlepoints, /* GAcrossover     crossover */
       NULL,                     /* GAreplace              replace */
       NULL                      /* void *                 userdata */
            );
    fprintf(stderr, "ga_genesis_bitstring complete\n");

    ga_population_set_parameters(
       pop,                     /* population              *pop */
       GA_SCHEME_DARWIN,        /* const ga_class_type     class */
       GA_ELITISM_PARENTS_DIE,  /* const ga_elitism_type   elitism */
       0.9,                     /* double                  crossover */
       0.2,                     /* double                  mutation */
       0.0                      /* double                  migration */
                              );
fprintf(stderr, "ga_population_set_parameters complete\n");
                              ga_evolution(
                                     pop,                     /* population              *pop */
                                     500                      /* const int               max_generations */
                                   );
  //char* tex = (char*) calloc(1, sizeof(int));
  //long unsigned int texnum = sizeof(tex);

  printf( "The final solution found was:\n");

  size_t len = 3;
  char* tex = (char *) calloc(1, sizeof(int)*len);
  printf("%s\n", ga_chromosome_integer_to_string(pop, ga_get_entity_from_rank(pop,0), tex, &len));

  printf( "Fitness score = %f\n", ga_get_entity_from_rank(pop,0)->fitness);


  ga_extinction(pop);	/* Deallocates all memory associated with
  		 * the population and it's entities.
  		 */

  exit(EXIT_SUCCESS);
}
