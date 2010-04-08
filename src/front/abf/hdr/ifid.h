/*
**  Copyright (c) 2004 Ingres Corporation
*/
typedef struct
{
	char *name;	/* name of frame */
	i4 id;
	i4 mode;	/* mode (DB vs. image file).  For future use */
} FID;
