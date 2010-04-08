/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DM1SD.H - Smart Disk Header File
**
** Description:
**	This file describes the interface to the Smart Disk Manipulation
**	services.
**
** History:
**	 09-jul-92 (jrb)
**	    Created for DMF prototyping.
**	05_dec_1992 (kwatts)
**	    Smart Disk enhancement, remove one parameter from
dm1sd_position.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

FUNC_EXTERN DB_STATUS dm1sd_disktype(
				DMP_TCB *t,
				i4 *errcode);

FUNC_EXTERN DB_STATUS dm1sd_position(
				DMP_RCB *rcb,
				u_char *dmr_sd_cb,
				i4 *errcode);

FUNC_EXTERN DB_STATUS dm1sd_get(
				DMP_RCB *rcb,
				DM_TID *tid,
				char *dest,
				i4 flag,
				i4 *errcode);
