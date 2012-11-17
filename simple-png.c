
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define PNG_DEBUG 3
#include <png.h>
#include <setjmp.h>

int ReadPNG(const char *fname, unsigned char (**data)[3], int *w, int *h)
{
    FILE *fp = fopen(fname, "rb");
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_infop end_info = NULL;
    int transforms = PNG_TRANSFORM_STRIP_16
                   | PNG_TRANSFORM_STRIP_ALPHA
                   | PNG_TRANSFORM_PACKING
                   | PNG_TRANSFORM_PACKSWAP
                   | PNG_TRANSFORM_EXPAND
                   | PNG_TRANSFORM_SHIFT;
    int channels;
    int bytes;
    png_bytepp row_pointers = NULL;
    int rc = 0;

    if (fp == NULL)
      return -1;

    (*data) = NULL;

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (png_ptr == NULL) goto err_nomem;

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) goto err_nomem;

    end_info = png_create_info_struct(png_ptr);
    if (end_info == NULL) goto err_nomem;

    if (setjmp(png_jmpbuf(png_ptr))) goto err_corrupt;
    png_init_io(png_ptr, fp);
    png_read_png(png_ptr, info_ptr, transforms, NULL);

    *w = png_get_image_width(png_ptr, info_ptr);
    *h = png_get_image_height(png_ptr, info_ptr);
    channels = png_get_channels(png_ptr, info_ptr);
    bytes = png_get_rowbytes(png_ptr, info_ptr);

    if (bytes != channels*(*w))
      goto err_unsupported;

    row_pointers = png_get_rows(png_ptr, info_ptr);
    if (row_pointers == NULL) goto err_nomem;

    (*data) = malloc(*w * *h * sizeof **data);
    if (*data == NULL) goto err_nomem;

    if (channels == 1) {
        int j;
        for (j=0; j < *h; ++j) {
            int i;
            unsigned char (*imrow)[3] = (*data) + j*(*w);
            for (i=0; i < *w; ++i)
              memset(imrow[i], row_pointers[j][i], 3);
        }
    }
    else if (channels == 3) {
        int j;
        for (j=0; j < *h; ++j) {
            unsigned char (*imrow)[3] = (*data) + j*(*w);
            memcpy(imrow, row_pointers[j], 3*(*w));
        }
    }
    else goto err_unsupported;

go_return:
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    fclose(fp);
    return rc;

err_corrupt:
    rc = -2;
    free(data);
    goto go_return;
err_nomem:
    rc = -3;
    free(data);
    goto go_return;
err_unsupported:
    rc = -4;
    free(data);
    goto go_return;
}


int WritePNG(const char *fname, unsigned char (*data)[3], int w, int h)
{
    FILE *fp = fopen(fname, "wb");
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_bytep *row_pointers = NULL;
    int j;

    if (fp == NULL)
      return -1;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (png_ptr == NULL)
      goto err_png;

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
      goto err_png;

    if (setjmp(png_jmpbuf(png_ptr)))
      goto err_png;

    png_init_io(png_ptr, fp);

    png_set_IHDR(png_ptr, info_ptr, w, h,
                 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_ptr, info_ptr);

    row_pointers = png_malloc(png_ptr, h * sizeof *row_pointers);
    for (j=0; j < h; ++j)
      row_pointers[j] = (png_byte*)data[j*w+0];

    png_write_image(png_ptr, row_pointers);
    png_free(png_ptr, row_pointers);

    png_write_end(png_ptr, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return 0;

err_png:
    if (row_pointers) png_free(png_ptr, row_pointers);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return -2;
}
