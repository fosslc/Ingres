# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	"vffiles.h"

/*
**  data.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  Initialized data for VIFRED.
**
**	history:
**		(... time passes)
**		2/1/85 (peterk) - removed unused write file buffers
**			as they appear to be unused.
**			- add GLOBALDEF vfuchkon, vfdupfound,
**			resequenced
**	03/13/87 (dkh) - Added support for ADTs.
**	08/14/87 (dkh) - ER changes.
**	11/28/87 (dkh) - Added testing support for calling QBF.
**	18-may-88 (sylviap)
**		Changed the length of dbname from FE_MAXNAME to FE_MAXDBNM.
**	24-apr-89 (bruceb)
**		Removed vfNsFile; no longer saving any nonseq fields.
**	01-jun-89 (bruceb)
**		Defined IIVFmtMiscTag for memory allocated with no tag
**		in VIFRED when not within the vffrmtag/vflinetag defaults.
**	01-sep-89 (cmr)	variables COLHEAD and DETAIL no longer needed.
**	14-dec-89 (bruceb)
**		Added IIVFrfRoleFlag, IIVFpwPasswd and IIVFgidGroupID.
**	12/23/89 (dkh) - VMS shared lib changes.
**	19-apr-90 (bruceb)
**		Removed upLimit, lowLimit; no longer used in nextFeat/prevFeat.
**	02-may-90 (bruceb)
**		Removed IIVFrfRoleFlag.
**	06/09/90 (esd) - Add definition of IIVFsfaSpaceForAttr
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

/*
** c file to put all uninitialized data
*/

GLOBALDEF i4		globy = 0;
GLOBALDEF i4		globx = 0;
GLOBALDEF i4		endFrame = 0;
GLOBALDEF i4		endxFrm = 0;
GLOBALDEF i4		endFrmX = 0;
GLOBALDEF i4		numTrim = 0;
GLOBALDEF i4		numField = 0;
GLOBALDEF FILE		*vfdfile = NULL;
GLOBALDEF char		*vfdname = NULL;
GLOBALDEF char		*vfuname = NULL;
GLOBALDEF char		vfspec = '\0';
GLOBALDEF bool		vfuserflag = FALSE;
GLOBALDEF char		*Vfformname = NULL;
GLOBALDEF char		*Vfcompile = NULL;
GLOBALDEF char		*Vfcfsym = NULL;
GLOBALDEF i4		(*ExitFn)() = NULL;
GLOBALDEF i2		vffrmtag = 0;	/* tag for frame memory allocation */
GLOBALDEF i2		vflinetag = 0;	/* tag for line table memory alloc */
GLOBALDEF i2		IIVFmtMiscTag = 0;	/* tag for misc memory alloc */
GLOBALDEF bool		vfendfrmtag = FALSE;	/* end frame storage tag */
GLOBALDEF bool		vfuchkon = FALSE;
GLOBALDEF bool		vfdupfound = FALSE;
GLOBALDEF bool		vfnoload = FALSE;
GLOBALDEF bool		iivfRTIspecial = FALSE;
GLOBALDEF i4		vforigintype = 0;
GLOBALDEF char		vforgname[FE_MAXNAME + 2 + 1] = {0};
GLOBALDEF char		IIVFtoTblOwner[FE_MAXNAME + 2 + 1] = {0};
GLOBALDEF bool		IIVFiflg = FALSE;
GLOBALDEF bool		IIVFzflg = FALSE;
GLOBALDEF char		*IIVFpwPasswd = NULL;
GLOBALDEF char		*IIVFgidGroupID = NULL;

/*
** PIPES for equel call
*/

GLOBALDEF char		*Vfxpipe = NULL;

GLOBALDEF FILE		*vfFldFile = NULL;
GLOBALDEF FILE		*vfTrFile = NULL;
GLOBALDEF FILE		*vfCfFile = NULL;

GLOBALDEF char		*IIVF_yes1 = NULL;
GLOBALDEF char		*IIVF_no1 = NULL;

GLOBALDEF FRAME		*frame = NULL;
GLOBALDEF VFNODE	**line = NULL;
GLOBALDEF char		dbname[FE_MAXDBNM + 1] = {0};
GLOBALDEF char		*vfrname = NULL;
GLOBALDEF bool		Vfeqlcall = FALSE;
GLOBALDEF bool		noChanges = TRUE;
GLOBALDEF bool		RBF = FALSE;
 
/*
** Indicate whether current form (pointed to by 'frame')
** needs space for attribute bytes (fdFRMSTRTCHD bit set in frmflags).
** Always 0 for RBF.
*/
GLOBALDEF i4		IIVFsfaSpaceForAttr = 0;
