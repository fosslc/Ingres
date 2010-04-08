/*
** Copyright (c) 1999, 2002 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.dam;

/*
** Name: TlConst.java
**
** Description:
**	Defines the global constants used in the Data Access Messaging
**	Transport Layer (DAM-TL) communications protocol.
**
**  Interfaces
**
**	TlConst
**
** History:
**	 8-Sep-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Parameterized the TL CR/CC/DR data.
**	22-Sep-99 (gordy)
**	    Added TL CR parameter for character set.
**	 4-Nov-99 (gordy)
**	    Added TL packet Interrupt.
**	28-Jan-00 (gordy)
**	    Added ID map for transport level message IDs.
**	20-Apr-00 (gordy)
**	    Moved to package io.  Removed MSG & DBMS constants.
**	 1-Oct-02 (gordy)
**	    Moved to GCF dam package.  Renamed to associate with
**	    transport layer.  Changed JDBC references to DAM.
**	20-Dec-02 (gordy)
**	    Added protocol level 2, new header ID and connection
**	    parameter for character-set ID.
**	15-Jan-03 (gordy)
**	    Added MSG layer ID connection parameter.
*/

import com.ingres.gcf.util.IdMap;


/*
** Name: TlConst
**
** Description:
**	Interface defining the global constants used in the DAM-TL
**	communications protocol.
**
**  Constants
**
**	DAM_TL_ID_1		Packet header ID (protocol level 1).
**	DAM_TL_ID_2		Packet header ID (beginning with proto level 2).
**	DAM_TL_CR		Packet type: Connection Request.
**	DAM_TL_CC		Packet type: Connection Confirm.
**	DAM_TL_DT		Packet type: Data.
**	DAM_TL_DR		Packet type: Disconnect Request.
**	DAM_TL_DC		Packet type: Disconnect Confirm.
**	DAM_TL_INT		Packet type: Interrupt.
**	DAM_TL_CP_PROTO		Connect param: protocol
**	DAM_TL_PROTO_1		Transport Layer protocol level 1.
**	DAM_TL_PROTO_2		Transport Layer protocol level 2.
**	DAM_TL_PROTO_DFLT	TL supported protocol level.
**	DAM_TL_CP_SIZE		Connect param: packet size.
**	DAM_TL_PKT_MAX		Max packet length.
**	DAM_TL_PKT_MIN		Min packet length.
**	DAM_TL_CP_MSG		Connect param: MSG layer info.
**	DAM_TL_CP_CHRSET	Connect param: character set.
**	DAM_TL_CP_CHRSET_ID	Connect param: character set ID.
**	DAM_TL_CP_MSG_ID	Connect param: MSG layer ID.
**	DAM_TL_MSG_ID		DAM-ML MSG layer ID.
**	DAM_TL_DP_ERR		Disconnect param: error code.
**	DAM_TL_F4_LEN		Length of f4 in network format.
**	DAM_TL_F8_LEN		Length of f8 in network format.
**
**  Public Data:
**
**	tlMap			Transport layer packet types.
**
** History:
**	 8-Sep-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Parameterized the TL CR/CC/DR data.
**	22-Sep-99 (gordy)
**	    Added TL CR parameter for character set.
**	 4-Nov-99 (gordy)
**	    Added TL packet for interrupt.
**	28-Jan-00 (gordy)
**	    Added ID map for transport level message IDs.
**	 1-Oct-02 (gordy)
**	    Renamed to associate with transport layer.  
**	    Changed JDBC references to DAM.
**	20-Dec-02 (gordy)
**	    Added protocol level 2, new header ID and connection
**	    parameter for character-set ID.
**	15-Jan-03 (gordy)
**	    Added MSG layer ID connection parameter.
*/

interface
TlConst
{
    /*
    ** DAM Transport Layer constants.
    */
    int		DAM_TL_ID_1	= 0x4C54434A;	// Header ID: "JCTL"
    int		DAM_TL_ID_2	= 0x4C544D44;	// Header ID: "DMTL"

    short	DAM_TL_CR	= 0x5243;	// Connect request: "CR"
    short	DAM_TL_CC	= 0x4343;	// Connect confirm: "CC"
    short	DAM_TL_DT	= 0x5444;	// Data: "DT"
    short	DAM_TL_DR	= 0x5244;	// Disconnect request: "DR"
    short	DAM_TL_DC	= 0x4344;	// Disconnect confirm: "DC"
    short	DAM_TL_INT	= 0x5249;	// Interrupt: "IR"

    IdMap	tlMap[] =
    {
	new IdMap( DAM_TL_CR,	"CR" ),
	new IdMap( DAM_TL_CC,	"CC" ),
	new IdMap( DAM_TL_DT,	"DT" ),
	new IdMap( DAM_TL_DR,	"DR" ),
	new IdMap( DAM_TL_DC,	"DC" ),
	new IdMap( DAM_TL_INT,	"INT" ),
    };

    byte	DAM_TL_CP_PROTO	    = 0x01;	// Connect param: protocol
    byte	DAM_TL_PROTO_1	    = 0x01;	// Protocol level 1
    byte	DAM_TL_PROTO_2	    = 0x02;
    byte	DAM_TL_PROTO_DFLT   = DAM_TL_PROTO_2;

    byte	DAM_TL_CP_SIZE	    = 0x02;	// Connect param: packet size
    byte	DAM_TL_PKT_MAX	    = 0x0F;	// 32K packets
    byte	DAM_TL_PKT_MIN	    = 0x0A;	// 1K packets

    byte	DAM_TL_CP_MSG	    = 0x03;	// Connect param: ML conn parms.
    byte	DAM_TL_CP_CHRSET    = 0x04;	// Connect param: character set.
    byte	DAM_TL_CP_CHRSET_ID = 0x05;	// Connect param: charset ID.

    byte	DAM_TL_CP_MSG_ID    = 0x06;	// Connect param: Messaging ID.
    int		DAM_TL_MSG_ID	    = 0x4C4D4D44;	// Messaging ID: "DMML".

    byte	DAM_TL_DP_ERR	    = 0x01;	// Disconnect param: error code.

    byte	DAM_TL_F4_LEN	    = 6;	// Length of f4 network format.
    byte	DAM_TL_F8_LEN	    = 10;	// Length of f8 network format.


} // interface TlConst
