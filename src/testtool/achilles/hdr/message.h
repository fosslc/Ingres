/*}
** Name: MESSAGE - Achilles message description.
**
** Description:
**      This structure defines the format of an Achilles event message.
**
** History:
**
**  13-Aug-93 (Jeffr)
**	Cloned from CSP routine to hold Async Messages from Children
**	writing to a Mailbox (i.e. log-messages)
**	Note: this routine is required for Achilles Version 2 
**  19-jun-95 (dougb)
**	Remove i4 definition -- set up in compat.h already.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
typedef struct _MESSAGE MESSAGE;        /*  EVENT description. */

#define                     descriptor      struct dsc$descriptor_s

typedef struct _MESSAGE
{
#define			MAX_MESSAGES	    256
/*-----------------------Message-Header---------------------------------------*/
    MESSAGE	    *msg_next;          /* Next message. */
    MESSAGE	    *msg_prev;		/* Previous message. */
    i4	    msg_sequence;	/* Message sequence number. */
    short	    msg_dest;		/* Message destination. */
#define			MSG_PROCESSOR	    0xffffffff
    short	    msg_source;		/* Message source. */
    i4	    msg_iosb[2];	/* Message I/O status. */
    i4	    msg_action;		/* Message action. */
#define			MSG_A_START		1
#define			MSG_A_START_CONNECT	2
#define			MSG_A_START_NEXT	3
#define			MSG_A_START_ACCEPT	4
#define			MSG_A_START_REJECT	5
#define			MSG_A_START_RESTART	6
#define			MSG_A_START_START	7
#define			MSG_A_START_MASTER	8

#define			MSG_A_JOIN_START	12
#define			MSG_A_JOIN_SEND		13
#define			MSG_A_JOIN_ERROR	14
#define			MSG_A_JOIN_RECEIVE	15

#define			MSG_A_ADD_START		16
#define			MSG_A_ADD_SEND		17
#define			MSG_A_ADD_RECEIVE	18
#define			MSG_A_ADD_ERROR		19

#define			MSG_A_ADD_ACK_SEND	24
#define			MSG_A_ADD_ACK_RECEIVE	25
#define			MSG_A_ADD_ACK_ERROR	26
#define			MSG_A_ADD_NAK_SEND	27
#define			MSG_A_ADD_NAK_RECEIVE	28
#define			MSG_A_ADD_NAK_ERROR	29

#define			MSG_A_JOIN_ACK_SEND	32
#define			MSG_A_JOIN_ACK_RECEIVE	33
#define			MSG_A_JOIN_ACK_ERROR	34
#define			MSG_A_JOIN_NAK_SEND	35
#define			MSG_A_JOIN_NAK_RECEIVE	36
#define			MSG_A_JOIN_NAK_ERROR	37

#define			MSG_A_CONNECT_START	40
#define			MSG_A_CONNECT_SEND	41
#define			MSG_A_CONNECT_RECEIVE	42
#define			MSG_A_CONNECT_ACCEPT	43
#define			MSG_A_CONNECT_REJECT	44

#define			MSG_A_LEAVE_START	48
#define			MSG_A_LEAVE_RESTART	49
#define			MSG_A_LEAVE_NODE	50
#define			MSG_A_LEAVE_MASTER	51

#define			MSG_A_RUN_START		56
#define			MSG_A_RUN_SEND		57
#define			MSG_A_RUN_RECEIVE	58
#define			MSG_A_RUN_ERROR		59
#define			MSG_A_RESET_START	60
#define			MSG_A_RESET_SEND	61
#define			MSG_A_RESET_RECEIVE	62
#define			MSG_A_RESET_ERROR	63

#define			MSG_A_MEMBER_SEND	    64
#define			MSG_A_MEMBER_RECEIVE	    65
#define			MSG_A_MEMBER_ERROR	    66
#define			MSG_A_MEMBER_ACK_SEND	    67
#define			MSG_A_MEMBER_ACK_RECEIVE    68
#define			MSG_A_MEMBER_ACK_ERROR	    69
#define			MSG_A_MEMBER_NAK_SEND	    70
#define			MSG_A_MEMBER_NAK_RECEIVE    71
#define			MSG_A_MEMBER_NAK_ERROR	    72

#define			MSG_A_DEADLOCK_RECEIVE	    76
#define			MSG_A_DEADLOCK_SEND	    77
#define			MSG_A_DEADLOCK_SEND_ERROR   78


#define			MSG_A_NODE_READ		    80
#define			MSG_A_DECNET_READ	    81
#define			MSG_A_NODE_DISCONNECT	    82
#define			MSG_A_NODE_CANCEL	    83

#define                 MSG_A_TIMER_COMPLETE        88
#define			MSG_A_LEAVE_DELAY	    89

                                                    /* The Delay leave timer
                                                    ** has completed.
                                                    */
    descriptor	    msg_desc;
/*-------------------------Message-Body---------------------------------------*/
    long	    msg_type;		/* Type of message. */
#define			MSG_T_START		1
#define			MSG_T_JOIN		2
#define			MSG_T_JOIN_ACK		3
#define			MSG_T_JOIN_NAK		4
#define			MSG_T_ADD		5
#define			MSG_T_ADD_ACK		6
#define			MSG_T_ADD_NAK		7
#define			MSG_T_CONNECT		8
#define			MSG_T_CONNECT_ACK	9
#define			MSG_T_CONNECT_NAK	10
#define			MSG_T_RUN		11
#define			MSG_T_RESET		12
#define			MSG_T_MEMBER		13
#define			MSG_T_MEMBER_ACK	14
#define			MSG_T_MEMBER_NAK	15
#define			MSG_T_DEADLOCK		16
                                                /* Message to indicate 
                                                ** which timer went off */
    union
    {
#define				MAX_MSG_SIZE		8192
	struct
	{
	    short	    msgtype;		    /*	DECnet connect information. */
	    short	    unit;
	    char	    name[1];
	}   decnet;
	struct
	{
	    i4	    mask;		    /*	Mask of running systems. */
	}   run;
	struct
	{
	    i4	    node;		    /*  Node to add. */
	}   add;
	struct
	{
	    i4      dlck_info;		    /*	Deadlock message. */
	}   deadlock;
	char		    data[MAX_MSG_SIZE - 4];
    }	msg;
};

