/*
** Copyright (c) 1988, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<qu.h>
# include	<gccpl.h>

/**
** Name: GCCPLFSM.ROC - State machine for Presentation Layer
**
** History:
**      25-Jan-88 (jbowers)
**          Initial module creation
**      18-Jan-89 (jbowers)
**          Added valid intersections to accommodate certain obscure error
**	    conditions. 
**      16-Mar-89 (jbowers)
**	    Fix for bug # 5075:
**          Fix for failure to clean up sessions when ARU PPDU is received on a
**	    session currently in the process of closing.
**	12-Jan-90 (seiwald)
**	     Ironed out ownership of MDE's: an MDE now belongs to exactly 
**	     one layer at a time.  See change comments in gccal.c.
**	25-Jan-90 (seiwald)
**	    Revised output events for MDE's which undergo network
**	    conversion:  previously, the OPDCIE, OPDCIN, OPDCOE, or
**	    OPDCON events converted a single input MDE into a chain of
**	    MDE's, each element of which was then handed to the OPPDI,
**	    OPPEI, OPSDQ, or OPSEQ output events for sending to the
**	    next layer.  Now, new output events OPPDIH, OPPEIH, OPSDQH,
**	    and OPSEQH both convert the MDE and send MDE's to the next 
**	    layer as necessary.  A new event OPMDE deletes the input
**	    MDE.  Code to load the in-transit tuple descriptors is
**	    now performed directly by the het output events, rather
**	    than by special states.
**	1-Mar-91 (seiwald)
**	    Extracted from GCCPL.H.
**	2-May-91 (cmorris)
**	    Allow user abort after connection release requested.
**	20-May-91 (cmorris)
**	    Added support for release collisions.
**	20-May-91 (seiwald)
**	    Include <qu.h> to accomodate new structure definitions in
**	    gccpl.h.
**  	12-Aug-91 (cmorris)
**  	    Added transitions to support deletion of any tuple object
**  	    descriptor on connection abort or release.
**  	15-Aug-91 (cmorris)
**  	    After we have issued/received an abort of the session
**  	    connection, no further interaction with the SL occurs.
**  	15-Aug-91 (cmorris)
**  	    Ditto for interactions with AL.
**  	16-Aug-91 (cmorris)
**  	    Fix aborts in closed state.
**	19-Nov-92 (seiwald)
**	    Changed nat's to char's in state table for compactness.
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
** Name: PLFSM_TBL - FSM matrix
**
**
** Description:
**      There are two static data structures which together
**	define the Finite State Machine (FSM) for the PL.  They are
**	statically initialized as read only structures.  They drive the
**	operation of the PL FSM.  They are both documented here, even though
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
**	specified in the preamble to the module GCCPL.C. The description is
**	actually more complicated than the mechanism.
**	
**      The following structure PLFSM_TBL is the representation of the FSM
**	matrix.  The hex values are required by CMS.
**	
** History:
**      10-Jun-88 (jbowers)
**          Initial structure implementation
*/


GLOBALCONSTDEF char
plfsm_tbl[IPMAX][SPMAX]
=
{
/*   0         1         2	   3         4                */
/* SPCLD     SPIPL     SPRPL     SPOPN     SPCLG              */
   { 1,       00,       00,       00,       00},  /* 00 IPPCQ */
   {00,       00,        2,       00,       00},  /* 01 IPPCA */
   {00,       00,        3,       00,       00},  /* 02 IPPCJ */
   {00,        4,        5,        6,        6},  /* 03 IPPAU */
   {00,       00,       00,        7,       00},  /* 04 IPPDQ */
   {00,       00,       00,        8,       00},  /* 05 IPPEQ */
   {00,       00,       00,       11,       30},  /* 06 IPPRQ */
   {00,       00,       00,       00,       12},  /* 07 IPPRS */
   {13,       00,       00,       00,       00},  /* 08 IPSCI */
   {00,       14,       00,       00,       00},  /* 09 IPSCA */
   {00,       15,       00,       00,       00},  /* 10 IPSCJ */
   {00,       16,       16,        9,        9},  /* 11 IPSAP */
   {00,       19,       19,       10,       10},  /* 12 IPARU */
   {00,       16,       16,        9,        9},  /* 13 IPARP */
   {00,       00,       00,       22,       00},  /* 14 IPSDI */
   {00,       00,       00,       23,       00},  /* 15 IPSEI */
   {00,       00,       00,       26,       31},  /* 16 IPSRI */
   {00,       00,       00,       00,       27},  /* 17 IPSRA */
   {00,       00,       00,       00,       28},  /* 18 IPSRJ */
   {00,       00,       00,       29,       00},  /* 19 IPPDQH */
   {00,       00,       00,       00,       34},  /* 20 IPPRSC */
   {00,       00,       00,       00,       49},  /* 21 IPSRAC */
   {00,       00,       00,       32,       00},  /* 22 IPPEQH */
   {00,       00,       00,       33,       00},  /* 23 IPSDIH */
   {00,       00,       00,       00,       00},  /* 24       */
   {00,       00,       00,       35,       00},  /* 25 IPSEIH */
   {25,       36,       36,       24,       24},  /* 26 IPABT */
   {00,       00,       00,       37,       38},  /* 27 IPPXF */
   {00,       00,       00,       39,       40},  /* 28 IPPXN */
   {00,       00,       00,       41,       42},  /* 29 IPSXF */
   {00,       00,       00,       43,       44},  /* 30 IPSXN */
};

/*}
** Name: PLFSM_ENTRY - FSM matrix
**
** Description:
**      This is the linear array containing the entries for the FSM matrix.  The
**	entries in the preceeding table PLFSM_TBL are index values
**	corresponding to entries in the current array.  The array size limits on
**	the individual elements of each array entry, PL_OUT_MAX and PL_ACTN_MAX
**	are set to values which are currently adequate.  They are confined in
**	scope to the PL.  If future expansion requires these to be increased,
**	this can be done without affecting anything outside PL.  If the values
**	are changed in GCCPL.H and the PL recompiled, everything will work.
**
** History:
**      10-Jun-88 (jbowers)
**          Initial structure implementation
*/


GLOBALCONSTDEF struct _plfsm_entry 
plfsm_entry[]
=
{
 /* ( 0) */ {SPCLD, {OPARP, OPPAP, 0, 0}},/* Illegal intrsctn  */
 /* ( 1) */ {SPIPL, {OPSCQ, 0, 0, 0}},
 /* ( 2) */ {SPOPN, {OPSCA, 0, 0, 0}},
 /* ( 3) */ {SPCLD, {OPSCJ, 0, 0, 0}},
 /* ( 4) */ {SPCLD, {OPARU, 0, 0, 0}},
 /* ( 5) */ {SPCLD, {OPARU, 0, 0, 0}},
 /* ( 6) */ {SPCLD, {OPTOD, OPARU, 0, 0}},
 /* ( 7) */ {SPOPN, {OPSDQ, 0, 0, 0}},
 /* ( 8) */ {SPOPN, {OPSEQ, 0, 0, 0}},
 /* ( 9) */ {SPCLD, {OPTOD, OPPAP, 0, 0}},
 /* (10) */ {SPCLD, {OPTOD, OPPAU, 0, 0}},
 /* (11) */ {SPCLG, {OPSRQ, 0, 0, 0}},
 /* (12) */ {SPCLD, {OPTOD, OPSRS, 0, 0}},
 /* (13) */ {SPRPL, {OPPCI, 0, 0, 0}},
 /* (14) */ {SPOPN, {OPPCA, 0, 0, 0}},
 /* (15) */ {SPCLD, {OPPCJ, 0, 0, 0}},
 /* (16) */ {SPCLD, {OPPAP, 0, 0, 0}},
 /* (17) */ {SPCLD, {OPPAP, 0, 0, 0}},
 /* (18) */ {SPCLD, {OPPAP, 0, 0, 0}},
 /* (19) */ {SPCLD, {OPPAU, 0, 0, 0}},
 /* (20) */ {SPCLD, {OPPAU, 0, 0, 0}},
 /* (21) */ {SPCLD, {OPPAU, 0, 0, 0}},
 /* (22) */ {SPOPN, {OPPDI, 0, 0, 0}},
 /* (23) */ {SPOPN, {OPPEI, 0, 0, 0}},
 /* (24) */ {SPCLD, {OPTOD, OPPAP, OPARP, 0}},
 /* (25) */ {SPCLD, {OPMDE, 0, 0, 0}},
 /* (26) */ {SPCLG, {OPPRI, 0, 0, 0}},
 /* (27) */ {SPCLD, {OPTOD, OPPRC, 0, 0}},
 /* (28) */ {SPCLD, {OPTOD, OPPRC, 0, 0}},
 /* (29) */ {SPOPN, {OPSDQH, OPMDE, 0, 0}},
 /* (30) */ {SPCLG, {OPSCR, OPSRQ, 0, 0}},
 /* (31) */ {SPCLG, {OPSCR, OPPRI, 0, 0}},
 /* (32) */ {SPOPN, {OPSEQH, OPMDE, 0, 0}},
 /* (33) */ {SPOPN, {OPPDIH, OPMDE, 0, 0}},
 /* (34) */ {SPCLG, {OPRCR, OPSRS, 0, 0}},
 /* (35) */ {SPOPN, {OPPEIH, OPMDE, 0, 0}},
 /* (36) */ {SPCLD, {OPARP, OPPAP, 0, 0}},
 /* (37) */ {SPOPN, {OPSXF, 0, 0, 0}},
 /* (38) */ {SPCLG, {OPSXF, 0, 0, 0}},
 /* (39) */ {SPOPN, {OPSXN, 0, 0, 0}},
 /* (40) */ {SPCLG, {OPSXN, 0, 0, 0}},
 /* (41) */ {SPOPN, {OPPXF, 0, 0, 0}},
 /* (42) */ {SPCLG, {OPPXF, 0, 0, 0}},
 /* (43) */ {SPOPN, {OPPXN, 0, 0, 0}},
 /* (44) */ {SPCLG, {OPPXN, 0, 0, 0}},
 /* (45) */ {SPCLD, {0, 0, 0, 0}}, /* NOT USED */
 /* (46) */ {SPCLD, {0, 0, 0, 0}}, /* NOT USED */
 /* (47) */ {SPCLD, {OPPXF, 0, 0, 0}},
 /* (48) */ {SPCLD, {OPPXN, 0, 0, 0}},
 /* (49) */ {SPCLG, {OPRCR, OPPRC, 0, 0}}
};
