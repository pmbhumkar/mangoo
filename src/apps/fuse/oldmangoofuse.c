/*
	Copyright (C) 2014 Amit S. Kale.
	This file is part of the mangoo filestore framework.

	The mangoo filestore framework is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public
	License version 3 as published by the Free Software Foundation.
*/

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <utime.h>
#include <malloc.h>

#define FUSE_USE_VERSION 26
#include <fuse/fuse.h>
#include <fuse/fuse_lowlevel.h>

#include <mangooapp.h>

static struct mangoo_store *ms;

struct mf_dir {
	struct mangoo_handle *handle;
	off_t cur_off;
	off_t starting_off;
	char *saved_name;
	off_t saved_off;
	enum mangoo_types saved_type;
	fuse_fill_dir_t filler;
	void *buf;
};

static int mf_getattr(const char *path, struct stat *stbuf)
{
	struct mangoo_handle *handle = NULL;
	mode_t type;
	enum mangoo_types mt;
	int error;
	printf("mf_getattr:1 %s\n", path);

	if ((error = mangoo_open(ms, path, mangoo_attr_interface, &handle))) {
		goto errout1;
	}

	printf("mf_getattr:2 %s\n", path);
	if ((error = mattr_getuid(handle, &stbuf->st_uid)))
		goto errout2;

	printf("mf_getattr:3 %s\n", path);
	if ((error = mattr_getgid(handle, &stbuf->st_gid)))
		goto errout2;

	printf("mf_getattr:4 %s\n", path);
	if ((error = mattr_getmtime(handle, &stbuf->st_mtime)))
		goto errout2;
	stbuf->st_ctime = stbuf->st_mtime;

	printf("mf_getattr:5 %s\n", path);
	if ((error = mattr_getatime(handle, &stbuf->st_atime)))
		goto errout2;

	printf("mf_getattr:6 %s\n", path);
	if ((error = mattr_getsize(handle, &stbuf->st_size)))
		goto errout2;

	printf("mf_getattr:7 %s\n", path);
	if ((error = mattr_getmode(handle, &stbuf->st_mode)))
		goto errout2;

	printf("mf_getattr:8 %o\n", (int)stbuf->st_mode);
	if ((error = mattr_gettype(handle, &mt)))
		goto errout2;

	printf("mf_getattr:9 %d\n", (int)mt);
	switch (mt) {
		case mangoo_data:
			printf("mf_getattr:9.1 data\n");
			stbuf->st_nlink = 1;
			stbuf->st_mode |= S_IFREG;
			break;

		case mangoo_container:
			printf("mf_getattr:9.2 container\n");
			stbuf->st_nlink = 2;
			stbuf->st_mode |= S_IFDIR;
			break;

		default:
			error = EINVAL;
			goto errout2;
	}

	printf("mf_getattr:10\n");
	if ((error = mangoo_close(handle)))
		goto errout1;

	return 0;

errout2:
	(void)mangoo_close(handle);

errout1:
	printf("mf_getattr:12 %s\n", strerror(error));
	return -error;
}

static int mf_chmod(const char *path, mode_t mode)
{
  struct mangoo_handle *handle = NULL;
  int error;
  printf("mf_chmod\n");
  if ((error = mangoo_open(ms, path, mangoo_attr_interface, &handle))) { 
    printf("mf_getattr: %s\n", strerror(error));
    return -error;
  }
  if((error = mattr_chmod(handle,mode)))
      (void)mangoo_close(handle);
  return 0;  
}

static int mf_chown(const char *path,uid_t newuid,gid_t newgid)
{
  struct mangoo_handle *handle = NULL;
  int error;
  printf("mf_chown\n");
  if ((error = mangoo_open(ms, path, mangoo_attr_interface, &handle))) { 
    printf("mf_getattr: %s\n", strerror(error));
    return -error;
  }
  if((error = mattr_chown(handle,newuid,newgid)))
      (void)mangoo_close(handle);
  return 0;  
}

static int mf_utime(const char *path, struct utimbuf *utb)
{
	struct mangoo_handle *handle = NULL;
	int error;

	printf("mf_utime:1 %s\n", path);
	if ((error = mangoo_open(ms, path, mangoo_attr_interface, &handle))) {
		goto errout1;
	}

	printf("mf_utime:2\n");
	if ((error = mattr_setmtime(handle, utb->modtime)))
		goto errout2;

	printf("mf_utime:3\n");
	if ((error = mattr_setatime(handle, utb->actime)))
		goto errout2;

	printf("mf_utime:4\n");
	if ((error = mangoo_close(handle)))
		goto errout1;

	return 0;

errout2:
	(void)mangoo_close(handle);

errout1:
	printf("mf_utime:5 %s\n", strerror(error));
	return -error;
}

static int mf_open(const char *path, struct fuse_file_info *fi)
{
	int error;
	struct mangoo_handle *handle;

	printf("mf_open:1 %s\n", path);
	fi->fh = 0;
	if ((error = mangoo_open(ms, path, mangoo_stream_interface, &handle))) {
		goto errout;
	}

	fi->fh = (uint64_t)handle;
	printf("mf_open:2 %lx\n", (uint64_t)handle);
	return 0;

errout:
	return -error;
}

static int mf_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	int error;
	struct mangoo_handle *handle;

	printf("mf_create:1 %s mode %o\n", path, (int)mode);
	fi->fh = 0;
	if ((error = mangoo_create(ms, path, mode, mangoo_data))) {
		goto errout;
	}

	if ((error = mangoo_open(ms, path, mangoo_stream_interface, &handle))) {
		goto errout;
	}

	fi->fh = (uint64_t)handle;
	printf("mf_create:2 %lx\n", (uint64_t)handle);
	return 0;

errout:
	return -error;
}

static int mf_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	struct mangoo_handle *handle = (struct mangoo_handle*)fi->fh;
	int error;

	printf("mf_read:1 %s\n", path);
	if (!fi->fh)
		return -EINVAL;
	printf("mf_read:2 %lx\n", (uint64_t)handle);
	if ((error = mstream_read(handle, buf, &size, offset)))
		return -error;
	return size;
}

static int mf_write(const char *path, const char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	struct mangoo_handle *handle = (struct mangoo_handle*)fi->fh;
	int error;

	printf("mf_write:1 %s\n", path);
	if (!fi->fh)
		return -EINVAL;
	printf("mf_write:2 %lx\n", (uint64_t)handle);
	if ((error = mstream_write(handle, buf, &size, offset)))
		return -error;
	return size;
}

static int mf_release(const char *path, struct fuse_file_info *fi)
{
	struct mangoo_handle *handle = (struct mangoo_handle*)fi->fh;

	printf("mf_release:1 %s\n", path);
	if (!fi->fh)
		return -EINVAL;
	printf("mf_release:2 %lx\n", (uint64_t)handle);
	return -mangoo_close(handle);
}

static int mf_filler(void *fhandle, const char *name, enum mangoo_types type)
{
	struct mf_dir *dir = (struct mf_dir*)fhandle;
	struct stat stbuf;

	printf("mf_filler:1 %s %d\n", name, (int)type);
	switch(type)
	{
		case mangoo_data:
			stbuf.st_mode = S_IFREG;
			break;

		case mangoo_container:
			stbuf.st_mode = S_IFDIR;
			break;

		default:
			stbuf.st_mode = 0;
			break;
	}
	stbuf.st_ino = 0;
	printf("mf_filler:2 cur_off %lu starting_off %lu\n", dir->cur_off, dir->starting_off);
	if (dir->cur_off >= dir->starting_off) {
		printf("mf_filler:3 %s\n", name);
		if (dir->filler(dir->buf, name, &stbuf, dir->cur_off)) {
			printf("mf_filler:4\n");
			dir->saved_name = strdup(name);
			dir->saved_type = type;
			dir->saved_off = dir->cur_off;
			return ENOBUFS;
		}
	}

	dir->cur_off++;
	return 0;
}

static int mf_opendir(const char *path, struct fuse_file_info *fi)
{
	struct mf_dir *dir;
	int error;	
	
	printf("mf_opendir:1\n");
	if(!(dir = malloc(sizeof(struct mf_dir))))
		return -ENOMEM;

	dir->saved_name = NULL;
	dir->cur_off = 0;

	if ((error = mangoo_open(ms, path, mangoo_directory_interface, &dir->handle))) {
		printf("mf_opendir:2\n");
		return -error;
	}

	fi->fh = (uint64_t)dir;
	return 0;
}

static int mf_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi)
{
	struct mf_dir *dir = (struct mf_dir *)fi->fh;
	int error;

	printf("mf_readdir:1\n");
	dir->filler = filler;
	dir->buf = buf;
	if (!offset) {
		dir->starting_off = 0;
		mf_filler(dir, ".", mangoo_container);
		mf_filler(dir, "..", mangoo_container);
		offset += 2;
	} else {
		if (dir->saved_name) {
			if(dir->saved_off == offset) {
				mf_filler(dir, dir->saved_name, dir->saved_type);
				offset++;
			}
			free(dir->saved_name);
			dir->saved_name = NULL;
		}
	}
	dir->starting_off = offset;
	dir->buf = buf;
	error = mdir_read(dir->handle, mf_filler, dir);
	if (error == ENOBUFS)
		error = 0;
	return -error;
}

static int mf_releasedir(const char *path, struct fuse_file_info *fi)
{
	struct mf_dir *dir = (struct mf_dir *)fi->fh;

	printf("mf_releasedir:1\n");
	(void)mangoo_close(dir->handle);
	if (dir->saved_name)
		free(dir->saved_name);
	free(dir);
	return 0;
}

static int mf_mkdir(const char *path, mode_t mode)
{
	int error;

	printf("mf_mkdir:1 %s\n", path);
	if ((error = mangoo_create(ms, path, mode, mangoo_container))) {
		goto errout;
	}

	return 0;

errout:
	return -error;

}

static int mf_truncate(const char *path, off_t offset)
{
	int error;
	struct mangoo_handle *handle;

	printf("mf_truncate:1\n");
	if ((error = mangoo_open(ms, path, mangoo_attr_interface, &handle)))
		return -error;
	error = mattr_setsize(handle, offset);
	(void)mangoo_close(handle);
	return -error;
}

static int mf_remove(const char *path)
{
	int error;

	printf("mf_remove:1\n");
	error = mangoo_delete(ms, path);

	return -error;
}

static struct fuse_operations mf_ops = {
	.getattr = mf_getattr,
	.utime = mf_utime,
	.open = mf_open,
	.read = mf_read,
	.write = mf_write,
	.release = mf_release,
	.create = mf_create,
	.opendir = mf_opendir,
	.releasedir = mf_releasedir,
	.readdir = mf_readdir,
	.mkdir = mf_mkdir,
	.truncate = mf_truncate,
	.unlink = mf_remove,
	.rmdir = mf_remove,
	.chmod = mf_chmod,
	.chown= mf_chown,
};

int main(int argc, char *argv[])
{
	struct fuse_args args;
	const char *progname = argv[0];
	const char *url = argv[1];
	int error;
	char *rl;

	if (argc < 3 || !(rl = strstr(url, ":"))) {
		fprintf(stderr, "Format: %s scheme:resource-locator mountpoint\n", progname);
		error = 1;
		goto out;
	}

	*rl = '\0';
	rl++;
	if ((error = mangoo_initstore(url, rl, &ms))) {
		error = 1;
		fprintf(stderr, "%s: Error in mangoo_initstore\n", progname);
		goto out;
	}

	/* Skip store locator and pass the rest to fuse */
	args.argc = argc - 1;
	args.argv = argv + 1;
	args.allocated = 0;
	argv[1] = argv[0];

	error = fuse_main(args.argc, args.argv, &mf_ops, NULL);

	mangoo_closestore(ms);

out:
	return error;
}
