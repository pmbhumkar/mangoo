/*
	Copyright (C) 2014 Amit S. Kale.
	This file is part of the mangoo filestore framework.

	The mangoo filestore framework is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public
	License version 3 as published by the Free Software Foundation.
*/

#include <string.h>
#include <errno.h>
#include <malloc.h>

#include "dbfs.h" 

static int dbfs_crupd_fsinfo(DB *dbp, struct dbfs_fsinfo_data *dfd, int create)
{
	DBT fsi_key, fsi_data;

	struct dbfs_fsinfo_key dfk;
	int error;

	dfk.dfk_dk.dk_type = kt_fsinfo;
	memset(&fsi_key, 0, sizeof(DBT));
	fsi_key.data = &dfk;
	fsi_key.size = sizeof(dfk);

	memset(&fsi_data, 0, sizeof(DBT));
	fsi_data.data = dfd;
	fsi_data.size = sizeof(dfd);

	if ((error = dbp->put(dbp, NULL, &fsi_key, &fsi_data, create ? DB_NOOVERWRITE : 0))) {
		dbp->err(dbp, error, "Can't add filesystem information record.");
		return EIO;
	}
	return 0;
}

int dbfs_update_fsinfo(DB *dbp, struct dbfs_fsinfo_data *dfd)
{
	return dbfs_crupd_fsinfo(dbp, dfd, 0);	
}

int dbfs_create_fsinfo(DB *dbp, struct dbfs_fsinfo_data *dfd)
{
	return dbfs_crupd_fsinfo(dbp, dfd, 1);	
}

int dbfs_fetch_fsinfo(DB *dbp, struct dbfs_fsinfo_data *dfd)
{
	DBT fsi_key, fsi_data;

	struct dbfs_fsinfo_key dfk;
	int error;

	dfk.dfk_dk.dk_type = kt_fsinfo;
	memset(&fsi_key, 0, sizeof(DBT));
	fsi_key.data = &dfk;
	fsi_key.size = sizeof(dfk);
	fsi_key.ulen = fsi_key.size;
	fsi_key.flags = DB_DBT_USERMEM;

	memset(&fsi_data, 0, sizeof(DBT));
	fsi_data.data = dfd;
	fsi_data.size = sizeof(dfd);
	fsi_data.ulen = sizeof(dfd);
	fsi_data.flags = DB_DBT_USERMEM;

	if ((error = dbp->get(dbp, NULL, &fsi_key, &fsi_data, 0))) {
		dbp->err(dbp, error, "Can't add filesystem information record.");
		return EIO;
	}
	return 0;
}

struct dbfs_handle *dbfs_createhandle(const char *locator, DB *dbp)
{
	struct dbfs_handle *dh;
	size_t ddk_size;

	ddk_size = dbfs_ddk_size(locator);

	if (!(dh = calloc(1, sizeof(struct dbfs_handle))) ||
	    !(dh->attr_ddk = malloc(ddk_size)) ||
	    !(dh->locator = strdup(locator))) {
		if (dh)
			dbfs_freehandle(dh);
		return NULL;
	}

	dbfs_init_dentry_key(locator, dh->attr_ddk);
	dh->ddk_size = dbfs_ddk_size(locator);
	dh->dbp = dbp;

	return dh;
}

void dbfs_freehandle(struct dbfs_handle *dh)
{
	if (dh->dbc)
		(void)dh->dbc->close(dh->dbc);
	if (dh->attr_ddk)
		free(dh->attr_ddk);
	if (dh->locator)
		free(dh->locator);
	free(dh);
}
