/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	ioi.h -	IOI definitions
**
** Description:
**	Contains the type definitions for calling OI
*/

/*
** current version number being placed in OLIMHDR.  If changed, this number
** should be incremented sequentially with new versions, or the check made
** in ioitrl.c (0 < version <= IMAGE_VERSION) modified.
*/
#define IMAGE_VERSION 1

typedef struct
{
	i4 version;	/* version identifier */
	i4 iact;	/* ILRF interaction mode */
	ABRTSOBJ *rtt;	/* in-memory runtime table */
	FILE *fp;	/* open image file */
	LOCATION loc;	/* image file location */
} OLIMHDR;
