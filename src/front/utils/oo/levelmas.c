/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** History:
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
**
*/

/*
**
LIBRARY = IMPFRAMELIBDATA
**
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>

GLOBALDEF i4	LevelMask[32] = {
	0x1,		0x3,		0x7,		0xf,
	0x1f,		0x3f,		0x7f,		0xff,
	0x1ff,		0x3ff,		0x7ff,		0xfff,
	0x1fff,		0x3fff,		0x7fff,		0xffff,
	0x1ffff,	0x3ffff,	0x7ffff,	0xfffff,
	0x1fffff,	0x3fffff,	0x7fffff,	0xffffff,
	0x1ffffff,	0x3ffffff,	0x7ffffff,	0xfffffff,
	0x1fffffff,	0x3fffffff,	0x7fffffff,	0xffffffff,
};

