/*
**	Copyright (c) 2004 Ingres Corporation
*/

/*}
** Name:	GT_DEVICE -	Graphics System Device Description Structure
**
** Description:
**	This structure contains fields that describe a device for the Graphics
**	system.  Most of these fields are set from values obtained from the
**	TERMCAP entry for the device or from GKS.
**
**	The data structure is 'G_dev'.
**
** History:
**		
**		2/22/89 (Mike S)
**		add number of marker type supported
**		add metafile fields
**
**		Revision 40.1  86/01/14  12:59:57  bobm
**		add wide line flag
**		
**		Revision 40.0  86/01/06  18:16:38  wong
**		Initial revision.
**	24-sep-96 (mcgem01)
**	    G_dev is a GLOBALREF not an extern.
**		
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

typedef struct
{
	i4	g_cnum;		/* Number of colors on device */
	i4	g_mnum;		/* Number of marker types defined */
	bool	g_mf;		/* TRUE for metafile plots */
	float	g_aspect;	/* Aspect ratio of device */
	/* Flags for device */
	i4	gindelible : 1;	/* device has indelible graphics (includes storage scopes) */
	i4	gplane : 1;	/* device has separate graphics plane */
	i4	govst : 1;	/* text overwrites graphics on device */
	i4	gwline: 1;	/* device has very wide lines */
} GT_DEVICE;

GLOBALREF GT_DEVICE	G_dev;

# define G_cnum		(G_dev.g_cnum)
# define G_mnum		(G_dev.g_mnum)
# define G_mf		(G_dev.g_mf)
# define G_aspect	(G_dev.g_aspect)
# define G_indelible	(G_dev.gindelible)
# define G_gplane	(G_dev.gplane)
# define G_govst	(G_dev.govst)
# define G_wline	(G_dev.gwline)
