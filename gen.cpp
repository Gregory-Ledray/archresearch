//one big file to try to resolve compiling problems

//input processing with a LUT
/*
cmd line: ./genetic LUT_Test.txt
// lookup table format: nop_instructions vector_instructions power
lookup table data structure:
to look up the data or its nearest neighbor in O(1):

*/

#include <stdio.h>
extern "C"{
#include <gaul.h>
}
#include <stdlib.h>
#include <string>
#include "gen.hpp"

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

#define ADD_INST_TO_TIME_CHUNK(ptr_byte_ptr, current_size,new_byte_ptr) \
ptr_byte_ptr = (byte **) realloc(ptr_byte_ptr, current_size+1); \
ptr_byte_ptr[current_size] = new_byte_ptr; \
current_size++

int target_fitness = 50;

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

boolean struggle_score(population *pop, entity *entity)
  {
  int           k;              /* Loop variable over all alleles. */

  entity->fitness = 0.0; /* entity stores details about individual solutions */
  int c; /* chromosome number aka instruction number */
  int fit=0;
  for (c=0;c<pop->num_chromosomes;c++)
  {
    /* loop over alleles (individual instructions) in chromosome */
    for (k=0;k<pop->len_chromosomes;k++){
      if (fit = perceptron((byte *)entity->chromosome[c]) == -1) {
        entity->fitness=0;
        return TRUE;
      }
      else{
        entity->fitness += fit;
      }
    }
  }

  return TRUE;
}

void e(std::string string){
  fprintf(stderr, "%s", string.c_str());
  exit(0);
}

/* cmd line: ./gen_exec sim_inst.txt supported_inst.txt v.csv */
int main(int argc, char **argv)
  {
    if (argc != 4) {
      fprintf(stderr, "%s", "wrong number of cmd line args\n");
      return -1;
    }

    /* process sim_inst and supported_inst files for length */
    FILE* sim_inst = fopen(argv[1], "r");
    if (sim_inst == NULL){
      e("failed to open sim_inst file");
    }
    FILE* v = fopen(argv[3], "r");
    if (v==NULL)
    {
      e("failed to open v.csv file");
    }
    FILE* supported_inst = fopen(argv[2], "r");
    if (supported_inst == NULL)
    {
      e("failed to open supported_inst file");
    }
    int supported_inst_length = how_long_is_file(supported_inst);
    if (supported_inst_length < 0) {
      fclose(sim_inst);
      fclose(supported_inst);
      fclose(v);
      e("failed to process supported_inst");
    }

    supported_inst_list = (struct inst *) calloc(supported_inst_length, sizeof(struct inst));
    if (supported_inst_list == NULL){
      fclose(sim_inst);
      fclose(supported_inst);
      fclose(v);
      e("createing supported instruction list failed");
    }

    if (read_inst(sim_inst, supported_inst, time_chunk_list, v) < 0){
      fclose(sim_inst);
      fclose(supported_inst);
      fclose(v);
      e("failed to process supported_inst or sim_inst_len - 2");

    }
    fclose(sim_inst);
    fclose(supported_inst);
    fclose(v);

  population *pop=NULL;	/* The population of solutions. */

  random_seed(2003);	/* Random seed requires any integer parameter. */

/*input parameters */
  pop = ga_genesis_bitstring(
       500,                      /* const int              population_size large due to improper instrucion losses*/
       50,                        /* const int              num_chromosome */
       ga_bit_sizeof(supported_inst_list[0].inst_len),      /* const int              len_chromo */
       NULL,                     /* GAgeneration_hook      generation_hook */
       NULL,                     /* GAiteration_hook       iteration_hook */
       NULL,                     /* GAdata_destructor      data_destructor */
       NULL,                     /* GAdata_ref_incrementor data_ref_incrementor */
       struggle_score,           /* GAevaluate             evaluate */
       ga_seed_printable_random, /* GAseed                 seed */
       NULL,                     /* GAadapt                adapt */
       ga_select_one_sus,        /* GAselect_one           select_one */
       ga_select_two_sus,        /* GAselect_two           select_two */
       ga_mutate_printable_singlepoint_drift, /* GAmutate  mutate */
       ga_crossover_char_allele_mixing, /* GAcrossover     crossover */
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
  ga_evolution(
         pop,                     /* population              *pop */
         100                      /* const int               max_generations */
                );
  char* tex = (char*) calloc(1, sizeof(supported_inst_list[0].inst_len));
  long unsigned int texnum = sizeof(tex);

  printf( "The final solution found was:\n");
  int i;
  for (i=0;i<pop->num_chromosomes;i++)
  {
    printf( "%s\n", ga_chromosome_bitstring_to_string(pop, ga_get_entity_from_rank(pop,i), tex, &texnum));
  }
  printf( "Fitness score = %f\n", ga_get_entity_from_rank(pop,0)->fitness);


  ga_extinction(pop);	/* Deallocates all memory associated with
  		 * the population and it's entities.
  		 */

  exit(EXIT_SUCCESS);
}
