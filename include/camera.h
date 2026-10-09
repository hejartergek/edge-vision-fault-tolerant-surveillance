#ifndef CAMERA_H
#define CAMERA_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int camera_init(void);
int camera_start(void);

int camera_capture(
    unsigned char **data,
    size_t *data_size,
    int *width,
    int *height,
    int *stride
);

void camera_release_frame(unsigned char *data);
void camera_stop(void);
int camera_get_dropped_frames(void);

#ifdef __cplusplus
}
#endif

#endif