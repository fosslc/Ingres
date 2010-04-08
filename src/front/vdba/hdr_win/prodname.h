/**************************************************************************
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**   Source   : prodname.h
**
**   Project : Ingres Visual DBA
**   Author  : Francois Noirot-Nerin
**
**   functions returning Captions, or executable file names
**
**  History after 01-12-2000
**  20-Dec-2000 (noifr01)
**   (SIR 103548) removed functions returning DBMS and gateways executable 
**   file names, since they are no more needed because the criteria for 
**   determining whether the DBMS or gateways are started has changed
**  22-Dec-2000 (noifr01)
**   (SIR 103548) new prototypes related to the fact that we now append
**   the installation ID in the application title.
***************************************************************************/
//
// prodname.h : interface for product rename purposes
//

#ifndef _PRODNAME_INCLUDED
#define _PRODNAME_INCLUDED

//
// All functions are "extern "C" so they can be used on the C side
//
#ifdef __cplusplus
extern "C"
{
#endif

char * VDBA_GetInstallPathVar();

char * VDBA_GetInstallName4Messages();
char * VDBA_GetIntermPathString();

BOOL InitInstallIDInformation(); /* initialize the install ID info */
char *GetInstallationID();       /* returns the installation ID */

void VDBA_GetProductCaption(char* buf, int bufsize);
void VDBA_GetBaseProductCaption(char* buf, int bufsize);
void VDBA_GetErrorCaption(char* buf, int bufsize);

char * VDBA_GetVCBF_ExeName();
char * VDBA_GetWinstart_CmdName();

char * VDBA_GetHelpFileName();

UINT VDBA_GetSplashScreen_Bitmap_id();

char * VDBA_ErrMsgIISystemNotSet();

#ifdef __cplusplus
}
#endif


#endif  // _PRODNAME_INCLUDED
