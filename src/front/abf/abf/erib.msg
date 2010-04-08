/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	erib.msg -	Message file for class IB
**
** Description:
**	This file defines the messages for class IB.
**
** History:
**	31-jul-1987 (bobm)
**		Created.
**/

# define	_IB_CLASS	48
/*
**	ibdoimg.qc
*/
/*
** status message - argument is application name
*/
S_IB0001_Building	"Building runnable image of %0c"
/*
**	Error message components, for 0 & 2 arguments of E_IB003
*/
F_IB0001_runtime_table	"runtime table"
F_IB0002_reading	"reading"
F_IB0003_image	"image"
F_IB0004_writing	"writing"
/*
**	Fatal error - not supposed to happen
*/
S_IB0002_IIORmfMemFree	"IIORmfMemFree returns failure"	/* DIAG */
/*
**	error.32
*/
E_IB0003_Could_not_open	"Could not open %0c file, \"%1c\", for %2c."
E_IB0004_RTT_format	"Runtime table file, \"%0c\", does not have the correct format."
E_IB0005_Read_frame	"Could not obtain frame, \"%0c\", from the database."
E_IB0006_write_frame	"Could not write frame, \"%0c\", to image file \"%1c\"."
E_IB0007_write_trailer	"Could not write trailer information to image file \"%0c\"."
E_IB0008_init_form	"Could not initialize form \"%0c\"."
E_IB0009_write_form	"Could not write form \"%0c\"."
E_IB000A_interrupt	"Image creation interrupted."
