#include <stdlib.h>

#include "image_processor.h"

int calculate_average_brightness(
    const unsigned char *pixels,
    int width,
    int height,
    int stride)
{
    if (pixels == NULL || width <= 0 || height <= 0 || stride <= 0)
    {
        return -1;
    }

    unsigned long long total_brightness = 0;
    unsigned long long pixel_count = 0;

    for (int y = 0; y < height; y++)
    {
        const unsigned char *row =
            pixels + (y * stride);

        for (int x = 0; x < width; x++)
        {
            const unsigned char b = row[x * 4];
            const unsigned char g = row[x * 4 + 1];
            const unsigned char r = row[x * 4 + 2];

            int brightness =
                (299 * r + 587 * g + 114 * b) / 1000;

            total_brightness += brightness;
            pixel_count++;
        }
    }

    if (pixel_count == 0)
    {
        return -1;
    }

    return (int)(total_brightness / pixel_count);
}

int calculate_motion(
    const unsigned char *previous_pixels,
    const unsigned char *current_pixels,
    int width,
    int height,
    int stride)
{
    if (previous_pixels == NULL ||
        current_pixels == NULL ||
        width <= 0 ||
        height <= 0 ||
        stride <= 0)
    {
        return -1;
    }

    unsigned long long difference = 0;
    unsigned long long samples = 0;

    for (int y = 0; y < height; y++)
    {
        const unsigned char *previous_row =
            previous_pixels + (y * stride);

        const unsigned char *current_row =
            current_pixels + (y * stride);

        /*
         * Sample every 10th pixel.
         * This reduces CPU usage on Raspberry Pi.
         */
        for (int x = 0; x < width; x += 10)
        {
            int previous_brightness =
                (299 * previous_row[x * 4 + 2] +
                 587 * previous_row[x * 4 + 1] +
                 114 * previous_row[x * 4]) / 1000;

            int current_brightness =
                (299 * current_row[x * 4 + 2] +
                 587 * current_row[x * 4 + 1] +
                 114 * current_row[x * 4]) / 1000;

            int pixel_difference =
                abs(current_brightness - previous_brightness);

            difference += pixel_difference;
            samples++;
        }
    }

    if (samples == 0)
    {
        return -1;
    }

    return (int)(difference / samples);
}