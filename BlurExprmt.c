#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "Utils.h"
#include "Picture.h"
#include "PicProcess.h"

#define BILLION 1000000000 

typedef void blur_func(struct picture *pic);

static bool test_blur_func(blur_func func, char *pic_path, char *save_path, char *label);

// ---------- MAIN PROGRAM ---------- \\

  int main(int argc, char **argv){
    struct picture pic;
    char *pic_path = argv[1];

    /* Finds the file name of the picture and adds the prefix 
       "blur_" to it to save the modified picture as. */
    char tmp[strlen(pic_path) + 1];
    strncpy(tmp, pic_path, strlen(pic_path) + 1);

    char *file_name;
    strtok_r(tmp, "/", &file_name);

    char *prefix = "blur_";
    char save_path[strlen(prefix) + strlen(file_name) + 1];
    *save_path = 0;
    strncat(save_path, prefix, strlen(prefix));
    strncat(save_path, file_name, strlen(file_name));
    
    test_blur_func(&blur_picture, pic_path, save_path, "Sequential");
    test_blur_func(&parallel_blur_picture, pic_path, save_path, "Pixel by pixel");
    test_blur_func(&parallel_row_blur_picture, pic_path, save_path, "Row by row");
    test_blur_func(&parallel_column_blur_picture, pic_path, save_path, "Column by column");
    test_blur_func(&parallel_half_sector_blur_picture, pic_path, save_path, "Half segments");
    test_blur_func(&parallel_quarter_sector_blur_picture, pic_path, save_path, "Quarter segments");
    
    return EXIT_SUCCESS;
  }

  /* Creates and tests the picture at the provided pic_path with the provided blur function
     called label. Returns whether loading and saving the picture is successful. */
  static bool test_blur_func(blur_func func, char *pic_path, char *save_path, char *label) {
    struct picture pic;

    if (!init_picture_from_file(&pic, pic_path)) {
      return false;
    }

    struct timespec start;
    struct timespec end;
    clock_gettime (CLOCK_MONOTONIC, &start);

    func(&pic);

    clock_gettime (CLOCK_MONOTONIC, &end);
    u_int64_t diff = BILLION * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec; 

    printf("%s:\n", label);
    printf("Time Taken: %lu.%09lus\n\n", diff / BILLION, diff % BILLION);

    if (!save_picture_to_file(&pic, save_path)) {
      return false;
    }

    return true;
  }
