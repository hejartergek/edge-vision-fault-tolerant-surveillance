#include <stdlib.h>
#include <stdio.h>

#include "frame_queue.h"

void queue_init(FrameQueue *queue)
{
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    queue->shutdown = 0;

    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->not_empty, NULL);
    pthread_cond_init(&queue->not_full, NULL);
}

void queue_destroy(FrameQueue *queue)
{
    pthread_mutex_lock(&queue->mutex);

    for (int i = 0; i < queue->count; i++)
    {
        int index =
            (queue->head + i) % QUEUE_SIZE;

        free(queue->buffer[index].data);
        queue->buffer[index].data = NULL;
    }

    pthread_mutex_unlock(&queue->mutex);

    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);
}

int queue_push(FrameQueue *queue, Frame frame)
{
    pthread_mutex_lock(&queue->mutex);

    while (queue->count == QUEUE_SIZE && !queue->shutdown)
{
    printf("[QUEUE] Queue full, capture thread waiting...\n");

    pthread_cond_wait(
        &queue->not_full,
        &queue->mutex);
}

if (queue->shutdown)
{
    pthread_mutex_unlock(&queue->mutex);
    return -1;
}

    queue->buffer[queue->tail] = frame;

    queue->tail =
        (queue->tail + 1) % QUEUE_SIZE;

    queue->count++;

    pthread_cond_signal(&queue->not_empty);

    pthread_mutex_unlock(&queue->mutex);
    return 0;
}

int queue_pop(FrameQueue *queue, Frame *frame)
{
    pthread_mutex_lock(&queue->mutex);

    while (queue->count == 0 && !queue->shutdown)
    {
        pthread_cond_wait(
            &queue->not_empty,
            &queue->mutex);
    }

    if (queue->count == 0 && queue->shutdown)
    {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }

    *frame = queue->buffer[queue->head];

    queue->head =
        (queue->head + 1) % QUEUE_SIZE;

    queue->count--;

    pthread_cond_signal(&queue->not_full);

    pthread_mutex_unlock(&queue->mutex);

    return 0;
}

void queue_shutdown(FrameQueue *queue)
{
    pthread_mutex_lock(&queue->mutex);

    queue->shutdown = 1;

    pthread_cond_broadcast(&queue->not_empty);
    pthread_cond_broadcast(&queue->not_full);

    pthread_mutex_unlock(&queue->mutex);
}