/*
** Copyright (c) 1991, 2008 Ingres Corporation
*/

/*
**
LIBRARY = IMPFRAMELIBDATA
**
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ooclass.h>
#include	<cu.h>
#include	"ercu.h"
#include	"cuesc.h"

/*
** Name:	cuesc.roc	- Table of extended system catalogs
**
** Description:
**	Contains extended system catalogs.
**	Since these catalogs are optional copyapp should only give a warning,
**	not an error, if it is copying an application from a database which
**	has them to a database which does not.
**
** Defines:
**	iiCUescExtSysCatalogs[]
**
** History:
**	14-Jun-91 (blaise)
**		Initial Version
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
*/

/*
** Name:	iiCUescExtSysCatalogs[]
*/

GLOBALDEF const CUESC iiCUescExtSysCatalogs[] =
{
	{CUT_FRAMEVARS,	ERx("ii_framevars"),	S_CU0014_VISION_CATALOG},
	{CUT_MENUARGS,	ERx("ii_menuargs"),	S_CU0014_VISION_CATALOG},
	{CUT_VQJOINS,	ERx("ii_vqjoins"),	S_CU0014_VISION_CATALOG},
	{CUT_VQTABCOLS,	ERx("ii_vqtabcols"),	S_CU0014_VISION_CATALOG},
	{CUT_VQTABLES,	ERx("ii_vqtables"),	S_CU0014_VISION_CATALOG},
	{CU_BADTYPE,	ERx(""),		0}
};
