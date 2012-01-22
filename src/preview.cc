#include <cstdlib>
#include <string>
#include <gphoto2/gphoto2-camera.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#define OLDLIB

#ifdef OLDLIB
static void error_func (GPContext *context, const char *format, va_list args, void *data) {
    fprintf  (stdout, "\n");
    fprintf  (stdout, "*** Contexterror ***              \n");
    vfprintf (stdout, format, args);
    fprintf  (stdout, "\n");
    fflush   (stdout);
}
#else
static void
error_func (GPContext *context, const char *str, void *data)
{
	printf ("### %s\n", str);
}
#endif
int main(){
  CameraFile *file;
  Camera *camera;
  GPContext *context = gp_context_new();
  
  gp_context_set_error_func (context, error_func, NULL);
	gp_context_set_status_func (context, error_func, NULL);
	
  int fd,ret;
  ret = gp_camera_new(&camera);
  if(ret < GP_OK) return -1;
  ret = gp_camera_init (camera, context);
  if(ret < GP_OK) return -1;
  const char* data;
  size_t length;
  time_t last=time(NULL);
  int cnt = 0;
  char* filename = (char*)malloc(128);
  for(unsigned int i=0; i<20000; i++) {
    time_t now = time(NULL);
    if(now-last >= 1){
      for(int j=0; j<i/20000.0*80.0;j++){
        printf("|");
      }
      printf("\n%.02f%%\n%d fps\033[2A\r",i/20000.0*100.0, cnt);
      fflush(stdout);
      cnt = 0;
    }else{
      cnt++;
    }
    last = now;
    sprintf(filename, "./frames/frame%.05d.jpg", i);
    fd = open(filename,O_WRONLY|O_CREAT,0660);
    ret = gp_file_new_from_fd (&file, fd);
//    ret = gp_file_new(&file);
    if(ret< GP_OK){ printf("gp_file_new returned %d\n", ret); break;}
    ret = gp_camera_capture_preview(camera, file, context);
    if(ret< GP_OK){ printf("gp_camera_capture_preview returned %d\n", ret); break;}
//    ret = gp_file_get_data_and_size(file, &data, &length);
    if(ret< GP_OK){ printf("gp_file_get_data_and_size returned %d\n", ret); break;}
//    if((data[0] & 0xFF) != 0xFF || (data[1] & 0xD8) != 0xD8){printf("invalid JPEG header!\n");break;}
    gp_file_free(file);    
    close(fd);
  }
  free(filename);
  gp_camera_exit(camera, context);
  gp_camera_free(camera);
  printf("\n\n\n");
  return 0;
}