#include <unistd.h>
#include <string.h>
#include <stdio.h>
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
struct dbfs_handle *dh;

int main()
{
  int err;
  off_t size;
  size_t sz;
  mode_t mode = 0x81B4;
  char *path="/newfile";
  char *buf="hello",buffer[30];
  char *url="dbfs\0";
  char *rl="/tmp/dbfs_vol\0";
  off_t offset=0;
  DB_ENV *dbenv;
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
      env_open(&dbenv);
      dh = (struct dbfs_handle*)handle;
      printf("Size : %jd\n ",(intmax_t)(dh->attr_ddd.ddd_size));
      printf("Mode : %o\n ",(dh->attr_ddd.ddd_mode));
      offset=0;
      sz=(size_t)(dh->attr_ddd.ddd_size);
      if(err=mstream_read(handle,buffer,&sz,offset))
	printf("Error while reading %d\n",err);
      else
	printf("%s\n",buffer);
      (void)mangoo_close(handle);
    }
  mangoo_closestore(ms);
return 0;
}
