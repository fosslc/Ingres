/*
**  ftrange.h
**
**  Header file for support of range specification in QBF.
**
**  Copyright (c) 2004 Ingres Corporation
**
**
**  History:
**
**	Created - 08/08/85 (dkh)
 * Revision 60.3  87/04/08  00:02:54  joe
 * Added compat, dbms and fe.h
 * 
 * Revision 60.2  86/11/18  21:55:12  dave
 * Initial60 edit of files
 * 
 * Revision 60.1  86/11/18  21:55:02  dave
 * *** empty log message ***
 * 
 * Revision 40.5  85/09/26  20:20:04  dave
 * Changed to support new scroll commands. (dkh)
 * 
 * Revision 1.3  85/09/26  17:29:30  dave
 * Changes for new scroll commands. (dkh)
 * 
 * Revision 40.3  85/09/13  22:53:36  dave
 * FTrnglast mechanism redone to better support QBF. (dkh)
 * 
 * Revision 40.2  85/09/07  22:49:39  dave
 * Added support for table field scrolling, etc. (dkh)
 * 
 * Revision 1.2  85/09/07  20:46:06  dave
 * Added support for table field scrolling, etc. (dkh)
 * 
 * Revision 1.1  85/08/21  20:28:43  dave
 * Initial revision
 * 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*
**  Maximum number of rows supported for a range specification
**  for a table field.
*/

# define	MAX_RGROWS		25

/*
**  Maximum buffer size to initially allocate for a field
**  or column in a table field.
*/

# define	MAX_I_BFSIZE		100


# define	RGREGFLD		0
# define	RGTBLFLD		1


# define	RG_H_ADD		0
# define	RG_T_ADD		1


typedef struct rrgfld
{
	char	*hcur;
	char	*hend;
	char	*hbuf;
	char	*tcur;
	char	*tend;
	char	*tbuf;
	char	*rwin;
	char	*pdat;
} RGRFLD;




typedef struct trgfld
{
	i4	rgmaxrows;
	i4	rgmaxcol;
	i4	rgtoprow;
	i4	rgretrow;
	i4	*colwidth;
	RGRFLD	**rgrows;
	i4	rglastrow;
	i4	rgllstrow;
} RGTFLD;




typedef struct rgfld
{
	i4	rgfltype;
	union
	{
		RGRFLD	*rgregfld;
		RGTFLD	*rgtblfld;
	} rg_var;
} RGFLD;
