#ifndef FRAME_QUEUE_H
#define FRAME_QUEUE_H

#include <pthread.h>
#include <stddef.h>

#define QUEUE_SIZE 5
#define FILENAME_SIZE 256

typedef struct
{
    unsigned char *data;
    size_t data_size;

    int width;
    int height;
    int stride;

    int frame_id;

    long long capture_timestamp_ns;

} Frame;

typedef struct
{
    Frame buffer[QUEUE_SIZE];

    int head;
    int tail;
    int count;
    int shutdown;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;

} FrameQueue;

void queue_init(FrameQueue *queue);
void queue_destroy(FrameQueue *queue);

int queue_push(FrameQueue *queue, Frame frame);
int queue_pop(FrameQueue *queue, Frame *frame);
void queue_shutdown(FrameQueue *queue);

#endif