#ifndef PICLIB_H
#define PICLIB_H

#include "Picture.h"
#include "Utils.h"
  
  // picture transformation routines
  void invert_picture(struct picture *pic);
  void grayscale_picture(struct picture *pic);
  void rotate_picture(struct picture *pic, int angle);
  void flip_picture(struct picture *pic, char plane);
  void blur_picture(struct picture *pic);
  void parallel_blur_picture(struct picture *pic);
  void parallel_row_blur_picture(struct picture *pic);
  void parallel_column_blur_picture(struct picture *pic);
  void parallel_half_sector_blur_picture(struct picture *pic);
  void parallel_quarter_sector_blur_picture(struct picture *pic);

#endif

