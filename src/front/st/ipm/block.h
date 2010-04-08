/*
**      Copyright (c) 2004 Ingres Corporation
**      All rights reserved.
*/

/*
** block.h - this file contains the typedefs and other data for
**	the routines which find blocked/blocking locks
**
** History
**	4/18/89		tomt	written
**	9/24/89		tomt	defined modes for calling show_block
*/

/*
** MOdes to call show_block with.  These mainly define what title is displayed
** on the form.   The caller determines what resource ID will be examined.
** When called from the Lock List Display form, the show_block routine
** is supplied  with the blocking resource ID
*/
#define SHOWBLOCK	1	/* show all locks on blocking resource */
#define SHOWRES		2	/* show all locks on specified resource */
