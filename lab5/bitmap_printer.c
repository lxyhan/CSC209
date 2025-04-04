#include <stdio.h>
#include <stdlib.h>
#include "bitmap.h"

// the program name is the first argument, the file name is the second
int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: bitmap_printer input_bmp\n");
        exit(1);
    }

    // open the file and store the file in a pointer called image
    FILE *image = fopen(argv[1], "rb");

    // if the file is null, print an error message and exit the program
    if (image == NULL)
    {
        fprintf(stderr, "Cannot open file\n");
        exit(1);
    }

    // Read in bitmap file metadata
    int pixel_array_offset, width, height;
    read_bitmap_metadata(image, &pixel_array_offset, &width, &height);

    // Print out metadata.
    printf("Pixel array offset: %d\n", pixel_array_offset);
    printf("Width: %d\n", width);
    printf("Height: %d\n", height);

    // Read in the pixel data
    struct pixel **pixels = read_pixel_array(image, pixel_array_offset, width, height);

    fclose(image);

    // Print out some pixels from each of the image's corners.
    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            print_pixel(pixels[i][j]);
            print_pixel(pixels[i][width - 1 - j]);
            print_pixel(pixels[height - 1 - i][j]);
            print_pixel(pixels[height - 1 - i][width - 1 - j]);
        }
    }

    // TODO: Clean up so that valgrind reports no memory is still in use

    for (int i = 0; i < height; i++)
    {
        free(pixels[i]);
    }
    free(pixels);

    return 0;
}
