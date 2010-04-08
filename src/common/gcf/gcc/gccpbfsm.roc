/*
** Copyright (c) 1991, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<qu.h>
# include   	<tm.h>
# include	<gcccl.h>
# include	<gccpb.h>

/**
** Name: GCCPBFSM.ROC - State machine for Network Bridge
**
** History: $Log-for RCS$
**	6-Mar-91 (cmorris)
**	    Created.
**	12-Jul-91 (cmorris)
**	    Fix transitions to hold onto data received before
**          Connect Confirm.
**	15-Jul-91 (cmorris)
**	    For un-updated protocol drivers, throw away completed
**	    receives/sends after disconnect has been requested.
**  	13-Aug-91 (cmorris)
**  	    Added include of tm.h.
**      10-Jan-92 (cmorris)
**  	    Added Listen failure event and associated transitions. Previously
**          this situation shared an event with Open failure; however, the
**  	    two cases require different events/actions to be executed.
**	19-Nov-92 (seiwald)
**	    Changed nat's to char's in state table for compactness.
**	16-Feb-93 (seiwald)
**	    Don't bother including gcc.h since we don't need it.
**	06-Feb-96 (rajus01)
**	    Put the Bridge back in SBOPN state when it receives IBNCLUP
**	    event,  while it is waiting for network connections (SBWNC ).
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPGCFLIBDATA
**	    in the Jamfile.
**      18-Nov-2004 (Gordon.Thorpe@ca.com and Ralph.Loen@ca.com)
**          Removed LIBRARY jam hint.  This file is now included in the
**          GCC local library only.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	19-dec-2008 (joea)
**	    Replace GLOBALDEF const by GLOBALCONSTDEF.
**/


/*}
** Name: PBFSM_TBL - FSM matrix
**
**
** Description:
**      There are two static data structures which together
**	define the Finite State Machine (FSM) for the PB. They are
**	statically initialized as read only structures.  They drive the
**	operation of the PB FSM.  They are both documented here, even though
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
**	specified in the preamble to the module GCCPB.C. The description is
**	actually more complicated than the mechanism.
**	
**      the following structure PBFSM_TBL is the representation of the FSM
**	matrix.
**	
** History:
**      10-Jun-88 (jbowers)
**          Initial structure implementation
*/

GLOBALCONSTDEF    char
pbfsm_tbl[IBMAX][SBMAX]
=
{
/*   0	   1     2     3     4	   5     6    */
/* SBCLD SBWNC SBOPN SBSDP SBOPX SBSDX SBCLG  */
   { 1,   00,   00,   00,   00,   00,   00},  /* 00 IBOPN  */
   { 2,   00,   00,   00,   00,   00,   00},  /* 01 IBNOC  */
   { 3,    4,   00,   00,   00,   00,   00},  /* 02 IBNRS  */
   { 5,   00,   00,   00,   00,   00,   00},  /* 03 IBNCI  */
   { 6,   00,   00,   00,   00,   00,   00},  /* 04 IBNCQ  */
   {40,    7,   00,   00,   00,   00,   00},  /* 05 IBNCC  */
   { 8,    9,    9,    9,    9,    9,   00},  /* 06 IBNDR  */
   {40,   10,   10,   10,   10,   10,   19},  /* 07 IBNDI  */
   {00,   00,   00,   00,   00,   00,    3},  /* 08 IBNDC  */
   {00,   36,   11,   12,   13,   14,   00},  /* 09 IBSDN  */
   {00,   00,   15,   16,   00,   00,   19},  /* 10 IBDNI  */
   {00,   00,   00,   17,   00,   18,   19},  /* 11 IBDNC  */
   {00,   00,   00,   20,   00,   21,   19},  /* 12 IBDNQ  */
   { 8,   10,   10,   10,   10,   10,    3},  /* 13 IBABT  */
   {00,   00,   22,   23,   24,   25,   00},  /* 14 IBXOF  */
   {00,   00,   26,   27,   28,   29,   00},  /* 15 IBXON  */
   {00,   00,   30,   31,   00,   00,   00},  /* 16 IBRDX  */
   {00,   37,   00,   32,   00,   33,   00},  /* 17 IBSDNQ */
   {00,   00,   00,   34,   00,   35,   19},  /* 18 IBDNX  */
   {00,   38,   00,   00,   00,   00,   00},  /* 19 IBNCCQ */
   { 9,   00,   00,   00,   00,   00,   00},  /* 20 IBNLSF */
   {39,   00,   00,   00,   00,   00,   00}   /* 21 IBNCIP */
};

/*}
** Name: PBFSM_ENTRY - FSM entry table
**
** Description:
**      This is the linear array containing the entries for the FSM matrix.  The
**	entries in the preceeding table PBFSM_TBL are index values
**	corresponding to entries in the current array.  The array size limits on
**	the individual elements of each array entry, PB_OUT_MAX and PB_ACTN_MAX
**	are set to values which are currently adequate.  They are confined in
**	scope to the PB.  If future expansion requires these to be increased,
**	this can be done without affecting anything outside PB.  If the values
**	are changed in GCCPB.H and the PB recompiled, everything will work.
**
** History:
**      3-Apr-91 (cmorris)
**          Initial structure implementation
**      08-Jul-91 (cmorris)
**	    Fixes to disconnect state transitions
**	12-Jul-91 (cmorris)
**	    Fix transition on receipt of N_RESET
**  	10-Apr-92 (cmorris)
**  	    Added state transition for IBNCIP event
**	06-Feb-96 (rajus01)
**	    Added state transition for IBNDI event.
*/

GLOBALCONSTDEF struct _pbfsm_entry 
pbfsm_entry[]
=
{
 /* ( 0) */ {SBCLD, {OBNDR, 0}, {ABDCN, 0, 0, 0}},/* Illegal FSM intersection */
 /* ( 1) */ {SBCLD, {0, 0}, {ABOPN, 0, 0, 0}},
 /* ( 2) */ {SBCLD, {0, 0}, {ABLSN, ABCCB, ABMDE, 0}},
 /* ( 3) */ {SBCLD, {0, 0}, {ABCCB, ABMDE, 0, 0}},
 /* ( 4) */ {SBCLD, {OBNDR, 0}, {ABCCB, ABMDE, 0, 0}},
 /* ( 5) */ {SBOPN, {OBNCQ, 0}, {ABRVN, 0, 0}},
 /* ( 6) */ {SBWNC, {0, 0}, {ABCNC, 0, 0, 0}},
 /* ( 7) */ {SBOPN, {0, 0}, {ABRVN, ABMDE, 0, 0}},
 /* ( 8) */ {SBCLD, {0, 0}, {ABMDE, 0, 0, 0}},
 /* ( 9) */ {SBCLG, {0, 0}, {ABDCN, 0, 0, 0}},
 /* (10) */ {SBCLG, {OBNDR, 0}, {ABDCN, 0, 0, 0}},
 /* (11) */ {SBSDP, {0, 0}, {ABSDN, 0, 0, 0}},
 /* (12) */ {SBSDP, {0, 0}, {ABSNQ, 0, 0, 0}},
 /* (13) */ {SBSDX, {0, 0}, {ABSDN, 0, 0, 0}},
 /* (14) */ {SBSDX, {0, 0}, {ABSNQ, 0, 0, 0}},
 /* (15) */ {SBOPN, {OBSDN, 0}, {ABRVN, 0, 0, 0}},
 /* (16) */ {SBSDP, {OBSDN, 0}, {ABRVN, 0, 0, 0}},
 /* (17) */ {SBOPN, {0, 0}, {ABMDE, 0, 0, 0}},
 /* (18) */ {SBOPX, {0, 0}, {ABMDE, 0, 0, 0}},
 /* (19) */ {SBCLG, {0, 0}, {ABMDE, 0, 0, 0}},
 /* (20) */ {SBSDP, {0, 0}, {ABSDQ, ABMDE, 0, 0}},
 /* (21) */ {SBSDX, {0, 0}, {ABSDQ, ABMDE, 0, 0}},
 /* (22) */ {SBOPN, {0, 0}, {ABSXF, ABMDE, 0, 0}},
 /* (23) */ {SBSDP, {0, 0}, {ABSXF, ABMDE, 0, 0}},
 /* (24) */ {SBOPX, {0, 0}, {ABSXF, ABMDE, 0, 0}},
 /* (25) */ {SBSDX, {0, 0}, {ABSXF, ABMDE, 0, 0}},
 /* (26) */ {SBOPN, {0, 0}, {ABRXF, ABMDE, 0, 0}},
 /* (27) */ {SBSDP, {0, 0}, {ABRXF, ABMDE, 0, 0}},
 /* (28) */ {SBOPN, {0, 0}, {ABRXF, ABRVN, ABMDE, 0}},
 /* (29) */ {SBSDP, {0, 0}, {ABRXF, ABRVN, ABMDE, 0}},
 /* (30) */ {SBOPX, {OBSDN, 0}, {0, 0, 0, 0}},
 /* (31) */ {SBSDX, {OBSDN, 0}, {0, 0, 0, 0}},
 /* (32) */ {SBSDP, {OBXOF, 0}, {ABSNQ, 0, 0, 0}},
 /* (33) */ {SBSDX, {OBXOF, 0}, {ABSNQ, 0, 0, 0}},
 /* (34) */ {SBOPN, {OBXON, 0}, {ABMDE, 0, 0, 0}},
 /* (35) */ {SBOPX, {OBXON, 0}, {ABMDE, 0, 0, 0}},
 /* (36) */ {SBWNC, {0, 0}, {ABSNQ, 0, 0, 0}},
 /* (37) */ {SBWNC, {OBXOF, 0}, {ABSNQ, 0, 0, 0}},
 /* (38) */ {SBSDP, {0, 0}, {ABRVN, ABSDQ, ABMDE, 0}},
 /* (39) */ {SBOPN, {OBNCQ, 0}, {ABRVN, ABLSN, 0}},
 /* (40) */ {SBOPN, {0, 0}, {0, 0, 0, 0}}
};
