#include <stdlib.h>
#include <stdio.h>

#include "camera.h"

#define SYNTH_WIDTH  640
#define SYNTH_HEIGHT 480

int camera_init(void)
{
    printf("[SYNTHETIC CAMERA] Initialized.\n");
    return 0;
}

int camera_start(void)
{
    printf("[SYNTHETIC CAMERA] Started.\n");
    return 0;
}

int camera_capture(
    unsigned char **data,
    size_t *data_size,
    int *width,
    int *height,
    int *stride)
{
int size = SYNTH_WIDTH * SYNTH_HEIGHT * 4;

    unsigned char *buffer = malloc(size);

    if (buffer == NULL)
    {
        printf("[SYNTHETIC CAMERA] Memory allocation failed!\n");
        return -1;
    }

    static unsigned char pattern = 0;

   for (int y = 0; y < SYNTH_HEIGHT; y++)
{
    for (int x = 0; x < SYNTH_WIDTH; x++)
    {
        int index = (y * SYNTH_WIDTH + x) * 4;

        unsigned char value =
            (unsigned char)((x + y + pattern) % 256);

        buffer[index + 0] = value;  // R
        buffer[index + 1] = value;  // G
        buffer[index + 2] = value;  // B
        buffer[index + 3] = 255;    // A
    }
}

    pattern += 5;

    *data = buffer;
    *data_size = size;
    *width = SYNTH_WIDTH;
    *height = SYNTH_HEIGHT;
    *stride = SYNTH_WIDTH*4;

    return 0;
}

void camera_release_frame(unsigned char *data)
{
    free(data);
}

void camera_stop(void)
{
    printf("[SYNTHETIC CAMERA] Stopped.\n");
}

int camera_get_dropped_frames(void)
{
    return 0;
}