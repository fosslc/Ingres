/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	abffile.h -	ABF File Structure.
*/

typedef struct abfileType {
	char		*fisdir;	/* The source dir for the file */
	char		*fisname;	/* The name of the source file */
	char		*finoext;	/* file name with no ext */
	LOCATION	fiofile;	/* The object file */
	char		*fiobuf;	/* The buffer for the object file */
	i4		firefcnt;	/* files are reference counted */
} ABFILE;
