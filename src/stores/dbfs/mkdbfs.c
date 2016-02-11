/*
	Copyright (C) 2014 Amit S. Kale.
	This file is part of the mangoo filestore framework.

	The mangoo filestore framework is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public
	License version 3 as published by the Free Software Foundation.
*/

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <malloc.h>

#include "dbfs.h"

int main(int argc, const char *argv[])
{
	DB *dbp;
	int error;
	const char *progname = argv[0];
	const char *dbname = argv[1];
	int retval = 0;
	struct dbfs_fsinfo_data dfd;
	struct dbfs_dentry_key *ddk;
	struct dbfs_dentry_data ddd;
	size_t ddk_size;

	if (argc != 2) {
		fprintf(stderr, "Format: %s file name\n", progname);
		goto out1;
	}
	if ((error = db_create(&dbp, NULL, 0))) {
		fprintf(stderr, "%s: db_create failed", progname);	
		goto out1;
	}
	dbp->set_errfile(dbp, stderr);
	dbp->set_errpfx(dbp, progname);

	if ((error = dbp->set_bt_compare(dbp, dbfs_bt_compare))) {
		dbp->err(dbp, error, "%s: open failed", dbname);
		goto out1;
	}

	/* Create database */
	if ((error = dbp->open(dbp,
	    NULL, dbname, NULL, DB_BTREE, DB_CREATE | DB_EXCL | DB_THREAD, 0664))) {
		dbp->err(dbp, error, "open: %s", dbname);
		goto out1;
	}

	ddk_size = dbfs_ddk_size("/");
	if (!(ddk = malloc(ddk_size))) {
		goto out2;
	}

	dbfs_init_dentry_key("/", ddk);

	if ((error = dbfs_create_attr(dbp, ddk, ddk_size, &ddd, mangoo_container, 
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, DBFS_ROOT_INO))) {
		goto out2;
	}

	dfd.dfd_lastino = DBFS_ROOT_INO;

	if ((error = dbfs_create_fsinfo(dbp, &dfd)))
		goto out2;

	printf("dbfs created successfully.\n");

out2:
	if ((dbp->close(dbp, 0))) {
		fprintf(stderr, "%s: close failed", progname);	
	}

out1:
	return retval;
}
