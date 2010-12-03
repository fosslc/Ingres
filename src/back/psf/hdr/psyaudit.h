/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/
/*
** Name: PSYAUDIT.H
**
** Description:
**	This file contains the prototype for the function psy_secaudit(), it
**	should be included when calls to this routine are made. It is placed 
**	in a seperate file because the file sxf.h should also be included to 
**	define the SXF structures used in the prototype, sxf.h is not
**	included in all psf modules.
**
** History:
**	11-nov-93 (stephenb)
**	    Initial creation.
**	18-feb-94 (robf)
**          Upgraded to match Secure 2.0 interface, with security labels
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
*/

/* psyaudit.c */
FUNC_EXTERN i4 psy_audit(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb);
FUNC_EXTERN i4 psy_secaudit(
	int force,
	PSS_SESBLK *sess_cb,
	char *objname,
	DB_OWN_NAME *objowner,
	i4 objlength,
	SXF_EVENT auditevent,
	i4 msg_id,
	SXF_ACCESS accessmask,
	DB_ERROR *err);
