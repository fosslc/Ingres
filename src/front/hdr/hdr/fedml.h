/*	
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**
**	Contains definition of symbols for SQL utility routines.
**
** History
**
 * Revision 61.1  88/03/25  23:34:47  sylviap
 * *** empty log message ***
 * 
 * Revision 61.1  88/03/25  21:17:10  sylviap
 * *** empty log message ***
 * 
 * Revision 60.6  88/03/24  10:42:22  sylviap
 * Integrated with DG path.
 * 
 * Revision 1.2  88/01/27  15:00:14  danielt
 * *** empty log message ***
 * 
 * Revision 60.5  87/12/15  11:05:35  bobm
 * FEDMLERROR
 * 
 * Revision 60.4  87/09/04  16:19:42  bobm
 * no change
 * 
 * Revision 60.3  87/04/08  00:38:26  joe
 * Added compat, dbms and fe.h
 * 
 * Revision 60.2  86/11/19  13:32:47  barbara
 * Initial60 edit of files
 * 
 * Revision 60.1  86/11/19  13:32:37  barbara
 * *** empty log message ***
 * 
**		Revision 41.5  85/12/20  17:37:42  joe
**		One query lang fix.
**		
**		Revision 41.4  85/10/28  16:13:39  joe
**		Y-line integration
**		
**		Revision 41.2  85/09/10  13:00:02  joe
**		Updated automatically with ibm/sql/international changes.
**		
 * Revision 4.0  85/08/12  17:37:58  joe
 * *** empty log message ***
 * 
**		Revision 41.1  85/07/17  09:18:54  robin
**		Stable version as of 7/15/85.  Copied from sql/src/hdr development path.
**		
**		Revision 40.1  85/06/18  15:47:01  joe
**		Initial version
**		
 * Revision 1.1  85/06/18  15:46:11  joe
 * Initial revision
 * 
**		Revision 41.3  85/10/22  11:35:26  peter
**		Add FEDMLNONE constant.
**		
*/

/*
** Constants used by fedml()
**
** NB
**	These constants are stored in the ABF catalogs
**	and must not be changed without first conferring
**	with ABF implementers.
**
*/

# define	FEDMLNONE	0
# define	FEDMLSQL	1
# define	FEDMLQUEL	2
# define	FEDMLBOTH	3
# define	FEDMLERROR	4
# define	FEDMLGSQL	5

/*
** Constants for maximum sizes of type names in
** different languages.
*/

# define	FEQUELSIZE	11	/* text(2000) + 1 for NULL */
# define	FESQLSIZE	12	/* vchar(2000) + 1 for NULL */
# define	FEDMLSIZE	12

