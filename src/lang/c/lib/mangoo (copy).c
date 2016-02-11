/*
	Copyright (C) 2014 Amit S. Kale.
	This file is part of the mangoo filestore framework.

	The mangoo filestore framework is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License version 3 as published by the Free Software Foundation.
*/

#include <errno.h>
#include <malloc.h>
#include <string.h>

#include <db.h>
#include <mangooapp.h>
#include <mangoostore.h>

#define DBENV_DIR "/tmp/TXNAPP"

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
			      DB_CREATE | DB_INIT_LOG | DB_INIT_LOCK |
			 DB_INIT_MPOOL | DB_INIT_TXN | DB_RECOVER | DB_THREAD,
			 S_IRUSR | S_IWUSR)) != 0) {

    printf("Error in OPENING ENVIRONMENT...!!! \n");    
    (void)mydbenv->close(mydbenv, 0);
    mydbenv->err(mydbenv, ret, "dbenv->open: %s", DBENV_DIR);
    exit (1);
  }
  printf("ENVIRONMENT CREATED...!!!!\n");
  *dbenvp = mydbenv;
}

int mangoo_initstore(const char *scheme, const char *rl, struct mangoo_store **mspp)
{
	int error;
	extern struct mangoo_operations dbfs_msops;

	printf("mangoo_initstore:1 '%s':'%s'\n", scheme, rl);
	//only one scheme supported right now
	if (strcmp(scheme, "dbfs")) {
		printf("mangoo_initstore:2\n");
		error = ENOENT;	
		goto errout;
	}

	if ((error = dbfs_msops.mo_initstore(rl, mspp))) {
		printf("mangoo_initstore:3\n");
		goto errout;
	}
	return 0;

errout:
	return error;
}

int mangoo_closestore(struct mangoo_store *ms)
{
	int error;

	if((error = ms->ms_ops->mo_closestore(ms))) {
		return error;
	}
	free(ms);
	return 0;
}

int mangoo_open(struct mangoo_store *ms, const char *locator, enum mangoo_interfaces mi,
		struct mangoo_handle **handle)
{
	if (!ms || !ms->ms_ops)
		return EINVAL;
	if (!ms->ms_ops->mo_open)
		return ENOSYS;
	return ms->ms_ops->mo_open(ms, locator, mi, handle);
}

int mangoo_create(struct mangoo_store *ms, const char *locator, mode_t mode, enum mangoo_types mt)
{
	if (!ms || !ms->ms_ops)
		return EINVAL;
	if (!ms->ms_ops->mo_create)
		return ENOSYS;
	return ms->ms_ops->mo_create(ms, locator, mode, mt);
}

int mangoo_delete(struct mangoo_store *ms, const char *locator)
{
	if (!ms || !ms->ms_ops)
		return EINVAL;
	if (!ms->ms_ops->mo_delete)
		return ENOSYS;
	return ms->ms_ops->mo_delete(ms, locator);
}

int mstream_read(struct mangoo_handle *mh, char *buf, size_t *size, off_t offset)
{
	struct mangoo_stream_operations *msop = (struct mangoo_stream_operations *)mh->mh_fvector;
	if (!msop)
		return EINVAL;
	if (!msop->mso_read)
		return ENOSYS;
	return msop->mso_read(mh, buf, size, offset);
}

int mstream_write(struct mangoo_handle *mh, const char *buf, size_t *size, off_t offset)
{
	struct mangoo_stream_operations *msop = (struct mangoo_stream_operations *)mh->mh_fvector;
	if (!msop)
		return EINVAL;
	if (!msop->mso_write)
		return ENOSYS;
	return msop->mso_write(mh, buf, size, offset);
}

int mangoo_close(struct mangoo_handle *mh)
{
	int error;

	if (!mh)
		return EINVAL;

	switch(mh->mh_interface) {
	case mangoo_stream_interface:
		error = ((struct mangoo_stream_operations *)mh->mh_fvector)->mso_close(mh);
		break;

	case mangoo_directory_interface:
		error = ((struct mangoo_directory_operations *)mh->mh_fvector)->mdo_close(mh);
		break;

	case mangoo_attr_interface:
		error = ((struct mangoo_attr_operations *)mh->mh_fvector)->mao_close(mh);
		break;

	default:
		error = EINVAL;
	}
	return error;
} 

int mdir_read(struct mangoo_handle *mh, mdir_filler_t filler, void *fhandle)
{
	struct mangoo_directory_operations *msop =
		(struct mangoo_directory_operations *)mh->mh_fvector;
	if (!msop)
		return EINVAL;
	if (!msop->mdo_read)
		return ENOSYS;
	return msop->mdo_read(mh, filler, fhandle);
}

int mattr_gettype(struct mangoo_handle *mh, enum mangoo_types *mt)
{
	struct mangoo_attr_operations *maop =
		(struct mangoo_attr_operations *)mh->mh_fvector;
	if (!maop)
		return EINVAL;
	if (!maop->mao_gettype)
		return ENOSYS;
	return maop->mao_gettype(mh, mt);
}

int mattr_getsize(struct mangoo_handle *mh, off_t *size)
{
	struct mangoo_attr_operations *maop =
		(struct mangoo_attr_operations *)mh->mh_fvector;
	if (!maop)
		return EINVAL;
	if (!maop->mao_getsize)
		return ENOSYS;
	return maop->mao_getsize(mh, size);
}

int mattr_setsize(struct mangoo_handle *mh, off_t size)
{
	struct mangoo_attr_operations *maop =
		(struct mangoo_attr_operations *)mh->mh_fvector;
	if (!maop)
		return EINVAL;
	if (!maop->mao_setsize)
		return ENOSYS;
	return maop->mao_setsize(mh, size);
}

int mattr_getmtime(struct mangoo_handle *mh, time_t *mtime)
{
	struct mangoo_attr_operations *maop =
		(struct mangoo_attr_operations *)mh->mh_fvector;
	if (!maop)
		return EINVAL;
	if (!maop->mao_getmtime)
		return ENOSYS;
	return maop->mao_getmtime(mh, mtime);
}

int mattr_setmtime(struct mangoo_handle *mh, time_t mtime)
{
	struct mangoo_attr_operations *maop =
		(struct mangoo_attr_operations *)mh->mh_fvector;
	if (!maop)
		return EINVAL;
	if (!maop->mao_setmtime)
		return ENOSYS;
	return maop->mao_setmtime(mh, mtime);
}

int mattr_getatime(struct mangoo_handle *mh, time_t *atime)
{
	struct mangoo_attr_operations *maop =
		(struct mangoo_attr_operations *)mh->mh_fvector;
	if (!maop)
		return EINVAL;
	if (!maop->mao_getatime)
		return ENOSYS;
	return maop->mao_getatime(mh, atime);
}

int mattr_setatime(struct mangoo_handle *mh, time_t atime)
{
	struct mangoo_attr_operations *maop =
		(struct mangoo_attr_operations *)mh->mh_fvector;
	if (!maop)
		return EINVAL;
	if (!maop->mao_setatime)
		return ENOSYS;
	return maop->mao_setatime(mh, atime);
}

int mattr_getmode(struct mangoo_handle *mh, mode_t *mode)
{
	struct mangoo_attr_operations *maop =
		(struct mangoo_attr_operations *)mh->mh_fvector;
	if (!maop)
		return EINVAL;
	if (!maop->mao_getmode)
		return ENOSYS;
	return maop->mao_getmode(mh, mode);
}

int mattr_setmode(struct mangoo_handle *mh, mode_t mode)
{
	struct mangoo_attr_operations *maop =
		(struct mangoo_attr_operations *)mh->mh_fvector;
	if (!maop)
		return EINVAL;
	if (!maop->mao_setmode)
		return ENOSYS;
	return maop->mao_setmode(mh, mode);
}

int mattr_chmod(struct mangoo_handle *mh,mode_t mode)
{
  struct mangoo_attr_operations *maop = 
    (struct mangoo_attr_operations *)mh->mh_fvector;
  if (!maop)
    return EINVAL;
  if (!maop->mao_chmod)
    return ENOSYS;
  return maop->mao_chmod(mh,mode);
}

int mattr_chown(struct mangoo_handle *mh,uid_t newuid,gid_t newgid)
{
  struct mangoo_attr_operations *maop = 
    (struct mangoo_attr_operations *)mh->mh_fvector;
  if (!maop)
    return EINVAL;
  if (!maop->mao_chown)
    return ENOSYS;
  return maop->mao_chown(mh,newuid,newgid);
}

int mattr_getuid(struct mangoo_handle *mh, uid_t *uid)
{
	struct mangoo_attr_operations *maop =
		(struct mangoo_attr_operations *)mh->mh_fvector;
	if (!maop)
		return EINVAL;
	if (!maop->mao_getuid)
		return ENOSYS;
	return maop->mao_getuid(mh, uid);
}

int mattr_setuid(struct mangoo_handle *mh, uid_t uid)
{
	struct mangoo_attr_operations *maop =
		(struct mangoo_attr_operations *)mh->mh_fvector;
	if (!maop)
		return EINVAL;
	if (!maop->mao_setuid)
		return ENOSYS;
	return maop->mao_setuid(mh, uid);
}

int mattr_getgid(struct mangoo_handle *mh, gid_t *gid)
{
	struct mangoo_attr_operations *maop =
		(struct mangoo_attr_operations *)mh->mh_fvector;
	if (!maop)
		return EINVAL;
	if (!maop->mao_getgid)
		return ENOSYS;
	return maop->mao_getgid(mh, gid);
}

int mattr_setgid(struct mangoo_handle *mh, gid_t gid)
{
	struct mangoo_attr_operations *maop =
		(struct mangoo_attr_operations *)mh->mh_fvector;
	if (!maop)
		return EINVAL;
	if (!maop->mao_setgid)
		return ENOSYS;
	return maop->mao_setgid(mh, gid);
}
