/*
**	Copyright (c) 1987, 2004 Ingres Corporation
*/

#include <compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include <fe.h>
#include	<ug.h>
#include "iamstd.h"
#include <fdesc.h>
#include <adf.h>
#include <ade.h>
#include <ifid.h>
#include <iltypes.h>
#include <frmalign.h>
#include <ifrmobj.h>
#include "iamfrm.h"
#include "eram.h"

/**
** Name:	iamalgn.c - alignment structure handling
**
** Description:
**	Translates alignment / version information between ifmobj.h
**	structure, and AOMFRAME database object.  Both routines won't
**	be used in any executable, but we keep them in this file and
**	pay a small penalty in excess code to keep knowledge of the
**	relationship of an array in AOMFRAME and the items of the
**	FRMALIGN structure isolated to this file.  ADF_VERSION
**	is actually filled in at run time
*/

static i4 Alg_array [] =
{
	sizeof(i1) | ((i4)(sizeof(i2)) << 16),	/* X_I1_I2 */
	sizeof(i4) | ((i4)(sizeof(f4)) << 16),/* X_I4_F4 */
	sizeof(f8) | ((i4)(sizeof(char)) << 16),	/* X_F8_CH */
	sizeof(f8) | ((i4)(sizeof(f8)) << 16),	/* X_MXF_MXA */
	0x10000						/* X_ADF_VERS */
};

/*
** IMPORTANT:
**	these definitions MUST match the order of elements in Alg_array
**	They are used to index into a copy of that array to explicitly
**	assign FRMALIGN elements.  The convention followed here is that
**	the first item of the definition is the low 16 bits, the second
**	the one that was left shifted 16 bits.
*/
#define X_I1_I2 0
#define X_I4_F4 1
#define X_F8_CH 2
#define X_MXF_MXA 3
#define X_ADF_VERS 4

/*{
** Name:	set_align - set alignment information
**
** Description:
**	Sets alignment and version information in AOMFRAME structure.
**
** Inputs:
**	frm	AOMFRAME to set.
**
** History:
**	3/87 (bobm)	written
**	10/89 (jhw)  Commented word-swap for CX versions.
**	04-mar-92 (leighb) DeskTop Porting Change:
**		Moved function declaration outside of calling function.
**		This is so function prototypes will work.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

ADF_CB	*FEadfcb();		 

i4
set_align(frm)
AOMFRAME *frm;
{
	i2	v[2];

#ifdef ADE_13REQ_ADE_VERSION

	if (afc_cxinfo(FEadfcb(), NULL, ADE_13REQ_ADE_VERSION, v) != OK)
		IIUGerr(S_AM0001_afc_cxinfo, UG_ERR_FATAL, 0);
	/*
	** Note:  This stores ADE_CXHIVERSION in the low word and
	** ADE_CXLOVERSION in the high word for historical reasons.
	** (Previously generated interpreted objects will continue to
	** work.)  The 'get_align()' routine below sets them correctly.
	*/
	Alg_array[X_ADF_VERS] = v[0] | (((i4)v[1]) << 16);
#endif

	frm->feversion = FRM_VERSION;
	frm->align = Alg_array;
	frm->num_align = sizeof(Alg_array)/sizeof(i4);
}

/*{
** Name:	get_align - get alignment information
**
** Description:
**	Gets alignment and version information from AOMFRAME structure,
**	placing in FRMALIGN structure.
**
** Inputs:
**	frm	AOMFRAME
**
** Outputs:
**	fa	FRMALIGN structure
**
**
** History:
**	3/87 (bobm)	written
**	10/89 (jhw)  Corrected word-swap for CX versions.
*/

i4
get_align(frm,fa)
AOMFRAME *frm;
FRMALIGN *fa;
{
	fa->fe_version = frm->feversion;
	fa->fe_i1_align = (frm->align)[X_I1_I2] & 0xff;
	fa->fe_i2_align = (frm->align)[X_I1_I2] >> 16;
	fa->fe_i4_align = (frm->align)[X_I4_F4] & 0xff;
	fa->fe_f4_align = (frm->align)[X_I4_F4] >> 16;
	fa->fe_f8_align = (frm->align)[X_F8_CH] & 0xff;
	fa->fe_char_align = (frm->align)[X_F8_CH] >> 16;
	fa->fe_max_align = (frm->align)[X_MXF_MXA] & 0xff;
	fa->afc_veraln.afc_max_align = (frm->align)[X_MXF_MXA] >> 16;
	/*
	** Note:  The versions are stored with ADE_CXHIVERSION in the low
	** order word and ADE_CXLOVERSION in the high order word by the
	** 'set_align()' routine above for historical reasons.
	*/
	fa->afc_veraln.afc_hi_version = (frm->align)[X_ADF_VERS] & 0xff;
	fa->afc_veraln.afc_lo_version = (frm->align)[X_ADF_VERS] >> 16;
}
