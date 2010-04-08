/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	ertb.msg -	Message file for class TB
**
** Description:
**	This file defines the messages for class TB.
**
** History:
**	15-aug-1987 (dave)
**		Created.
**	12/12/87 (dkh) - Modified messages E_TB0001* and E_TB0002*.
**	16-mar-94 (lan)
**		Fixed bug #58066 - Replaced 'Technical Support Representative'
**		with 'Site Technical Contact'.
**/

# define	_TB_CLASS	58
/*
**	iiclrcol.c
*/
E_TB0001_Cannot_clear_column
"Cannot clear column <%0c>.\n  An internal error prevents the
column from being cleared.  Please write down circumstances
that led to the error and report it to your Site Technical
Contact.  You may either try again or restart your
application."

/*%
**  Description:
**	Internal error occurred when the system tried to
**	clear the column.
**
**  System Status:
**	System will not clear the specified column.
**
**  User Action:
**	User can either try again or restart the application.
*/

/*
**	iiclrrow.c
*/
E_TB0002_Cannot_clear_row
"Cannot clear row <%0d>.  An internal error prevents all the columns
in the row from being cleared.  Please write down circumstances that led
to the error and report it to your Site Technical Contact.
You may either try again or restart your application."

/*%
**  Description:
**	Internal error occurred when one of the columns in the
**	specified row was being cleared.
**
**  System Status:
**	System will not complete the clear row operation.
**
**  User Action:
**	User can either try again or restart the application.
*/

/*
**	iitbbug.c
*/
S_TB0003_INTERNAL_EQUEL_BUG	"INTERNAL EQUEL BUG: %s."	/* DIAG */
/*
**	iitbdata.c
*/
S_TB0004_IItdata_table_reset	"IItdata: table reset"	/* DIAG */
/*
**	iitbputf.c
*/
F_TB0001_No_scrolling_allowed	"No scrolling allowed on bare table fields"
F_TB0002_No_scrolling_allowed	"No scrolling allowed on bare table fields"
F_TB0003_Nothing_to_scroll	"Nothing to scroll, table field is empty"
F_TB0004_Out_of_data	"Out of data"
/*
**	iitbscrll.c
*/
F_TB0005_Out_of_data_below	"Out of data below"
F_TB0006_Out_of_data_above	"Out of data above"
/*
**	iitbunld.c
*/
S_TB0005_IItunload_IIcurtbl	"IItunload: IIcurtbl is NULL"	/* DIAG */
S_TB0006_IItunload_table_not	"IItunload: table not unloading"/* DIAG */
/*
**	iitsetcol.c
*/
S_TB0007_Bad_table_state	"ds_set: Bad table state"	/* DIAG */
