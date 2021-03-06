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
#include<float.h>
#include <fftw3.h>
#include "audio.c"
#include "comparison.h"

#define NOTE_BYTES 12

chromosome* population; 
chromosome* new_population; //for switchover
int mpi_myrank;
int mpi_commsize;
int population_size;//population size
int max_generations;//max generations to run for
double max_fitness; //best fitness of current generation
int threads_per_rank;//number of threads per rank
double mutation_rate = 0.05;//mutation rate
double crossover_rate = 0.97;//crossover rate

unsigned int song_max_samples = 48000;
double song_max_duration = 60;
double note_max_duration = 0.1;
double frequency_max = 25000;

int blockSize2 = 512;

//thread-safe rng stuff
struct drand48_data drand_buf;
double dv;

//DFT data for input file
double** file_dft_data;
int file_dft_length;

typedef struct {
	int threadid;
	Audio audio;
	double*	fftw_in;
	fftw_complex* fftw_out;
	fftw_plan plan;
} t_data;

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
	t_data t_input = *((t_data *)input);
	int threadID = t_input.threadid;
	double* fftw_in = t_input.fftw_in;
	fftw_complex* fftw_out = t_input.fftw_out;
	fftw_plan plan = t_input.plan;

	int chunk_size = population_size / threads_per_rank;
	int remainder;
	if(chunk_size == 0){
		remainder = population_size;
	}
	else{
		remainder = population_size % chunk_size;
	}
	int start = threadID * chunk_size;
	start += (threadID < remainder) ? threadID : remainder;
	if (threadID < remainder) chunk_size += 1;
	int i;
	
	for(i=start; i < start + chunk_size; i++){
		chromosome chromo = population[i];
		
		/* DO ACTUAL EVALUATION HERE */
		Track track = track_initialize_from_binary(chromo.genes, chromo.length,
			song_max_duration, note_max_duration, frequency_max);
		track_audio_preallocated(&track, &t_input.audio);

		chromo.fitness = 0;
		if (audio_duration(&t_input.audio) > 0) {
			chromo.fitness = AudioComparison(t_input.audio.samples, t_input.audio.count, file_dft_data, file_dft_length, &fftw_in, &fftw_out, &plan);
			if (chromo.fitness > 0) {
				chromo.fitness = (1000000000.0 / chromo.fitness);
			} else {
				chromo.fitness = DBL_MAX;
			}
		}
		
		track_free(&track);
		
		population[i] = chromo;
	}

	return 0;
}

void* breed(void* input){
	//do crossover and mutation
	//Thread I is responsible for chromosomes (I*P/N to I*P/N + P/N).
	t_data t_input = *((t_data *)input);
	int threadID = t_input.threadid;
	int chunk_size = population_size / threads_per_rank;
	int remainder;
	if(chunk_size == 0){
		remainder = population_size;
	}
	else{
		remainder = population_size % chunk_size;
	}
	int start = threadID * chunk_size;
	start += (threadID < remainder) ? threadID : remainder;
	if (threadID < remainder) chunk_size += 1;
	int i;
	
	for(i=start + 1; i < start + chunk_size; i+=2){
		chromosome ch1 = *tournament_selection(8);
		chromosome ch2 = *tournament_selection(8);
		chromosome ret[2];//return buffer for new chromosomes
		
		//do crossover
		if(randv() < crossover_rate){
			one_point_crossover(ch1, ch2, ret);
		}
		else{
			ret[0] = ch1;
			ret[1] = ch2;
		}
		//do mutations
		mutate(&ret[0]);
		mutate(&ret[1]);
		new_population[i-1] = ret[0];
		new_population[i] = ret[1];
	}
	
	return 0;
}

void one_point_crossover(chromosome ch1, chromosome ch2, chromosome* out){
        //Perform one point crossover between two chromosomes
		double r = randv();
		//need to round to nearest note to prevent offset problem
		
        int r1 = ((int)(ch1.length * r / NOTE_BYTES)) * NOTE_BYTES;
        int r2 = ((int)(ch2.length * r / NOTE_BYTES)) * NOTE_BYTES;
		
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

	int i,j;
	for(i=0; i<chromo->length;i+=NOTE_BYTES){
		switch(randr(0,2)){
			case 0://insertion
				if(randv() < mutation_rate){//randomly mutate based on mutation rate
					if(chromo->length + NOTE_BYTES < MAX_GENES ){//only insert if room left in memory
						memmove(chromo->genes + i + NOTE_BYTES, chromo->genes + i, chromo->length-i);
						for(j=0;j<NOTE_BYTES;j++){
							chromo->genes[i+j] = (char)randr(0,255);//RAND_CHAR;
						}
						chromo->length += NOTE_BYTES;
					}
				}
				break;
				
			case 1://deletion
				if(chromo->length != NOTE_BYTES){
					if(randv() < mutation_rate){//randomly mutate based on mutation rate
						if(chromo->length >= NOTE_BYTES + i ){//only insert if room left in memory
							memmove(chromo->genes + i,chromo->genes + i + NOTE_BYTES,chromo->length-i-NOTE_BYTES);
							chromo->length -= NOTE_BYTES;
						}
					}
				}
				break;
				
			case 2://substitution
				for(j=0;j<NOTE_BYTES;j++){
					if(randv() < mutation_rate){//randomly mutate based on mutation rate
						chromo->genes[i+j] = (char)randr(0,255);//RAND_CHAR;
					}
				}
				break;
			
			default:
				printf("should not happen...\n");
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

chromosome get_best_chromosome(){
	int i;
	max_fitness = -1;
	chromosome* chromo = NULL;
	for(i=0; i<population_size; i++){
		chromosome tmp = population[i];
		if(tmp.fitness > max_fitness){
			max_fitness = tmp.fitness;
			chromo = &population[i];
		}
	}
	return *chromo;
}

int main(int argc, char *argv[]){
	double starttime, endtime;

	//initialize MPI for K ranks
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &mpi_commsize);
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi_myrank);

	starttime = MPI_Wtime();

	if(argc != 7){
		if(mpi_myrank == 0){
			printf("Incorrect number of args\n\t[1] population_size\n\t[2] max_generations\n\t[3]threads_per_rank\n\t[4]generations_between_wav_output\n\t[5]input_file\n\t[6]output_directory\n");
		}
		MPI_Finalize();
		return 0;
	}
	population_size = atoi(argv[1]);
	max_generations = atoi(argv[2]); 
	if (max_generations <= 0) max_generations = INT_MAX;
	threads_per_rank = atoi(argv[3]);
	if (threads_per_rank <= 0) threads_per_rank = 1;
	int generations_between_wav_output = atoi(argv[4]);
	if (generations_between_wav_output <= 0) generations_between_wav_output = INT_MAX;
	char* input_file = argv[5];
	char* output_directory = argv[6];
	//create output filename
	char out_filename[100];
    sprintf(out_filename,"%s/output_%d_%d_%d_%d.txt", output_directory, mpi_commsize, threads_per_rank, population_size, max_generations);
    //test if valid output directory
    FILE* fout = fopen(out_filename, "w");	
    if(!fout){
    	if(mpi_myrank == 0){ 
    		printf("error: Invalid output directory %s\n", output_directory);
    	}
    	MPI_Finalize();
		return 0;
    }
	fprintf(fout, "%s \tfilename\n%d \t\tranks\n%d \t\tthreads/rank\n%d \t\tpopulation\n%d \t\tgenerations\n", input_file, mpi_commsize, threads_per_rank, population_size, max_generations);
    fclose(fout);
	
	//read input file
	unsigned int sample_rate = 0;
	unsigned int sample_count = 0;
	file_dft_length = ReadAudioFile(input_file, &file_dft_data, &sample_rate, &sample_count);
	if(file_dft_length == 0){
		//error while reading in file
		MPI_Finalize();
		return 0;
	}
	song_max_duration = (sample_count * 1.0 / sample_rate);
	song_max_samples = sample_count;
	if (song_max_duration < note_max_duration) {
		note_max_duration = song_max_duration;
	}
	SAMPLE_RATE = sample_rate;
	if (mpi_myrank == 0) {
		printf("Input File:\n\tDuration: %f\n\tSample Rate: %u\n", song_max_duration, sample_rate);
	}

	int i,j,generation;//loop vars

	//set RNG seed	
	srand48_r (1202107158 + mpi_myrank * 1999, &drand_buf);
	
	//MPI_Request request;
	MPI_Status status;
	
	pthread_t* threads = malloc(threads_per_rank * sizeof(pthread_t));
	t_data* threadData = malloc(threads_per_rank * sizeof(t_data));
	
	//create initial population
	population = malloc(population_size * sizeof(chromosome));
	new_population = malloc(population_size * sizeof(chromosome));

	//create a plan for each thread
	for(i = 0; i<threads_per_rank; i++){
		threadData[i].threadid = i;
		
		threadData[i].audio = audio_initialize(song_max_samples);

		threadData[i].fftw_in = fftw_malloc( sizeof(double) * blockSize2);
	    if ( !threadData[i].fftw_in ) {
			printf("error: fftw_malloc 1 failed\n");
			for( j=0; j < i; j++ ){
				audio_free( &threadData[i].audio );
				fftw_free( threadData[j].fftw_in );
				fftw_destroy_plan( threadData[j].plan );
				fftw_free( threadData[j].fftw_out );
			}
			free( threadData );
			free( threads );
			return 0;
		}

		threadData[i].fftw_out = fftw_malloc( sizeof(fftw_complex) * blockSize2 );
	    if ( !threadData[i].fftw_out ) {
			printf("error: fftw_malloc 2 failed\n");
			for( j=0; j < i; j++ ){
				fftw_free( threadData[j].fftw_in );
				fftw_destroy_plan( threadData[j].plan );
				fftw_free( threadData[j].fftw_out );
			}
			fftw_free( threadData[i].fftw_in );
			free( threadData );
			free( threads );
			return 0;
	    }

		threadData[i].plan = fftw_plan_dft_r2c_1d( blockSize2, threadData[i].fftw_in, threadData[i].fftw_out, FFTW_MEASURE );
		if ( !threadData[i].plan ) {
			printf("error: Could not create plan\n");
			for( j=0; j < i; j++ ){
				fftw_free( threadData[j].fftw_in );
				fftw_destroy_plan( threadData[j].plan );
				fftw_free( threadData[j].fftw_out );
			}
			fftw_free( threadData[i].fftw_in );
			fftw_free( threadData[i].fftw_out );
			free( threadData );
			free( threads );
			return 0;
		}
	}
	
	/* create a mpi struct for chromosome */
    int blocklengths[3] = {MAX_GENES,1,1};
    MPI_Datatype types[3] = {MPI_CHAR, MPI_DOUBLE, MPI_INT};
    MPI_Datatype MPI_CHROMO;
    MPI_Aint offsets[3];
    offsets[0] = offsetof(chromosome, genes);
    offsets[1] = offsetof(chromosome, fitness);
	offsets[2] = offsetof(chromosome, length);
    MPI_Type_create_struct(3, blocklengths, offsets, types, &MPI_CHROMO);
    MPI_Type_commit(&MPI_CHROMO);
	
	for(i=0; i<population_size;i++){
		chromosome tmp;
		tmp.fitness = 0;
		int length = randr(150,250)*NOTE_BYTES;//start chromosomes between with random size
		tmp.length = length;
		for(j=0;j<length;j++){//assign random char values (0-255)
			tmp.genes[j] = (char)randr(0,255);//RAND_CHAR;
		}
		population[i] = tmp;
		///printf("Rank: %d chromo: <%.*s> %d \n",mpi_myrank,tmp.length,tmp.genes,tmp.length);
	}
	
	MPI_Barrier(MPI_COMM_WORLD);	
	
	if (mpi_myrank == 0) {
		printf("Running\n");
	}	
	
	//run for max_generations
	for(generation=1; generation <= max_generations; generation++){

		/* 
		
		For population_size P and threads_per_rank N, N threads	evaluate
		the population, each thread being responsible for P/N chromosomes. 
		
		*/
		
		for (i = 0; i < threads_per_rank; i++) {
			pthread_create(&threads[i], NULL, evaluate, &(threadData[i]));
		}
		for (i = 0; i < threads_per_rank; i++) {
			pthread_join(threads[i], NULL);
		}
		
		chromosome best_chromo = get_best_chromosome();
		
		//do global exchange
		if(generation%generations_between_wav_output==0 || generation == max_generations){
			if(mpi_myrank == 0){//recv best from everything
				chromosome recv;
				int best_rank = 0;
				for(i=1;i<mpi_commsize;i++){
					MPI_Recv(&recv, 1, MPI_CHROMO, i, 1234, MPI_COMM_WORLD, &status);
					if(recv.fitness > best_chromo.fitness){
						best_chromo = recv;
						best_rank = i;
					}
				}	
				
				printf("Best among all populations:\nRank: %d\nGeneration %d:\n\tMax fitness: %.5f\n",best_rank,generation,max_fitness);
				
				//do detailed output
				Track track = track_initialize_from_binary(best_chromo.genes, best_chromo.length, song_max_duration, note_max_duration, frequency_max);
				Audio* audio = &(threadData[0].audio);
				track_audio_preallocated(&track, audio);
				char fname[256];
				sprintf(fname, "%s/audio_result_%d.wav", output_directory, generation);
				double similarity = AudioComparison(audio->samples, audio->count, file_dft_data, file_dft_length, &(threadData[0].fftw_in), &(threadData[0].fftw_out), &(threadData[0].plan) );
				printf("\tDifference Score: %.0f\n", similarity);
				audio_save(audio, fname);
				printf("\tNotes: %d (%d bytes)\n", track.count, best_chromo.length);
				double freqMax = DBL_MIN; double freqMin = DBL_MAX;
				double volMax = DBL_MIN; double volMin = DBL_MAX;
				double durMax = DBL_MIN; double durMin = DBL_MAX;
				for (i = 0; i < track.count; ++i) {
					Note* note = &track.notes[i];
					if (note->frequency < freqMin) freqMin = note->frequency;
					if (note->frequency > freqMax) freqMax = note->frequency;
					if (note->volume < volMin) volMin = note->volume;
					if (note->volume > volMax) volMax = note->volume;
					if (note->duration < durMin) durMin = note->duration;
					if (note->duration > durMax) durMax = note->duration;
				}
				printf("\tFrequency: %.0f - %.0f\n", freqMin, freqMax);
				printf("\tVolume: %.3f - %.3f\n", volMin, volMax);
				printf("\tDuration: %.3f - %.3f\n", durMin, durMax);
				track_free(&track);
			}
			else{//else send best to rank 0
				MPI_Send(&best_chromo, 1, MPI_CHROMO, 0, 1234, MPI_COMM_WORLD);
			}
		}else{
			if(mpi_myrank == 0){//recv best from everything
				//else just print for rank 0
				printf("Rank 0 Best\nGeneration %d:\n\tMax fitness: %.5f\n",generation,max_fitness);
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
			pthread_create(&threads[i], NULL, breed, &(threadData[i]));
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

	if(mpi_myrank == 0){ 
		endtime = MPI_Wtime();
		FILE* fout = fopen(out_filename, "a");	
		fprintf(fout, "%f \tTotalTime\n%.3f \tMax Fitness\n", endtime - starttime, max_fitness);
		fclose(fout);
    }

	MPI_Finalize();

	for( i=0; i < threads_per_rank; i++ ){
		audio_free( &threadData[i].audio );
		fftw_free( threadData[i].fftw_in );
		fftw_free( threadData[i].fftw_out );
		fftw_destroy_plan( threadData[i].plan );
	}
	free( threadData );

	free( threads );
	free(population);
	free(new_population);
	
    
	
    return 0;
}
