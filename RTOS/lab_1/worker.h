#pragma once
#include <cstddef>
#include <pthread.h>
#include "generator.h"

struct WorkerContext 
{
    char* input_text;
    char* pad;
    char* output_text;
    size_t length;
	pthread_barrier_t barrier;
};

void* worker(void* arg) 
{
    WorkerContext* context = (WorkerContext*)arg;
    
    for (size_t i = 0; i < context->length; ++i) 
	{
        context->output_text[i] = context->input_text[i] ^ context->pad[i];
    }

    pthread_barrier_wait(&context->barrier);
    return NULL;
}

struct ThreadParams 
{
    char* pad;
    size_t file_size;
    LCGParams lcg_params;
};

void* create_pad(void* arg) 
{
    ThreadParams* tp = (ThreadParams*)(arg);

    for (size_t i = 0; i < tp->file_size; ++i) 
	{
        tp->pad[i] = (char)(generate(&tp->lcg_params));
    }

    return NULL;
}