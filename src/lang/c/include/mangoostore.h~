/*
	Copyright (C) 2014 Amit S. Kale.
	This file is part of the mangoo filestore framework.

	The mangoo filestore framework is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License version 3 as published by the Free Software Foundation.
*/

#ifndef _MSTORE_H
#define _MSTORE_H

#include <mangoo.h>

/* Stores should define their structures by extending this structure */
struct mangoo_store {
	struct mangoo_operations *ms_ops;
};

/* Stores should define their structures by extending this structure */
struct mangoo_handle
{
	enum mangoo_interfaces mh_interface;
	void *mh_fvector;
};

struct mangoo_operations {
	int (*mo_initstore)(const char *rl, struct mangoo_store **storep);

	int (*mo_closestore)(struct mangoo_store *storep);
	int (*mo_getattr)(struct mangoo_store *storep, const char *locator, struct stat *statp);
	int (*mo_open)(struct mangoo_store *storep, const char *locator, enum mangoo_interfaces mi,
			struct mangoo_handle**handle);
	int (*mo_create)(struct mangoo_store *storep, const char *locator, mode_t mode,
			enum mangoo_types mt);
	int (*mo_delete)(struct mangoo_store *storep, const char *locator);
};

struct mangoo_stream_operations {
	int (*mso_read)(struct mangoo_handle *handle, char *buf, size_t *size, off_t offset);
	int (*mso_write)(struct mangoo_handle *handle, const char *buf, size_t *size, off_t offset);
	int (*mso_close)(struct mangoo_handle *handle);
};

struct mangoo_directory_operations {
	int (*mdo_read)(struct mangoo_handle *handle, mdir_filler_t filler, void *finfo);	
	int (*mdo_close)(struct mangoo_handle *handle);
};

struct mangoo_attr_operations {
	int (*mao_gettype)(struct mangoo_handle *handle, enum mangoo_types *type);	

	int (*mao_getsize)(struct mangoo_handle *handle, off_t *size);	
	int (*mao_setsize)(struct mangoo_handle *handle, off_t size);	

	int (*mao_getmtime)(struct mangoo_handle *handle, time_t *mtime);
	int (*mao_setmtime)(struct mangoo_handle *handle, time_t mtime);

	int (*mao_getatime)(struct mangoo_handle *handle, time_t *atime);
	int (*mao_setatime)(struct mangoo_handle *handle, time_t atime);

	int (*mao_getmode)(struct mangoo_handle *, mode_t *mode);
	int (*mao_setmode)(struct mangoo_handle *, mode_t mode);
	int (*mao_chmod)(struct mangoo_handle *,mode_t mode);

	int (*mao_getuid)(struct mangoo_handle *, uid_t *uid);
	int (*mao_setuid)(struct mangoo_handle *, uid_t uid);

	int (*mao_getgid)(struct mangoo_handle *, gid_t *gid);
	int (*mao_setgid)(struct mangoo_handle *, gid_t gid);
  int (*mao_chown)(struct mangoo_handle *,uid_t newuid,gid_t newgid);

	int (*mao_close)(struct mangoo_handle *handle);
};

#endif /*_MSTORE_H*/
