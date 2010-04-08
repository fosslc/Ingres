/*
** Copyright (c) 1990, Ingres Corporation
** All Rights Reserved
*/

/**
** Name: QEAIPROC.H - Constants used with Internal Procedures
**
** Description:
**      This file contains constants that are used with internal procedures
**
** History:
**      02-oct-92 (teresa)
**          Initial Creation
**	29-Dec-2004 (schka24)
**	    Add update-cversion flag.
[@history_template@]...
**/

/*
**      Constants Used with INTERNAL PROCEDURE ii_read_config
*/
#define	    QEA_STATUS      0x0001   /* read dsc_status from config file */
#define	    QEA_CMPTLVL     0x0002   /* read dsc_dbcmptlvl from config file */
#define	    QEA_CMPTMINOR   0x0004   /* read dsc_1dbcmptminor from config file */
#define	    QEA_ACCESS	    0x0008   /* read access from confif file */
#define	    QEA_SERVICE	    0x0010   /* read service value from config file*/

/*
**      Constants Used with INTERNAL PROCEDURE ii_update_config
*/
#define	    QEA_UPD_STATUS    0x00001 /* use status value to upd. config file */
#define	    QEA_UPD_CMPTLVL   0x00002 /* use cmptlvl value to upd. config file*/
#define	    QEA_UPD_CMPTMINOR 0x00004 /* use cmptminor value to upd. config file
				      */
#define	    QEA_UPD_ACCESS    0x0010  /* use access value to upd. config file */
#define	    QEA_UPD_SERVICE   0x0020  /* use service value to upd. config file*/
#define	    QEA_UPD_CVERSION  0x0040  /* use cversion value to upd. config */
