#include <pthread.h>
#include "PicProcess.h"
#include "ThreadPool.h"

  #define NO_RGB_COMPONENTS 3
  #define BLUR_REGION_SIZE 9
  #define THREAD_POOL_DEFAULT_THREADS 16

  static void blur_individual_pixel(struct picture *pic, struct picture *tmp, int i, int j);

  void invert_picture(struct picture *pic){
    // iterate over each pixel in the picture
    for(int i = 0 ; i < pic->width; i++){
      for(int j = 0 ; j < pic->height; j++){
        struct pixel rgb = get_pixel(pic, i, j);
        
        // invert RGB values of pixel
        rgb.red = MAX_PIXEL_INTENSITY - rgb.red;
        rgb.green = MAX_PIXEL_INTENSITY - rgb.green;
        rgb.blue = MAX_PIXEL_INTENSITY - rgb.blue;
        
        // set pixel to inverted RBG values
        set_pixel(pic, i, j, &rgb);
      }
    }   
  }

  void grayscale_picture(struct picture *pic){
    // iterate over each pixel in the picture
    for(int i = 0 ; i < pic->width; i++){
      for(int j = 0 ; j < pic->height; j++){
        struct pixel rgb = get_pixel(pic, i, j);
        
        // compute gray average of pixel's RGB values
        int avg = (rgb.red + rgb.green + rgb.blue) / NO_RGB_COMPONENTS;
        rgb.red = avg;
        rgb.green = avg;
        rgb.blue = avg;
        
        // set pixel to gray-scale RBG value
        set_pixel(pic, i, j, &rgb);
      }
    }    
  }

  void rotate_picture(struct picture *pic, int angle){
    // make temporary copy of picture to work from
    struct picture tmp;
    tmp.img = copy_image(pic->img);
    tmp.width = pic->width;
    tmp.height = pic->height; 
  
    int new_width = tmp.width;
    int new_height = tmp.height;
  
    // adjust output picture size as necessary
    if(angle == 90 || angle == 270){
      new_width = tmp.height;
      new_height = tmp.width;
    }
    clear_picture(pic);
    init_picture_from_size(pic, new_width, new_height);
  
    // iterate over each pixel in the picture
    for(int i = 0 ; i < new_width; i++){
      for(int j = 0 ; j < new_height; j++){
        struct pixel rgb;
        // determine rotation angle and execute corresponding pixel update
        switch(angle){
          case(90):
            rgb = get_pixel(&tmp, j, new_width -1 - i); 
            break;
          case(180):
            rgb = get_pixel(&tmp, new_width - 1 - i, new_height - 1 - j);
            break;
          case(270):
            rgb = get_pixel(&tmp, new_height - 1 - j, i);
            break;
          default:
            printf("[!] rotate is undefined for angle %i (must be 90, 180 or 270)\n", angle);
            clear_picture(&tmp);
            exit(IO_ERROR);
        }
        set_pixel(pic, i,j, &rgb);
      }
    }
    
    // temporary picture clean-up
    clear_picture(&tmp);
  }

  void flip_picture(struct picture *pic, char plane){
    // make temporary copy of picture to work from
    struct picture tmp;
    tmp.img = copy_image(pic->img);
    tmp.width = pic->width;
    tmp.height = pic->height;  
    
    // iterate over each pixel in the picture
    for(int i = 0 ; i < tmp.width; i++){
      for(int j = 0 ; j < tmp.height; j++){    
        struct pixel rgb;
        // determine flip plane and execute corresponding pixel update
        switch(plane){
          case('V'):
            rgb = get_pixel(&tmp, i, tmp.height - 1 - j);
            break;
          case('H'):
            rgb = get_pixel(&tmp, tmp.width - 1 - i, j);
            break;
          default:
            printf("[!] flip is undefined for plane %c\n", plane);
            clear_picture(&tmp);
            exit(IO_ERROR);
        } 
        set_pixel(pic, i, j, &rgb);
      }
    }

    // temporary picture clean-up
    clear_picture(&tmp);
  }

  static void blur_individual_pixel(struct picture *pic, struct picture *tmp, int i, int j) {
    // set-up a local pixel on the stack
    struct pixel rgb;  
    int sum_red = 0;
    int sum_green = 0;
    int sum_blue = 0;
  
    // check the surrounding pixel region
    for(int n = -1; n <= 1; n++){
      for(int m = -1; m <= 1; m++){
        rgb = get_pixel(tmp, i+n, j+m);
        sum_red += rgb.red;
        sum_green += rgb.green;
        sum_blue += rgb.blue;
      }
    }
  
    // compute average pixel RGB value
    rgb.red = sum_red / BLUR_REGION_SIZE;
    rgb.green = sum_green / BLUR_REGION_SIZE;
    rgb.blue = sum_blue / BLUR_REGION_SIZE;
  
    // set pixel to region average RBG value
    set_pixel(pic, i, j, &rgb);
  }
  
  void blur_picture(struct picture *pic){
    // make temporary copy of picture to work from
    struct picture tmp;
    tmp.img = copy_image(pic->img);
    tmp.width = pic->width;
    tmp.height = pic->height;  
  
    // iterate over each pixel in the picture (ignoring boundary pixels)
    for(int i = 1 ; i < tmp.width - 1; i++){
      for(int j = 1 ; j < tmp.height - 1; j++){
        blur_individual_pixel(pic, &tmp, i, j);
      }
    }
    
    // temporary picture clean-up
    clear_picture(&tmp);
  }

  struct blur_pixel_args {
    struct picture *pic;
    struct picture *tmp;
    int i;
    int j;
  };

  void *thread_blur_pixel(void *vargs) {
    struct blur_pixel_args *args = (struct blur_pixel_args *) vargs;
    struct picture *pic = args->pic;
    struct picture *tmp = args->tmp;
    int i = args->i;
    int j = args->j;

    blur_individual_pixel(pic, tmp, i, j);
    
    free(vargs);
    return NULL;
  }
  
  /* Uses a thread pool to parallelise blurring pixel by pixel. */
  void parallel_blur_picture(struct picture *pic){
    // make temporary copy of picture to work from
    struct picture tmp;
    tmp.img = copy_image(pic->img);
    tmp.width = pic->width;
    tmp.height = pic->height; 

    thread_pool_t tpool;
    thread_pool_init(&tpool, THREAD_POOL_DEFAULT_THREADS, (pic->width - 2) * (pic->height - 2));

    // iterate over each pixel in the picture (ignoring boundary pixels)
    for(int i = 1 ; i < tmp.width - 1; i++){
      for(int j = 1 ; j < tmp.height - 1; j++){
        struct blur_pixel_args *args = malloc(sizeof(struct blur_pixel_args));
        args->pic = pic;
        args->tmp = &tmp;
        args->i = i;
        args->j = j;
        thread_pool_submit_job(&tpool, &thread_blur_pixel, args);
      }
    }

    thread_pool_run_and_wait(&tpool);
    thread_pool_destroy(&tpool);

    // temporary picture clean-up
    clear_picture(&tmp);
  }

  struct blur_row_args {
    struct picture *pic;
    struct picture *tmp;
    int j;
  };

  void *thread_blur_row(void *vargs) {
    struct blur_row_args *args = (struct blur_row_args *) vargs;
    struct picture *pic = args->pic;
    struct picture *tmp = args->tmp;
    int j = args->j;

    for(int i = 1; i < tmp->width - 1; i++) {
      blur_individual_pixel(pic, tmp, i, j);
    }

    free(vargs);
    return NULL;
  }

  /* Uses a thread pool to parallelise blurring row by row. */
  void parallel_row_blur_picture(struct picture *pic){
    // make temporary copy of picture to work from
    struct picture tmp;
    tmp.img = copy_image(pic->img);
    tmp.width = pic->width;
    tmp.height = pic->height; 

    thread_pool_t tpool;
    thread_pool_init(&tpool, THREAD_POOL_DEFAULT_THREADS, pic->height - 2);

    // iterate over each row in the picture (ignoring boundary rows)
    for(int j = 1; j < tmp.height - 1; j++){
      struct blur_row_args *args = malloc(sizeof(struct blur_row_args));
      args->pic = pic;
      args->tmp = &tmp;
      args->j = j;
      thread_pool_submit_job(&tpool, &thread_blur_row, args);
    }

    thread_pool_run_and_wait(&tpool);
    thread_pool_destroy(&tpool);

    // temporary picture clean-up
    clear_picture(&tmp);
  }

  struct blur_column_args {
    struct picture *pic;
    struct picture *tmp;
    int i;
  };

  void *thread_blur_column(void *vargs) {
    struct blur_column_args *args = (struct blur_column_args *) vargs;
    struct picture *pic = args->pic;
    struct picture *tmp = args->tmp;
    int i = args->i;

    for(int j = 1; j < tmp->height - 1; j++) {
      blur_individual_pixel(pic, tmp, i, j);
    }

    free(vargs);
    return NULL;
  }

  /* Uses a thread pool to parallelise blurring column by column. */
  void parallel_column_blur_picture(struct picture *pic){
    // make temporary copy of picture to work from
    struct picture tmp;
    tmp.img = copy_image(pic->img);
    tmp.width = pic->width;
    tmp.height = pic->height; 

    thread_pool_t tpool;
    thread_pool_init(&tpool, THREAD_POOL_DEFAULT_THREADS, pic->width - 2);

    // iterate over each row in the picture (ignoring boundary rows)
    for(int i = 1; i < tmp.width - 1; i++){
      struct blur_column_args *args = malloc(sizeof(struct blur_column_args));
      args->pic = pic;
      args->tmp = &tmp;
      args->i = i;
      thread_pool_submit_job(&tpool, &thread_blur_column, args);
    }

    thread_pool_run_and_wait(&tpool);
    thread_pool_destroy(&tpool);

    // temporary picture clean-up
    clear_picture(&tmp);
  }
  
  struct blur_segment_args {
    struct picture *pic;
    struct picture *tmp;
    int start_i;
    int end_i;
    int start_j;
    int end_j;
  };

  void *thread_blur_segment(void *vargs) {
    struct blur_segment_args *args = (struct blur_segment_args *) vargs;
    struct picture *pic = args->pic;
    struct picture *tmp = args->tmp;
    int start_i = args->start_i;
    int end_i = args->end_i;
    int start_j = args->start_j;
    int end_j = args->end_j;

    for(int i = start_i; i < end_j; i++){
      for(int j = start_j; j < end_j; j++){
        blur_individual_pixel(pic, tmp, i, j);
      }
    }

    free(vargs);
    return NULL;
  }

  /* Uses a thread pool to parallelise blurring half of the picture per thread. */
  void parallel_half_segment_blur_picture(struct picture *pic){
    // make temporary copy of picture to work from
    struct picture tmp;
    tmp.img = copy_image(pic->img);
    tmp.width = pic->width;
    tmp.height = pic->height; 

    thread_pool_t tpool;
    thread_pool_init(&tpool, THREAD_POOL_DEFAULT_THREADS, pic->width - 2);

    /* Prepare and submit job for left side of the image. */
    struct blur_segment_args *left_args = malloc(sizeof(struct blur_segment_args));
    left_args->pic = pic;
    left_args->tmp = &tmp;
    left_args->start_i = 1;
    left_args->end_i = (tmp.width - 1) / 2;
    left_args->start_j = 1;
    left_args->end_j = tmp.height - 1;

    thread_pool_submit_job(&tpool, &thread_blur_segment, left_args);

    /* Prepare and submit job for right side of the image. */
    struct blur_segment_args *right_args = malloc(sizeof(struct blur_segment_args));
    *right_args = *left_args;
    right_args->start_i = (tmp.width - 1) / 2;
    right_args->end_i = tmp.width - 1;

    thread_pool_submit_job(&tpool, &thread_blur_segment, right_args);

    thread_pool_run_and_wait(&tpool);
    thread_pool_destroy(&tpool);

    // temporary picture clean-up
    clear_picture(&tmp);
  }