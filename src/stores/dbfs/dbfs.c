/*
	Copyright (C) 2014 Amit S. Kale.
	This file is part of the mangoo filestore framework.

	The mangoo filestore framework is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public
	License version 3.0 as published by the Free Software Foundation.
*/

#include <errno.h>
#include <malloc.h>

#include "dbfs.h"

static int dbfs_initstore(const char *rl, struct mangoo_store **mspp);

static int dbfs_create(struct mangoo_store *storep, const char *locator, mode_t mode,
	enum mangoo_types mt)
{
	int error;
	struct dbfs_dentry_key *ddk;
	struct dbfs_dentry_data ddd;
	struct dbfs_store *dsp = (struct dbfs_store *)storep;
	size_t ddk_size;

	printf("dbfs_create:1\n");

	ddk_size = dbfs_ddk_size(locator);
	if (!(ddk = malloc(ddk_size))) {
		error = ENOMEM;
		goto errout1;
	}

	dbfs_init_dentry_key(locator, ddk);

	/* XXX last_ino++ is not thread safe. Also increase filesystem's last_inode value */
	if ((error = dbfs_create_attr(dsp->dbp, ddk, ddk_size, &ddd, mt, mode,
		dsp->last_inode++))) {
		goto errout2;
	}
	
	printf("dbfs_create:10\n");

errout2:
	free(ddk);

errout1:
	return error;
}

static int dbfs_delete(struct mangoo_store *storep, const char *locator)
{
	struct dbfs_handle *dh;
	struct dbfs_store *dsp = (struct dbfs_store *)storep;
	int error;

	printf("dbfs_delete:1 %s\n", locator);
	if (!strcmp(locator, "/")) {
		error = EINVAL;
		goto errout;
	}
	if (!(dh = dbfs_createhandle(locator, dsp->dbp))) {
		error = ENOMEM;
		goto errout;
	}

	if ((error = dbfs_read_attr(dsp->dbp, dh->attr_ddk, dbfs_ddk_size(locator),
		&dh->attr_ddd))) {
		goto errout;
	}

	switch((enum mangoo_types)dh->attr_ddd.ddd_type) {
	case mangoo_container:
		printf("dbfs_delete:2 %s\n", locator);
		if (!(error = dbfs_deld(dh))) {
			printf("dbfs_delete:2.1 %s\n", locator);
			error = dbfs_dela(dh);
		}
		break;

	case mangoo_data:
		printf("dbfs_delete:3 %s\n", locator);
		if (!(error = dbfs_dels(dh))) {
			printf("dbfs_delete:4 %s\n", locator);
			error = dbfs_dela(dh);
		}
		break;

	default:
		error = EINVAL;
		break;
	}

errout:
	return error;
}

static int dbfs_open(struct mangoo_store *storep, const char *locator, enum mangoo_interfaces mi,
	struct mangoo_handle **mhpp)
{
	int error;
	printf("dbfs_open:1\n");
	
	switch(mi) {
	case mangoo_stream_interface:
		error = dbfs_sopen(storep, locator, mhpp);
		break;

	case mangoo_directory_interface:
		error = dbfs_dopen(storep, locator, mhpp);
		break;

	case mangoo_attr_interface:
		error = dbfs_aopen(storep, locator, mhpp);
		break;

	default:
		error = EINVAL;
		break;
	}

	return error;
}

static int dbfs_closestore(struct mangoo_store *storep)
{
	int error;
	struct dbfs_store *dsp = (struct dbfs_store *)storep;
	
	printf("dbfs_closestore:1\n");
	error = dsp->dbp->close(dsp->dbp, 0);

	return error;
}

struct mangoo_operations dbfs_msops = {
	.mo_initstore = dbfs_initstore,
	.mo_closestore = dbfs_closestore,
	.mo_open = dbfs_open,
	.mo_create = dbfs_create,
	.mo_delete = dbfs_delete,
};

static int dbfs_initstore(const char *rl, struct mangoo_store **mspp)
{
	int	error;
	DB	*dbp;
	struct dbfs_store *dsp;
	struct dbfs_fsinfo_data dfd;

	*mspp = NULL;

	printf("dbfs_initstore:1\n");
	if (!(dsp = malloc(sizeof(struct dbfs_store)))) {
		error = ENOMEM;
		goto errout;
	}	
	
	printf("dbfs_initstore:2\n");
	/* Initialize database */
	if ((error = db_create(&dbp, NULL, 0))) {
		fprintf(stderr, "%s: db_create failed", rl);	
		goto errout;
	}
	dbp->set_errfile(dbp, stderr);
	dbp->set_errpfx(dbp, rl);

	printf("dbfs_initstore:3\n");

	if ((error = dbp->set_bt_compare(dbp, dbfs_bt_compare))) {
		dbp->err(dbp, error, "%s: open failed", rl);
		goto errout;
	}

	/* Open database */
	if ((error = dbp->open(dbp,
	    NULL, rl, NULL, DB_BTREE, DB_THREAD, 0))) {
		dbp->err(dbp, error, "%s: open failed", rl);
		goto errout;
	}

	printf("dbfs_initstore:4\n");
	if ((error = dbfs_fetch_fsinfo(dbp, &dfd))) {
		goto errout;
	}
		
	dsp->last_inode = dfd.dfd_lastino;

	dfd.dfd_lastino += 1000;

	printf("dbfs_initstore:5\n");
	if ((error = dbfs_update_fsinfo(dbp, &dfd))) {
		goto errout;
	}

	dsp->dbp = dbp;
	dsp->ms.ms_ops = &dbfs_msops;
	*mspp = (struct mangoo_store *)dsp;
	return 0;

errout:
	if (dsp)
		free(dsp);
	return error;
}
