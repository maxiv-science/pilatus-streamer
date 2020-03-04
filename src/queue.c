#include "queue.h"

void queue_init(Queue* queue, int size)
{
    queue->size = size;
    queue->buffer = malloc(sizeof(void*) * queue->size);
    queue->write_index = 0;
    queue->read_index = 0;
    queue->terminate = 0;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);
}

void queue_free(Queue* queue)
{
    free(queue->buffer);
}

int queue_empty(Queue* queue)
{
    return queue->read_index == queue->write_index;
}

int queue_push(Queue* queue, void* item)
{
    int64_t next = queue->read_index + 1;
    // queue is full
    if (next == queue->read_index) {
        return 0;
    }
    else {
        queue->buffer[queue->write_index % queue->size] = item;
        int64_t index = queue->write_index;
        queue->write_index++;
        //asm volatile("mfence" ::: "memory");
        __sync_synchronize();
        // signal consumer that queue is no longer empty
        if (index == queue->read_index) {
            pthread_mutex_lock(&queue->mutex);
            pthread_cond_signal(&queue->cond); 
            pthread_mutex_unlock(&queue->mutex);
        }
        return 1;
    }
}

int queue_pop(Queue* queue, void** item)
{
    // check if queue is empty
    if (queue->read_index == queue->write_index) {
        pthread_mutex_lock(&queue->mutex);
        while (queue_empty(queue) && !queue->terminate) {
            pthread_cond_wait(&queue->cond, &queue->mutex);
        }
        pthread_mutex_unlock(&queue->mutex);
        if (queue->terminate && queue_empty(queue)) {
            return 0;
        }
    }
    *item = queue->buffer[queue->read_index % queue->size];
    queue->read_index++;
    return 1;
}

void queue_shutdown(Queue* queue)
{
    pthread_mutex_lock(&queue->mutex);
    queue->terminate = 1;
    pthread_cond_signal(&queue->cond); 
    pthread_mutex_unlock(&queue->mutex);
}
