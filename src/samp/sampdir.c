#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <utime.h>
#include <malloc.h>
#include <inttypes.h>
#include <db.h>
#include <mangooapp.h>
#include <mangoostore.h>
#include "../stores/dbfs/dbfs.h"

static struct mangoo_store *ms;
struct mangoo_handle *handle = NULL;
char *path="/newdir";
int error;

struct my_dir
{
  struct mangoo_handle *handle;
  off_t cur_off;
  off_t starting_off;
  char *saved_name;
  off_t saved_off;
  enum mangoo_types saved_type;
  mdir_filler_t *filler;
  void *buf;
};


int my_filler(void *fhandle,const char *name,enum mangoo_types type)
{
  struct my_dir *dir = (struct my_dir*)fhandle;
	struct stat stbuf;
  printf("OPEN MY_FILLER..!!!\n");
  printf("my_filler: %s %d\n",name,(int)type);

  printf("my_filler:1 %s %d\n", name, (int)type);
  stbuf.st_ino = 0;
  printf("my_filler:2 cur_off %lu starting_off %lu\n", dir->cur_off, dir->starting_off);
  if (dir->cur_off >= dir->starting_off) {
    printf("my_filler:3 %s\n", name);
    if (dir->filler(dir->buf, name, type)) {
      printf("my_filler:4\n");
      dir->saved_name = strdup(name);
      dir->saved_type = type;
      dir->saved_off = dir->cur_off;
      return ENOBUFS;
    }
  }


  dir->cur_off++;
  return 0;
}

int main()
{
  int err;
  off_t size;
  size_t sz;
  mode_t mode = 0x81B4;
  enum mangoo_types mt;
  char *url="dbfs\0";
  char *rl="/tmp/dbfs_vol\0";
  off_t offset=0;
  DB_ENV *dbenv;
  struct my_dir *dir;
  void *buf;
  if(err=mangoo_initstore(url,rl,&ms))
    printf("Error in store\n");
  else
    printf("store opened\n");
  if(err=mangoo_open(ms,path,mangoo_stream_interface,&handle))
    {
      printf("Error Opening %d\n",err);
    }
  else
    {
      printf("OPEN\n");
      //env_open(&dbenv);
      offset=0;
      dir=(struct my_dir *)handle;
      dir->handle=handle;
      dir->buf=buf;
         /*if(err=dir->filler(handle,"/newdir",mangoo_directory_interface))
	{
	  printf("Error Filler\n");
	  exit(1);
	  }*/
      //dir->filler=0;
      printf("Saved data : %s %d\n",dir->saved_name,dir->saved_type);
      printf("Read directory\n");
      dir->saved_name="newdir";
      if(dir->saved_name)
	{
	  printf("First If\n");
	  my_filler(dir,dir->saved_name,dir->saved_type);
	  offset++;
	  //free(dir->saved_name);
	  dir->saved_name=NULL;
	}
       dir->starting_off=0;
      printf("Mdir_read starts\n");
      error = mdir_read(dir->handle,my_filler,dir);
      if(error == ENOBUFS)
	printf("Error occurred..!!\n");
      printf("saved name %s",dir->saved_name);
      printf("error number %d\n",error);
      (void)mangoo_close(handle);
    }
  mangoo_closestore(ms);
return 0;
}
