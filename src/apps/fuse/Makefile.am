AM_CFLAGS = -D_FILE_OFFSET_BITS=64
LDADD = ../../lang/c/lib/libmangoo.a ../../stores/dbfs/libdbfs.a -lfuse -ldb
AM_LDFLAGS = -L../../stores/dbfs -L../../lang/c/lib

bin_PROGRAMS = mangoofuse

mangoofuse_SOURCES = mangoofuse.c
