#include <cstdio>
#include <jpeglib.h>

unsigned char *jpeg_load(char const *filename, int *x, int *y, int *channels_in_file, int req_comp)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        return NULL;
    }

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, file);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    unsigned int width          = cinfo.output_width;
    unsigned int height         = cinfo.output_height;
    unsigned int num_components = cinfo.output_components;

    unsigned char *scanlines        = new unsigned char[width * height * num_components];
    unsigned char *current_scanline = scanlines;

    while (cinfo.output_scanline < height)
    {
        unsigned char *scanline_array[1] = {current_scanline};
        jpeg_read_scanlines(&cinfo, scanline_array, 1);
        current_scanline += width * num_components;
    }

    *x                = width;
    *y                = height;
    *channels_in_file = num_components;

    return scanlines;
}

// https://stackoverflow.com/questions/9098881/convert-from-yuv-to-rgb-in-c-android-ndk
void convertYUY2toRGB(unsigned char *yuy2Data, int width, int height, unsigned char *rgbData)
{
    int y, cr, cb;
    double r, g, b;

    for (int i = 0, j = 0; i < width * height * 3; i += 6, j += 4)
    {
        // first pixel
        y  = yuy2Data[j];
        cb = yuy2Data[j + 1];
        cr = yuy2Data[j + 3];

        r = y + (1.4065 * (cr - 128));
        g = y - (0.3455 * (cb - 128)) - (0.7169 * (cr - 128));
        b = y + (1.7790 * (cb - 128));

        // This prevents colour distortions in your rgb image
        if (r < 0)
            r = 0;
        else if (r > 255)
            r = 255;
        if (g < 0)
            g = 0;
        else if (g > 255)
            g = 255;
        if (b < 0)
            b = 0;
        else if (b > 255)
            b = 255;

        rgbData[i]     = (unsigned char)r;
        rgbData[i + 1] = (unsigned char)g;
        rgbData[i + 2] = (unsigned char)b;

        // second pixel
        y  = yuy2Data[j + 2];
        cb = yuy2Data[j + 1];
        cr = yuy2Data[j + 3];

        r = y + (1.4065 * (cr - 128));
        g = y - (0.3455 * (cb - 128)) - (0.7169 * (cr - 128));
        b = y + (1.7790 * (cb - 128));

        if (r < 0)
            r = 0;
        else if (r > 255)
            r = 255;
        if (g < 0)
            g = 0;
        else if (g > 255)
            g = 255;
        if (b < 0)
            b = 0;
        else if (b > 255)
            b = 255;

        rgbData[i + 3] = (unsigned char)r;
        rgbData[i + 4] = (unsigned char)g;
        rgbData[i + 5] = (unsigned char)b;
    }
}
