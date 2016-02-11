#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <utime.h>
#include <malloc.h>
#include <inttypes.h>
#include <stdlib.h>

#include <mangooapp.h>
#include <mangoostore.h>
#include "../stores/dbfs/dbfs.h"
#define DBENV_DIR "/home/pravin/TXNAPP"
static struct mangoo_store *ms;
struct mangoo_handle *handle = NULL;
struct dbfs_handle *dh;


const char *mypath="/a/TXNAPP";

void env_dir_create()
{
  struct mangoo_handle *handle=NULL;
  enum mangoo_types mt;
  int error;
  if(error=mangoo_open(ms,mypath,mangoo_attr_interface,&handle))
    {
      printf("OPEN ERROR\n");
      if(error=mangoo_create(ms,mypath,S_IRWXU,mangoo_container))
	{
	  printf("Directory Create ERROR\n");
	  exit(1);
	}
      printf("Directory created\n");
      if(error=mangoo_open(ms,mypath,mangoo_attr_interface,&handle))
	{
	  printf("OPEN ERROR AGAIN\n");
	  exit(1);
	}
    }
  if(error=mattr_gettype(handle,&mt))
    {
      printf("TYPE ERROR\n");
      return;
    }
  if(mt==mangoo_container)
    printf("Directory\n");
  else
    printf("File\n");
  (void)mangoo_close(handle);
}

void env_open(DB_ENV **dbenvp)
{
  DB_ENV *mydbenv;
  int ret;
  
  /* Create the environment handle. */
  if ((ret = db_env_create(&mydbenv, 0)) != 0) {
    fprintf(stderr,
	    "txnapp: db_env_create: %s\n", db_strerror(ret));
    exit (1);
  }

  printf("Environment Created\n");
  /* Set up error handling. */
  mydbenv->set_errpfx(mydbenv, "txnapp");


  /*
   * Open a transactional environment:
   *create if it doesn't exist
   *free-threaded handle
   *run recovery
   *read/write owner only
   */
  if ((ret = mydbenv->open(mydbenv, DBENV_DIR,
			      DB_CREATE | DB_INIT_LOCK | DB_INIT_LOG |
			 DB_INIT_MPOOL | DB_INIT_TXN | DB_RECOVER | DB_THREAD,
			 S_IRUSR | S_IWUSR)) != 0) {
    if(ret==ENOENT)
      printf("Directory does not exist\n");
    printf("Error in OPENING ENVIRONMENT...!!! \n");    
    (void)mydbenv->close(mydbenv, 0);
    //mydbenv->err(mydbenv, ret, "dbenv->open: %s", DBENV_DIR);
    exit (1);
  }

  *dbenvp = mydbenv;
}


int main()
{
  int err;
  off_t size;
  size_t sz;
  mode_t mode = 0x100664;
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

  env_dir_create();
  env_open(&dbenv);
  
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
      dh = (struct dbfs_handle*)handle;
      printf("Size : %jd\n ",(intmax_t)(dh->attr_ddd.ddd_size));
      printf("Mode : %X\n ",(dh->attr_ddd.ddd_mode));
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
