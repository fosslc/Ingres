/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project : CA/OpenIngres Visual DBA
//
//    Source : dbatime.h
//    time management for background refresh
//
********************************************************************/

#ifndef DBATIME_H
#define DBATIME_H

#include <time.h>

#define FREQ_UNIT_SECONDS 1
#define FREQ_UNIT_MINUTES 2
#define FREQ_UNIT_HOURS   3
#define FREQ_UNIT_DAYS    4
#define FREQ_UNIT_WEEKS   5
#define FREQ_UNIT_MONTHS  6
#define FREQ_UNIT_YEARS   7

extern long DOMUpdver;

typedef struct dbafrequency {
   int  count;
   int  unit;
   BOOL bSyncOnParent;
   BOOL bSync4SameType;
   BOOL bOnLoad;
} FREQUENCY, FAR * LPFREQUENCY;

typedef struct DomRefreshParams {
   time_t LastRefreshTime;
   long   LastDOMUpdver;
   BOOL   bUseLocal;
   int    iobjecttype;
   FREQUENCY frequency;
} DOMREFRESHPARAMS, FAR * LPDOMREFRESHPARAMS;

BOOL DOMIsTimeElapsed(time_t newtime, LPDOMREFRESHPARAMS lprefreshparams, BOOL bParentRefreshed);
BOOL UpdRefreshParams(LPDOMREFRESHPARAMS lpRefreshParams, int iobjecttype);


#endif  //DBATIME_H
