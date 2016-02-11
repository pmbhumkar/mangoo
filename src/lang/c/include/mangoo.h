/*
	Copyright (C) 2014 Amit S. Kale.
	This file is part of the mangoo filestore framework.

	The mangoo filestore framework is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License version 3 as published by the Free Software Foundation.
*/

#ifndef _MANGOO_H
#define _MANGOO_H

#include <mangooconf.h>

#include <sys/stat.h>

enum mangoo_types {
	mangoo_container,
	mangoo_data,
};

enum mangoo_interfaces {
	mangoo_stream_interface,
	mangoo_directory_interface,
	mangoo_attr_interface
};

typedef int mdir_filler_t(void *fhandle, const char *name, enum mangoo_types type);

#endif /*_MANGOO_H*/
