/*
** Copyright (c) 2004 Ingres Corporation
*/

package ca.edbc.io;

/*
** Name: IoConst.java
**
** Description:
**	Defines the global constants used in the driver-server
**	transport layer communication system.
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
*/

import ca.edbc.util.IdMap;


/*
** Name: IoConst
**
** Description:
**	Interface defining the global constants used in driver-
**	server transport layer communications.
**
**  Constants
**
**	JDBC_TL_ID		Packet header ID.
**	JDBC_TL_CR		Packet type: Connection Request.
**	JDBC_TL_CC		Packet type: Connection Confirm.
**	JDBC_TL_DT		Packet type: Data.
**	JDBC_TL_DR		Packet type: Disconnect Request.
**	JDBC_TL_DC		Packet type: Disconnect Confirm.
**	JDBC_TL_INT		Packet type: Interrupt.
**	JDBC_TL_CP_PROTO	Connect param: protocol
**	JDBC_TL_PROTO_1		Transport Layer protocol level 1.
**	JDBC_TL_DFLT_PROTO	TL supported protocol level.
**	JDBC_TL_CP_SIZE		Connect param: packet size.
**	JDBC_TL_PKT_MAX		Max packet length.
**	JDBC_TL_PKT_MIN		Min packet length.
**	JDBC_TL_CP_MSG		Connect param: MSG layer info.
**	JDBC_TL_CP_CHRSET	Connect param: character set.
**	JDBC_TL_DP_ERR		Disconnect param: error code.
**	JDBC_TL_F4_LEN		Length of f4 in network format.
**	JDBC_TL_F8_LEN		Length of f8 in network format.
**
**	JDBC_MSG_ID		Message header ID.
**	JDBC_MSG_EOD		Header flag: end-of-data.
**	JDBC_MSG_EOG		Header flag: end-of-group.
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
*/

interface
IoConst
{
    /*
    ** EDBC Transport layer constants.
    */
    int		JDBC_TL_ID = 0x4C54434A;	// Header ID: "JCTL"

    short	JDBC_TL_CR = 0x5243;		// Connect request: "CR"
    short	JDBC_TL_CC = 0x4343;		// Connect confirm: "CC"
    short	JDBC_TL_DT = 0x5444;		// Data: "DT"
    short	JDBC_TL_DR = 0x5244;		// Disconnect request: "DR"
    short	JDBC_TL_DC = 0x4344;		// Disconnect confirm: "DC"
    short	JDBC_TL_INT = 0x5249;		// Interrupt: "IR"

    IdMap	tlMap[] =
    {
	new IdMap( JDBC_TL_CR,	"CR" ),
	new IdMap( JDBC_TL_CC,	"CC" ),
	new IdMap( JDBC_TL_DT,	"DT" ),
	new IdMap( JDBC_TL_DR,	"DR" ),
	new IdMap( JDBC_TL_DC,	"DC" ),
	new IdMap( JDBC_TL_INT,	"INT" ),
    };

    byte	JDBC_TL_CP_PROTO =  0x01;	// Connect param: protocol
    byte	JDBC_TL_PROTO_1 =   0x01;	// Protocol level 1
    byte	JDBC_TL_DFLT_PROTO = JDBC_TL_PROTO_1;

    byte	JDBC_TL_CP_SIZE =   0x02;	// Connect param: packet size
    byte	JDBC_TL_PKT_MAX =   0x0F;	// 32K packets
    byte	JDBC_TL_PKT_MIN =   0x0A;	// 1K packets

    byte	JDBC_TL_CP_MSG =    0x03;	// Connect param: upper layer info.
    byte	JDBC_TL_CP_CHRSET = 0x04;	// Connect param: character set.
    byte	JDBC_TL_DP_ERR =    0x01;	// Disconnect param: error code.

    byte	JDBC_TL_F4_LEN  = 6;		// Length of f4 network format.
    byte	JDBC_TL_F8_LEN	= 10;		// Length of f8 network format.

    /*
    ** EDBC Message layer encapsulation constants.
    */
    int		JDBC_MSG_ID = 0x4342444A;	// Message ID: "JDBC"
    byte	JDBC_MSG_EOD = 0x01;		// end-of-data
    byte	JDBC_MSG_EOG = 0x02;		// end-of-group

} // interface IoConst
