/*
** Copyright (c) 2004 Ingres Corporation
*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "imp.h"

/*
** Name:	impcommon.sc
**
** This file contains the definitions of the structures to be used with the 
** IMA based IPM lookalike tool.
** 
** The definitions are held in this file so that they may be included in all
** source files to prevent confusing the esqlc preprocessor.
**
## History:
##
## 26-jan-94	(johna)
##		Started on RIF day from earlier ad-hoc queries and programs
## 01-feb-94	(johna)
##		added the menu definitions
## 04-may-94	(johna)
##		some form of doc before the handover
## 02-Jul-2002 (fanra01)
##      Sir 108159
##      Added to distribution in the sig directory.
## 03-Sep-03 	(flomi02) mdf040903
##			added lastquery information
## 06-Apr-04    (srisu02) 
##      Sir 108159
##      Integrating changes made by flomi02  
*/

/*
** GCN Server registry. - used in the server submenu to contain details
** of a server registered with the gcn. The original intention was to use
** the pid of the server as a unique id. This turns out to be a 'bad thing'
** as the Pid is a cl object and hence different for VMS and Unix
*/

exec sql begin declare section;


struct _gcnEntry {
	char	nameServer[SERVER_NAME_LENGTH];
	char	serverAddress[SERVER_NAME_LENGTH];
	char	serverClass[SHORT_STRING];
	int	serverFlags;
	char	serverObjects[SHORT_STRING];
	int	maxActive;
	int	maxSessions;
	int	numActive;
	int	numSessions;
	long	serverPid;
	struct _gcnEntry	*p;
	struct _gcnEntry	*n;
};

typedef struct _gcnEntry GcnEntry;

/*
** Session Entry - used to hold information about a thread in the server.
** some stuff such as the association id is not particularly useful.
*/
struct _sessEntry {
	char	serverAddress[SERVER_NAME_LENGTH];
	char	sessionId[SESSION_ID_LENGTH];
	char	state[20];
	char	wait_reason[20];
	char	mask[20];
	int	priority;
	char	thread_type[5];
	int	timeout;
	int	mode;
	int 	uic;
	int	cputime;
	int 	dio;
	int	bio;
	int 	locks;
	char	username[30];
	int	assoc_id;
	char	euser[20];
	char	ruser[20];
	char	database[30];
	char	dblock[10];
	int	facility_id;
	char	facility[4];
	char	activity[80];
	char	act_detail[80];
/*
## mdf030903
*/
	char	lastquery[1024];   
	char	query[1024];
	char	dbowner[10];
	char	terminal[10];
	char	s_name[30];
	char	groupid[10];
	char	role[10];
	int	appcode;
	int	scb_pid;
	struct 	_sessEntry	*p;
	struct 	_sessEntry	*n;
};

typedef struct _sessEntry SessEntry;

/*
** Menu Entry
**
** used in all of the menu screens to hold the relevant options.
*/

struct _menuEntry {
	int	item_number;
	char	short_name[16];
	char	long_name[60];
};

typedef struct _menuEntry MenuEntry;

exec sql end declare section;

