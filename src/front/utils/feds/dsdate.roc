
# include <compat.h>
# include	<ds.h>
# include	<dateadt.h>

/*
** Copyright (c) 2004 Ingres Corporation
**
** dsdate.c -- Data Template Structure Definition for a DATENTRNL data structure.
**
**	History:
**		Revision 1.2  86/04/24  11:29:20  daveb
**		remove comment cruft.
**		
**		Revision 1.2  85/03/25  15:43:51  ron
**		Integration from VMS - now uses a file pointer and CL routines
**		instead of file descriptors and Unix system calls.
**
**		Revision 1.1  85/02/13  09:37:37  ron
**		Initial version of new DS routines from the CFE project.
*/


static	DSTEMPLATE DSdatentrnl = {
	/* ds_id */		DS_DATENTRNL,
	/* ds_size */		sizeof(DATENTRNL),
	/* ds_cnt */		0,
	/* dstemp_type */	DS_IN_CORE,
	/* ds_ptrs */		NULL,
	/* dstemp_file */	NULL,
	/* dstemp_func */	NULL	
	};
