#include <stdio.h>
#include <stdlib.h>
#include "image_loader.h"
#include <png.h>
#include <jpeglib.h>
#include <jerror.h>

int ImageLoader_LoadPNG(FILE *fp, SongImage *img){

    unsigned char header[8];
    fread(header, 1, 8, fp);
    int ispng = !png_sig_cmp(header, 0, 8);
    if(!ispng) return 0;

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png_ptr) return 0;

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if(!info_ptr){
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return 0;
    }

    if(setjmp(png_jmpbuf(png_ptr))){
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return 0;
    }

    png_set_sig_bytes(png_ptr, 8);
    png_init_io(png_ptr, fp);
    png_read_info(png_ptr, info_ptr);

    int bit_depth, color_type;
    png_uint_32 twidth, theight;

    png_get_IHDR(png_ptr, info_ptr, &twidth, &theight, &bit_depth, &color_type, NULL, NULL, NULL);

    if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);

    if(color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);

    png_get_IHDR(png_ptr, info_ptr, &twidth, &theight, &bit_depth, &color_type, NULL, NULL, NULL);

    if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);

    if(bit_depth < 8)
        png_set_packing(png_ptr);

    if(color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY)
        png_set_add_alpha(png_ptr, 255, PNG_FILLER_AFTER);

    png_get_IHDR(png_ptr, info_ptr, &twidth, &theight, &bit_depth, &color_type, NULL, NULL, NULL);

    img->width = twidth;
    img->height = theight;

    png_read_update_info(png_ptr, info_ptr);

    int rowbytes = png_get_rowbytes(png_ptr, info_ptr);

    png_byte *image_data = (png_byte *)malloc(sizeof(png_byte) * rowbytes * img->height);
    if(!image_data){
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        free(img);
        return 0;
    }

    png_bytep *row_pointers = (png_bytep *) malloc(sizeof(png_bytep) * img->height);
    if(!row_pointers){
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        free(image_data);
        free(img);
        return 0;
    }

    int k;
    for( k = 0; k < img->height; ++k)
        row_pointers[img->height - 1 - k] = &image_data[(img->height - 1 - k) * rowbytes];

    png_read_image(png_ptr, row_pointers);
    png_read_end(png_ptr, NULL);

    img->pixels = (char *)image_data;
    img->channels = 4;

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    if(row_pointers) free(row_pointers);

    return 1;
}

int ImageLoader_LoadJPEG(FILE *fp, SongImage *img){
    struct jpeg_decompress_struct info;
    struct jpeg_error_mgr jerror;
    jmp_buf jmp_buffer;
    int err = 0;


    info.err = jpeg_std_error(&jerror);

    void func(j_common_ptr cinfo){
        err = 1;
        longjmp(jmp_buffer, 1);
    }

    info.err->error_exit = func;
    if(setjmp(jmp_buffer)){
        jpeg_destroy_decompress(&info);
        return 0;
    }

    if(err) {
        jpeg_finish_decompress(&info);
        jpeg_destroy_decompress(&info);
        return 0;
    }

    jpeg_create_decompress(&info);
    jpeg_stdio_src(&info, fp);
    jpeg_read_header(&info, TRUE);
    jpeg_start_decompress(&info);

    img->pixels = NULL;
    img->width =  info.output_width;
    img->height = info.output_height;
    img->channels = info.num_components;

    int data_size = img->width * img->height * img->channels;
    img->pixels = (char *)malloc(sizeof(char) * data_size);
    char *rowptr[1];
    while(info.output_scanline < info.output_height){
        rowptr[0] = (char *)img->pixels + 3 * info.output_width * info.output_scanline;
        jpeg_read_scanlines(&info, (JSAMPARRAY)rowptr, 1);
    }

    jpeg_finish_decompress(&info);
    jpeg_destroy_decompress(&info);

    return 1;
}