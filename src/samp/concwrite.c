#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <utime.h>
#include <malloc.h>
#include <inttypes.h>
#include <db.h>
#include <unistd.h>
#include <pthread.h>

#include <mangooapp.h>
#include <mangoostore.h>
#include "../stores/dbfs/dbfs.h"

static struct mangoo_store *ms;
struct mangoo_handle *handle = NULL;
struct dbfs_handle *dh;
DB_ENV *dbenv;

void env_dir_create()
{
  struct stat sb;
  char *path="/tmp/TXNAPP";

  /*
   * If the directory exists, we're done.  We do not further check
   * the type of the file, DB will fail appropriately if it's the
   * wrong type.
   */
  if(stat(path, &sb) == 0)
    return;


  /* Create the directory, read/write/access owner only. */
  if(mkdir(path, S_IRWXU) != 0) {
    fprintf(stderr,"txnapp: mkdir: %s: %s\n", path, strerror(errno));
    exit (1);
  }
}


void sampwrite(char *path,char *buf,size_t *sz,off_t *offset)
{
  int err;
  mode_t mode=0x81B4;
 back:
  if(err=mangoo_open(ms,path,mangoo_stream_interface,&handle))
    {
      printf("ERROR %d\n",err);
      if(err=mangoo_create(ms,path,mode,mangoo_data))
	{
	  printf("Error creating\n");
	  goto back;
	}
      else
	{
	  printf("File created\n");
	  goto back;
	}
    }
  else
    {
      printf("OPEN\n");
      dh = (struct dbfs_handle*)handle;
      printf("Size : %jd\n ",(intmax_t)(dh->attr_ddd.ddd_size));
      printf("Mode : %o\n ",(dh->attr_ddd.ddd_mode));
      *sz=strlen(buf)+1;
      env_open(&dbenv);
      if(err=mstream_write(handle,buf,sz,*offset))
	printf("Error while writing buf : %d\n",err);
      else
	*offset+=strlen(buf);      
      (void)dbenv->close(dbenv,0);
      (void)mangoo_close(handle);
    }
}

void sampread(char *path)
{
  off_t offset=0;
  size_t sz;
  char buffer[30];
  int err;
  if(err=mangoo_open(ms,path,mangoo_stream_interface,&handle))
    {
      printf("Error Opening %d\n",err);
    }
  else
    {
      printf("OPEN\n");
      dh = (struct dbfs_handle*)handle;
      printf("Size : %jd\n ",(intmax_t)(dh->attr_ddd.ddd_size));
      printf("Mode : %o\n ",(dh->attr_ddd.ddd_mode));
      offset=0;
      sz=(size_t)(dh->attr_ddd.ddd_size);
      env_open(&dbenv);
      if(err=mstream_read(handle,buffer,&sz,offset))
	printf("Error while reading %d\n",err);
      else
	puts(buffer);
      (void)dbenv->close(dbenv,0);
      (void)mangoo_close(handle);
    }

}

void makedir(char *path)
{
  int err,mode=0x8180;
  if(err=mangoo_create(ms,path,mode,mangoo_container))
    {
      printf("Error creating directory!!\n");
      return;
    } 
  printf("directory created!!\n");
}

void help()
{
  printf("1: write\n");
  printf("Write data to any file.\n");
  printf("Syntax: write data_to_enter file_name\n\n");
  printf("2: read\n");
  printf("Rread data from any file.\n");
  printf("Syntax: read file_name\n\n");
  printf("3: getattr\n");
  printf("Get attributes of file.\n\n");
  printf("4: exit\n");
  printf("Exit the mangoostore.\n\n");
  printf("5: help\n");
  printf("Displays the commands help.\n\n");
}

static int getattr(const char *path, struct stat *stbuf)
{
  struct mangoo_handle *handle = NULL;
  mode_t type;
  enum mangoo_types mt;
  int error;
  printf("getattr:1 %s\n", path);

  if ((error = mangoo_open(ms, path, mangoo_attr_interface, &handle))) {
    goto errout1;
  }

  printf("getattr:2 %s\n", path);
  if ((error = mattr_getuid(handle, &stbuf->st_uid)))
    goto errout2;

  printf("getattr:3 %s\n", path);
  if ((error = mattr_getgid(handle, &stbuf->st_gid)))
    goto errout2;

  printf("getattr:4 %s\n", path);
  if ((error = mattr_getmtime(handle, &stbuf->st_mtime)))
    goto errout2;
  stbuf->st_ctime = stbuf->st_mtime;

  printf("getattr:5 %s\n", path);
  if ((error = mattr_getatime(handle, &stbuf->st_atime)))
    goto errout2;

  printf("getattr:6 %s\n", path);
  if ((error = mattr_getsize(handle, &stbuf->st_size)))
    goto errout2;

  printf("getattr:7 %s\n", path);
  if ((error = mattr_getmode(handle, &stbuf->st_mode)))
    goto errout2;

  printf("getattr:8 %o\n", (int)stbuf->st_mode);
  if ((error = mattr_gettype(handle, &mt)))
    goto errout2;

  printf("getattr:9 %d\n", (int)mt);
  switch (mt) {
  case mangoo_data:
    printf("getattr:9.1 data\n");
    stbuf->st_nlink = 1;
    stbuf->st_mode |= S_IFREG;
    break;

  case mangoo_container:
    printf("getattr:9.2 container\n");
    stbuf->st_nlink = 2;
    stbuf->st_mode |= S_IFDIR;
    break;

  default:
    error = EINVAL;
    goto errout2;
  }

  printf("getattr:10\n");
  if ((error = mangoo_close(handle)))
    goto errout1;

  return 0;

 errout2:
  (void)mangoo_close(handle);

 errout1:
  printf("getattr:12 %s\n", strerror(error));
  return -error;
}

void * concwrt(void *param)
{
  char *path="/newfile";
  char *buf="hello";
  off_t *off;
  size_t sz;
  off=(off_t *)param;
  sampwrite(path,buf,&sz,off);
  printf("%jd\n",(intmax_t)*off);
}

int main()
{
  int err,i;
  off_t size;
  size_t sz;
  pthread_t th1;
  mode_t mode = 0x81B4;
  char *path,*buf,buffer[30];
  char *url="dbfs\0";
  char *rl="/tmp/dbfs_vol\0";
  char input[100],tok1[20],tok2[20],tok3[20];
  off_t offset=0,*off;
  struct stat stbuf;
  off=&offset;
  if(err=mangoo_initstore(url,rl,&ms))
    printf("Error in store\n");
  else
    printf("store opened\n");
  env_dir_create();
  for(i=0;i<4;i++)
    pthread_create(&th1,NULL,concwrt,(void*)off);
  mangoo_closestore(ms);
  return 0;
}
