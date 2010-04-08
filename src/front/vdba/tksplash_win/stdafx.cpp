/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : stdafx.h : include file for standard system include files
**    Project  : Extension DLL (Task Animation).
**    Author   : Sotheavut UK (uk$so01)
**
**    Purpose  : Precompile Header
**               ource file that includes just the standard includes
**               tksplash.pch will be the pre-compiled header
**               stdafx.obj will contain the pre-compiled type information.
**
**  History:
**
**  06-Nov-2001 (uk$so01)
**     created
**  05-Oct-2004 (uk$so01)
**     BUG #113185
**     Manage the Prompt for password for the Ingres DBMS user that 
**     has been created with the password when connecting to the DBMS.
*/

#include "stdafx.h"
#include "rctools.h"
#include "sessimgr.h"
#include "xdpasswd.h"

//
// Manage the Prompt for password for the Ingres DBMS user that created with
// password.
// I personally prefer to manage these codes in a seperate new DLL called 
// IIDBPROM.DLL that is isolated and can be used by any application that required this
// functionality.
extern "C" int DoPrompt4Paword( char *mesg, int  noecho, char *reply, int  replen)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	TCHAR tchszBuff[256];

	if (Prompt4Password (tchszBuff, 256))
		_tcscpy(reply, tchszBuff);
	else
		reply[0] = '\0';
	return 0;
}

extern "C" void PASCAL EXPORT INGRESII_llSetPrompt()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	INGRESII_llInitPasswordPromptHandler((PfnPasswordPrompt)DoPrompt4Paword);
}

