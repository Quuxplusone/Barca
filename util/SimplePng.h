#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int ReadPNG(const char *fname, unsigned char (**data)[3], int *w, int *h);

int WritePNG(const char *fname, unsigned char (*data)[3], int w, int h);

#ifdef __cplusplus
}
#endif
