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
  char *buf="\nhello",buffer[30];
  char *buf1="hello1",*buf2="hello2\n";
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
      printf("ERROR %d\n",err);
      if(err=mangoo_create(ms,path,mode,mangoo_data))
	printf("Error creating\n");
      else
	{
	  printf("File created\n");
	}
    }
  else
    {
      printf("OPEN\n");
      env_open(&dbenv);
      dh = (struct dbfs_handle*)handle;
      printf("Size : %jd\n ",(intmax_t)(dh->attr_ddd.ddd_size));
      printf("Mode : %o\n ",(dh->attr_ddd.ddd_mode));
      sz=strlen(buf)+1;
      if(err=mstream_write(handle,buf,&sz,offset))
	printf("Error while writing buf : %d\n",err);
      else
	offset+=strlen(buf);      
      sz=strlen(buf1)+1;
      if(err=mstream_write(handle,buf1,&sz,offset))
	printf("Error while writing buf1 : %d\n",err);
      else
	offset+=strlen(buf1);      
      sz=strlen(buf2)+1;
      if(err=mstream_write(handle,buf2,&sz,offset))
	printf("Error while writing buf2 : %d\n",err);
      (void)mangoo_close(handle);
    }
  mangoo_closestore(ms);
return 0;
}
