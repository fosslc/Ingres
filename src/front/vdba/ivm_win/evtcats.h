/***************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : evtcats.h
**
**
**    Project  : Ingres II / Visual Manager
**    Author   : noifr01
**
**    Purpose  : predefined categories and misc associated functions
**
**  History:
**  14-Fev-2001 (noifr01)
**     (bug 104001) added the #define for the new "Misc Messages" Category
**  20-Nov-2002 (noifr01)
**    (bug 109143) management of gateway messages
**  04-Mar-2003 (noifr01)
**    (sir 109773) manage also front-end messages (for parsing correctly the
**    messages.txt file, and allowing future display of the explanation of
**    front-end messages)
****************************************************************************/

#if !defined(EVTCATS_HEADER)
#define EVTCATS_HEADER

typedef struct
{
	BOOL	bNotBackend;
	char	*classprefix;
	long    lclassnum;
	char	*subclassprefix;
	long	lclasssubmask;
	long	lmaxnum;
	BOOL	bIsClass4Interface;
	long	lInterfaceClass;
	char	*classdescription;   
} INGERRCLASS;

#define OFFSET_CAT_MASK 0x10000

#define CLASS_SAME				 0xFFF00000
#define CLASS_OTHERS			 0xFFF10000
#define MSG_NOCODE				(CLASS_OTHERS + 1L)
#define CLASS_NOT_ERROR_MESSAGE	 0xFFF20000
#define CLASS_INTERFACE_MISC	 0xFFF30000
#define INVALID_MESSAGE_CODE	 CLASS_NOT_ERROR_MESSAGE

/* values for gateway start/stop messages that are not defined like ingres */
/* messages (defining (dummy) hardcoded values in the GWF facility )       */

#define CLASS_GW_STARTSTOP		(15L * OFFSET_CAT_MASK ) /* use GWF category */

#define GW_ORA_START					(CLASS_GW_STARTSTOP + 0xF801)
#define GW_ORA_START_CONNECTLIMIT		(CLASS_GW_STARTSTOP + 0xF802)
#define GW_MSSQL_START					(CLASS_GW_STARTSTOP + 0xF803)
#define GW_MSSQL_START_CONNECTLIMIT		(CLASS_GW_STARTSTOP + 0xF804)
#define GW_INF_START					(CLASS_GW_STARTSTOP + 0xF805)
#define GW_INF_START_CONNECTLIMIT		(CLASS_GW_STARTSTOP + 0xF806)
#define GW_SYB_START					(CLASS_GW_STARTSTOP + 0xF807)
#define GW_SYB_START_CONNECTLIMIT		(CLASS_GW_STARTSTOP + 0xF808)
#define GW_DB2UDB_START					(CLASS_GW_STARTSTOP + 0xF809)
#define GW_DB2UDB_START_CONNECTLIMIT	(CLASS_GW_STARTSTOP + 0xF80A)

#define GW_ORA_STOP						(CLASS_GW_STARTSTOP + 0xF820)
#define GW_MSSQL_STOP					(CLASS_GW_STARTSTOP + 0xF821)
#define GW_INF_STOP						(CLASS_GW_STARTSTOP + 0xF822)
#define GW_SYB_STOP						(CLASS_GW_STARTSTOP + 0xF823)
#define GW_DB2UDB_STOP					(CLASS_GW_STARTSTOP + 0xF824)

#define GW_OTHER_STARTSTOP_MSG			(CLASS_GW_STARTSTOP + 0xF828)
/* end of gateway start/stop messages hardcoded definitions*/

typedef struct
{
	long lMsgClass;
	long lMsgFullID;
} MSGCLASSANDID;

MSGCLASSANDID getMsgStringClassAndId(char * pmsg, char * pbufreturnedsubstring, int ibufsize);

// Get First/Next Class4interface:
// returns TRUE or FALSE (last Class or not
// updates the class ID and the string description passed to the function

BOOL GetFirstIngClass4Interface(long &lClassRef, CString &csDesc);
BOOL GetNextIngClass4Interface(long &lClassRef, CString &csDesc);
BOOL GetFirstIngClass4MessageExplanation(long &lClassRef, CString &csDesc);
BOOL GetNextIngClass4MessageExplanation(long &lClassRef, CString &csDesc);


BOOL GetFirstIngCatMessage(long lcatID, long &lmsgID, CString &csDesc);
BOOL GetNextIngCatMessage( long &lmsgID, CString &csDesc);

#endif // define EVTCATS_HEADER
