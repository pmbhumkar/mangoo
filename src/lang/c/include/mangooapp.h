/*
	Copyright (C) 2014 Amit S. Kale.
	This file is part of the mangoo filestore framework.

	The mangoo filestore framework is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License version 3 as published by the Free Software Foundation.
*/

#ifndef _MAPP_H
#define _MAPP_H

#include <mangoo.h>
#include <db.h>

struct mangoo_store;
struct mangoo_handle;

int mangoo_initstore(const char *scheme, const char *rl, struct mangoo_store **ms);
int mangoo_closestore(struct mangoo_store *ms);
void env_open(DB_ENV **dbenv);
/* Functions for the store. */
int mangoo_open(struct mangoo_store *ms, const char *locator, enum mangoo_interfaces mi,
		struct mangoo_handle **handle);
int mangoo_create(struct mangoo_store *ms, const char *locator, mode_t mode, enum mangoo_types mt);
int mangoo_close(struct mangoo_handle *handle);
int mangoo_delete(struct mangoo_store *ms, const char *locator);

/* Stream interface. */
int mstream_read(struct mangoo_handle *handle, char *buf, size_t *size, off_t offset);
int mstream_write(struct mangoo_handle *handle, const char *buf, size_t *size, off_t offset);

/* Directory interface. */
int mdir_read(struct mangoo_handle *handle, mdir_filler_t filler, void *fhandle);

/* Attributes interface. */
int mattr_gettype(struct mangoo_handle *handle, enum mangoo_types *mt);

int mattr_getsize(struct mangoo_handle *handle, off_t *size);
int mattr_setsize(struct mangoo_handle *handle, off_t size);

int mattr_getmtime(struct mangoo_handle *handle, time_t *mtime);
int mattr_setmtime(struct mangoo_handle *handle, time_t mtime);

int mattr_getatime(struct mangoo_handle *handle, time_t *atime);
int mattr_setatime(struct mangoo_handle *handle, time_t atime);

int mattr_getmode(struct mangoo_handle *mh, mode_t *mode);
int mattr_setmode(struct mangoo_handle *mh, mode_t mode);
int mattr_chmod(struct mangoo_handle *mh,mode_t mode);
int mattr_chown(struct mangoo_handle *mh,uid_t newuid,gid_t newgid);

int mattr_getuid(struct mangoo_handle *mh, uid_t *uid);
int mattr_setuid(struct mangoo_handle *mh, uid_t uid);

int mattr_getgid(struct mangoo_handle *mh, gid_t *gid);
int mattr_setgid(struct mangoo_handle *mh, gid_t gid);

#endif /*_MAPP_H*/
