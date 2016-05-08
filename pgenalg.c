/***************************************************************************/
/* Parallel Computing Project **********************************************/
/* Craig Carlson ***********************************************************/
/* Richard Pietrzak ********************************************************/
/* Greg Weil ***************************************************************/
/***************************************************************************/

#include "pgenalg.h"
#include<stddef.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<mpi.h>
#include<pthread.h>
#include "audio.c"
#include "comparison.h"

chromosome* population; 
chromosome* new_population; //for switchover
int mpi_myrank;
int mpi_commsize;
int population_size;//population size
int max_generations;//max generations to run for
int threads_per_rank;//number of threads per rank
double mutation_rate = 0.05;//mutation rate
double crossover_rate = 0.97;//crossover rate

double song_max_duration = 60;
double note_max_duration = 5;
double frequency_max = 25000;

//thread-safe rng stuff
struct drand48_data drand_buf;
double dv;

//DFT data for input file
double** file_dft_data;

double randv(){
	//return random value, assumes seed has been called
	drand48_r(&drand_buf, &dv);
	return dv;
}

unsigned int randr(unsigned int min, unsigned int max){
	//rand value in range
	return (max - min +1)*randv() + min;
}

void* evaluate(void* input) {
	//evaluate the fitness of a chromosome
	//Thread I is responsible for chromosomes (I*P/N to I*P/N + P/N).
	int threadID = *((int *)input);
	int chunk_size = population_size / threads_per_rank;
	int i;
	
	for(i=threadID*chunk_size; i < threadID*chunk_size + chunk_size; i++){
		chromosome chromo = population[i];
		/*//begin test evaluation
		chromo.fitness = chromo.length;
		population[i] = chromo;
		//end test evaluation*/
		
		///printf("thread: %d index: %d fitness: %.5f size: %d\n",threadID,i,chromo.fitness,chromo.length);
		
		/* DO ACTUAL EVALUATION HERE */
		
		Track track = track_initialize_from_binary(chromo.genes, chromo.length,
			song_max_duration, note_max_duration, frequency_max);
		Audio audio = track_audio(&track);
		//audio_save(&audio, path);
		
		//audio.samples[audio.count]
		//audio_duration(&audio) -> seconds
		chromo.fitness = audio_duration(&audio) + chromo.length;
		
		audio_free(&audio);
		track_free(&track);
		
		population[i] = chromo;
	}

	return 0;
}

void* breed(void* input){
	//do crossover and mutation
	//Thread I is responsible for chromosomes (I*P/N to I*P/N + P/N).
	int threadID = *((int *)input);
	int chunk_size = population_size / threads_per_rank;
	int i;
	
	for(i=threadID*chunk_size; i < threadID*chunk_size + chunk_size; i+=2){
		chromosome ch1 = *tournament_selection(8);
		chromosome ch2 = *tournament_selection(8);
		chromosome ret[2];//return buffer for new chromosomes
		
		//do crossover
		if(randv() < crossover_rate){
			one_point_crossover(ch1, ch2, ret);
		}

		//do mutations
		mutate(&ret[0]);
		mutate(&ret[1]);
		new_population[i] = ret[0];
		new_population[i+1] = ret[1];
	}
	
	return 0;
}

void one_point_crossover(chromosome ch1, chromosome ch2, chromosome* out){
        //Perform one point crossover between two chromosomes
		double r = randv();
        int r1 = (int)(ch1.length * r);
        int r2 = (int)(ch2.length * r);
		//slice ch1 and ch2 and swap the partitions
		int nlen1 = (r1 + ch2.length - r2);
		int nlen2 = (r2 + ch1.length - r1);
		chromosome newch1, newch2;
		memcpy(newch1.genes, ch1.genes, ch1.length);//copy ch1 to newch1
		memcpy(newch2.genes, ch2.genes, ch2.length);//copy ch2 to newch2
		memcpy(newch1.genes + r1, ch2.genes + r2, ch2.length - r2);//copy right chunk of ch2 to right chunk of newch1
		memcpy(newch2.genes + r2, ch1.genes + r1, ch1.length - r1);//copy right chunk of ch1 to right chunk of newch2
		newch1.length = nlen1;
		newch2.length = nlen2;
		out[0] = newch1;
		out[1] = newch2;
}

void mutate(chromosome* chromo){
	//mutate a chromosome in place

	int i;
	for(i=0; i<chromo->length;i++){
		if(randv() < mutation_rate){//randomly mutate based on mutation rate
			switch(randr(0,2)){
				case 0://insertion
					if(chromo->length < MAX_GENES){//only insert if room left in memory
						memmove(chromo->genes + i + 1,chromo->genes + i, chromo->length-i);
						chromo->genes[i] = (char)randr(0,255);//RAND_CHAR;
						chromo->length += 1;
					}
					break;
					
				case 1://deletion
					memmove(chromo->genes + i,chromo->genes + i + 1,chromo->length-i-1);
					chromo->length -= 1;
					break;
					
				case 2://substitution
					chromo->genes[i] = (char)randr(0,255);//RAND_CHAR;
					break;
				
				default:
					printf("should not happen...\n");
			}
		}
	}
}

chromosome* random_chromosome_from_population(){
	//return pointer to a random chromosome in the population
	int random_index = (int)(randv() * (population_size-1));
	return population + random_index;
}

chromosome* tournament_selection(int tournament_size){
		//returns pointer to the most fit candidate from tournament of size <tournament_size>
        chromosome* best = random_chromosome_from_population();
		int i;
		for(i=0; i<tournament_size; i++){
            chromosome* chromo = random_chromosome_from_population();
			if(chromo->fitness > best->fitness){//assumes higher fitness is better
				best = chromo;
			}
		}
        return best;
}

int main(int argc, char *argv[]){
	
	//initialize MPI for K ranks
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &mpi_commsize);
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi_myrank);

	if(argc != 6){
		if(mpi_myrank == 0){
			printf("Incorrect number of args\n\t[1] population_size\n\t[2] max_generations\n\t[3]threads_per_rank\n\t[4]input_file\n\t[5]output_directory\n");
		}
		MPI_Finalize();
		return 0;
	}
	population_size = atoi(argv[1]);
	max_generations = atoi(argv[2]); 
	threads_per_rank = atoi(argv[3]);
	char* input_file = argv[4];
	char* output_directory = argv[5];
	
	//read input file
	ReadAudioFile(input_file, &file_dft_data);

	//set RNG seed	
	srand48_r (1202107158 + mpi_myrank * 1999, &drand_buf);
	
	//MPI_Request request;
	MPI_Status status;
	
	pthread_t* threads = malloc(threads_per_rank * sizeof(pthread_t));
	int* threadData = malloc(threads_per_rank * sizeof(int));
	
	//create initial population
	population = malloc(population_size * sizeof(chromosome));
	new_population = malloc(population_size * sizeof(chromosome));
	
	/* create a mpi struct for chromosome */
    int blocklengths[3] = {MAX_GENES,1,1};
    MPI_Datatype types[3] = {MPI_CHAR, MPI_FLOAT, MPI_INT};
    MPI_Datatype MPI_CHROMO;
    MPI_Aint offsets[3];
    offsets[0] = offsetof(chromosome, genes);
    offsets[1] = offsetof(chromosome, fitness);
	offsets[2] = offsetof(chromosome, length);
    MPI_Type_create_struct(3, blocklengths, offsets, types, &MPI_CHROMO);
    MPI_Type_commit(&MPI_CHROMO);
	
	int i,j,generation;//loop vars
	
	for(i=0; i<population_size;i++){
		chromosome tmp;
		tmp.fitness = 0;
		int length = randr(5,20);//start chromosomes between 5 and 50 genes
		tmp.length = length;
		for(j=0;j<length;j++){//assign random char values (0-255)
			tmp.genes[j] = (char)randr(0,255);//RAND_CHAR;
		}
		population[i] = tmp;
		///printf("Rank: %d chromo: <%.*s> %d \n",mpi_myrank,tmp.length,tmp.genes,tmp.length);
	}
	
	MPI_Barrier(MPI_COMM_WORLD);	
	
	//run for max_generations
	for(generation=1; generation <= max_generations; generation++){

		/* 
		
		For population_size P and threads_per_rank N, N threads	evaluate
		the population, each thread being responsible for P/N chromosomes. 
		
		*/
		
		//printf("begin evaluation\n");
		
		for (i = 0; i < threads_per_rank; i++) {
			threadData[i] = i;
			pthread_create(&threads[i], NULL, evaluate, threadData + i);
		}
		for (i = 0; i < threads_per_rank; i++) {
			pthread_join(threads[i], NULL);
		}
		
		//print some metrics every 10 generations
		if(mpi_myrank == 0 && generation%10==0){
			float max_fitness = 0;
			chromosome* chromo = NULL;
			for(i=0; i<population_size; i++){
				chromosome tmp = population[i];
				///printf("index: %d fitness %.5f size: %d\n",i,tmp.fitness,tmp.length);
				if(tmp.fitness > max_fitness){
					max_fitness = tmp.fitness;
					chromo = &population[i];
				}
			}
			printf("Generation %d:\n\tMax fitness: %.5f\n",generation,max_fitness);
			if (chromo != NULL) {
				Track track = track_initialize_from_binary(chromo->genes, chromo->length,
					song_max_duration, note_max_duration, frequency_max);
				Audio audio = track_audio(&track);
				char fname[128];
				sprintf(fname, "%s/audio_result_%d.wav", output_directory, generation);
				audio_save(&audio, fname);
				printf("\tDuration: %.2fs\n\tNotes: %d\n",
					audio_duration(&audio), track.count);
				audio_free(&audio);
				track_free(&track);
			}
		}
		
		/*
		
		After all P chromosomes have been evaluated, P/(K-1) chromosomes are
		randomly chosen to be distributed among the other (K-1) populations. 
		
		*/

		//exchange one chromosome with every other population
		for(i=0; i<mpi_commsize; i++){
			if(i ==  mpi_myrank){//don't send to self
				continue;
			}else{
				chromosome* tmp = tournament_selection(8);//fitness-based random chromo to exchange
				///printf("rank %d sent <%.*s> %.5f to rank %d\n",mpi_myrank,tmp->length,tmp->genes,tmp->fitness,i);
				chromosome recv;				
				MPI_Sendrecv(tmp, 1, MPI_CHROMO, i, 0, &recv, 1, MPI_CHROMO, i, 0, MPI_COMM_WORLD, &status);
				*tmp = recv;
				///printf("rank %d received <%.*s> %.5f from rank %d\n",mpi_myrank,tmp->length,tmp->genes,tmp->fitness,i);
			}
		}	
		
		/*
		
		After each rank receives the chromosomes from the other populations,
		it begins the crossover sequence.
		
		Crossover occurs on N threads.
		
		*/
		
		for (i = 0; i < threads_per_rank; i++) {
			threadData[i] = i;
			pthread_create(&threads[i], NULL, breed, threadData + i);
		}
		for (i = 0; i < threads_per_rank; i++) {
			pthread_join(threads[i], NULL);
		}
		
		//switch to new population
		for(i=0; i<population_size;i++){
			population[i] = new_population[i];
		}

	}

	MPI_Barrier(MPI_COMM_WORLD);	
	MPI_Finalize();
	
	free(threads);
	free(threadData);
	free(population);
	free(new_population);
	
    
	
    return 0;
}
