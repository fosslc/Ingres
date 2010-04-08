/*
** Copyright (c) 1998 Ingres Corporation
*/

#ifndef  _PFMSG_H_
# define  _PFMSG_H_
/*{
** Name: 
**	pfmsg.h - Performance DLL message macros and definitions
**
** Description:
**	This file provides the macros and definitions used by the extensible
**	counters for reporting events to the event logging facility.
**
** History:
** 	15-oct-1998 (wonst02)
** 	    Created.
** 
*/

/*
** Report error message ID's for Counters
*/

# define APP_NAME  "oipfctrs"

/*
** The constant below defines how many (if any) messages will be reported
** to the event logger. As the number goes up in value more and more events
** will be reported. The purpose of this is to allow lots of messages during
** development and debugging (e.g. a message level of 3) to a minimum of
** messages (e.g. operational messages with a level of 1) or no messages if
**
**  Levels:  LOG_NONE = No event log messages ever
**           LOG_USER = User event log messages (e.g. errors)
**           LOG_DEBUG = Minimum Debugging 
**           LOG_VERBOSE = Maximum Debugging 
*/

# define  LOG_NONE     0
# define  LOG_USER     1
# define  LOG_DEBUG    2
# define  LOG_VERBOSE  3

# define  MESSAGE_LEVEL_DEFAULT  LOG_USER

/* define macros
**
** Format for event log calls without corresponding insertion strings is:
**    REPORT_xxx (message_value, message_level)
**       where:   
**          xxx is the severity to be displayed in the event log
**          message_value is the numeric ID from above
**
** if the message has a corresponding insertion string whose symbol conforms
** to the format CONSTANT = numeric value and CONSTANT_S = string constant for
** that message, then the 
** 
**    REPORT_xxx_STRING (message_value, message_level)
**
** macro may be used.
*/

/*
** REPORT_SUCCESS was intended to show Success in the error log, rather it
** shows "N/A" so for now it's the same as information, though it could 
** (should) be changed  in the future
**
*/

# define REPORT_SUCCESS(i,l) (MESSAGE_LEVEL >= l ? ReportEvent (hEventLog, EVENTLOG_INFORMATION_TYPE, \
   0, i, (PSID)NULL, 0, 0, NULL, (PVOID)NULL) : FALSE)

# define REPORT_INFORMATION(i,l) (MESSAGE_LEVEL >= l ? ReportEvent (hEventLog, EVENTLOG_INFORMATION_TYPE, \
   0, i, (PSID)NULL, 0, 0, NULL, (PVOID)NULL) : FALSE)

# define REPORT_WARNING(i,l) (MESSAGE_LEVEL >= l ? ReportEvent (hEventLog, EVENTLOG_WARNING_TYPE, \
   0, i, (PSID)NULL, 0, 0, NULL, (PVOID)NULL) : FALSE)

# define REPORT_ERROR(i,l) (MESSAGE_LEVEL >= l ? ReportEvent (hEventLog, EVENTLOG_ERROR_TYPE, \
   0, i, (PSID)NULL, 0, 0, NULL, (PVOID)NULL) : FALSE)

# define REPORT_INFORMATION_DATA(i,l,d,s) (MESSAGE_LEVEL >= l ? ReportEvent (hEventLog, EVENTLOG_INFORMATION_TYPE, \
   0, i, (PSID)NULL, 0, s, NULL, (PVOID)(d)) : FALSE)

# define REPORT_WARNING_DATA(i,l,d,s) (MESSAGE_LEVEL >= l ? ReportEvent (hEventLog, EVENTLOG_WARNING_TYPE, \
   0, i, (PSID)NULL, 0, s, NULL, (PVOID)(d)) : FALSE)

# define REPORT_ERROR_DATA(i,l,d,s) (MESSAGE_LEVEL >= l ? ReportEvent (hEventLog, EVENTLOG_ERROR_TYPE, \
   0, i, (PSID)NULL, 0, s, NULL, (PVOID)(d)) : FALSE)

# define REPORT_ERROR_DATA_W(i,l,d,s, WS) \
	(MESSAGE_LEVEL >= l ? ReportEventW (hEventLog, EVENTLOG_ERROR_TYPE, \
   	0, i, (PSID)NULL, 1, s, WS, (PVOID)(d)) : FALSE)

# define REPORT_ERROR_DATA_A(i,l,d,s, AS) \
	(MESSAGE_LEVEL >= l ? ReportEventA (hEventLog, EVENTLOG_ERROR_TYPE, \
   	0, i, (PSID)NULL, 1, s, AS, (PVOID)(d)) : FALSE)

/* External Variables */

extern HANDLE hEventLog;   /* handle to event log */
extern DWORD  dwLogUsers;  /* counter of event log using routines */
extern DWORD  MESSAGE_LEVEL; /* event logging detail level */

#endif /*_PFMSG_H_ */
