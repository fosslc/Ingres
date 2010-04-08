/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DMPBUILD.H - Access Method Build interfaces
**
** Description:
**      This file describes the interface to the Access Method Build
**	services.
**
** History:
**	 7-jul-1992 (rogerk)
**	    Created for DMF Prototyping.
** 	08-apr-1999 (wonst02)
** 	    Add prototypes for dm1m... (RTREE) build routines.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1?b? functions converted to DB_ERROR *
**/

FUNC_EXTERN DB_STATUS	dm1hbbegin(DM2U_M_CONTEXT *mct, 
				    DB_ERROR 	*dberr);
FUNC_EXTERN DB_STATUS	dm1hbend(DM2U_M_CONTEXT *mct,
				    DB_ERROR 	*dberr);
FUNC_EXTERN DB_STATUS	dm1hbput(
				    DM2U_M_CONTEXT  *mct,
				    i4         bucket,
				    char            *record,
				    i4         record_size,
				    i4         dup,
				    DB_ERROR 	*dberr);

FUNC_EXTERN DB_STATUS	dm1ibbegin(DM2U_M_CONTEXT *mct,
				    DB_ERROR 	*dberr);
FUNC_EXTERN DB_STATUS	dm1ibend(DM2U_M_CONTEXT *mct,
				    DB_ERROR 	*dberr);
FUNC_EXTERN DB_STATUS	dm1ibput(
				    DM2U_M_CONTEXT  *mct,
				    char            *record,
				    i4         record_size,
				    i4         dup,
				    DB_ERROR 	*dberr);

FUNC_EXTERN DB_STATUS	dm1sbbegin(DM2U_M_CONTEXT *mct,
				    DB_ERROR 	*dberr);
FUNC_EXTERN DB_STATUS	dm1sbend(DM2U_M_CONTEXT	*mct,
				    DB_ERROR 	*dberr);
FUNC_EXTERN DB_STATUS	dm1sbput(
				    DM2U_M_CONTEXT  *mct,
				    char            *record,
				    i4         record_size,
				    i4         dup,
				    DB_ERROR 	*dberr);

FUNC_EXTERN DB_STATUS	dm1bbbegin(DM2U_M_CONTEXT *mct,
				    DB_ERROR 	*dberr);
FUNC_EXTERN DB_STATUS	dm1bbend(DM2U_M_CONTEXT *mct,
				    DB_ERROR 	*dberr);
FUNC_EXTERN DB_STATUS	dm1bbput(
				    DM2U_M_CONTEXT  *mct,
				    char	    *record,
				    i4         record_size,
				    i4         dup,
				    DB_ERROR 	*dberr);

FUNC_EXTERN DB_STATUS	dm1mbbegin(
			DM2U_M_CONTEXT   *mct,
			DB_ERROR         *dberr);

FUNC_EXTERN DB_STATUS	dm1mbend(
			DM2U_M_CONTEXT	*mct,
			DB_ERROR         *dberr);

FUNC_EXTERN DB_STATUS	dm1mbput(
			DM2U_M_CONTEXT   *mct,
			char            *record,
			i4         record_size,
			i4         dup,
			DB_ERROR         *dberr);
