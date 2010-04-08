/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	feqdesc.h -	Front-End Attribute Descriptor Declaration File.
**
** Description:
**	Contains the declaration for an attribute descriptor as used by the
**	front-ends.  This file declares:
**
**	ATT_DESC	attribute descriptor structure declaration.
**
** History:
 * Revision 61.1  88/03/25  23:35:02  sylviap
 * *** empty log message ***
 * 
 * Revision 61.1  88/03/25  21:17:35  sylviap
 * *** empty log message ***
 * 
 * Revision 1.1  88/03/25  12:09:19  sylviap
 * Initial revision
 * 
 * Revision 60.3  87/04/08  00:38:39  joe
 * Added compat, dbms and fe.h
 * 
 * Revision 60.2  86/11/19  13:33:56  barbara
 * Initial60 edit of files
 * 
 * Revision 60.1  86/11/19  13:33:45  barbara
 * *** empty log message ***
 * 
**		Revision 50.0  86/04/08  11:43:58  wong
**		Initial revision
**/

/*}
** Name:	ATT_DESC -	Attribute Descriptor Structure.
**
** Description:
**	This structure describes the descriptor for one attribute of a query
**	target list.
*/
typedef struct {
	char	att_name[FE_MAXNAME+1];	/* attribute name */
	u_i2	att_type;		/* attribute type */
	u_i2	att_len;		/* attribute length */
} ATT_DESC;
