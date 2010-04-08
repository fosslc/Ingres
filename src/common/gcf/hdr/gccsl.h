/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: GCCSL.H - Data structures and constants for Session Layer
**
** Description:
**      GCCSL.H contains definitions of constants and data structures which
**      are specific to the Session Layer.
**
** History:
**      25-Jul-88 (jbowers)
**          Initial module creation
**	1-Mar-91 (seiwald)
**	    Moved state machine into gccplfsm.roc.
**	14-May-91 (cmorris)
**	    Actions are no more:- incorporate only remaining action as event.
**  	16-Aug-91 (cmorris)
**  	    Added support for OSTFX event.
**	19-Nov-92 (seiwald)
**	    Changed nat's to char's in state table for compactness.
**      22-Mar-93 (huffman)
**          Change "extern READONLY" to "GLOBALCONSTREF"
**	12-Jul-00 (gordy)
**	    Added SL protocol level.
**/


/*
**  Defines of other constants.
*/

/*
**      Session Layer Finite State Machine constant definitions
*/

/*
**      States for FSM
*/

#define                 SSCLD		0
#define                 SSISL		1
#define                 SSRSL		2
#define                 SSWAC		3
#define                 SSWCR		4
#define                 SSOPN		5
#define                 SSCDN		6
#define                 SSCRR		7
#define                 SSCTD		8
#define                 SSMAX		9

/*
**      Input events for Session Layer FSM
*/

#define                 ISSCQ            0
#define                 ISTCI            1
#define                 ISTCC            2
#define                 ISSCA            3
#define                 ISSCJ            4
#define                 ISCN 		 5
#define                 ISAC             6
#define                 ISRF             7
#define                 ISSDQ            8
#define                 ISSEQ            9
#define                 ISDT            10
#define                 ISEX            11
#define                 ISSAU           12
#define                 ISAB            13
#define                 ISSRQ           14
#define                 ISSRS           15
#define                 ISFN            16
#define                 ISDN            17
#define                 ISTDI           18
#define                 ISNF            19
#define                 ISABT		20
#define                 ISSXF		21
#define                 ISSXN		22
#define                 ISTXF		23
#define                 ISTXN		24
#define			ISDNCR		25
#define			ISRSCI          26
#define                 ISMAX		27

/*
**      Output events for Session Layer FSM
*/

#define                 OSSCI            1
#define                 OSSCA            2
#define                 OSSCJ            3
#define                 OSSAX            4
#define                 OSSAP		 5
#define                 OSSDI		 6
#define                 OSSEI            7
#define                 OSSRI            8
#define                 OSRCA            9
#define                 OSRCJ           10
#define                 OSTCQ           11
#define                 OSTCS           12
#define                 OSTDC           13
#define                 OSCN            14
#define                 OSAC            15
#define                 OSRF            16
#define                 OSDT            17
#define                 OSEX            18
#define                 OSAB            19
#define                 OSFN            20
#define                 OSDN            21
#define                 OSNF            22
#define                 OSSXF		23
#define                 OSSXN		24
#define                 OSTXF		25
#define                 OSTXN		26
#define                 OSCOL		27
#define                 OSDNR		28
#define			OSMDE		29
#define			OSPGQ		30
#define	    	    	OSTFX	    	31

/*
**      Definitions for constructing Session Layer FSM
*/

#define                 SL_OUT_MAX	3   /* max # output events/entry */

/*
** SL protocol levels:
**
**	VRSN_0		Original level.
**	VRSN_1		Password Encryption.
**
** Protocol VRSN_1 is actually negotiated by the TL during its
** initial connection and signalled in the CCB header flags as
** CCB_PWD_ENCRYPT.  Higher protocol levels are negotiated
** during SL connection/response.
*/

#define                 SL_PROTOCOL_VRSN_0	0
#define                 SL_PROTOCOL_VRSN_1	1
#define                 SL_PROTOCOL_VRSN	SL_PROTOCOL_VRSN_1


GLOBALCONSTREF    char	     slfsm_tbl[ISMAX][SSMAX];

GLOBALCONSTREF    struct    _slfsm_entry
{
    char		    state;
    char		    output[SL_OUT_MAX];
} slfsm_entry[];
