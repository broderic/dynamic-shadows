/*
 * example.c
 *
 * This file illustrates how to use the IJG code as a subroutine library
 * to read or write JPEG image files.  You should look at this code in
 * conjunction with the documentation file libjpeg.doc.
 *
 * This code will not do anything useful as-is, but it may be helpful as a
 * skeleton for constructing routines that call the JPEG library.  
 *
 * We present these routines in the same coding style used in the JPEG code
 * (ANSI function definitions, etc); but you are of course free to code your
 * routines in a different style if you prefer.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * Include file for users of JPEG library.
 * You will need to have included system headers that define at least
 * the typedefs FILE and size_t before you can include jpeglib.h.
 * (stdio.h is sufficient on ANSI-conforming systems.)
 * You may also wish to include "jerror.h".
 */

// FIXME: need to do this because mingw doesn't have a jpeglib.h file
#ifdef CYGWIN 
#include "jpeglib.h"
#include "jerror.h"
#else
#include <jpeglib.h>
#include <jerror.h>
#endif

/*
 * <setjmp.h> is used for the optional error recovery mechanism shown in
 * the second part of the example.
 */

#include <setjmp.h>


/*
 * IMAGE DATA FORMATS:
 *
 * The standard input image format is a rectangular array of pixels, with
 * each pixel having the same number of "component" values (color channels).
 * Each pixel row is an array of JSAMPLEs (which typically are unsigned chars).
 * If you are working with color data, then the color values for each pixel
 * must be adjacent in the row; for example, R,G,B,R,G,B,R,G,B,... for 24-bit
 * RGB color.
 *
 * For this example, we'll assume that this data structure matches the way
 * our application has stored the image in memory, so we can just pass a
 * pointer to our image buffer.  In particular, let's say that the image is
 * RGB color and is described by:
 */



/******************** JPEG DECOMPRESSION SAMPLE INTERFACE *******************/

/* This half of the example shows how to read data from the JPEG decompressor.
 * It's a bit more refined than the above, in that we show:
 *   (a) how to modify the JPEG library's standard error-reporting behavior;
 *   (b) how to allocate workspace using the library's memory manager.
 *
 * Just to make this example a little different from the first one, we'll
 * assume that we do not intend to put the whole image into an in-memory
 * buffer, but to send it line-by-line someplace else.  We need a one-
 * scanline-high JSAMPLE array as a work buffer, and we will let the JPEG
 * memory manager allocate it for us.  This approach is actually quite useful
 * because we don't need to remember to deallocate the buffer separately: it
 * will go away automatically when the JPEG object is cleaned up.
 */


/*
 * ERROR HANDLING:
 *
 * The JPEG library's standard error handler (jerror.c) is divided into
 * several "methods" which you can override individually.  This lets you
 * adjust the behavior without duplicating a lot of code, which you might
 * have to update with each future release.
 *
 * Our example here shows how to override the "error_exit" method so that
 * control is returned to the library's caller when a fatal error occurs,
 * rather than calling exit() as the standard error_exit method does.
 *
 * We use C's setjmp/longjmp facility to return control.  This means that the
 * routine which calls the JPEG library must first execute a setjmp() call to
 * establish the return point.  We want the replacement error_exit to do a
 * longjmp().  But we need to make the setjmp buffer accessible to the
 * error_exit routine.  To do this, we make a private extension of the
 * standard JPEG error handler object.  (If we were using C++, we'd say we
 * were making a subclass of the regular error handler.)
 *
 * Here's the extended error handler struct:
 */

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}


/****************************************************************************
  Load a JPEG
****************************************************************************/
unsigned char* loadJPEG (char * filename, int *image_width, 
			 int *image_height, int *image_comp)
{
  struct jpeg_decompress_struct cinfo;
  struct my_error_mgr jerr;
  FILE * infile;		/* source file */
  JSAMPARRAY buffer;		/* Output row buffer */
  unsigned char *image_buffer;  /* output image buffer */
  int row_stride;		/* physical row width in output buffer */
  int current_offset = 0;

  /* In this example we want to open the input file before doing anything else,
   * so that the setjmp() error recovery below can assume the file is open.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to read binary files.
   */
  if ((infile = fopen(filename, "rb")) == NULL) {
	  //FIXME: __imp___iob if we use the below.  Cygwin doesn't like. 
	  //fprintf(stderr, "can't open %s\n", filename);
	  printf("can't open %s\n", filename);
	  return NULL;
  }

  /* Step 1: allocate and initialize JPEG decompression object */

  /* We set up the normal JPEG error routines, then override error_exit. */
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;
  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp(jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error.
     * We need to clean up the JPEG object, close the input file, and return.
     */
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    return NULL;
  }
  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress(&cinfo);

  /* Step 2: specify data source (eg, a file) */
  jpeg_stdio_src(&cinfo, infile);

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header(&cinfo, TRUE);
  /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.doc for more info.
   */

  /* Step 4: set parameters for decompression */

  /* Step 5: Start decompressor */
  (void) jpeg_start_decompress(&cinfo);

  /* JSAMPLEs per row in output buffer */
  *image_width = cinfo.output_width;
  *image_height = cinfo.output_height;
  *image_comp = cinfo.output_components;
  image_buffer = (unsigned char *) malloc(*image_height * *image_width * 
					  cinfo.output_components);
  if (!image_buffer) {
	  printf("read_jpeg: not enough memory!\n");
	  exit(1);
  }

  row_stride = cinfo.output_width * cinfo.output_components;

  /* Make a one-row-high sample array that will go away when done with image */
  buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

  /* Step 6: while (scan lines remain to be read) */
  //current_offset = (cinfo.output_height-1)*row_stride;
  while (cinfo.output_scanline < cinfo.output_height) {
    jpeg_read_scanlines(&cinfo, buffer, 1);
    memcpy(image_buffer + current_offset, *buffer, row_stride);
    current_offset += row_stride;
    //current_offset -= row_stride;
  }

  /* Step 7: Finish decompression */
  (void) jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  fclose(infile);

  /* And we're done! */
  return image_buffer;
}


/****************************************************************************
  Read TGA files. Taken from:
     http://steinsoft.net/index.php?site=Programming/Code%20Snippets/Cpp/no8
****************************************************************************/

unsigned char tbuffer[1<<16];

unsigned char* loadTGA (char * filename, int *image_width, 
			int *image_height, int *image_comp)
{
   FILE *file;
   unsigned char badChar, imageTypeCode, bitCount, *imageData;
   short int badInt, imageWidth, imageHeight;
   long	imageSize;
   int	colorMode;

   file = fopen(filename, "rb");
   if (!file) {
	   printf("loadTGA: couldn't load '%s'\n", filename);
	   return false;
   }
 
   fread(&badChar, sizeof(unsigned char), 1, file);
   fread(&badChar, sizeof(unsigned char), 1, file);
 
   fread(&imageTypeCode, sizeof(unsigned char), 1, file);
 
   //image type either 2 (color) or 3 (greyscale)
   if ((imageTypeCode != 2) && (imageTypeCode != 3))
   {
	   fclose(file);
	   return NULL;
   }
 
   //13 bytes of useless data
   fread(&badInt, sizeof(short int), 1, file);
   fread(&badInt, sizeof(short int), 1, file);
   fread(&badChar, sizeof(unsigned char), 1, file);
   fread(&badInt, sizeof(short int), 1, file);
   fread(&badInt, sizeof(short int), 1, file);
 
   //image dimensions
   fread(&imageWidth, sizeof(short int), 1, file);
   fread(&imageHeight, sizeof(short int), 1, file);
   *image_width = imageWidth;
   *image_height = imageHeight;

   //image bit depth
   fread(&bitCount, sizeof(unsigned char), 1, file);
   if (bitCount != 24 && bitCount != 32) {
	   printf("Bad bitcount (%d)\n", bitCount);
	   fclose(file);
	   return NULL;
   }
 
   //1 byte of garbage data
   fread(&badChar, sizeof(unsigned char), 1, file);
 
   //colorMode -> 3 = BGR, 4 = BGRA 
   colorMode = bitCount / 8;
   *image_comp = colorMode;
   imageSize = imageWidth * imageHeight * colorMode;
 
   //allocate memory for image data
   imageData = (unsigned char*) malloc (imageSize);
   if (!imageData) {
	   printf("Insufficient memory for image! (%ld bytes)\n", imageSize);
	   fclose(file);
	   return NULL;
   }
 
   //read in image data
   if (fread(imageData, sizeof(unsigned char), imageSize, file) != (unsigned)imageSize) {
	   printf("Failed to read the whole file!\n");
	   fclose(file);
	   return NULL;
   }
 
   //swap blue and red colour value 
   for (int i = 0; i < imageSize; i += colorMode)
   {
      imageData[i] ^= imageData[i+2] ^= imageData[i] ^= imageData[i+2];
   }
 
   // flip vertically
   int stride = imageWidth * colorMode;
   for (int y = 0; y < imageHeight/2; y++) {
	   memcpy(tbuffer, &imageData[y*stride], stride);
	   memcpy(&imageData[y*stride], &imageData[(imageHeight-y-1)*stride], stride);
	   memcpy(&imageData[(imageHeight-y-1)*stride], tbuffer, stride);
   }

   fclose(file);
   return imageData;
}
