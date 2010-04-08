/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:    gtdef.h    - Includes differences in FTframe.h between FT and GT.
**
** History:
**/

/*
**	This is the GT version of gtdef.h which actually defines the
**	symbols needed by the common routines to define the GT parts
**	of FT.  This include will be an empty file for the FT version.
*/

# define GRAPHICS 1	/* Used to ifdef code in common routines. */

# define FTGTACT 1	/* GT active flag - a graphics frame is associated */
# define FTGTEDIT 2	/* GT edit flag - call GTedit to interact with frame */
# define FTGTREF 4	/* graphics frame requires refresh */

/*
**	We also define some symbols commonly used throughout GT in here.
*/

/*
**	mode definitions LOC[ON|OFF] used in gtlocdev for user interaction
**	with mouse or other locator device.
*/

#define ALPHAMODE 1
#define GRAPHMODE 2
#define LOCONMODE 3
#define LOCOFFMODE 4

/*
**	color translation macro - leave color 0 (black) and color
**	1 (white) translated to 0 and 1, cycle the rest modulo number
**	of colors - 2, biasing by 2
*/
#define X_COLOR(X) (G_cnum < 3 ? (X == 0 ? 0 : 1) : (X < 2 ? X : ((X - 2) % (G_cnum - 2)) + 2))

/* default aspect ratio - overidable by device definition */
#define DEFASPECT 0.7727

/* Special lines for edit */

# define G_MROW	(G_lines-1)	/* Menu line for edit */
# define G_SROW	(G_MROW-1)	/* Status line for edit */
