#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "Utils.h"
#include "Picture.h"
#include "PicProcess.h"

#define NUM_TEST_RUNS 200
#define BILLION 1000000000
#define PRINT_TIME(label, time) printf("%s Time Taken: %lu.%09lus\n", label, time / BILLION, time % BILLION)

typedef void blur_func(struct picture *pic);

static bool test_blur_func(blur_func func, char *pic_path, char *save_path, char *label, bool save);

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
    
    test_blur_func(&blur_picture, pic_path, save_path, "Sequential", false);
    test_blur_func(&parallel_blur_picture, pic_path, save_path, "Pixel by pixel", false);
    test_blur_func(&parallel_row_blur_picture, pic_path, save_path, "Row by row", false);
    test_blur_func(&parallel_column_blur_picture, pic_path, save_path, "Column by column", false);
    test_blur_func(&parallel_v_half_sector_blur_picture, pic_path, save_path, "Vertical half segments", false);
    test_blur_func(&parallel_h_half_sector_blur_picture, pic_path, save_path, "Horizontal half segments", false);
    test_blur_func(&parallel_quarter_sector_blur_picture, pic_path, save_path, "Quarter segments", false);
    
    return EXIT_SUCCESS;
  }

  /* Creates and tests the picture at the provided pic_path with the provided blur function
     called label. The save boolean determines whether you want the picture to be saved.
     Returns whether loading and saving the picture is successful. */
  static bool test_blur_func(blur_func func, char *pic_path, char *save_path, char *label, bool save) {
    struct picture pic;

    /* Load image from path */
    if (!init_picture_from_file(&pic, pic_path)) {
      return false;
    } 

    struct timespec start;
    struct timespec end;

    u_int64_t avg_time_ns = 0;
    u_int64_t min_time_ns = 0;
    u_int64_t max_time_ns = 0;
    u_int64_t first_time_ns = 0;

    for (int i = 0; i < NUM_TEST_RUNS; i++) {
      struct picture tmp_pic;
      tmp_pic.img = copy_image(pic.img);
      tmp_pic.width = pic.width;
      tmp_pic.height = pic.height;
      
      clock_gettime (CLOCK_MONOTONIC, &start);
      func(&tmp_pic);
      clock_gettime (CLOCK_MONOTONIC, &end);

      /* Calculate how long it took to blur the image. */
      u_int64_t diff_ns = BILLION * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec; 

      if (i == 0) {
        first_time_ns = diff_ns;
        min_time_ns = diff_ns;
        max_time_ns = diff_ns;
      }

      if (i == NUM_TEST_RUNS - 1 && save) {
        clear_picture(&pic);
        pic.img = copy_image(tmp_pic.img);
      }

      avg_time_ns += diff_ns;
      min_time_ns = diff_ns < min_time_ns ? diff_ns : min_time_ns;
      max_time_ns = diff_ns > max_time_ns ? diff_ns : max_time_ns;

      clear_picture(&tmp_pic);
    }

    avg_time_ns /= NUM_TEST_RUNS;
    
    printf("%s:\n", label);
    PRINT_TIME("Average", avg_time_ns);
    PRINT_TIME("Minimum", min_time_ns);
    PRINT_TIME("Maximum", max_time_ns);
    PRINT_TIME("First", first_time_ns);
    printf("\n");

    if (save && !save_picture_to_file(&pic, save_path)) {
      return false;
    }

    return true;
  }
