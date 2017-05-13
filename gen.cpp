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
#include <ctime>


//LUT code
double euclidean_distance(uint x, uint y, uint x2, uint y2){
  double op1, op2;
  if (x>x2) op1 = (double) (x-x2)*(x-x2);
  else op1 = (double) (x2-x)*(x2-x);
  if (y>y2) op2 = (double) (y-y2)*(y-y2);
  else op2 = (double) (y2-y)*(y2-y);

  return sqrt( op1+op2 );
}

int target_fitness = 50;
/* requires the file to already be open for reading
 */

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
} // this works - it pulls in all 2110 data points

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
  int a = t->nop_instructions - nop_instructions;
	int b = t->vector_instructions - vector_instructions;
	double aa = (double) a*a;
	double bb = (double) b*b;
	return sqrt(aa+bb);
}

double find_in_LUT(unsigned int nop_instructions, unsigned int vector_instructions)
{
  //fprintf(stderr, "find_in_LUT: nop: %d, vect: %d\n", nop_instructions, vector_instructions);
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
      //fprintf(stderr, "d: %f\n", d);
		}
	}
	if (power == -1.0) e("couldn't find power");
	return power;
}
int times_called = 0;
char* text = NULL;
boolean struggle_score(population *pop, entity *entity)
{
  entity->fitness = 0.0; /* entity stores details about individual solutions */

  size_t textlen;
  textlen = 10;
  if (text == NULL) text = (char*) calloc(1, sizeof(char)*10);
  else{
    for (int i=0;i<textlen;i++) text[i] = '\0';
  }

  ga_chromosome_integer_to_string(pop, entity, text, &textlen);

  int a = atoi( strtok(text, " ") );
  //fprintf(stderr, "a");
  int b = atoi( strtok(NULL, "\n\0") );
  //fprintf(stderr, "%d, %d\n", a, b);
  //free(text);

  if (a >= 100) a = a % 100;
  if (b >= 100) b = b % 100;

  double fit = find_in_LUT(a, b);
  if (fit < 0.0) fit = 0.0;
  //fprintf(stderr, "fit: %f\ntimes called: %d\n", fit, times_called++);
  entity->fitness = fit;

  return TRUE;
}

void e(std::string string){
  fprintf(stderr, "%s", string.c_str());
  exit(0);
}
int num_gens_til_convergene = 0;
char* tex2 = NULL;
boolean print_best(int throw_away, population * poppers)
{

  size_t len = 10;
  free(tex2);
  tex2 = (char *) calloc(1, sizeof(int)*len);
  entity* dos = ga_get_entity_from_rank(poppers,0);

  ga_chromosome_integer_to_string(poppers, ga_get_entity_from_rank(poppers,0), tex2, &len);

  int a = atoi( strtok(tex2, " ") );
  int b = atoi( strtok(NULL, "\n\0"));
  if (a >= 100) a = a % 100;
  if (b >= 100) b = b % 100;
  //prints num_gens_til_convergene number of nops, number of vectors, fitness
  num_gens_til_convergene++;
  if (dos->fitness >= 0.5598) printf("%d %d %d %f\n", num_gens_til_convergene, a, b, dos->fitness);
}

/* cmd line: ./genetic LUT_Test.txt sim_inst.txt*/
/* this version uses integer chromosomes */
/* solve with integer chromosomes where the chromosome length is three, one for each type. no allele processing */
int main(int argc, char **argv)
  {
    if (argc != 3) {
      fprintf(stderr, "%s", "wrong number of cmd line args\n");
      return -1;
    }

  #if GEN_NEW_LUT
    if (populate_LUT(argv[1], argv[2]) < 0) return -1;
  #endif

    if (populate_LUT_vector(argv[1]) < 0){
      fprintf(stderr, "populating the LUT vector failed\n");
      return -1;
    }

    int pop_size_lp = 10;
    int num_gens = 10;
    double crossover_rate = 0.1;
    double mutation_rate = 0.1;
    //fprintf(stderr, "new run population_size: %d; num_gens: %d\n", pop_size_lp, num_gens);
    for (pop_size_lp = 10;pop_size_lp < 501; pop_size_lp+=10)
    {
      for (num_gens = 10; num_gens < 100; num_gens += 10)
      {
        for (crossover_rate=0.1; crossover_rate < 0.9; crossover_rate+=0.1)
        {
          for (mutation_rate = 0.1; mutation_rate < 0.9; mutation_rate += 0.1)
          {

            printf("New: %d %d %f %f\n", pop_size_lp, num_gens, crossover_rate, mutation_rate);

      population *pop=NULL;	/* The population of solutions. */

      random_seed(200312);	/* Random seed requires any integer parameter. */
    /*input parameters */
      pop = ga_genesis_integer(
           pop_size_lp,                      /* const int              population_size large due to improper instrucion losses*/
           2,                        /* const int              num_chromosome */
           1,                        /* const int              len_chromo */
           print_best,                     /* GAgeneration_hook      generation_hook */
           NULL,                     /* GAiteration_hook       iteration_hook */
           NULL,                     /* GAdata_destructor      data_destructor */
           NULL,                     /* GAdata_ref_incrementor data_ref_incrementor */
           struggle_score,           /* GAevaluate             evaluate */
           ga_seed_integer_random, /* GAseed                 seed */
           NULL,                     /* GAadapt                adapt */
           ga_select_one_sus,        /* GAselect_one           select_one */
           ga_select_two_sus,        /* GAselect_two           select_two */
           ga_mutate_integer_singlepoint_randomize, /* GAmutate  mutate */
           ga_crossover_integer_singlepoints, /* GAcrossover     crossover */
           NULL,                     /* GAreplace              replace */
           NULL                      /* void *                 userdata */
                );

        ga_population_set_parameters(
           pop,                     /* population              *pop */
           GA_SCHEME_DARWIN,        /* const ga_class_type     class */
           GA_ELITISM_PARENTS_SURVIVE,  /* const ga_elitism_type   elitism */
           crossover_rate,              /* double                  crossover */
           mutation_rate,               /* double                  mutation */
           0.0                      /* double                  migration */
                                  );
      int start = clock();
        ga_evolution(
               pop,                     /* population              *pop */
               num_gens                      /* const int               max_generations */
             );
      int end = clock();
      char* tex2 = NULL;
      size_t len = 10;
      tex2 = (char *) calloc(1, sizeof(int)*len);
      entity* dos = ga_get_entity_from_rank(pop,0);

      ga_chromosome_integer_to_string(pop, ga_get_entity_from_rank(pop,0), tex2, &len);

      int a = atoi( strtok(tex2, " ") );
      int b = atoi( strtok(NULL, "\n\0"));
      if (a >= 100) a = a % 100;
      if (b >= 100) b = b % 100;
      printf("Final Generation!\n");
      printf("%d %d %f\n", a, b, dos->fitness);
      num_gens_til_convergene = 0;

      ga_extinction(pop);	/* Deallocates all memory associated with
      		 * the population and it's entities.
      		 */
           free(tex2);

      printf("time_taken: %f\n", double(end-start) / CLOCKS_PER_SEC);
    }}
    }
  }

  exit(EXIT_SUCCESS);
}
