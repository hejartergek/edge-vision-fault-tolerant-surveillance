#ifndef IMAGE_PROCESSOR_H
#define IMAGE_PROCESSOR_H

int calculate_average_brightness(
    const unsigned char *pixels,
    int width,
    int height,
    int stride
);

int calculate_motion(
    const unsigned char *previous_pixels,
    const unsigned char *current_pixels,
    int width,
    int height,
    int stride
);

#endif