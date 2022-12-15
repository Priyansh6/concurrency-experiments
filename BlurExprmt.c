#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "Utils.h"
#include "Picture.h"
#include "PicProcess.h"

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
    
    if (!init_picture_from_file(&pic, pic_path)) {
      return EXIT_FAILURE;
    }
    parallel_blur_picture(&pic);
    if (!save_picture_to_file(&pic, save_path)) {
      return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
  }
