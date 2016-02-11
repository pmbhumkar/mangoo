/*
	Copyright (C) 2014 Amit S. Kale.
	This file is part of the mangoo filestore framework.

	The mangoo filestore framework is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public
	License version 3 as published by the Free Software Foundation.
*/

#include <pthread.h>
#include <malloc.h>
#include <errno.h>

#include "dbfs.h"

int dbfs_create_attr(DB *dbp, struct dbfs_dentry_key *attr_ddk, size_t ddk_size, struct dbfs_dentry_data *attr_ddd,
	uint32_t type, uint32_t mode, uint64_t ino)
{
	int error;
	DBT key, data;

	attr_ddk->ddk_dk.dk_type = kt_dentry;

	memset(&key, 0, sizeof(DBT));
	key.data = attr_ddk;
	key.size = ddk_size;

	attr_ddd->ddd_type = type;
	attr_ddd->ddd_mode = mode;
	attr_ddd->ddd_ino = ino;
	attr_ddd->ddd_size = 0;
	attr_ddd->ddd_mtime = time(NULL);
	attr_ddd->ddd_atime = attr_ddd->ddd_mtime;
	attr_ddd->ddd_uid = geteuid();
	attr_ddd->ddd_gid = getegid();

	memset(&data, 0, sizeof(DBT));
	data.data = attr_ddd;
	data.size = sizeof(*attr_ddd);

	if ((error = dbp->put(dbp, NULL, &key, &data, DB_NOOVERWRITE))) {
		dbp->err(dbp, error, "Can't create attribute record for %s\n", attr_ddk->ddk_path);
		error = EINVAL;
		goto errout;
	}
	return 0;

errout:
	return error;
}

int dbfs_write_attr(DB *dbp, struct dbfs_dentry_key *attr_ddk, struct dbfs_dentry_data *attr_ddd)
{
	int error;
	DBT key, data;

	attr_ddk->ddk_dk.dk_type = kt_dentry;

	memset(&key, 0, sizeof(DBT));
	key.data = attr_ddk;
	key.size = dbfs_ddk_size(attr_ddk->ddk_path);

	memset(&data, 0, sizeof(DBT));
	data.data = attr_ddd;
	data.size = sizeof(*attr_ddd);

	if ((error = dbp->put(dbp, NULL, &key, &data, 0))) {
		dbp->err(dbp, error, "Can't update attribute record for %s\n", attr_ddk->ddk_path);
		error = EINVAL;
		goto errout;
	}
	return 0;

errout:
	return error;
}


int dbfs_read_attr(DB *dbp, struct dbfs_dentry_key *attr_ddk, size_t ddk_size, struct dbfs_dentry_data *attr_ddd)
{
	DBT key, data;
	int error = 0;

	memset(&key, 0, sizeof(DBT));
	key.data = attr_ddk;
	key.size = ddk_size;
	key.flags = DB_DBT_USERMEM;
	key.ulen = key.size;

	memset(&data, 0, sizeof(DBT));
	data.data = attr_ddd;
	data.size = sizeof(*attr_ddd);
	data.ulen = sizeof(*attr_ddd);
	data.flags = DB_DBT_USERMEM;

	if ((error = dbp->get(dbp, NULL, &key, &data, 0))) {
		if (error == DB_NOTFOUND) {
			error = ENOENT;
		} else {
			dbp->err(dbp, error, "Can't fetch attribute record for %s\n", attr_ddk->ddk_path);
			error = EIO;
		}
		goto out;
	}

out:
	return error;

}

#if 0
int dbfs_stat(DB *dbp, const char *locator, struct stat *stbuf)
{
	struct dbfs_dentry_key *ddk;
	struct dbfs_dentry_data ddd;
	int ddk_size;
	uint32_t ddd_type;
	uint64_t ino;
	int type;

	int error;

	printf("dbfs_stat:1 %s\n", locator);
	ddk_size = dbfs_ddk_size(locator);
	if (!(ddk = malloc(ddk_size))) {
		error = ENOMEM;
		goto errout;
	}
	ddk->ddk_dk.dk_type = kt_dentry;
	strcpy(ddk->ddk_path, locator);

	if ((error = dbfs_read_attr(dbp, ddk, ddk_size, &ddd))) {
		goto errout;
	}

	ino = ddd.ddd_ino;

	memset(stbuf, 0, sizeof(*stbuf));
	stbuf->st_ino = ino;

	/* Hardcoded for now */
	stbuf->st_uid = geteuid();
	stbuf->st_gid = getegid();
	stbuf->st_ctime = time(NULL);
	stbuf->st_mtime = time(NULL);
	stbuf->st_atime = time(NULL);
	stbuf->st_size = ddd.ddd_size;

	ddd_type = ddd.ddd_type;
	if (ddd_type == S_IFREG) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
	} else if (ddd_type == S_IFDIR) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else {
		error = EINVAL;
		goto errout;
	}

	printf("dbfs_stat:10\n");
	return 0;

errout:
	return error;
}

#endif
static int dbfs_gettype(struct mangoo_handle *mh, enum mangoo_types *mt)
{
	struct dbfs_handle *dh = (struct dbfs_handle*)mh;

	printf("dbfs_gettype:1\n");
	*mt = (enum mangoo_types)dh->attr_ddd.ddd_type;
	return 0;
}

static int dbfs_getsize(struct mangoo_handle *mh, off_t *size)
{
	struct dbfs_handle *dh = (struct dbfs_handle*)mh;

	printf("dbfs_getsize:1\n");
	*size = dh->attr_ddd.ddd_size;

	return 0;
}

static int dbfs_getmtime(struct mangoo_handle *mh, time_t *mtime)
{
	struct dbfs_handle *dh = (struct dbfs_handle*)mh;

	printf("dbfs_getmtime:1\n");
	*mtime = dh->attr_ddd.ddd_mtime;

	return 0;
}

static int dbfs_setmtime(struct mangoo_handle *mh, time_t mtime)
{
	struct dbfs_handle *dh = (struct dbfs_handle*)mh;

	printf("dbfs_setmtime:1\n");
	dh->attr_ddd.ddd_mtime = mtime;

	return dbfs_write_attr(dh->dbp, dh->attr_ddk, &dh->attr_ddd);
}

static int dbfs_getatime(struct mangoo_handle *mh, time_t *atime)
{
	struct dbfs_handle *dh = (struct dbfs_handle*)mh;

	printf("dbfs_getatime:1\n");
	*atime = dh->attr_ddd.ddd_atime;

	return 0;
}

static int dbfs_setatime(struct mangoo_handle *mh, time_t atime)
{
	struct dbfs_handle *dh = (struct dbfs_handle*)mh;

	printf("dbfs_setatime:1\n");
	dh->attr_ddd.ddd_atime = atime;
	return dbfs_write_attr(dh->dbp, dh->attr_ddk, &dh->attr_ddd);

}


static int dbfs_getmode(struct mangoo_handle *mh, mode_t *mode)
{
	struct dbfs_handle *dh = (struct dbfs_handle*)mh;

	printf("dbfs_getmode:1\n");
	*mode = (mode_t)dh->attr_ddd.ddd_mode;

	return 0;
}

static int dbfs_chmod(struct mangoo_handle *mh, mode_t mode)
{
  struct dbfs_handle *dh = (struct dbfs_handle *)mh;
  printf("changing mode : %o -> %o\n",dh->attr_ddd.ddd_mode,(uint32_t)mode);
  dh->attr_ddd.ddd_mode=(uint32_t)mode;
  return dbfs_write_attr(dh->dbp, dh->attr_ddk, &dh->attr_ddd);
}

static int dbfs_chown(struct mangoo_handle *mh, uid_t newuid, gid_t newgid)
{
  struct dbfs_handle *dh = (struct dbfs_handle *)mh;
  //printf("changing mode : %o -> %o\n",dh->attr_ddd.ddd_mode,(uint32_t)mode);
  dh->attr_ddd.ddd_uid=(uint32_t)newuid;
  dh->attr_ddd.ddd_gid=(uint32_t)newgid;
  return dbfs_write_attr(dh->dbp, dh->attr_ddk, &dh->attr_ddd);
}

static int dbfs_getuid(struct mangoo_handle *mh, uid_t *uid)
{
	struct dbfs_handle *dh = (struct dbfs_handle*)mh;

	printf("dbfs_getuid:1\n");
	*uid = (uid_t)dh->attr_ddd.ddd_uid;

	return 0;
}

static int dbfs_getgid(struct mangoo_handle *mh, gid_t *gid)
{
	struct dbfs_handle *dh = (struct dbfs_handle*)mh;

	printf("dbfs_getgid:1\n");
	*gid = (gid_t)dh->attr_ddd.ddd_gid;

	return 0;
}

static int dbfs_close(struct mangoo_handle *mh)
{
	struct dbfs_handle *dh = (struct dbfs_handle*)mh;

	printf("dbfs_close:1\n");
	dbfs_freehandle(dh);
	return 0;
}

static int dbfs_setsize(struct mangoo_handle *mh, off_t size)
{
	int error = 0;
	struct dbfs_handle *dh = (struct dbfs_handle*)mh;

	if (dh->attr_ddd.ddd_size > size) {
		if ((error = dbfs_truncateblocks(dh->dbp, dh->attr_ddd.ddd_ino, size,
			dh->attr_ddd.ddd_size))) {
			return error;
		}
	}
	dh->attr_ddd.ddd_size = size;
	error = dbfs_write_attr(dh->dbp, dh->attr_ddk, &dh->attr_ddd);

	return error;
}

struct mangoo_attr_operations dbfs_attr_ops = {
	.mao_gettype = dbfs_gettype,
	.mao_getsize = dbfs_getsize,
	.mao_setsize = dbfs_setsize,
	.mao_getmtime = dbfs_getmtime,
	.mao_setmtime = dbfs_setmtime,
	.mao_getatime = dbfs_getatime,
	.mao_setatime = dbfs_setatime,
	.mao_getmode = dbfs_getmode,
	.mao_getuid = dbfs_getuid,
	.mao_getgid = dbfs_getgid,
	.mao_close = dbfs_close,
	.mao_chmod = dbfs_chmod,
	.mao_chown = dbfs_chown,
};

int dbfs_aopen(struct mangoo_store *storep, const char *locator, struct mangoo_handle **mhpp)
{
	int error;
	struct dbfs_handle *dh;
	struct dbfs_store *dsp = (struct dbfs_store *)storep;

	printf("dbfs_aopen:1\n");
	if (!(dh = dbfs_createhandle(locator, dsp->dbp))) {
		error = ENOMEM;
		goto errout;
	}

	printf("dbfs_aopen:2\n");
	dh->mh.mh_interface = mangoo_attr_interface;
	dh->mh.mh_fvector = (void *)&dbfs_attr_ops;
	
	if ((error = dbfs_read_attr(dsp->dbp, dh->attr_ddk, dbfs_ddk_size(locator),
		&dh->attr_ddd))) {
		goto errout;
	}
	printf("dbfs_aopen:3\n");

	*mhpp = (struct mangoo_handle *)dh;

	return 0;

errout:
	return error;
}

int dbfs_dela(struct dbfs_handle *dh)
{
	DBT key;
	int error = 0;

	memset(&key, 0, sizeof(DBT));
	key.data = dh->attr_ddk;
	key.size = dh->ddk_size;
	key.flags = DB_DBT_USERMEM;
	key.ulen = key.size;

	if ((error = dh->dbp->del(dh->dbp, NULL, &key, 0))) {
		if (error == DB_NOTFOUND) {
			error = ENOENT;
		} else {
			dh->dbp->err(dh->dbp, error, "Can't delete attribute record for %s\n",
					dh->attr_ddk->ddk_path);
			error = EIO;
		}
		goto out;
	}

	printf("dbfs_dela:2\n");
out:
	return error;
}
