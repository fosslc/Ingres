/*
**  Copyright (c) 2004 Ingres Corporation
*/

/*{
** Name:	imextern.h	- global declarations for INGMENU
**
** Description:
**	Global FUNC_EXTERNs and GLOBALREFS for INGMENU.  The GLOBALDEFS
**	are in imdata.c.
**
** History:
**	02-sep-1987 (peter)
**		Cleaned up completely for 6.0.
*/

/*	External Declarations of Routines */

FUNC_EXTERN	VOID	abfstart();
#ifndef CMS
FUNC_EXTERN	VOID	graphstart();
#endif
FUNC_EXTERN	VOID	ingresstart();
FUNC_EXTERN	VOID	mqbfstart();
FUNC_EXTERN	VOID	naive();
FUNC_EXTERN	VOID	rbfstart();
FUNC_EXTERN	VOID	reportstart();
FUNC_EXTERN	VOID	swaddforms();
FUNC_EXTERN	STATUS	swcichk();
FUNC_EXTERN	STATUS	swspawn();
FUNC_EXTERN	VOID	tablestart();
FUNC_EXTERN	VOID	vifredstart();
FUNC_EXTERN	VOID	vigstart();
FUNC_EXTERN	VOID	vqlstart();

/*	Exterrnal GlobalRefs for Global Variables */

GLOBALREF	char	*im_dbname;
GLOBALREF	bool	im_empty_catalogs;
GLOBALREF	char	*im_uflag;
GLOBALREF	char	*im_xpipe;
GLOBALREF	bool	testingSW;
GLOBALREF	bool	SWsettest;
