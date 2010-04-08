/*
** Copyright (c) 1987, 2003 Ingres Corporation
*/
#include <iosbdef.h>

/**
** Name: GCDECNET.H - Header file for DECNET protocol support
**
** Description:
**	This module contains all data structure definitions required by the
**	DECNET protocol driver.
**
** History: $Log-for RCS$
**      11-May-88 (berrow)
**          Initial module implementation.
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	29-aug-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _DECNET_MSG DECNET_MSG;
typedef struct _DECNET_FUNC DECNET_FUNC;
typedef struct _DECNET_NCB DECNET_NCB;
typedef struct _DECNET_PCB DECNET_PCB;

/*
**  Defines of other constants.
*/

/*
**      Definitions of default protocol listen ID
*/

# define    DECNET_DFLT		"II_COMS_0"


/*}
** Name: DECNET_MSG - Layout of DECNET control message
**
** Description:
**	This structure describes the layout of messages received from DECNET
**	supporting optional network protocol features (completion status, 
**	interrupts etc.) provided with non-transparent communication.  It is
**	also used to receive an NCB accompanying a connection request.  The info
**	area of the NCB is returned with the request acceptance.
**
** History:
**      11-May-88 (berrow)
**          Initial structure implementation.
[@history_template@]...
*/
typedef struct _DECNET_MSG
{
    u_i2	    msg_type;
    u_i2	    unit;
    char	    name_count;
    /* ONLY in messages from the network -- name is "NET" (3 characters) */
    char	    name[3];	
    char	    info_count;
    /* DECNET network mailbox messages are limited to 16 bytes */
    char	    info[100];
};

/*}
** Name: DECNET_FUNC - DECNET function descriptor
**
** Description:
**	This structure describes the layout of a "5-byte block consisting
**	of a function type (one byte) and a longword parameter".
**	It is required as a paremeter to select whether we are declaring
**	a network name or a network object number.
**
** History:
**      12-May-88 (berrow)
**          Initial structure implementation.
[@history_template@]...
*/
typedef struct _DECNET_FUNC
{
    char	    type;
    i4		    parm;
};

/*}
** Name: DECNET_PCB - Protocol Control Block for DECNET
**
** Description:
**	DECNET_PCB holds information pertinent to each individual DECNET
**	connection.
**
**	Following are the definitions of the elements:
**
**	    net_desc			Used to assign a mailbox channel to the
**					network psuedo-device.
**	    mbx_desc			Used to hold name of mailbox created for
**					use with network psuedo-device.
**	    ncb_desc			Used to hold Network Connect Block (NCB)
**					descriptor.
**	    ctl_iosb			Receives QIO completion status for
**					control operations (OPEN, CONNECT,
**					LISTEN, DISCONNECT)
**	    snd_iosb			Receives QIO completion status for send
**					operations.
**	    rcv_iosb			Receives QIO completion status for
**					receive operations.
**					LISTEN, DISCONNECT)
**	    msg_area			Area to receive DECNET status messages.
**					Eg. Accept/reject status, interrupt
**					messages (up to 16 bytes).
**					The DECNET_MSG structure decribes the 
**					layout of messages sent on behalf 
**					of logical link operations.
**	    mchan			Channel to network mailbox.
**	    dchan			Channel to network device.
**	    istate			Input State
**	    iestate			Input (Expedited) State
**	    ostate			Output State
**	    net_func_desc		Used to ask to declare network name.
**	    net_name_desc		Descriptor for declared network name.
**	    net_func			Area for "function type block".
**	    ncb_area			Area for Network Connect Block.
**	    netname			Area for declared network name.
[@comment_line@]...
**
** History:
**      11-May-88 (berrow)
**          Initial structure impementation
[@history_template@]...
*/
typedef struct _DECNET_PCB
{
    struct dsc$descriptor_s	net_desc;
    struct dsc$descriptor_s	netmbx_desc;
    struct dsc$descriptor_s	ncb_desc;	/* Network Connect Block */
    IOSB	               	ctl_iosb;
    IOSB	               	snd_iosb;
    IOSB	               	rcv_iosb;
    DECNET_MSG			msg_area;
    u_i2			mchan;		/* Mailbox channel */
    u_i2			dchan;		/* Device channel */
    u_i2			istate;		/* Input State */
    u_i2			iestate;	/* Input (Expedited) State */
    u_i2			ostate;		/* Output State */
    u_i2			dstate;		/* Disconnect State */
    struct dsc$descriptor_s     net_func_desc;
    struct dsc$descriptor_s     net_name_desc;
    DECNET_FUNC                 net_func;
    char			ncb_area[100];
    char			netname[12];
};
