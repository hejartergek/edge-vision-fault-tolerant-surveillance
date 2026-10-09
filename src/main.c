#define _GNU_SOURCE
#define DEBUG_MODE 0

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <sched.h>

#include "image_processor.h"
#include "camera.h"
#include "frame_queue.h"

FrameQueue frame_queue;

typedef struct
{
    int total_frames;
    int dropped_frames;
    int shutdown_rejected_frames;

    double total_processing_ms;
    double max_processing_ms;

    double total_queue_latency_ms;
    double max_queue_latency_ms;

} PerformanceStats;

PerformanceStats performance_stats = {0};

volatile sig_atomic_t stop_requested = 0;
volatile long long last_capture_heartbeat_ns = 0;
volatile long long last_processing_heartbeat_ns = 0;
static long long get_time_ns(void);

void handle_signal(int signal)
{
    (void)signal;
    stop_requested = 1;
}

void *capture_thread(void *arg)
{
    (void)arg;

        cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(2, &cpuset);

    if (pthread_setaffinity_np(
            pthread_self(),
            sizeof(cpu_set_t),
            &cpuset) != 0)
    {
        perror("[CAPTURE] CPU affinity failed");
    }
    else
    {
        printf("[CAPTURE] Affinity set to CPU 2\n");
    }

    int frame_id = 0;
    struct timespec start_time;
clock_gettime(CLOCK_MONOTONIC, &start_time);

int fps_frame_count = 0;

    while (!stop_requested)
    {
        Frame frame;

        frame.data = NULL;
        frame.data_size = 0;
        frame.width = 0;
        frame.height = 0;
        frame.stride = 0;
        frame.frame_id = frame_id;

        printf("[CAPTURE] Requesting frame %d\n",
               frame.frame_id);

        if (camera_capture(
                &frame.data,
                &frame.data_size,
                &frame.width,
                &frame.height,
                &frame.stride) == 0)
        {

            __atomic_store_n(&last_capture_heartbeat_ns,
                             get_time_ns(), __ATOMIC_RELAXED);

            struct timespec capture_time;
clock_gettime(CLOCK_MONOTONIC, &capture_time);

frame.capture_timestamp_ns =
    (long long)capture_time.tv_sec * 1000000000LL +
    capture_time.tv_nsec;


if (queue_push(&frame_queue, frame) == 0)
            {
                printf("[CAPTURE] Frame %d pushed to queue "
                       "(%zu bytes)\n",
                       frame.frame_id,
                       frame.data_size);

                frame_id++;
            }
            else
            {
                free(frame.data);
                __atomic_add_fetch(
                    &performance_stats.shutdown_rejected_frames,
                    1,
                    __ATOMIC_RELAXED);

                printf("[CAPTURE] Frame rejected during shutdown.\n");
                break;
            }
            fps_frame_count++;

struct timespec current_time;
clock_gettime(CLOCK_MONOTONIC, &current_time);

double elapsed =
    (current_time.tv_sec - start_time.tv_sec) +
    (current_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;

if (elapsed >= 5.0)
{
    double fps = fps_frame_count / elapsed;

    printf("[CAPTURE] FPS: %.2f frames/sec\n", fps);

    fps_frame_count = 0;
    start_time = current_time;
}
        }
        else
{
    if (stop_requested)
    {
        break;
    }

    printf("[CAPTURE] Camera error.\n");
}
    }

    return NULL;
}

void *processing_thread(void *arg)
{
    (void)arg;

        cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(3, &cpuset);

    if (pthread_setaffinity_np(
            pthread_self(),
            sizeof(cpu_set_t),
            &cpuset) != 0)
    {
        perror("[PROCESSING] CPU affinity failed");
    }
    else
    {
        printf("[PROCESSING] Affinity set to CPU 3\n");
    }

    Frame previous_frame;
    int has_previous_frame = 0;
    struct timespec processing_start;
clock_gettime(CLOCK_MONOTONIC, &processing_start);

int processing_frame_count = 0;

   while (!stop_requested)
{
    Frame frame;

    if (queue_pop(&frame_queue, &frame) != 0)
    {
        continue;
    }

    __atomic_store_n(&last_processing_heartbeat_ns,
                     get_time_ns(), __ATOMIC_RELAXED);

    struct timespec processing_receive_time;

clock_gettime(
    CLOCK_MONOTONIC,
    &processing_receive_time
);


long long processing_receive_timestamp_ns =
    (long long)processing_receive_time.tv_sec * 1000000000LL +
    processing_receive_time.tv_nsec;


long long latency_ns =
    processing_receive_timestamp_ns -
    frame.capture_timestamp_ns;

double queue_latency_ms =
    latency_ns / 1000000.0;

printf("[LATENCY] Frame %d queue latency: %.3f ms\n",
       frame.frame_id,
       queue_latency_ms);

performance_stats.total_queue_latency_ms += queue_latency_ms;

if (queue_latency_ms > performance_stats.max_queue_latency_ms)
{
    performance_stats.max_queue_latency_ms =
        queue_latency_ms;
}

    processing_frame_count++;

struct timespec processing_current;
clock_gettime(CLOCK_MONOTONIC, &processing_current);

double processing_elapsed =
    (processing_current.tv_sec - processing_start.tv_sec) +
    (processing_current.tv_nsec - processing_start.tv_nsec) / 1000000000.0;

if (processing_elapsed >= 5.0)
{
    double processing_fps =
        processing_frame_count / processing_elapsed;

    printf("[PROCESSING] FPS: %.2f frames/sec\n",
           processing_fps);

    processing_frame_count = 0;
    processing_start = processing_current;
}

        printf("[PROCESSING] Processing frame %d "
               "(%dx%d, stride=%d)\n",
               frame.frame_id,
               frame.width,
               frame.height,
               frame.stride);

        
         struct timespec processing_time_start;
        clock_gettime(CLOCK_MONOTONIC, &processing_time_start);

        /*
         * Calculate average brightness directly
         * from the camera pixel buffer.
         */
        int brightness =
            calculate_average_brightness(
                frame.data,
                frame.width,
                frame.height,
                frame.stride);

        if (brightness >= 0)
        {
            printf("[PROCESSING] Frame %d average brightness: %d/255\n",
                   frame.frame_id,
                   brightness);
        }
        else
        {
            printf("[PROCESSING] Frame %d image processing failed.\n",
                   frame.frame_id);
        }

        /*
         * Compare current frame with previous frame.
         */
        if (has_previous_frame)
        {
            int motion =
                calculate_motion(
                    previous_frame.data,
                    frame.data,
                    frame.width,
                    frame.height,
                    frame.stride);

            if (motion >= 0)
            {
                printf("[MOTION] Frame %d motion score: %d\n",
                       frame.frame_id,
                       motion);
            }
            else
            {
                printf("[MOTION] Motion calculation failed.\n");
            }

            /*
             * Previous frame is no longer needed.
             */
            camera_release_frame(previous_frame.data);
        }
        else
        {
            printf("[MOTION] Frame %d: first frame, "
                   "no comparison available.\n",
                   frame.frame_id);

            has_previous_frame = 1;
        }
                struct timespec processing_time_end;
        clock_gettime(CLOCK_MONOTONIC, &processing_time_end);

        double processing_time_ms =
            (processing_time_end.tv_sec - processing_time_start.tv_sec) * 1000.0 +
            (processing_time_end.tv_nsec - processing_time_start.tv_nsec) / 1000000.0;

        printf("[PROCESSING] Frame %d processing time: %.3f ms\n",
               frame.frame_id,
               processing_time_ms);

        performance_stats.total_processing_ms +=
    processing_time_ms;

if (processing_time_ms >
    performance_stats.max_processing_ms)
{
    performance_stats.max_processing_ms =
        processing_time_ms;
}

performance_stats.total_frames++;

        /*
         * Current frame becomes the previous frame.
         *
         * We keep ownership of this buffer until
         * the next frame arrives.
         */
        previous_frame = frame;

    }

        if (has_previous_frame)
    {
        camera_release_frame(previous_frame.data);
    }

    return NULL;
}


static long long get_time_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

static void *health_monitor_thread(void *arg)
{
    (void)arg;

    const long long timeout_ns = 2000000000LL;
    int capture_fault = 0;
    int processing_fault = 0;

    printf("[HEALTH] Monitor started");
    putchar(10);

    while (!stop_requested)
    {
        usleep(100000);

        long long now = get_time_ns();
        long long capture_last = __atomic_load_n(
            &last_capture_heartbeat_ns, __ATOMIC_RELAXED);
        long long processing_last = __atomic_load_n(
            &last_processing_heartbeat_ns, __ATOMIC_RELAXED);

        long long capture_age = now - capture_last;
        long long processing_age = now - processing_last;

        if (capture_age > timeout_ns && !capture_fault)
        {
            printf("[HEALTH] CAPTURE PROGRESS STALE (%.0f ms)",
                   capture_age / 1000000.0);
            putchar(10);
            capture_fault = 1;
        }
        else if (capture_age <= timeout_ns && capture_fault)
        {
            printf("[HEALTH] CAPTURE PROGRESS RECOVERED");
            putchar(10);
            capture_fault = 0;
        }

        if (processing_age > timeout_ns && !processing_fault)
        {
            printf("[HEALTH] PROCESSING PROGRESS STALE (%.0f ms)",
                   processing_age / 1000000.0);
            putchar(10);
            processing_fault = 1;
            printf("[HEALTH] Initiating controlled shutdown.\n");
            stop_requested = 1;
        }
        else if (processing_age <= timeout_ns && processing_fault)
        {
            printf("[HEALTH] PROCESSING PROGRESS RECOVERED");
            putchar(10);
            processing_fault = 0;
        }
    }

    printf("[HEALTH] Monitor stopped");
    putchar(10);
    return NULL;
}

int main(void)
{
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    pthread_t capture_tid;
    pthread_t processing_tid;
    pthread_t health_tid;

    queue_init(&frame_queue);

    printf("[MAIN] Edge Vision started.\n");

    if (camera_init() != 0)
    {
        printf("[MAIN] Camera initialization failed.\n");
        queue_destroy(&frame_queue);
        return 1;
    }

    if (camera_start() != 0)
    {
        printf("[MAIN] Camera start failed.\n");
        camera_stop();
        queue_destroy(&frame_queue);
        return 1;
    }

    long long startup_time_ns = get_time_ns();
    __atomic_store_n(&last_capture_heartbeat_ns,
                     startup_time_ns, __ATOMIC_RELAXED);
    __atomic_store_n(&last_processing_heartbeat_ns,
                     startup_time_ns, __ATOMIC_RELAXED);

    pthread_create(
        &capture_tid,
        NULL,
        capture_thread,
        NULL);

    pthread_create(
        &processing_tid,
        NULL,
        processing_thread,
        NULL);

    if (pthread_create(&health_tid, NULL, health_monitor_thread, NULL) != 0)
    {
        fprintf(stderr, "[MAIN] Health monitor thread creation failed\n");
        stop_requested = 1;
        queue_shutdown(&frame_queue);
        pthread_join(capture_tid, NULL);
        pthread_join(processing_tid, NULL);
        camera_stop();
        queue_destroy(&frame_queue);
        return 1;
    }

    /*
     * Wait for Ctrl+C or SIGTERM.
     * Then wake any thread blocked on the queue.
     */
    while (!stop_requested)
    {
        usleep(100000);
    }

    queue_shutdown(&frame_queue);

    pthread_join(capture_tid, NULL);
    pthread_join(processing_tid, NULL);
    pthread_join(health_tid, NULL);

printf("[MAIN] Threads stopped.\n");

if (performance_stats.total_frames > 0)
{
    double average_processing_ms =
        performance_stats.total_processing_ms /
        performance_stats.total_frames;

    double average_queue_latency_ms =
        performance_stats.total_queue_latency_ms /
        performance_stats.total_frames;

    printf("\n");
    printf("========== PERFORMANCE SUMMARY ==========\n");

    printf("Processed frames       : %d\n",
           performance_stats.total_frames);

    printf("Runtime dropped frames : %d\n",
           performance_stats.dropped_frames);

    printf("Shutdown rejected      : %d\n",
           performance_stats.shutdown_rejected_frames);

    printf("Average processing     : %.3f ms\n",
           average_processing_ms);

    printf("Maximum processing     : %.3f ms\n",
           performance_stats.max_processing_ms);

    printf("Average queue latency  : %.3f ms\n",
           average_queue_latency_ms);

    printf("Maximum queue latency  : %.3f ms\n",
           performance_stats.max_queue_latency_ms);

    printf("=========================================\n");
}

camera_stop();

queue_destroy(&frame_queue);

printf("[MAIN] Edge Vision stopped.\n");

return 0;

    return 0;
}
