/*
**	MWget.c
**	"@(#)mwget.c	1.23"
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include <compat.h>
# include <cv.h>		 
# include <si.h>
# include <st.h>
# include <te.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include <fe.h>
# include "mwmws.h"
# include "mwhost.h"
# include "mwform.h"
# include "mwintrnl.h"

/**
** Name:	MWget.c - INGRES Host Frontend Routines MacWorkStation.
**
** Usage:
**	INGRES frontends only.
**
** Description:
**	File contains low level I/O routines for the INGRES front ends.
**	The file MACtest.c is used, instead, on the Macintosh test host.
**
**	Routines defined here are:
**
**	Low level utility & I/O
**	  - IIMWgeGetEvent		Receive a MWS event from the Macintosh.
**	  - IIMWgseGetSpecificEvent	Request a specific MWS event from Mac.
**	  - IIMWpcPutCmd		Send a MWS command to the Macintosh.
**	  - IIMWpxcPutXCmd		Send a MWS EXEC command to the Mac.
**	  - IIMWmgdMsgGetDec		Get a decimal number from msg.
**	  - IIMWmgiMsgGetId		Get a class and id from msg.
**	  - IIMWmgsMsgGetStr		Get a string from msg.
**
** History:
**	05/19/89 (RGS) - Initial implementation.
**	03/26/90 (RGS) - Resend in IIMWgseGetSpecificEvent
**	04/14/90 (dkh) - Eliminated use of "goto"'s where safe to do so.
**	07/10/90 (dkh) - Integrated changed into r63 code line.
**	08/22/90 (nasser) - Added the function IIMWpxc().
**	08/28/90 (nasser) - Moved the framing of messages from mwio.c to
**		this file. Added sequncing and checksum to messages.
**	10/08/90 (nasser) - Changed to use TEwrite() and TEget() instead
**		of mwio calls.
**	06/08/92 (fredb) - Enclosed file in 'ifdef DATAVIEW' to protect
**		ports that are not using MacWorkStation from extraneous
**		code and data symbols.  Had to include 'fe.h' to get
**		DATAVIEW (possibly) defined.  Also 'dbms.h' to support
**		'fe.h'.
**	26-jun-97 (kitch01) - Bug 79761/83247. On NT allow us to receive ctrl-b
**			   as the mwSTOP_CHAR instead of ctrl-a. This is because some
**				if not most, telnet servers for NT use ctrl-a in a 
**				special way. Sending ctrl-a seems to be ok. Change to
**				get_msg().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/
# ifdef DATAVIEW

# define BLK_THRESHOLD	10
# define END_LINE	'\015'

static TRMsg	IIMWgMessage = {0}; /* The global MWS current input message */
static char	blkmsg[mwPACKET_SZ] = {0};
static i4	blklen = 0;

/* Start/stop strings for send messages */
static char	start_str[] = {mwSTART_CHAR};
static char	end_str[] = {mwSTOP_CHAR, END_LINE};

/* Sequence numbers */
static char	snd_seq_no = mwPACKET_SEQ_LO;
static char	rcv_seq_no = mwPACKET_SEQ_LO;


static i4	get_msg();
static STATUS	snd_msg();
static char	do_chksum();


/*{
** Name:	IIMWgeGetEvent - Receive a MWS event from the Macintosh.
**
** Description:
**	This routine will read an input record from the Macintosh. The
**	message is parsed to identify the message class and event
**	identifier.  Each record or event begins with a four character
**	control sequence. The first character of the sequence is the
**	message class. The message class identifies which MWS director
**	sent the message. The second, third and four characters are
**	the event identifier. This identifier specifies the event type
**	within the originating director. The remainder of the message
**	contains optional parameters associated with the event.
**	The incoming message may be broken into packets if it is tool
**	long.  IIMWgeGetEvent will gather the packets into one
**	message structure.
**
** Inputs:
**	None.
**
** Outputs:
**	class	IntegerPtr	(MWS director event is from)
**	id	IntegerPtr	(event received)
**	Returns:
**		theMsg	TPMsg 		(any associated parameters)
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/19/89 (RGS) - Initial implementation.
**	28-feb-92 (leighb) DeskTop Porting Change: Eliminated STscanf().
*/
TPMsg
IIMWgeGetEvent(class, id)
i4	*class;
i4	*id;
{
	TPMsg	 theMsg = &IIMWgMessage;
	i4	 msglen;
	char	 ch;
	i4	 tmp_id;
	u_i4	 rcvlen;
	char	*cp;
	char	 buf[mwPACKET_SZ + 1];
	i4	 buflen;

	msglen = get_msg(theMsg->msgData);
	if (msglen == -1)
		goto error;
chkmsg:
	theMsg->msgIndex = 0;
	theMsg->msgLength = msglen;
	if (IIMWmgiMsgGetId(theMsg, &ch, &tmp_id) != OK)
	{
		IIMWpelPrintErrLog(mwERR_BAD_MSG, theMsg->msgData);
		goto error;
	}
	/* Is this the beginning of extended message */
	if ((ch != 'X') || (tmp_id != 350))
		goto ok;

	if ((IIMWmgdMsgGetDec(theMsg, &msglen, NULLCHAR) != OK) ||
		(msglen <= 0))
	{
		IIMWpelPrintErrLog(mwERR_GENERIC,
			"Msg length not found in X350");
		goto error;
	
	}
	rcvlen = 0;
	cp = theMsg->msgData;
	while (rcvlen < msglen)
	{
		buflen = get_msg(buf);
		if (buflen == -1)
			goto error;

		if (((ch = buf[0]) < 'A') || (buf[1] < '0') 	 
		        || (buf[2] < '0') || (buf[3] < '0'))
		{
			IIMWpelPrintErrLog(mwERR_GENERIC,
				"Parse error in X351");
			goto error;
		}
		if ((ch != 'X') || (buf[1] != '3') || (buf[2] != '5')
				|| (buf[3]  < '1') || (buf[3]  > '2'))
		{
			IIMWpelPrintErrLog(mwERR_GENERIC,
				"Expected X351 missing");
			goto error;
		}
		if (buf[3] == '2')
		{
			IIMWplPrintLog(mwLOG_GENERIC, "Extended msg aborted");
			goto error;
		}

		tmp_id = 351;					 
		STcopy(buf + kMsgParms, cp);
		cp += buflen - kMsgParms;
		rcvlen += buflen - kMsgParms;
	}

	if (rcvlen != msglen)
	{
		IIMWpelPrintErrLog(mwERR_GENERIC,
			"Extended msg length error");
		goto error;
	}

	goto chkmsg;

ok:
	*class = ch;
	*id = tmp_id;
	return(theMsg);

error:
	*class = -1;
	return((TPMsg) NULL);
}

/*{
** Name:	IIMWgseGetSpecificEvent - Request a specific MWS event from Mac.
**
** Description:
**	This routine will read an input record from the Macintosh. The
**	message is parsed to identify the message class and event
**	identifier. They must match the requested class and event
**	identifier.
**
** Inputs:
**	class	Char	(MWS director of expected event)
**	id	Integer	(specific event to be received)
**
** Outputs:
**	Returns:
**		theMsg	TPMsg (any parameters that were returned)
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
**	08/22/90 (nasser) - Took out resend message stuff.
*/
TPMsg
IIMWgseGetSpecificEvent(class, id)
i4  class,id;
{
	i4 msgclass,msgid;
	TPMsg theMsg;

	while ((theMsg = IIMWgeGetEvent(&msgclass, &msgid)) != (TPMsg) NULL)
	{
		if ((class != msgclass) || (id != msgid))
			IIMWplPrintLog(mwLOG_EXTRA_MSG, "GetSpecificEvent");
		else
			return(theMsg);
	}
	return((TPMsg) NULL);
}

/*{
** Name:	IIMWpcPutCmd - Send a MWS command to the Macintosh.
**
** Description:
**	This routine will send a command to the Macintosh.
**	If the message is too long, it will be broken into
**	packets.
**
** Inputs:
**	class_id	PTR	(MWS director and command number)
**	theMsg		PTR	(any parameters to be included)
**
** Outputs:
**	Returns:
**		OK	If OK.
**		FAIL	If communication failure.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
**	08/22/90 (nasser) - Took out saving of message.
**	08/22/90 (nasser) - Send block message if any.
**	08/22/90 (nasser) - Parametrized with kMsgParms.
*/
STATUS
IIMWpcPutCmd(class_id, params)
char	*class_id;
char	*params;
{
	char	 theMsg[mwMAX_MSG_LEN + 1];
	i4	 msglen;
	char	 buf[mwPACKET_SZ + 1];
	u_i4	 buflen;
	char	*cp;

	msglen = STlength(class_id) + STlength(params);
	if (msglen > mwMAX_MSG_LEN)
		return(FAIL);
	_VOID_ STprintf(theMsg, "%s%s", class_id, params);

	if (blklen != 0)
	{
		if (snd_msg(blkmsg, (u_i4) blklen, FALSE) == FAIL)
			return(FAIL);
		blklen = 0;
	}
	
	if (msglen <= mwPACKET_SZ)
		return(snd_msg(theMsg, (u_i4) msglen, FALSE));

	/*
	**  Command too long for one message.  Send X050 to tell MWS
	**  that command is being broken into packets.
	*/
	_VOID_ STprintf(buf, "X050%d", msglen);
	if (snd_msg(buf, STlength(buf), FALSE) == FAIL)
		return(FAIL);
	
	/*  send the packets */
	for (cp = theMsg; msglen > 0; )
	{
		_VOID_ STprintf(buf, "X051%.*s", mwPACKET_SZ - kMsgParms, cp);
		buflen = STlength(buf);
		if (snd_msg(buf, buflen, FALSE) == FAIL)
			goto abort;
		cp += buflen - kMsgParms;
		msglen -= buflen - kMsgParms;
	}

	return(OK);

abort:
	/* Error occurred while sending the packets. */
	_VOID_ STprintf(buf, "X052");
	_VOID_ snd_msg(buf, STlength(buf), TRUE);
	return(FAIL);
}

/*{
** Name:	IIMWpxcPutXCmd - Send a MWS Exec command to the Macintosh.
**
** Description:
**	This routine will send an EXEC command to the Macintosh.
**	An EXEC command has the letter 'X' for class. The difference
**	between this function and IIWMpcPutCmd() is that IIMWpxc()
**	will block commands together and send them in groups if
**	appropriate.
**	If the message is too long, it will be broken into
**	packets.
**
** Inputs:
**	class_id	PTR	MWS director and command number
**	theMsg		PTR	any parameters to be included
**	flags		i4	flags
**
** Outputs:
**	Returns:
**		OK	If OK.
**		FAIL	If communication failure.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	22-aug-90 (nasser) - Initial definition based on IIMWpcPutCmd().
**	28-aug-90 (nasser) - Changed parameter flush to flags.
*/
STATUS
IIMWpxcPutXCmd(class_id, params, flags)
char	*class_id;
char	*params;
i4	 flags;
{
	char	 theMsg[mwMAX_MSG_LEN + 1];
	i4	 msglen;
	bool	 flush = (flags & mwMSG_FLUSH) != 0;
	bool	 seq_frc = (flags & mwMSG_SEQ_FORCE) != 0;
	char	 buf[mwPACKET_SZ + 1];
	u_i4	 buflen;
	char	*cp;

	msglen = STlength(class_id) + STlength(params);
	if (msglen > mwMAX_MSG_LEN)
		return(FAIL);
	_VOID_ STprintf(theMsg, "%s%s", class_id, params);
	
	if (blklen != 0)
	{
		if (blklen + msglen + 1 <= mwPACKET_SZ)
		{
			_VOID_ STprintf(blkmsg, "%s%c%s", blkmsg,
				mwBLKMSG_DELIMIT, theMsg);
			blklen += 1 + msglen;
			if (flush ||
				(mwPACKET_SZ - blklen < BLK_THRESHOLD))
			{
				if (snd_msg(blkmsg, (u_i4) blklen, seq_frc)
					!= OK)
				{
					return(FAIL);
				}
				blklen = 0;
			}
			return(OK);
		}
		else
		{
			if (snd_msg(blkmsg, (u_i4) blklen, seq_frc) != OK)
				return(FAIL);
			blklen = 0;
		}
	}

	if (! flush && (mwPACKET_SZ - msglen >= BLK_THRESHOLD))
	{
		STcopy(theMsg, blkmsg);
		blklen = msglen;
		return(OK);
	}
	if (msglen <= mwPACKET_SZ)
		return(snd_msg(theMsg, (u_i4) msglen, seq_frc));

	/*
	**  Command too long for one message.  Send X050 to tell MWS
	**  that command is being broken into packets.
	*/
	_VOID_ STprintf(buf, "X050%d", msglen);
	if (snd_msg(buf, STlength(buf), FALSE) == FAIL)
		return(FAIL);
	
	/*  send the packets */
	for (cp = theMsg; msglen > 0; )
	{
		_VOID_ STprintf(buf, "X051%.*s", mwPACKET_SZ - kMsgParms, cp);
		buflen = STlength(buf);
		if (snd_msg(buf, buflen, FALSE) == FAIL)
			goto abort;
		cp += buflen - kMsgParms;
		msglen -= buflen - kMsgParms;
	}

	return(OK);

abort:
	/* Error occurred while sending the packets. */
	_VOID_ STprintf(buf, "X052");
	_VOID_ snd_msg(buf, STlength(buf), TRUE);
	return(FAIL);
}

/*{
** Name:  IIMWmgdMsgGetDec -- Get a decimal number from msg.
**
** Description:
**	Parse the message upto the specified mark, and return
**	the decimal value.
**
** Inputs:
**	msg		Pointer to message structure
**	mark		delimiter character
**
** Outputs:
**	num		Pointer to number to return value
** 	Returns:
**		OK/FAIL
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	11-jun-90 (nasser)
**		Initial definition.
*/
STATUS
IIMWmgdMsgGetDec(msg, num, mark)
TPMsg	 msg;
i4	*num;
char	 mark;
{
	char	*pos;
	STATUS	 stat;

	if (msg->msgIndex > msg->msgLength)
		return(FAIL);
	if (mark == NULLCHAR)
	{
		pos = &msg->msgData[msg->msgLength];
	}
	else
	{
		pos = STindex(&msg->msgData[msg->msgIndex], &mark, 0);
		if (pos == NULL)
		{
			return(FAIL);
		}
	}
	*pos = NULLCHAR;
	stat = CVan(&msg->msgData[msg->msgIndex], num);
	*pos = mark;
	if (stat != OK)
	{
		return(FAIL);
	}
	msg->msgIndex = pos + 1 - msg->msgData;
	return(OK);
}

/*{
** Name:  IIMWmghMsgGetId -- Get class and id from msg.
**
** Description:
**	Get the class character and the 3-digit id code from
**	the message.
**
** Inputs:
**	msg		Pointer to message structure
**
** Outputs:
**	class		Class of message
**	id		Id of message
** 	Returns:
**		OK/FAIL
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	11-jun-90 (nasser)
**		Initial definition.
*/
STATUS
IIMWmgiMsgGetId(msg, ch, id)
TPMsg	 msg;
char	*ch;
i4	*id;
{
	char	*pos;
	char	 save_char;
	STATUS	 stat;

	if ((msg->msgIndex != 0) || (msg->msgLength < kMsgParms))
		return(FAIL);
	pos = &msg->msgData[kMsgParms];
	save_char = *pos;
	*pos = NULLCHAR;
	*ch = msg->msgData[0];
	stat = CVan(&msg->msgData[1], id);
	*pos = save_char;
	if (stat != OK)
	{
		return(FAIL);
	}
	msg->msgIndex = kMsgParms;
	return(OK);
}

/*{
** Name:  IIMWmgsMsgGetStr -- Get a string from msg.
**
** Description:
**	Parse the message upto the specified mark, and return
**	the string.
**
** Inputs:
**	msg		Pointer to message structure
**	mark		delimiter character
**
** Outputs:
**	str		Pointer to return string
** 	Returns:
**		OK/FAIL
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	11-jun-90 (nasser)
**		Initial definition.
*/
STATUS
IIMWmgsMsgGetStr(msg, str, mark)
TPMsg	 msg;
char	*str;
char	 mark;
{
	char	*pos;

	if (msg->msgIndex > msg->msgLength)
		return(FAIL);
	if (mark == NULLCHAR)
	{
		pos = &msg->msgData[msg->msgLength];
	}
	else
	{
		pos = STindex(&msg->msgData[msg->msgIndex], &mark, 0);
		if (pos == NULL)
		{
			return(FAIL);
		}
	}
	*pos = NULLCHAR;
	STcopy(&msg->msgData[msg->msgIndex], str);
	*pos = mark;
	msg->msgIndex = pos + 1 - msg->msgData;
	return(OK);
}

/*{
** Name:  get_msg -- get a message from the Mac.
**
** Description:
**	This function reads a message from the Mac. It checks the
**	message to verify that the checksum and sequence number
**	are correct.
**
** Inputs:
**	None.
**
** Outputs:
**	msg		message received.
** 	Returns:
**		length of the message.
**		-1 if error occurred.
** 	Exceptions:
**
** Side Effects:
**
** History:
**	28-aug-90 (nasser) -- Initial definition.
**	26-jun-97 (kitch01) - Bug 79761/83247. On NT allow us to receive ctrl-b
**			   as the mwSTOP_CHAR instead of ctrl-a. This is because some
**				if not most, telnet servers for NT use ctrl-a in a 
**				special way. Sending ctrl-a seems to be ok.
*/
static i4
get_msg(msg)
char	*msg;
{
	i4	 not_end = TRUE;
	i4	 rcv_len = 0;
	char	 rcv_char;
	char	 buf[mwPACKET_SZ + 2 + 1];
	char	 seq_no;

	/* wait for the start character */
	while (TRUE)
	{
		rcv_char = TEget(0);
		if (rcv_char == EOF)
			return(-1);
		if (rcv_char == mwSTART_CHAR)
			break;
	}

	/* read until end character received or message is too long */
	while (not_end && (rcv_len <= (mwPACKET_SZ + 2)))
	{
		rcv_char = TEget(0);
		if (rcv_char == EOF)
			return(-1);
		if ((rcv_char == mwSTOP_CHAR) || (rcv_char == mwSTART_CHAR))
			not_end = FALSE;
		else
			buf[rcv_len++] = rcv_char;
	}

	/* msg. too long; must be error that caused us to miss end char. */
	if (rcv_len == (mwPACKET_SZ + 2 + 1))
	{
		buf[mwPACKET_SZ + 2] = '\0';
		IIMWpelPrintErrLog(mwERR_GENERIC, "Stop char. missing in msg");
		return(-1);
	}

	buf[rcv_len] = '\0';
	IIMWplPrintLog(mwMSG_ALL_RCVD, buf);

	/* msg. must have at least 2 chars; seq. no. and checksum */
	if (rcv_len < 2)
	{
		IIMWpelPrintErrLog(mwERR_GENERIC, "Msg too short");
		return(-1);
	}

	/* verify the checksum */
	rcv_len--;
	if (do_chksum(buf, (u_i4) rcv_len) != buf[rcv_len])
	{
		IIMWpelPrintErrLog(mwERR_GENERIC, "Checksum error in msg");
		return(-1);
	}

	/* check the seq. no. */
	rcv_len--;
	seq_no = buf[rcv_len];
	if (seq_no == mwPACKET_SEQ_FORCE)
	{
		rcv_seq_no = mwPACKET_SEQ_LO;
		snd_seq_no = mwPACKET_SEQ_LO;
	}
	else
	{
		if (seq_no != rcv_seq_no)
		{
			IIMWpelPrintErrLog(mwERR_GENERIC,
				"Sequence No. error in msg");
			return(-1);
		}
		if (rcv_seq_no == mwPACKET_SEQ_HI)
			rcv_seq_no = mwPACKET_SEQ_LO;
		else
			rcv_seq_no++;
	}
	buf[rcv_len] = EOS;
	STcopy(buf, msg);
	return(rcv_len);
}

/*{
** Name:  snd_msg -- Send a message to the Mac.
**
** Description:
**	This function takes a buffer, adds a sequence number and
**	checksum, frames the whole message and sends it to the
**	Mac.
**
** Inputs:
**	msg		The message to be sent.
**	len		Length of the message.
**	seq_frc		Indicates if force value should be used for sequence.
**
** Outputs:
** 	Returns:
**		STATUS.
** 	Exceptions:
**
** Side Effects:
**
** History:
**	28-aug-90 (nasser)
**		Initial definition.
*/
static STATUS
snd_msg(msg, len, seq_frc)
char	*msg;
u_i4	 len;
bool	 seq_frc;
{
	char	buf[mwPACKET_SZ + sizeof(start_str) + sizeof(end_str) + 3];
	u_i4	buflen;

	if (seq_frc)
		snd_seq_no = mwPACKET_SEQ_FORCE;

	/* Construct the message with framing, sequence and checksum */
	STlcopy(start_str, buf, sizeof(start_str));
	_VOID_ STprintf(&buf[sizeof(start_str)], "%s%c", msg, snd_seq_no);
	buflen = sizeof(start_str) + len + 1;
	buf[buflen] = do_chksum(&buf[sizeof(start_str)], len + 1);
	buflen++;
	buf[buflen] = EOS;

	IIMWplPrintLog(mwMSG_ALL_SENT, &buf[sizeof(start_str)]);

	STlcopy(end_str, &buf[buflen], sizeof(end_str));
	buflen += sizeof(end_str);

	/*
	** Adjust the value of the sequence counter.
	** If the sequence number is being forced, adjust both
	** the send and receive counters.
	*/
	if (snd_seq_no == mwPACKET_SEQ_FORCE)
	{
		snd_seq_no = mwPACKET_SEQ_LO;
		rcv_seq_no = mwPACKET_SEQ_LO;
	}
	else if (snd_seq_no == mwPACKET_SEQ_HI)
	{
		snd_seq_no = mwPACKET_SEQ_LO;
	}
	else
	{
		snd_seq_no++;
	}

	TEcmdwrite(buf, (i4) buflen);
	return(OK);
}

/*{
** Name:  do_chksum -- Calculate the check sum of characters in buffer.
**
** Description:
**	This function calculates the checksum of the characters in a
**	message.
**
** Inputs:
**	buf	buffer containing the message.
**	len	length of the message.
**
** Outputs:
** 	Returns:
**		the checksum
** 	Exceptions:
**
** Side Effects:
**
** History:
**	28-aug-90 (nasser) -- Initial definition.
*/
static char
do_chksum(buf, len)
char	*buf;
u_i4	 len;
{
	char	sum;
	u_i4	i;

	sum = buf[0];
	for (i = 1; i < len; i++)
		sum += buf[i];
	return(((sum >> mwPKTSUM_RSHIFT) & mwPKTSUM_MASK) + mwPKTSUM_ADD);
}
# endif /* DATAVIEW */
