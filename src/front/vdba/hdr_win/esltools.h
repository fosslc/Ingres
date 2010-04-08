/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project : CA/OpenIngres Visual DBA
//
//    Source : esltools.h
//    environment-specific layer tools
//
**  26-Mar-2001 (noifr01)
**   (sir 104270) removal of code for managing Ingres/Desktop
**  23-Nov-2001 (noifr01)
**   added #if[n]def's for the libmon project
*******************************************************************/

#include <time.h>


#ifdef DEBUGMALLOC
int ESL_AllocCount (void);
#endif

LPVOID ESL_AllocMem(UINT uisize); /* locking no more needed. fills allocated
                                     memory area with blanks */
void   ESL_FreeMem (LPVOID lpArea);
LPVOID ESL_ReAllocMem(LPVOID lpArea, UINT uinewsize, UINT uioldsize);
    /* if uioldsize == 0, not used. Otherwise, if size is
       increased, the additional section is filled with zeroes */

time_t ESL_time(void);

BOOL AskIfKeepOldState(int iobjecttype,int errtype,LPUCHAR * parentstrings);

#ifndef LIBMON_HEADER
#include "rmcmd.h"    // remote commands : LPRMCMDPARAMS structure definition
DISPWND ESL_OpenRefreshBox    (void);
BOOL    ESL_SetRefreshBoxTitle(DISPWND dispwnd,LPUCHAR lpstring);
BOOL    ESL_SetRefreshBoxText (DISPWND dispwnd,LPUCHAR lpstring);
BOOL    ESL_CloseRefreshBox   (DISPWND dispwnd);
int SetDispErr(BOOL);

int ESL_RemoteCmdBox(LPRMCMDPARAMS lpRmCmdParams); // dialog box when remote command executing

BOOL ESL_IsRefreshStatusBarOpened (void);
DISPWND ESL_OpenRefreshStatusBar(void);
BOOL ESL_SetRefreshStatusBarText(DISPWND dispwnd, LPUCHAR lpstring);
BOOL ESL_CloseRefreshStatusBar (DISPWND dispwnd);
LPUCHAR inifilename(void);

void ActivateBkTask(void);
void StopBkTask(void);
BOOL IsBkTaskInProgress(void);

void StartSqlCriticalSection(void);
void StopSqlCriticalSection(void);
BOOL InSqlCriticalSection(void);

void StartCriticalConnectSection(void);
void StopCriticalConnectSection(void);
BOOL InCriticalConnectSection(void);

LPUCHAR ObjectTypeString(int iobjecttype,BOOL bMultiple);

#endif


int ESL_GetTempFileName(LPUCHAR buffer, int buflen);
int ESL_FillNewFileFromBuffer(LPUCHAR filename,LPUCHAR buffer, int buflen, BOOL bAdd0D0A);
int ESL_FillBufferFromFile   (LPUCHAR filename,LPUCHAR buffer, int buflen, int * pretlen, BOOL bAppend0);
int ESL_RemoveFile(LPUCHAR filename);
