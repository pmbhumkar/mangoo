/*
	Copyright (C) 2014 Amit S. Kale.
	This file is part of the mangoo filestore framework.

	The mangoo filestore framework is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public
	License version 3 as published by the Free Software Foundation.
*/

#include <sys/stat.h>
#include <errno.h>
#include <malloc.h>

#include "dbfs.h"

static int dbfs_dentry_compare(struct dbfs_dentry_key *key1, struct dbfs_dentry_key *key2)
{

	fprintf(stdout, "dbfs_dentry_compare:1\n");
	return strcmp(key1->ddk_path, key2->ddk_path);
}

static int dbfs_block_compare(struct dbfs_block_key *key1, struct dbfs_block_key *key2)
{
	//Extents are collected by inode number
	if (key1->dbk_ino > key2->dbk_ino)
		return 1;
	if (key1->dbk_ino < key2->dbk_ino)
		return -1;

	//Block for an inode number are sorted by offset
	if (key1->dbk_offset > key2->dbk_offset)
		return 1;
	if (key1->dbk_offset < key2->dbk_offset)
		return -1;

	return 0;
}
	
int dbfs_bt_compare(DB *db, const DBT *dbt1, const DBT *dbt2, size_t *sp)
{
	struct dbfs_key *key1 = dbt1->data;
	struct dbfs_key *key2 = dbt2->data;
	
	if (sp) {
		fprintf(stdout, "dbfs_bt_compare:1\n");
	}
		
	if (key1->dk_type != key2->dk_type) {
		fprintf(stdout, "dbfs_bt_compare:2 key1 %d key2 %d\n", (int)key1->dk_type, (int)key2->dk_type);
		//Keys are of different type. The ordering is by the key_type.
		return (int)key1->dk_type - (int)key2->dk_type;
	}

	//Keys are of the same type. Comparison is dependent on the type
	switch(key1->dk_type) {
	case kt_fsinfo:
		//There is only one fsinfo key.
		fprintf(stdout, "dbfs_bt_compare:3\n");
		return 0;

	case kt_dentry:
		return dbfs_dentry_compare((struct dbfs_dentry_key *)key1,
			(struct dbfs_dentry_key *)key2);

	case kt_block:
		fprintf(stdout, "dbfs_bt_compare:4\n");
		return dbfs_block_compare((struct dbfs_block_key *)key1,
			(struct dbfs_block_key *)key2);

	default:
		fprintf(stderr, "Unknown key type %d\n", (int)key1->dk_type);
		return 0;
	}
}

static void dbfs_initsearch(DBT *key, DBT *data, struct dbfs_handle *dh)
{
	memset(key, 0, sizeof(DBT));
	key->data = dh->attr_ddk;
	key->size = dh->ddk_size;
	key->flags = DB_DBT_REALLOC;
	key->ulen = key->size;

	memset(data, 0, sizeof(DBT));
	data->data = &dh->attr_ddd;
	data->size = sizeof(dh->attr_ddd);
	data->ulen = sizeof(dh->attr_ddd);
	data->dlen = data->ulen;
	data->doff = 0;
	data->flags = DB_DBT_USERMEM | DB_DBT_PARTIAL;
}

static int dbfs_readdir(struct mangoo_handle *handle, mdir_filler_t filler, void *finfo)
{
	struct dbfs_handle *dh=(struct dbfs_handle *)handle;
	DBT key, data;
	const char *entry, *name;
	int error = 0;
	int isroot = 0;

	printf("dbfs_readdir:1\n");
	dbfs_initsearch(&key, &data, dh);

	if (!strcmp(((struct dbfs_dentry_key *)key.data)->ddk_path, "/"))
		isroot = 1;	

	do {
		printf("dbfs_readdir:2\n");
		if ((error = dh->dbc->get(dh->dbc, &key, &data, DB_NEXT))) {
			if (error == DB_NOTFOUND) {
				error = 0;
			} else { 
				dh->dbp->err(dh->dbp, error, "Error fetch attribute record.\n");
				error = EIO;
			}
			printf("dbfs_readdir:3\n");
			break;
		}
	
		if (((struct dbfs_key *)key.data)->dk_type != kt_dentry) {
			printf("dbfs_readdir:4\n");
			/* Directory entries are over */
			break;
		}
		entry = ((struct dbfs_dentry_key *)key.data)->ddk_path;
		
		if (isroot) {
			if (strchr(entry + 1, '/')) {
				/* next directory */
				printf("dbfs_readdir:5.0 entry %s locator %s\n", entry, dh->locator);
				continue;
			}
		} else {
			if (strstr(entry, dh->locator) != entry) {
				printf("dbfs_readdir:5.1 entry %s locator %s\n", entry, dh->locator);
				/* next directory */
				break;
			}
		}
		
		printf("dbfs_readdir:6 entry %s locator %s\n", entry, dh->locator);
		entry += strlen(dh->locator);

		printf("dbfs_readdir:6.1 %s\n", entry);
		while (*entry == '/') {
			entry++;
		}
		name = entry;
		if (*name) {
			if (strchr(name, '/')) {
				printf("dbfs_readdir:6.3 skipping a subdirectory %s\n", name);
				continue;
			}
			printf("dbfs_readdir:7 %s type %d\n", name, (int)dh->attr_ddd.ddd_type);
			error = filler(finfo, name, (enum mangoo_types)dh->attr_ddd.ddd_type);
		}

	} while (!error);

	dh->ddk_size = key.size;
	return error;
}

static	int dbfs_closedir(struct mangoo_handle *handle)
{
	struct dbfs_handle *dh=(struct dbfs_handle *)handle;

	dbfs_freehandle(dh);

	return 0;
}

struct mangoo_directory_operations dbfs_directory_ops = {
	.mdo_read = dbfs_readdir,
	.mdo_close = dbfs_closedir
};

static int dbfs_cursorset(DB *dbp, struct dbfs_handle *dh, DBT *key, DBT *data)
{
	int error;

	if ((error = dbp->cursor(dbp, NULL, &dh->dbc, 0))) {
		dbp->err(dbp, error, "Cursor creation failed");
		error = EIO;
		goto errout;
	}

	memset(key, 0, sizeof(DBT));
	key->data = dh->attr_ddk;
	key->size = dh->ddk_size;
	key->flags = DB_DBT_USERMEM;
	key->ulen = key->size;

	memset(data, 0, sizeof(DBT));
	data->data = &dh->attr_ddd;
	data->size = sizeof(dh->attr_ddd);
	data->ulen = sizeof(dh->attr_ddd);
	data->flags = DB_DBT_USERMEM;

	if ((error = dh->dbc->get(dh->dbc, key, data, DB_SET))) {
		if (error == DB_NOTFOUND) {
			error = ENOENT;
		} else if (error == DB_BUFFER_SMALL) {
			error = ENOENT;
		} else {
			dh->dbp->err(dh->dbp, error, "Can't fetch directory entry.\n");
			error = EIO;
		}
		printf("dbfs_cursorset:2 %s error %s\n", dh->locator, strerror(error));
		goto errout;
	}
	if (strcmp(((struct dbfs_dentry_key *)key->data)->ddk_path, dh->locator)) {
		error = EIO;
		printf("dbfs_cursorset:3 found %s\n",
			((struct dbfs_dentry_key *)key->data)->ddk_path);
		goto errout;
	}

errout:
	return error;
}

int dbfs_dopen(struct mangoo_store *storep, const char *locator, struct mangoo_handle **mhpp)
{
	int error;
	struct dbfs_handle *dh;
	DBT key, data;
	struct dbfs_store *dsp = (struct dbfs_store *)storep;
	size_t	ddk_size;

	printf("dbfs_dopen:1 %s\n", locator);

	if ((dh = dbfs_createhandle(locator, dsp->dbp)) == NULL) {
		error = ENOMEM;
		goto errout;
	}
	dh->mh.mh_interface = mangoo_directory_interface;
	dh->mh.mh_fvector = (void *)&dbfs_directory_ops;

	if ((error = dbfs_cursorset(dsp->dbp, dh, &key, &data))){
		goto errout;
	}

	*mhpp = (struct mangoo_handle *)dh;
	return 0;

errout:
	if (dh)
		dbfs_freehandle(dh);

	return error;
}

int dbfs_deld(struct dbfs_handle *dh)
{
	DBT key, data;
	int error;
	const char *entry, *name;

	printf("dbfs_deld:1\n");
	if ((error = dbfs_cursorset(dh->dbp, dh, &key, &data))){
		goto errout;
	}

	dbfs_initsearch(&key, &data, dh);

	if ((error = dh->dbc->get(dh->dbc, &key, &data, DB_NEXT))) {
		if (error == DB_NOTFOUND) {
			printf("dbfs_deld:2\n");
			error = 0;
		} else { 
			dh->dbp->err(dh->dbp, error, "Error fetch attribute record.\n");
			error = EIO;
		}
		goto errout;
	}

	if (((struct dbfs_key *)key.data)->dk_type != kt_dentry) {
		printf("dbfs_deld:4\n");
		/* Directory entries are over */
		goto errout;;
	}


	entry = ((struct dbfs_dentry_key *)key.data)->ddk_path;
	if (strstr(entry, dh->locator) != entry) {
		printf("dbfs_deld:5 entry %s locator %s\n", entry, dh->locator);
		/* next directory */
		goto errout;
	}

	entry += strlen(dh->locator);
	while (*entry == '/') {
		entry++;
	}
	name = entry;
	if (*name) {
		/* Sub directory or a file */
		error = ENOTEMPTY;
		printf("dbfs_deld:6 name %s\n", entry, dh->locator);
	}

errout:
	return error;
}
