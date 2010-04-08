/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	"derive.h"
# include	<afe.h>
# include	<cm.h>
# include	<er.h>
# include	<si.h>
# include	<st.h>
# include	"erfv.h"

/*
** Name:        data.c
**
** Description: Global data for  facility.
**
** History:
**
**      24-sep-96 (hanch04)
**          Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
*/

/* Jam hints
**
LIBRARY = IMPFRAMELIBDATA
**
*/

/*
**	dinit.c
*/

/* Field being parse for derivation formula */
GLOBALDEF	FIELD	*IIFVcfCurFld = NULL;

/* DeriveFld debug/print flag */
GLOBALDEF	bool	IIFVddpDebugPrint = FALSE;

/* Flag indicating if derived formula was ALL constants */
GLOBALDEF	bool	IIFVocOnlyCnst = TRUE;

/*
**	dlex.c
*/

GLOBALDEF       char    IIFVfloat_str[fdVSTRLEN];

/*
**	dnode.c
*/

GLOBALDEF       i4      IIFVorcOprCnt = 0;
GLOBALDEF       i4      IIFVodcOpdCnt = 0;

/*
**	drveval.c
*/

GLOBALDEF       bool            IIFVcvConstVal = FALSE;
GLOBALDEF       DB_DATA_VALUE   *IIFVdc1DbvConst = NULL;
GLOBALDEF       DB_DATA_VALUE   *IIFVdc2DbvConst = NULL;

/*
**	except.c
*/

GLOBALDEF i4    vl_help = TRUE;

/*
**	vinit.c
*/

GLOBALDEF char          *vl_buffer = ERx("");
GLOBALDEF FLDHDR        *IIFVflhdr = NULL;
GLOBALDEF bool          *IIFVxref = FALSE;
