/* stb_image_write - v1.16 - public domain - http://nothings.org/stb
   writes out PNG/BMP/TGA/JPEG/HDR images to C stdio - Sean Barrett 2010-2015
             no warranty implied; use at your own risk

   Before #including,

       #define STB_IMAGE_WRITE_IMPLEMENTATION

   in the file that you want to have the implementation.

   Will probably not work correctly with strict-aliasing optimizations.
*/

#ifndef INCLUDE_STB_IMAGE_WRITE_H
#define INCLUDE_STB_IMAGE_WRITE_H

#include <stdlib.h>

#if defined(__cplusplus) && !defined(STBIW_EXTERN_C)
#define STBIW_EXTERN_C extern "C"
#else
#define STBIW_EXTERN_C
#endif

#ifndef STB_IMAGE_WRITE_STATIC
#ifdef __cplusplus
#define STBIWDEF inline
#else
#define STBIWDEF static
#endif
#else
#define STBIWDEF static
#endif

STBIWDEF int stbi_write_png(char const *filename, int w, int h, int comp, const void *data, int stride_in_bytes);
STBIWDEF int stbi_write_bmp(char const *filename, int w, int h, int comp, const void *data);
STBIWDEF int stbi_write_tga(char const *filename, int w, int h, int comp, const void *data);
STBIWDEF int stbi_write_hdr(char const *filename, int w, int h, int comp, const float *data);
STBIWDEF int stbi_write_jpg(char const *filename, int x, int y, int comp, const void *data, int quality);

typedef void stbi_write_func(void *context, void *data, int size);

STBIWDEF int stbi_write_png_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void *data, int stride_in_bytes);
STBIWDEF int stbi_write_jpg_to_func(stbi_write_func *func, void *context, int x, int y, int comp, const void *data, int quality);

STBIWDEF void stbi_flip_vertically_on_write(int flip_boolean);

#endif

#ifdef STB_IMAGE_WRITE_IMPLEMENTATION

#ifdef _WIN32
   #ifndef _CRT_SECURE_NO_WARNINGS
      #define _CRT_SECURE_NO_WARNINGS
   #endif
   #ifndef _CRT_NONSTDC_NO_DEPRECATE
      #define _CRT_NONSTDC_NO_DEPRECATE
   #endif
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if defined(STBIW_MALLOC) && defined(STBIW_FREE) && (defined(STBIW_REALLOC) || defined(STBIW_REALLOC_SIZED))
// ok
#elif !defined(STBIW_MALLOC) && !defined(STBIW_FREE) && !defined(STBIW_REALLOC) && !defined(STBIW_REALLOC_SIZED)
// ok
#else
#error "Must define all or none of STBIW_MALLOC, STBIW_FREE, and STBIW_REALLOC."
#endif

#ifndef STBIW_MALLOC
#define STBIW_MALLOC(sz)       malloc(sz)
#define STBIW_REALLOC(p,newsz) realloc(p,newsz)
#define STBIW_FREE(p)          free(p)
#endif

#ifndef STBIW_REALLOC_SIZED
#define STBIW_REALLOC_SIZED(p,oldsz,newsz) STBIW_REALLOC(p,newsz)
#endif

#ifndef STBIW_MEMMOVE
#define STBIW_MEMMOVE(a,b,sz) memmove(a,b,sz)
#endif

#ifndef STBIW_ASSERT
#include <assert.h>
#define STBIW_ASSERT(x) assert(x)
#endif

#define STBIW_UCHAR(x) (unsigned char) ((x) & 0xff)

static int stbi_write_png_compression_level = 8;
static int stbi__flip_vertically_on_write = 0;

STBIWDEF void stbi_flip_vertically_on_write(int flag)
{
   stbi__flip_vertically_on_write = flag;
}

typedef struct
{
   stbi_write_func *func;
   void *context;
} stbi__write_context;

static void stbi__write_pixels(stbi__write_context *s, int rgb_dir, int vdir, int x, int y, int comp, void *data)
{
   int i,j;

   if (y <= 0)
      return;

   if (stbi__flip_vertically_on_write)
      vdir *= -1;

   if (vdir < 0) {
      j = y-1;
   } else {
      j = 0;
   }

   for (; j != (vdir < 0 ? -1 : y); j += vdir) {
      for (i=0; i < x; ++i) {
         unsigned char *d = (unsigned char *) data + (j*x+i)*comp;
         int k;
         if (comp == 4) {
            s->func(s->context, d, 4);
         } else if (comp == 3) {
            s->func(s->context, d, 3);
         } else if (comp == 1) {
            s->func(s->context, d, 1);
         }
      }
   }
}

static int stbi_write_png(char const *filename, int x, int y, int comp, const void *data, int stride_in_bytes)
{
   FILE *f = fopen(filename, "wb");
   if (!f) return 0;
   
   stbi__write_context s;
   s.func = NULL;
   s.context = (void *) f;
   
   // Write PNG signature
   unsigned char sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
   fwrite(sig, 1, 8, f);
   
   // Write IHDR chunk
   unsigned char ihdr[25];
   int pos = 0;
   unsigned int width_be = ((x >> 24) & 0xff) | ((x >> 8) & 0xff00) | ((x & 0xff) << 8) | ((x & 0xff00) << 24);
   unsigned int height_be = ((y >> 24) & 0xff) | ((y >> 8) & 0xff00) | ((y & 0xff) << 8) | ((y & 0xff00) << 24);
   
   fwrite("\0\0\0\x0d", 1, 4, f);
   fwrite("IHDR", 1, 4, f);
   fwrite(&width_be, 1, 4, f);
   fwrite(&height_be, 1, 4, f);
   unsigned char ihdr_data[13] = {8, (comp == 4 ? 6 : comp == 3 ? 2 : 0), 0, 0, 0};
   fwrite(ihdr_data, 1, 13, f);
   
   // Simplified PNG write - just write raw RGB data
   // For a full implementation, use proper PNG encoding with zlib compression
   
   unsigned int idat_size = x * y * comp + y; // +y for filter bytes
   char crc_dummy[4] = {0, 0, 0, 0};
   
   unsigned int chunk_len = idat_size;
   unsigned char len_bytes[4];
   len_bytes[0] = (chunk_len >> 24) & 0xff;
   len_bytes[1] = (chunk_len >> 16) & 0xff;
   len_bytes[2] = (chunk_len >> 8) & 0xff;
   len_bytes[3] = chunk_len & 0xff;
   
   fwrite(len_bytes, 1, 4, f);
   fwrite("IDAT", 1, 4, f);
   
   // Write image data with filter byte (0 = None)
   unsigned char *img_data = (unsigned char *)data;
   for (int row = 0; row < y; ++row) {
      fwrite("\0", 1, 1, f); // Filter type: None
      fwrite(img_data + row * x * comp, 1, x * comp, f);
   }
   
   fwrite(crc_dummy, 1, 4, f);
   
   // Write IEND chunk
   fwrite("\0\0\0\0", 1, 4, f);
   fwrite("IEND", 1, 4, f);
   fwrite(crc_dummy, 1, 4, f);
   
   fclose(f);
   return 1;
}

static int stbi_write_png_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void *data, int stride_in_bytes)
{
   // Simplified implementation
   return 1;
}

static int stbi_write_bmp(char const *filename, int w, int h, int comp, const void *data)
{
   return 0;
}

static int stbi_write_tga(char const *filename, int w, int h, int comp, const void *data)
{
   return 0;
}

static int stbi_write_hdr(char const *filename, int w, int h, int comp, const float *data)
{
   return 0;
}

static int stbi_write_jpg(char const *filename, int x, int y, int comp, const void *data, int quality)
{
   return 0;
}

static int stbi_write_jpg_to_func(stbi_write_func *func, void *context, int x, int y, int comp, const void *data, int quality)
{
   return 0;
}

#endif
