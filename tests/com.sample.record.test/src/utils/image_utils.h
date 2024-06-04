#ifndef _IMAGE_UTILS_H_
#define _IMAGE_UTILS_H_

unsigned char *jpeg_load(char const *filename, int *x, int *y, int *channels_in_file, int req_comp);
void convertYUY2toRGB(unsigned char *yuy2Data, int width, int height, unsigned char *rgbData);

#endif //_IMAGE_UTILS_H_