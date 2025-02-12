#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <getopt.h>
#include <string.h>

#include "generator.h"
#include "worker.h"

#ifdef _WIN32
	#include <windows.h>
#endif

#define MAX_SIZE 1048576

pthread_barrier_t barrier;

int main(int argc, char** argv) 
{
    char* input_file_path = NULL;
    char* output_file_path = NULL;
	
    LCGParams lcg_params;
	errno = 0;
    
    int option;
    while ((option = getopt(argc, argv, "i:o:a:c:x:m:")) != -1) 
	{
        switch (option) 
		{
            case 'i':
                input_file_path = optarg;
                break;
            case 'o':
                output_file_path = optarg;
                break;
            case 'a':
                lcg_params.a = strtoul(optarg, NULL, 10);
                break;
            case 'x':
                lcg_params.x = strtoul(optarg, NULL, 10);
                break;
            case 'c':
                lcg_params.c = strtoul(optarg, NULL, 10);
                break;
            case 'm':
                lcg_params.m = strtoul(optarg, NULL, 10);
                break;
            default:
                fprintf(stderr, "Usage: %s -i input_file_path -o output_file_path -a a -c c -x x -m m\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
	
	if (!check_params(&lcg_params) || errno == ERANGE)
	{
		fprintf(stderr, "Your params wrong! a, x, c from [0, m) and m from [2, ULONG_MAX].\n");
		fprintf(stderr, "Usage: %s -i input_file_path -o output_file_path -a a -c c -x x -m m\n", argv[0]);
		exit(EXIT_FAILURE);
	}

    FILE* input_file = fopen(input_file_path, "rb");
	if (input_file == NULL)
	{
		fprintf(stderr, "Openning input_file failed!\n");
		exit(EXIT_FAILURE);
	}
    fseek(input_file, 0, SEEK_END);
    size_t file_size = ftell(input_file);
    rewind(input_file);

    if (file_size > MAX_SIZE) 
	{
        fprintf(stderr, "File size exceeds the maximum limit.\n");
        fclose(input_file);
        exit(EXIT_FAILURE);
    }

    char* input_text = (char*)malloc(file_size);
    fread(input_text, 1, file_size, input_file);
    fclose(input_file);
    
    // One time pad generation
    char* pad = (char*)malloc(file_size);
	ThreadParams tp = {pad, file_size, lcg_params};
	
	pthread_t thread;
    if (pthread_create(&thread, NULL, create_pad, &tp) != 0) 
	{
        fprintf(stderr, "Error creating thread!\n");
        free(pad);
        exit(EXIT_FAILURE);
    }

    // Waiting for generation ended
    pthread_join(thread, NULL);

    // Creating n threads (n = number of kernels)
	int num_workers;
	#ifdef _WIN32
		SYSTEM_INFO systemInfo;
		GetSystemInfo(&systemInfo);
		num_workers = systemInfo.dwNumberOfProcessors;
    #else
        num_workers = sysconf(_SC_NPROCESSORS_ONLN);
    #endif
    
    pthread_t* threads = (pthread_t*)malloc(num_workers * sizeof(pthread_t));
    WorkerContext* contexts = (WorkerContext*)malloc(num_workers * sizeof(WorkerContext));
    
    pthread_barrier_init(&barrier, NULL, num_workers + 1);

    // Quantitty of data for each worker
    size_t segment_length = file_size / num_workers;

    for (int i = 0; i < num_workers; ++i) 
	{
        contexts[i].input_text = input_text + i * segment_length;
        contexts[i].pad = pad + i * segment_length;
        contexts[i].output_text = (char*)malloc(segment_length);
        contexts[i].length = segment_length;
		contexts[i].barrier = barrier;

        pthread_create(&threads[i], NULL, worker, (void*)&contexts[i]);
    }

    // Main thread wait for workers
    pthread_barrier_wait(&barrier);

    // Joinning processed data
    char* output_text = (char*)malloc(file_size);
    for (int i = 0; i < num_workers; ++i) 
	{
        memcpy(output_text + i * segment_length, contexts[i].output_text, segment_length);
        free(contexts[i].output_text);
    }

    FILE* output_file = fopen(output_file_path, "wb");
	if (output_file == NULL)
	{
		fprintf(stderr, "Openning output_file failed!\n");
		exit(EXIT_FAILURE);
	}
    fwrite(output_text, 1, file_size, output_file);
    fclose(output_file);

    free(input_text);
    free(pad);
    free(output_text);
    free(contexts);
    free(threads);
    pthread_barrier_destroy(&barrier);

    return 0;
}
