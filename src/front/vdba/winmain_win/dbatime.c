/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project : Ingres Visual DBA
**
**    Source : dbatime.c
**    Time management for background refresh
**
**    Author : Francois Noirot-Nerin
**
**   History after 01-01-2000
**    
**   29-Feb-2000 (noifr01)
**     (bug 100637) detection of the leap year for year 2000 was wrong
**     because although the double exception for a multiple of 100 and
**     a multiple of 400 was coded, the year was 1900 based instead of
**     0 based ...
********************************************************************/

#include "dba.h"
#include "dbatime.h"
#include "settings.h"
#include "error.h"

long DOMUpdver;

static int daycount(int month, int year)
{
   int year0 = year+1900;
   while (month>11) {
      month-=12;
      year++;
   }

   switch (month) {
      case 0 : // January
      case 2 : // March   
      case 4 : // Mai    
      case 6 : // July   
      case 7 : // August 
      case 9 : // October                  
      case 11: // December 
         return 31;
         break;
      case 3 : // April
      case 5 : // June
      case 8 : // September
      case 10: // November
         return 30;
         break;
      case 1 : // February
         if (year0 % 4)
            return 28; // non multiple of 4
         if (year0 % 100)
            return 29; // multiple of 4 but not of 100
         if (year0 % 400)
           return 28;  // multiple of 100 but not of 400
         return 29;    // multiple of 400
         break;
      default:
         myerror(ERR_TIME);
         return 0;
         break;
   }
}

static void addrefreshparms2tm(struct tm * ptmloc,
                               LPDOMREFRESHPARAMS prefreshparams)
{
   int count;
   LPFREQUENCY lpfreq = & (prefreshparams->frequency);
   if (!prefreshparams->bUseLocal)
      lpfreq=GetSettingsFrequencyParams(prefreshparams->iobjecttype);
   count=lpfreq->count;
   switch (lpfreq->unit) {
      case FREQ_UNIT_SECONDS :
         ptmloc->tm_sec+=count;
         break;
      case FREQ_UNIT_MINUTES :
         ptmloc->tm_min+=count;
         break;
      case FREQ_UNIT_HOURS   :
         ptmloc->tm_hour+=count;
         break;
      case FREQ_UNIT_DAYS    :
         ptmloc->tm_mday+=count;
         break;
      case FREQ_UNIT_WEEKS   :
         ptmloc->tm_mday+=(7*count);
         break;
      case FREQ_UNIT_MONTHS  :
         ptmloc->tm_mon+=count;
         break;
      case FREQ_UNIT_YEARS   :
         ptmloc->tm_year+=count;
         break;
      default:
         myerror(ERR_TIME);
         break;
   }

   while (ptmloc->tm_sec>59) {
      ptmloc->tm_sec-=60;
      ptmloc->tm_min++;
   }
   while (ptmloc->tm_min>59) {
      ptmloc->tm_min-=60;
      ptmloc->tm_hour++;
   }
   while (ptmloc->tm_hour>23) {
      ptmloc->tm_hour-=24;
      ptmloc->tm_mday++;
   }
   while (ptmloc->tm_mday>daycount(ptmloc->tm_mon,ptmloc->tm_year)){
      ptmloc->tm_mday -=(daycount(ptmloc->tm_mon,ptmloc->tm_year));
      ptmloc->tm_mon  ++;
      while (ptmloc->tm_mon>11) {
         ptmloc->tm_mon-=12;
         ptmloc->tm_year++;
      }
   }
   while (ptmloc->tm_mon>11) {
      ptmloc->tm_mon-=12;
      ptmloc->tm_year++;
   }
}

BOOL DOMIsTimeElapsed(time_t newtime, LPDOMREFRESHPARAMS lprefreshparams, BOOL bParentRefreshed)
{
   struct tm tmloc;
   time_t timtloc;
   LPFREQUENCY lpfreq = & (lprefreshparams->frequency);
   if (!lprefreshparams->bUseLocal)
      lpfreq=GetSettingsFrequencyParams(lprefreshparams->iobjecttype);
   if (bParentRefreshed && lpfreq->bSyncOnParent)
      return TRUE;

   memcpy(&tmloc,
          localtime(&(lprefreshparams->LastRefreshTime)),
          sizeof(struct tm));
   addrefreshparms2tm(&tmloc,lprefreshparams);
   timtloc=mktime(&tmloc);
   if (newtime>timtloc-1) // -1 to minimize the average error due to the timer (3.333 seconds)
      return TRUE;        // this allows the average error to be +.66 seconds, and the minimum
   else                   // never "more than 1 second in advance" (normally less due to the
      return FALSE;       // delay of the refresh itself), and the max error + 2.33 (+ refresh time)
						  // instead of previous values (timer was 4 secs): avg: +2. Min: 0 Max +4 (+ refresh time)
						  // the timer is not decreased because the user needs the time to access the
						  // menu and disable background refresh if he wishes
}

BOOL UpdRefreshParams(LPDOMREFRESHPARAMS lpRefreshParams, int iobjecttype)
{
   lpRefreshParams->LastRefreshTime=ESL_time();
   lpRefreshParams->iobjecttype   = iobjecttype;
   lpRefreshParams->LastDOMUpdver = DOMUpdver;
   return TRUE;
}
