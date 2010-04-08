/*
** Copyright (c) 1988, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<gccsl.h>

/**
** Name: GCCSLFSM.ROC - State machine for Session Layer
**
** History:
**      25-Jul-88 (jbowers)
**          Initial module creation
**      18-Jan-89 (jbowers)
**          Added valid intersections to accommodate certain obscure error
**	    conditions. 
**      16-Feb-89 (jbowers)
**          Add local flow control: expand FSM to handle XON and XOFF
**	    primitives. 
**      18-APR-89 (jbowers)
**          Added handling of (10, 6) intersection to FSM.
**	12-Jan-90 (seiwald)
**	     Ironed out ownership of MDE's: an MDE now belongs to exactly 
**	     one layer at a time.  See change comments in gccal.c.
**	1-Mar-91 (seiwald)
**	    Extracted from GCCPL.H.
**	13-May-91 (cmorris)
**	    Fixed a number of potential memory leaks. 
**	14-May-91 (cmorris)
**	    Actions are no more:- incorporate only remaining action as event.
**	28-May-91 (cmorris)
**	    Separate out freeing of saved Connect Request MDE into
**	    an output event to facilitate removing memory leak in abort cases.
**  	15-Aug-91 (cmorris)
**  	    After we have issued/received a transport disconnect, no
**          further interaction with the TL occurs. Similarly don't
**  	    send XON/XOFF up after session (as opposed to underlying
**  	    transport) connection has been released.
**  	15-Aug-91 (cmorris)
**  	    After we have issued/received a session abort, no further 
**  	    interaction with PL occurs.
**  	16-Aug-91 (cmorris)
**  	    Added event for forcing XON in abort cases.
**  	16-Aug-91 (cmorris)
**  	    Fix aborts in closed state.
**  	12-Mar-92 (cmorris)
**  	    If we get a transport disconnect indication or an internal abort
**          when we're waiting for a Connect SPDU, don't issue an abort up to
**  	    Presentation:- there's no presentation connection at this stage.
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
** Name: SLFSM_TBL - FSM matrix
**
**
** Description:
**      There are two static data structures which together
**	define the Finite State Machine (FSM) for the SL.  They are
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
**	specified in the preamble to the module GCCSL.C. The description is
**	actually more complicated than the mechanism.
**	
**      the following structure SLFSM_TBL is the representation of the FSM
**	matrix.  The hex specifications are needed for the CMS compiler.
**	
** History:
**      10-Jun-88 (jbowers)
**          Initial structure implementation
*/


GLOBALCONSTDEF char
slfsm_tbl[ISMAX][SSMAX]
=
{
/*  0     1     2     3     4     5     6     7     8                 */
/* CLD   ISL   RSL   WAC   WCR   OPN   CDN   CRR   CTD                */

   { 1,   00,   00,   00,   00,   00,   00,   00,   00  },   /* 00 ISSCQ */
   { 2,   00,   00,   00,   00,   00,   00,   00,   00  },   /* 01 ISTCI */
   {00,    3,   00,   00,   00,   00,   00,   00,   00  },   /* 02 ISTCC */
   {00,   00,   00,   00,    4,   00,   00,   00,   00  },   /* 03 ISSCA */
   {00,   00,   00,   00,    5,   00,   00,   00,   00  },   /* 04 ISSCJ */
   {00,   00,    6,   00,   00,   00,   00,   00,    7  },   /* 05 ISCN  */
   {00,    9,    8,    9,   00,   00,   00,   00,   00  },   /* 06 ISAC  */
   {00,   00,   10,   11,   00,   00,   00,   00,   00  },   /* 07 ISRF  */
   {00,   00,   00,   00,   00,   12,   00,   00,   00  },   /* 08 ISSDQ */
   {00,   00,   00,   00,   00,   13,   00,   00,   00  },   /* 09 ISSEQ */
   {00,   00,   00,   00,   00,   14,   51,   00,    8  },   /* 10 ISDT  */
   {00,   00,   00,   00,   00,   15,   00,   00,    8  },   /* 11 ISEX  */
   {00,   52,   00,   17,   17,   17,   17,   17,   00  },   /* 12 ISSAU */
   {00,   00,   16,   18,   18,   18,   18,   18,   16  },   /* 13 ISAB  */
   {00,   00,   00,   00,   00,   19,   00,   20,   00  },   /* 14 ISSRQ */
   {00,   00,   00,   00,   00,   00,   00,   21,   00  },   /* 15 ISSRS */
   {00,   00,   16,   00,   00,   22,   23,   00,    8  },   /* 16 ISFN  */
   {00,   00,   16,   00,   00,   00,   24,   00,    8  },   /* 17 ISDN  */
   {27,   53,   27,   26,   26,   26,   26,   26,   27  },   /* 18 ISTDI */
   {00,   00,   16,   00,   00,   00,   28,   00,    8  },   /* 19 ISNF  */
   {50,   54,   50,   50,   50,   50,   50,   50,   50  },   /* 20 ISABT */
   {00,   00,   00,   00,   00,   30,   31,   32,   00  },   /* 21 ISSXF */
   {00,   00,   00,   00,   00,   35,   36,   37,   00  },   /* 22 ISSXN */
   {00,   00,   00,   00,   00,   40,   41,   42,   43  },   /* 23 ISTXF */
   {00,   00,   00,   00,   00,   45,   46,   47,   48  },   /* 24 ISTXN */
   {00,   00,   00,   00,   00,   00,   00,   49,   00  },   /* 25 ISDNCR */
   {00,   00,   00,   00,   00,   00,   00,   25,   00  }    /* 26 ISRSCI */

};


/*}
** Name: SLFSM_ENTRY - FSM matrix
**
** Description:
**      This is the linear array containing the entries for the FSM matrix.  The
**	entries in the preceeding table SL_FSM_TBL are index values
**	corresponding to entries in the current array.  The array size limit on
**	the individual elements of each array entry, SL_OUT_MAX,
**	is set to a value which is currently adequate.  This is confined in
**	scope to the SL.  If future expansion requires this to be increased,
**	this can be done without affecting anything outside SL.  If the values
**	are changed in GCCSL.H and the SL recompiled, everything will work.
**
** History:
**      10-Jun-88 (jbowers)
**          Initial structure implementation
*/


GLOBALCONSTDEF struct _slfsm_entry 
slfsm_entry[]
=
{
 /* ( 0)  */ {SSCLD, {0, 0, 0}},/* Illegal intrsctn  */
 /* ( 1)  */ {SSISL, {OSTCQ, 0, 0}},
 /* ( 2)  */ {SSRSL, {OSTCS, 0, 0}},
 /* ( 3)  */ {SSWAC, {OSCN, OSMDE, 0}},
 /* ( 4)  */ {SSOPN, {OSAC, 0, 0}},
 /* ( 5)  */ {SSCTD, {OSRF, 0, 0}},
 /* ( 6)  */ {SSWCR, {OSSCI, 0, 0}},
 /* ( 7)  */ {SSCLD, {OSTDC, OSMDE, 0}},
 /* ( 8)  */ {SSCTD, {OSMDE, 0}},
 /* ( 9)  */ {SSOPN, {OSSCA, 0, 0}},
 /* (10)  */ {SSCLD, {OSTDC, OSMDE, 0}},
 /* (11)  */ {SSCLD, {OSSCJ, OSTDC, 0}},
 /* (12)  */ {SSOPN, {OSDT, 0, 0}},
 /* (13)  */ {SSOPN, {OSEX, 0, 0}},
 /* (14)  */ {SSOPN, {OSSDI, 0, 0}},
 /* (15)  */ {SSOPN, {OSSEI, 0, 0}},
 /* (16)  */ {SSCLD, {OSTDC, OSMDE, 0}},
 /* (17)  */ {SSCTD, {OSTFX, OSAB, 0}},
 /* (18)  */ {SSCLD, {OSTDC, OSSAX, 0}},
 /* (19)  */ {SSCDN, {OSFN, 0, 0}},
 /* (20)  */ {SSCRR, {OSCOL, OSFN, 0}},
 /* (21)  */ {SSCTD, {OSDN, 0, 0}},
 /* (22)  */ {SSCRR, {OSSRI, 0, 0}},
 /* (23)  */ {SSCRR, {OSCOL, OSSRI, 0}},
 /* (24)  */ {SSCLD, {OSRCA, OSTDC, 0}},
 /* (25)  */ {SSCDN, {OSDN, 0, 0}},
 /* (26)  */ {SSCLD, {OSSAP, 0, 0}},
 /* (27)  */ {SSCLD, {OSMDE, 0, 0}},
 /* (28)  */ {SSOPN, {OSRCJ, 0, 0}},
 /* (29)  */ {SSCLD, {OSTXF, 0, 0}},
 /* (30)  */ {SSOPN, {OSTXF, 0, 0}},
 /* (31)  */ {SSCDN, {OSTXF, 0, 0}},
 /* (32)  */ {SSCRR, {OSTXF, 0, 0}},
 /* (33)  */ {SSCTD, {OSTXF, 0, 0}},
 /* (34)  */ {SSCLD, {OSTXN, 0, 0}},
 /* (35)  */ {SSOPN, {OSTXN, 0, 0}},
 /* (36)  */ {SSCDN, {OSTXN, 0, 0}},
 /* (37)  */ {SSCRR, {OSTXN, 0, 0}},
 /* (38)  */ {SSCTD, {OSTXN, 0, 0}},
 /* (39)  */ {SSCLD, {0, 0, 0}}, /* NOT USED */
 /* (40)  */ {SSOPN, {OSSXF, 0, 0}},
 /* (41)  */ {SSCDN, {OSSXF, 0, 0}},
 /* (42)  */ {SSCRR, {OSSXF, 0, 0}},
 /* (43)  */ {SSCTD, {OSMDE, 0, 0}},
 /* (44)  */ {SSCLD, {0, 0, 0}}, /* NOT USED */
 /* (45)  */ {SSOPN, {OSSXN, 0, 0}},
 /* (46)  */ {SSCDN, {OSSXN, 0, 0}},
 /* (47)  */ {SSCRR, {OSSXN, 0, 0}},
 /* (48)  */ {SSCTD, {OSMDE, 0, 0}},
 /* (49)  */ {SSCRR, {OSDNR, OSRCA, 0}},
 /* (50)  */ {SSCLD, {OSTDC, OSSAP, 0}},
 /* (51)  */ {SSCDN, {OSMDE, 0, 0}},
 /* (52)  */ {SSCLD, {OSPGQ, OSTDC, OSMDE}},
 /* (53)  */ {SSCLD, {OSPGQ, OSSAP, 0}},
 /* (54)  */ {SSCLD, {OSPGQ, OSTDC, OSSAP}}
};


