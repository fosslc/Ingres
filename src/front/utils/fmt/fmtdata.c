/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
**
LIBRARY = IMPEMBEDLIBDATA
**
*/

# include	<compat.h>
# include	<cv.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<fmt.h>
# include	<adf.h>
# include	<afe.h>
# include	<dateadt.h>
# include	"format.h"
# include	<st.h>

/*
** Name:        fmtdata.c
**
** Description: Global data for fmt facility.
**
** History:
**
**      26-sep-96 (mcgem01)
**          Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPEMBEDLIBDATA
**	    in the Jamfile.
**	16-Nov-2004 (komve01)
**	    Added cont_line for copy.in to work correct for views. Bug 113498.
**	30-Nov-2004 (komve01)
**	    Undo change# 473469 for Bug#113498.
**       18-apr-2006 (stial01)
**          Moved fcolfmt globals and statics to FM_WORKSPACE
*/

GLOBALDEF i4	monsize[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

GLOBALDEF KYW F_Keywords[] =
{
/*	F_Abbrmon: */
	NULL,		F_MON,		1,
	NULL,		F_MON,		2,
	NULL,		F_MON,		3,
	NULL,		F_MON,		4,
	NULL,		F_MON,		5,
	NULL,		F_MON,		6,
	NULL,		F_MON,		7,
	NULL,		F_MON,		8,
	NULL,		F_MON,		9,
	NULL,		F_MON,		10,
	NULL,		F_MON,		11,
	NULL,		F_MON,		12,
/*	F_Fullmon: */
	NULL,		F_MON,		1,
	NULL,		F_MON,		2,
	NULL,		F_MON,		3,
	NULL,		F_MON,		4,
	NULL,		F_MON,		5,
	NULL,		F_MON,		6,
	NULL,		F_MON,		7,
	NULL,		F_MON,		8,
	NULL,		F_MON,		9,
	NULL,		F_MON,		10,
	NULL,		F_MON,		11,
	NULL,		F_MON,		12,
/*	F_Ordinal: */
	NULL,		F_ORD,		0,
	NULL,		F_ORD,		1,
	NULL,		F_ORD,		2,
	NULL,		F_ORD,		3,
	NULL,		F_ORD,		4,
	NULL,		F_ORD,		5,
	NULL,		F_ORD,		6,
	NULL,		F_ORD,		7,
	NULL,		F_ORD,		8,
	NULL,		F_ORD,		9,
	NULL,		F_ORD,		10,
	NULL,		F_ORD,		11,
	NULL,		F_ORD,		12,
	NULL,		F_ORD,		13,
	NULL,		F_ORD,		14,
	NULL,		F_ORD,		15,
	NULL,		F_ORD,		16,
	NULL,		F_ORD,		17,
	NULL,		F_ORD,		18,
	NULL,		F_ORD,		19,
	NULL,		F_ORD,		20,
	NULL,		F_ORD,		21,
	NULL,		F_ORD,		22,
	NULL,		F_ORD,		23,
	NULL,		F_ORD,		24,
	NULL,		F_ORD,		25,
	NULL,		F_ORD,		26,
	NULL,		F_ORD,		27,
	NULL,		F_ORD,		28,
	NULL,		F_ORD,		29,
	NULL,		F_ORD,		30,
	NULL,		F_ORD,		31,
	NULL,		F_ORD,		32,
	NULL,		F_ORD,		33,
	NULL,		F_ORD,		34,
	NULL,		F_ORD,		35,
	NULL,		F_ORD,		36,
	NULL,		F_ORD,		37,
	NULL,		F_ORD,		38,
	NULL,		F_ORD,		39,
	NULL,		F_ORD,		40,
	NULL,		F_ORD,		41,
	NULL,		F_ORD,		42,
	NULL,		F_ORD,		43,
	NULL,		F_ORD,		44,
	NULL,		F_ORD,		45,
	NULL,		F_ORD,		46,
	NULL,		F_ORD,		47,
	NULL,		F_ORD,		48,
	NULL,		F_ORD,		49,
	NULL,		F_ORD,		50,
	NULL,		F_ORD,		51,
	NULL,		F_ORD,		52,
	NULL,		F_ORD,		53,
	NULL,		F_ORD,		54,
	NULL,		F_ORD,		55,
	NULL,		F_ORD,		56,
	NULL,		F_ORD,		57,
	NULL,		F_ORD,		58,
	NULL,		F_ORD,		59,
	NULL,		F_ORD,		60,
	NULL,		F_ORD,		61,
	NULL,		F_ORD,		62,
	NULL,		F_ORD,		63,
	NULL,		F_ORD,		64,
	NULL,		F_ORD,		65,
	NULL,		F_ORD,		66,
	NULL,		F_ORD,		67,
	NULL,		F_ORD,		68,
	NULL,		F_ORD,		69,
	NULL,		F_ORD,		70,
	NULL,		F_ORD,		71,
	NULL,		F_ORD,		72,
	NULL,		F_ORD,		73,
	NULL,		F_ORD,		74,
	NULL,		F_ORD,		75,
	NULL,		F_ORD,		76,
	NULL,		F_ORD,		77,
	NULL,		F_ORD,		78,
	NULL,		F_ORD,		79,
	NULL,		F_ORD,		80,
	NULL,		F_ORD,		81,
	NULL,		F_ORD,		82,
	NULL,		F_ORD,		83,
	NULL,		F_ORD,		84,
	NULL,		F_ORD,		85,
	NULL,		F_ORD,		86,
	NULL,		F_ORD,		87,
	NULL,		F_ORD,		88,
	NULL,		F_ORD,		89,
	NULL,		F_ORD,		90,
	NULL,		F_ORD,		91,
	NULL,		F_ORD,		92,
	NULL,		F_ORD,		93,
	NULL,		F_ORD,		94,
	NULL,		F_ORD,		95,
	NULL,		F_ORD,		96,
	NULL,		F_ORD,		97,
	NULL,		F_ORD,		98,
	NULL,		F_ORD,		99,
/*	F_Abdow: */
	NULL,		F_DOW,		0,
	NULL,		F_DOW,		1,
	NULL,		F_DOW,		2,
	NULL,		F_DOW,		3,
	NULL,		F_DOW,		4,
	NULL,		F_DOW,		5,
	NULL,		F_DOW,		6,
/*	F_Abbrdow: */
	NULL,		F_DOW,		0,
	NULL,		F_DOW,		1,
	NULL,		F_DOW,		2,
	NULL,		F_DOW,		3,
	NULL,		F_DOW,		4,
	NULL,		F_DOW,		5,
	NULL,		F_DOW,		6,
/*	F_Fulldow: */
	NULL,		F_DOW,		0,
	NULL,		F_DOW,		1,
	NULL,		F_DOW,		2,
	NULL,		F_DOW,		3,
	NULL,		F_DOW,		4,
	NULL,		F_DOW,		5,
	NULL,		F_DOW,		6,
/*	F_Abbrpm: */
	NULL,		F_PM,		0,
	NULL,		F_PM,		12,
/*	F_Fullpm: */
	NULL,		F_PM,		0,
	NULL,		F_PM,		12,
/*	F_Fullunit: */
	NULL,		F_TIUNIT,	0,
	NULL,		F_TIUNIT,	1,
	NULL,		F_TIUNIT,	2,
	NULL,		F_TIUNIT,	3,
	NULL,		F_TIUNIT,	4,
	NULL,		F_TIUNIT,	5,
/*	F_SFullunit: */
	NULL,		F_TIUNIT,	6,
	NULL,		F_TIUNIT,	7,
	NULL,		F_TIUNIT,	8,
	NULL,		F_TIUNIT,	9,
	NULL,		F_TIUNIT,	10,
	NULL,		F_TIUNIT,	11,
	0,		0,		0
};

GLOBALDEF KYW	*F_Abbrmon	= &F_Keywords[0];
GLOBALDEF KYW	*F_Fullmon	= &F_Keywords[12];
GLOBALDEF KYW	*F_Ordinal	= &F_Keywords[24];
GLOBALDEF KYW	*F_Abdow	= &F_Keywords[124];
GLOBALDEF KYW	*F_Abbrdow	= &F_Keywords[131];
GLOBALDEF KYW	*F_Fulldow	= &F_Keywords[138];
GLOBALDEF KYW	*F_Abbrpm	= &F_Keywords[145];
GLOBALDEF KYW	*F_Fullpm	= &F_Keywords[147];
GLOBALDEF KYW	*F_Fullunit	= &F_Keywords[149];
GLOBALDEF KYW	*F_SFullunit	= &F_Keywords[155];

GLOBALDEF i4	F_monlen = 0;	/* length of 2nd month name
				** (i.e. "February") */
GLOBALDEF i4	F_monmax = 0;	/* length of longest month name
				** (i.e. "September") */
GLOBALDEF i4	F_dowlen = 0;	/* length of 1st day of week name
				** (i.e. "Sunday") */
GLOBALDEF i4	F_dowmax = 0;	/* length of longest day of week name
				** (i.e. "Wednesday") */
GLOBALDEF i4	F_pmlen = 0;	/* length of "pm" */

/* fmdeffmt.c */
GLOBALDEF       i4      IIprecision = 0;
GLOBALDEF       i4      IIscale = 0;

