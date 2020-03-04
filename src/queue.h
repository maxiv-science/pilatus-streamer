#ifndef QUEUE_H
#define QUEUE_H


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

typedef struct
{
    int64_t write_index;
    char _pad[64 - sizeof(int64_t)];
    int64_t read_index;
    char _pad1[64 - sizeof(int64_t)];
    void** buffer;
    int size;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int terminate;
} Queue;

void queue_init(Queue* queue, int size);
void queue_free(Queue* queue);
int queue_empty(Queue* queue);
int queue_push(Queue* queue, void* item);
int queue_pop(Queue* queue, void** item);
void queue_shutdown(Queue* queue);

#endif // QUEUE_H
