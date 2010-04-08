/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** static Sccsid[] = "@(#)writefrm.h	1.1	1/24/85";
*/

/*
** Special data structure for communication of line table and
** frame to writefrm module.
*/
typedef struct {
	i4	linesize;
	VFNODE	**line;
	FRAME	*frame;
} WRITEFRM;

