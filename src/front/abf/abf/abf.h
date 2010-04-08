/*
** Copyright (c) 2004 Ingres Corporation
** All rights reserved.
*/

/*
**  abf.h -- ABF Includes File.
**
**	the main include file which includes all the right
**	include files to make everything work
*/

# include	<lo.h> 
# include	<si.h> 
# include	<tm.h>
# include	<adf.h> 
# include	<feconfig.h>
# include	<fdesc.h>
# include	<abfrts.h>
# include	<abftrace.h>
# include	<abfcnsts.h>
# include	<ablist.h>
# include	"abfdate.h"
# include	<abfcat.h>
# include	"abftypes.h"
# include	"abfglobs.h"

# ifndef VMS
# define osNewVersion	TRUE	/* only VMS has old OSL */
# endif
