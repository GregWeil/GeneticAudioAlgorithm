#define MAX_GENES 4096
typedef struct {
	char genes[MAX_GENES];
	double fitness;
	int length;//length of genes
	
} chromosome;
double randv();
unsigned int randr(unsigned int min, unsigned int max);
void* evaluate(void* input);
void* breed(void* input);
void one_point_crossover(chromosome ch1, chromosome ch2, chromosome* out);
void mutate(chromosome* chromo);
chromosome* random_chromosome_from_population();
chromosome* tournament_selection(int tournament_size);