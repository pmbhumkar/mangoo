/*
	Copyright (C) 2014 Amit S. Kale.
	This file is part of the mangoo filestore framework.

	The mangoo filestore framework is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public
	License version 3 as published by the Free Software Foundation.
*/

#ifndef _DBFS_H
#define _DBFS_H

#include <stdint.h>
#include <sys/stat.h>
#include <string.h>

#include <db.h>

#include <mangoostore.h>

enum dbfs_key_types {
	kt_fsinfo,
	kt_dentry,
	kt_block,
};

struct dbfs_key {
	enum dbfs_key_types dk_type;
};

struct dbfs_dentry_key {
	struct dbfs_key	ddk_dk;
	char		ddk_path[0];
};

static inline size_t dbfs_ddk_size(const char *locator)
{
	return sizeof(struct dbfs_dentry_key) + strlen(locator) + 1;
}

static inline void dbfs_init_dentry_key(const char *path, struct dbfs_dentry_key *ddk)
{
	ddk->ddk_dk.dk_type = kt_dentry;
	strcpy(ddk->ddk_path, path);
}

struct dbfs_dentry_data {
	uint32_t 	ddd_type;
	uint32_t	ddd_mode;
	uint64_t	ddd_ino;
	uint64_t	ddd_size;
	uint64_t	ddd_mtime;
	uint64_t	ddd_atime;
	uint32_t	ddd_uid;
	uint32_t	ddd_gid;
};

struct dbfs_block_key {
	struct dbfs_key		dbk_dk;
	uint64_t		dbk_ino;
	uint64_t		dbk_offset;
};

struct dbfs_fsinfo_key {
	struct dbfs_key	dfk_dk;
};

struct dbfs_fsinfo_data {
	uint64_t		dfd_lastino;
};


#define DBFS_ROOT_INO	1
#define DBFS_BLKSIZE	4096

int dbfs_bt_compare(DB *db, const DBT *dbt1, const DBT *dbt2, size_t *sp);

int dbfs_create_attr(DB *dbp, struct dbfs_dentry_key *attr_ddk, size_t ddk_size, struct dbfs_dentry_data *attr_ddd,
	uint32_t type, uint32_t mode, uint64_t ino);
int dbfs_read_attr(DB *dbp, struct dbfs_dentry_key *attr_ddk, size_t ddk_size,
		struct dbfs_dentry_data *attr_ddd);
int dbfs_write_attr(DB *dbp, struct dbfs_dentry_key *attr_ddk, struct dbfs_dentry_data *attr_ddd);
int dbfs_create_fsinfo(DB *dbp, struct dbfs_fsinfo_data *dfd);
int dbfs_fetch_fsinfo(DB *dbp, struct dbfs_fsinfo_data *dfd);
int dbfs_update_fsinfo(DB *dbp, struct dbfs_fsinfo_data *dfd);

int dbfs_truncateblocks(DB *dbp, uint64_t ino, off_t offset, off_t end);

struct dbfs_store {
	struct mangoo_store ms;
	DB *dbp;	
	uint64_t last_inode;
	struct dbfs_fsinfo_data dfd;
};

struct dbfs_handle {
	struct mangoo_handle mh;
	DB *dbp;
	DBC *dbc;

	char *locator;
	struct dbfs_dentry_key *attr_ddk;
	size_t ddk_size;
	struct dbfs_dentry_data attr_ddd;
};

extern struct mangoo_directory_operations dbfs_directory_ops;
extern struct mangoo_stream_operations dbfs_stream_ops;
extern struct mangoo_attr_operations dbfs_attr_ops;

int dbfs_dopen(struct mangoo_store *storep, const char *locator, struct mangoo_handle **mhpp);
int dbfs_sopen(struct mangoo_store *storep, const char *locator, struct mangoo_handle **mhpp);
int dbfs_aopen(struct mangoo_store *storep, const char *locator, struct mangoo_handle **mhpp);

struct dbfs_handle *dbfs_createhandle(const char *locator, DB *dbp);
void dbfs_freehandle(struct dbfs_handle *dh);

int dbfs_deld(struct dbfs_handle *dh);
int dbfs_dels(struct dbfs_handle *dh);
int dbfs_dela(struct dbfs_handle *dh);

#endif /*_DBFS_H*/
