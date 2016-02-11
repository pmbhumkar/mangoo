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
	/*New Declaration start*/
	DB_ENV *dbenvp,dbenv;
	const char *db_home;
	char *data_dir="/tmp";
	u_int32_t flags;
	int mode,err;
	flags=(DB_CREATE | DB_INIT_LOG | DB_INIT_LOCK | DB_INIT_MPOOL | DB_INIT_TXN);
	db_home=(char *)malloc(sizeof(50));
	/*New Declaration end*/
	/*New Code start*/
	db_home="/tmp";
	
	//mode=(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH );
	//DB_ENV->open(&dbenv,&db_home,flags,mode);*/
	

	/*Creating DB environment for locking*/
	if(error=db_env_create(&dbenvp,0))
	  printf("Error while creating\n");
	else
	  printf("DBFS created\n");
	//dbenvp->get_home(&dbenvp,&db_home);
	//dbenvp->set_errfile(dbenvp, errfp);
	//dbenvp->set_errpfx(dbenvp, progname);

	/*
	 * Specify the shared memory buffer pool cachesize: 5MB.
	 * Databases are in a subdirectory of the environment home.
	 */
	if ((error = dbenvp->set_cachesize(dbenvp, 0, 5 * 1024 * 1024, 0)) != 0) {
	  dbenvp->err(dbenvp, error, "set_cachesize");
	  goto err;
	}
	printf("Environment Cached!\n");
	if ((error = dbenvp->set_data_dir(dbenvp, data_dir)) != 0) {
	  dbenvp->err(dbenvp, error, "set_data_dir: %s", data_dir);
	  goto err;
	}
	printf("Data dir!\n");
	/* Open the environment with full transactional support. */
	if ((error = dbenvp->open(dbenvp, db_home , DB_CREATE |
			       DB_INIT_LOG | DB_INIT_LOCK | DB_INIT_MPOOL | DB_INIT_TXN | DB_PRIVATE, 0)) != 0) {
	  dbenvp->err(dbenvp, error, "environment open: %s", db_home );
	  goto err;
	}
	
	//return (dbenvp);
	//
	//return (NULL);
	//dbenvp->get_home(&dbenv,&db_home);
	/*printf("\n----%s\n%s\n%s\n-----\n",dbenvp->db_log_dir,dbenvp->db_md_dir,dbenvp->db_tmp_dir);
	if(err=dbenvp->open(&dbenv,db_home,,0))
	  printf("\nError while DB open: %d\n",err);
	else
	  printf("\nSuccess in db->open\n");*/
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
 err:(void)dbenvp->close(dbenvp, 0);
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
	  printf("removing data of size - %lu\n",size);
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
