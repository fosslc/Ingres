/*
** DECLS.H
**	Declaration header file for VIFRED.
**
**	Copyright (c) 2004 Ingres Corporation
**
** History:
**	static	char	Sccsid[] = "@(#)decls.h	1.2	2/6/85";
**	03/13/87 (dkh) - Added support for ADTs.
**	09/30/87 (dkh) - Deleted include of vifrbfc.h - not needed.
**	01-sep-89 (cmr)	put back include for vifrbfc.h.
*/

# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<ctrl.h>
# include	<list.h>
# include	<afe.h>
# include	<vifrbft.h>
# include	<vifrbfc.h>

/*
**	The next include file defines the FORRBF symbol 
**	only for the RFVIFRED directory.
*/
# include	<rbfdef.h>

# include	"pos.h"
# include	"node.h"
# include	"menus.h"
# include	"types.h"
/* # include	"trace.h" Took out (jrc) 4/11/84 */
# include	"globs.h"
