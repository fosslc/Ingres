/*
** Copyright (c) 2004 Ingres Corporation
*/
# ifndef TBLOBJS_H_INCLUDED
# define TBLOBJS_H_INCLUDED
# include <compat.h>

/**
** Name:	tblobjs.h - table objects
**
** Description:
**	Defines table object name constants and types.
**
** History:
**	16-dec-96 (joea)
**		Created based on tblobjs.h in replicator library.
**	23-jul-98 (abbjo03)
**		Remove objects made obsolete by generic RepServer and renumber
**		the rest.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# define TBLOBJ_MIN_TBLNO	1		/* min table number */
# define TBLOBJ_MAX_TBLNO	99999		/* max table number */
# define TBLOBJ_TBLNO_NDIGITS	5		/* max digits in table no. */

# define TBLOBJ_ARC_TBL		1		/* archive table */
# define TBLOBJ_SHA_TBL		2		/* shadow table */
# define TBLOBJ_SHA_INDEX1	3		/* shadow table index 1 */

# define TBLOBJ_REM_INS_PROC	4		/* remote insert procedure */
# define TBLOBJ_REM_UPD_PROC	5		/* remote update procedure */
# define TBLOBJ_REM_DEL_PROC	6		/* remote delete procedure */

# define TBLOBJ_TMP_TBL		7		/* CreateKeys temporary table */


FUNC_EXTERN STATUS RPtblobj_name(char *tbl_name, i4 tbl_no, i4  obj_type,
	char *obj_name);

# endif
