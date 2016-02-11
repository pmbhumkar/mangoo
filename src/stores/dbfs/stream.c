/*
	Copyright (C) 2014 Amit S. Kale.
	This file is part of the mangoo filestore framework.

	The mangoo filestore framework is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public
	License version 3 as published by the Free Software Foundation.
*/

#include <string.h>
#include <malloc.h>
#include <errno.h>

#include "dbfs.h" 

int dbfs_readwrite(DB *dbp, uint64_t ino, char *buf, size_t *sp, off_t offset, int read)
{
	DBT key, data;
	struct dbfs_block_key dbk;
	size_t offinblk, sizeinblk;
	size_t count = 0;
	int error = 0;
	size_t size = *sp;

	
	printf("dbfs_readwrite:1\n");
	memset(&key, 0, sizeof(DBT));
	key.data = &dbk;
	key.size = sizeof(dbk);
	key.flags = DB_DBT_USERMEM;
	key.ulen = key.size;

	dbk.dbk_ino = ino;
	dbk.dbk_dk.dk_type = kt_block;
	dbk.dbk_offset = offset - (offset & (DBFS_BLKSIZE - 1));

	memset(&data, 0, sizeof(DBT));
	data.data = NULL;
	data.size = 0;
	data.ulen = 0;

	//Iterate over blocks in the given range
	while(size) {
		offinblk = offset - dbk.dbk_offset;
		sizeinblk = size;
		if ((offinblk + sizeinblk) > DBFS_BLKSIZE)
			sizeinblk = DBFS_BLKSIZE - offinblk;

		printf("dbfs_readwrite:2 blkoff %lu offinblk %d sizeinblk %d\n", dbk.dbk_offset,
			(int)offinblk, (int)sizeinblk);
		data.ulen = data.size;
		if (data.size)
			data.flags = DB_DBT_USERMEM;
		else
			data.flags = DB_DBT_MALLOC;

		if ((error = dbp->get(dbp, NULL, &key, &data, 0))) {
			if (error != DB_NOTFOUND) {
				dbp->err(dbp, error, "Can't fetch block in inode %llu at offset "
					"%llu\n", ino, dbk.dbk_offset);
				error = EIO;
				goto out;
			}
		
			if (read) {
				memset(buf, 0, sizeinblk);
			} else {
				data.data = malloc(DBFS_BLKSIZE);
				data.size = DBFS_BLKSIZE;
			}
		}

		if (read) {
			if (!error) {
				memcpy(buf, ((char *)data.data) + offinblk, sizeinblk);
			}
		} else {
			//Update or create
			data.ulen = DBFS_BLKSIZE;
			data.flags = 0;

			memcpy(((char *)data.data) + offinblk, buf, sizeinblk);

			if ((error = dbp->put(dbp, NULL, &key, &data, 0))) {
				dbp->err(dbp, error, "Can't write block in inode %llu at offset %llu\n",
					ino, dbk.dbk_offset);
				error = EIO;
				goto out;
			}
		}

		offset += sizeinblk;
		buf += sizeinblk;
		size -= sizeinblk;
		dbk.dbk_offset = offset;
		count += sizeinblk;
	}

out:
	if (data.data) {
		free(data.data);
	}
	*sp = count;
	return error;
}

int dbfs_truncateblocks(DB *dbp, uint64_t ino, off_t offset, off_t end)
{
	DBT key;
	struct dbfs_block_key dbk;
	size_t offinblk, sizeinblk;
	int error = 0;
	size_t size = end - offset;

	printf("dbfs_truncateblocks:1\n");
	memset(&key, 0, sizeof(DBT));
	key.data = &dbk;
	key.size = sizeof(dbk);
	key.flags = DB_DBT_USERMEM;
	key.ulen = key.size;

	dbk.dbk_ino = ino;
	dbk.dbk_dk.dk_type = kt_block;
	dbk.dbk_offset = offset - (offset & (DBFS_BLKSIZE - 1));

	//Iterate over blocks in the given range
	while(size<=0) {
		printf("dbfs_truncateblocks:1 offset %lu\n", dbk.dbk_offset);
		offinblk = offset - dbk.dbk_offset;
		sizeinblk = size;
		if ((offinblk + sizeinblk) > DBFS_BLKSIZE)
			sizeinblk = DBFS_BLKSIZE - offinblk;

		if ((error = dbp->del(dbp, NULL, &key, 0))) {
			if (error != DB_NOTFOUND) {
				dbp->err(dbp, error, "Deleting a block at offset %llu\n", ino,
					dbk.dbk_offset);
				error = EIO;
				goto out;
			}
		}

		offset += sizeinblk;
		size -= sizeinblk;
		dbk.dbk_offset = offset;
	}

out:
	return error;
}

int dbfs_sopen(struct mangoo_store *storep, const char *locator, struct mangoo_handle **mhpp)
{
	int error;
	struct dbfs_handle *dh;
	struct dbfs_store *dsp = (struct dbfs_store *)storep;

	if (!(dh = dbfs_createhandle(locator, dsp->dbp))) {
		error = ENOMEM;
		goto errout;
	}

	dh->mh.mh_interface = mangoo_stream_interface;
	dh->mh.mh_fvector = (void *)&dbfs_stream_ops;

	if ((error = dbfs_read_attr(dsp->dbp, dh->attr_ddk, dbfs_ddk_size(locator),
		&dh->attr_ddd))) {
		goto errout;
	}

	*mhpp = (struct mangoo_handle *)dh;

	return 0;

errout:
	return error;
}

static int dbfs_read(struct mangoo_handle *mh, char *buf, size_t *size, off_t offset)
{
	struct dbfs_handle *dh = (struct dbfs_handle*)mh;

	printf("dbfs_read:1 size %d\n", (int)*size);
	if (offset > dh->attr_ddd.ddd_size)
		return 0;
	if ((offset + *size) > dh->attr_ddd.ddd_size)
		*size = dh->attr_ddd.ddd_size - offset;
	return dbfs_readwrite(dh->dbp, dh->attr_ddd.ddd_ino, buf, size, offset, 1);
}

static int dbfs_write(struct mangoo_handle *mh, const char *buf, size_t *size, off_t offset)
{
	int error;
	struct dbfs_handle *dh = (struct dbfs_handle*)mh;

	if ((error = dbfs_readwrite(dh->dbp, dh->attr_ddd.ddd_ino, (char *)buf, size, offset, 0))) {
		return error;
	}

	if (dh->attr_ddd.ddd_size < (offset + *size)) {
		dh->attr_ddd.ddd_size = offset + *size;
	}
	error = dbfs_write_attr(dh->dbp, dh->attr_ddk, &dh->attr_ddd);

	return error;
}

static int dbfs_close(struct mangoo_handle *mhp)
{
	struct dbfs_fhandle *dh=(struct dbfs_fhandle *)mhp;
	free(dh);

	return 0;
}

int dbfs_dels(struct dbfs_handle *dh)
{
	int error;

	error = dbfs_truncateblocks(dh->dbp, dh->attr_ddd.ddd_ino, 0, dh->attr_ddd.ddd_size);

	return error;
}

struct mangoo_stream_operations dbfs_stream_ops = {
	.mso_read = dbfs_read,
	.mso_write = dbfs_write,
	.mso_close = dbfs_close,
};
