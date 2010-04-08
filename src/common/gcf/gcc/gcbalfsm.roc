/*
** Copyright (c) 1996, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<gccpb.h>
 
/**
** Name: GCBALFSM.ROC - State machine for Protocol Bridge Application Layer
**
** History:
**	22-Jan-96 (rajus01)
**	   Culled from gccalfsm.roc
**      18-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	19-dec-2008 (joea)
**	    Replace GLOBALDEF const by GLOBALCONSTDEF.
**/


/*}
** Name: PBALFSM_TBL - FSM matrix
**
**
** Description:
**      There are two static data structures which together
**	define the Finite State Machine (FSM) for the PB_AL.  They are
**	statically initialized as read only structures.  They drive the
**	operation of the PB_AL FSM.  They are both documented here, even though
**	they are separate structures.
**
**	The FSM is a sparse matrix.  The two table mechanism is a way to deal
**	with this by not wasting too much memory on empty matrix elements, but
**	at the same time provide a simple and reasonably quick lookup scheme.
**	The first table is a matrix of integers, of the  same configuration as
**	the FSM.  Each matrix element which represents a valid FSM state/input
**	combination contains an index into the second table.  The second table
**	is a linear array whose size is exactly equal to the number of valid
**	intersections in the FSM.  Each index entry in the first table
**	references a corresponding positional entry in the second table.  This
**	latter entry specifies the new state, output events and actions which
**	are associated with the state/input intersection.  The FSM matrix is
**	specified in the preamble to the module GCBAL.C. The description is
**	actually more complicated than the mechanism.
**	
**      The following structure PB_AL_FSM_TBL is the representation of the FSM
**	matrix.  
**
** History:
**	21-Jan-96 (rajus01)
**          Initial structure implementation
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPGCFLIBDATA
**	    in the Jamfile.
**      18-Nov-2004 (Gordon.Thorpe@ca.com and Ralph.Loen@ca.com)
**          Removed LIBRARY jam hint.  This file is now included in the
**          GCC local library only.
*/
 
 
GLOBALCONSTDEF char 
pbalfsm_tbl[IBAMAX][SBAMAX] =
{
/*  0   1   2   3   */
/* CLD  SHT QSC CLG */
  { 1,  0,  0,  0 }, /* (00) IBALSN */
  { 0,  7,  4,  3 }, /* (01) IBAABQ */
  { 0,  7,  4,  3 }, /* (02) IBAACE */
  { 0,  7,  4,  3 }, /* (03) IBAARQ */
  { 5,  0,  0,  0 }, /* (04) IBASHT */
  { 6,  0,  0,  0 }, /* (05) IBAQSC */
  { 9,  2,  2,  8 }, /* (06) IBAABT */
  { 0,  0,  0,  8 }, /* (07) IBAAFN */
  { 2,  0,  0,  0 }, /* (08) IBALSF */
  { 10,  0,  0, 0 }  /* (09) IBAREJI */
};


/*}
** Name: PBALFSM_ENTRY - FSM matrix
**
** Description:
**	This is the linear array containing the entries for the FSM matrix.
**	The entries in the preceeding table PB_AL_FSM_TBL are index values
**	corresponding to entries in the current array.  The array size limits
**	on the individual elements of each array entry, PB_OUT_MAX and
**	PB_ACTN_MAX are set to values which are currently adequate.  They are
**	confined in scope to the PBAL.  If future expansion requires these to be
**	increased, this can be done without affecting anything outside PBAL.  If
**	the values are changed in GCCPB.H and the PBAL recompiled, everything
**	will work.
**
** History:
**	21-Jan-96 (rajus01)
**          Initial structure implementation
*/

GLOBALCONSTDEF struct _pbalfsm_entry 
pbalfsm_entry[] =
{
 /* ( 0)  */ {SBACLD, {0, 0}, {0, 0, 0, 0}},/* Illegal intrsctn  */
 /* ( 1)  */ {SBACLD, {0, 0},{ABALSN, 0, 0, 0}},
 /* ( 2)  */ {SBACLG, {0, 0},{ABADIS, ABAMDE, 0, 0}},
 /* ( 3)  */ {SBACLG, {0, 0},{ABAMDE, 0, 0, 0}},
 /* ( 4)  */ {SBACLG, {0, 0},{ABAQSC, ABADIS, ABAMDE, 0}},
 /* ( 5)  */ {SBASHT, {0, 0},{ABARSP, 0, 0, 0}},
 /* ( 6)  */ {SBAQSC, {0, 0},{ABARSP, 0, 0, 0}},
 /* ( 7)  */ {SBACLG, {0, 0},{ABASHT, ABADIS, ABAMDE, 0}},
 /* ( 8)  */ {SBACLD, {0, 0},{ABACCB, ABAMDE, 0, 0}},
 /* ( 9)  */ {SBACLD, {0, 0},{ABAMDE, 0, 0, 0}},
 /* (10)  */ {SBARJA, {0, 0},{ABARSP, 0, 0, 0}}
}; 
