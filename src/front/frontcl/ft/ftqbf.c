/*
**  ftqbf.c
**
**  Special routines for QBF to not validate data
**  entered into a table field.
**
**  Copyright (c) 2004 Ingres Corporation
**
**
**  History:
**
**	Created - 08/17/85 (dkh)
 * Revision 60.3  87/04/08  00:02:18  joe
 * Added compat, dbms and fe.h
 * 
 * Revision 60.2  86/11/18  21:54:17  dave
 * Initial60 edit of files
 * 
 * Revision 60.1  86/11/18  21:54:08  dave
 * *** empty log message ***
 * 
 * Revision 40.1  85/08/23  12:51:22  dave
 * Initial version in 4.0 rplus path. (dkh)
 * 
 * Revision 1.1  85/08/21  20:28:27  dave
 * Initial revision
 * 
**      23-sep-96 (mcgem01)
**              Global data moved to ftdata.c
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>

GLOBALREF	bool	FTqbf;

FTqbfval(value)
bool	value;
{
	FTqbf = value;
}
