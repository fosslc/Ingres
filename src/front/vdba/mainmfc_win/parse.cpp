/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**   Project : Ingres II / Visual DBA
**
**   Source : parse.cpp
**   misc parsing functionalities (trace output for QEP's, ingres 
**   pages, output of some rmcmd commands)
**
**   Author : Francois Noirot-Nerin
**
**   History after 01-01-2000
**    20-01-2000 (noifr01)
**     (bug 100088) fixed an assignment error, in the parsing for
**     getting table pages information, when memory limitations
**     result in the minimum "pages per cell" value to be bigger
**     than 1 (some grouping being done in this case directly in 
**     the parsing)
**    21-01-2000 (schph01)
**     (bug 100107) spatial and user defined data types are displayed as
**     "char" at some places in VDBA (right pane "statistics", and
**     "create index" dialog box).
**  24-Feb-2000 (uk$so01)
**     Bug #100562, Add "nKeySequence" to the COLUMNPARAMS to indicate if the columns
**     is part of primary key or unique key.
**  04-Apr-2000 (noifr01)
**     (bug 101430) manage the new carriage return information available 
**     through rmcmd
**  05-Jun-2000 (noifr01)
**     (bug 101620) increased value of MAXLINESINNODE (from 8 to 16), and added
**     display of a system error if this value is reached
**  06-Jun-2000 (noifr01)
**     (bug 99242) clean-up for DBCS compliance
**  06-Jun-2000 (uk$so01)
**     bug 99242 (DBCS) Remove cast (LPUCHAR) when calling  RegisterSQLStmt()
**  08-Dec-2000 (schph01)
**     Sir 102823 When columns type is Object Key or Table Key, add System
**     Maintained information.
**  17-Apr-2001 (noifr01)
**     (sir 102955) added SQLGetPageTypesPerTableTypes() function for detecting
**     the page type that will be used when creating a table or index
** 02-May-2001 (uk$so01)
**    SIR #102678
**    Support the composite histogram of base table and secondary index.
** 10-May-2001 (noifr01)
**    (bug 104687) changed the logic for parsing a specific string when getting
**    Ingres hash tables pages information (the previous criteria worked only
**    against Ingres II 2.5) 
** 10-May-2001 (noifr01)
**    (bug 104690) take into account the fact that the string 
**    "Longest overflow chain is <value>" is not always followed by "in bucket"
** 02-Oct-2001 (noifr01)
**    (bug 105933) the number of rows, in a QEP, could be truncated for "big"
**    tables because of the assumption of node widths always being 24
** 08-Oct-2001 (schph01)
**    (SIR 105881) filled m_nNumColumnKey (CaTableStatistic class) with the
**    number of columns in key column.
** 18-Feb-2002 (uk$so01)
**    SIR #106648, Integrate SQL Test ActiveX Control
** 17-Jun_2002 (noifr01)
**    (bug 108059) the parsing of the statdump output must take into account
**    a comma (,) as a possible decimal separator (set through II_DECIMAL),
**    and work independently of the locale
**  22-Oct-2003 (schph01)
**  (SIR 111153) add function GetRmcmdObjects_launcher() to manage new rmcmd
**   command 'ddprocessuser'
**  02-feb-2004 (somsa01)
**   Updated to "typecast" UCHAR into CString before concatenation.
**  28-apr-2009 (smeke01) b121545
**   Enable display of ISAM index levels 1 to 9. Enable code to cope with any 
**   unrecognised pagetypes that may happen in future, by displaying a '?'  
**   symbol. Improved error message handling so that an explanatory text  
**   message is displayed instead of "System Error 34".
*/

#include "stdafx.h"
#include <locale.h>
#include "rcdepend.h"

#include "mainfrm.h"
#include "childfrm.h"
#include "maindoc.h"
#include "mainview.h"

#include "parse.h"
#include "sqlselec.h"

extern "C"
{
#include "dba.h"        // necesssary before esltools
#include "esltools.h"   // critical section
#include "dll.h"
#include "msghandl.h"
#include "dbaginfo.h"
int IILQsthSvrTraceHdlr(VOID (*rtn)(void * ptr,char * buf,int i),void *cb, bool active, bool no_dbterm);

#include "resource.h"   // hdr directory - IDM_xyz
#include "error.h"
#include "domdata.h"
};

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


extern CString mystring;
static BOOL bTraceResult = TRUE;
static BOOL bTraceLinesInit = FALSE;

static void ReleaseTraceLines()
{
   if (!bTraceLinesInit)
      return;
   mystring.ReleaseBuffer();
   mystring="";
   bTraceLinesInit=FALSE;
   return;
}

extern "C"
{
// no DBCS there, the parsed strings are hard coded in English in back!opf!opt!optcotree.c
// the few following functions can be rewritten using a string array or list
static char TRACELINESEP = '\n';
VOID mycallback(void * p1, char * outbuf,int i)
{
   mystring +=outbuf;
   return;
}
} // extern "C"

BOOL StartSQLTrace()
{
   if (bTraceLinesInit)
      ReleaseTraceLines();
   mystring="";
   bTraceResult = TRUE;
   IILQsthSvrTraceHdlr(mycallback,(void *)NULL, TRUE, FALSE);
   return TRUE;
}
BOOL StopSQLTrace()
{
   IILQsthSvrTraceHdlr(mycallback,(void *)NULL, FALSE, FALSE);
   return TRUE;
}


static char * TLCurPtr;
char *GetNextTraceLine()
{
   char * plastchar;
   if (!TLCurPtr) {
      ReleaseTraceLines();
      return (char *)NULL;
   }
   char * presult = TLCurPtr;
   char * pc = x_strchr(TLCurPtr,TRACELINESEP);
   if (pc) {
      *pc='\0';
      TLCurPtr=pc+1;
      if (TLCurPtr[0]=='\0')
         TLCurPtr=NULL;

   }
   else 
      TLCurPtr=NULL;

   while ( (plastchar = x_strgetlastcharptr(presult))!=NULL ) {
	  char c = *plastchar;
      if (c=='\n' || c=='\r')
         *plastchar ='\0';
      else
         break;
   }
   
   suppspace((LPUCHAR)presult);
   return presult;
}
char *GetNextSignificantTraceLine()
{
   char * presult;
   while ((presult=GetNextTraceLine())!=(char *) NULL) {
      if (*presult!='\0' && x_strncmp(presult,"----",4))
         return presult;
   }
   return (char *)NULL;
}

char *GetFirstTraceLine()
{
   TLCurPtr=mystring.GetBuffer(0);
   if (TLCurPtr[0]=='\0')
      TLCurPtr=NULL;
   bTraceLinesInit=TRUE;
   return GetNextTraceLine();
}

#include "cellintc.h"

static char *szPage        ="PAGE ";
static char *szTotPages    =", Total pages:";
static char *szMore4Hash   = "Display structure for HASH table:";
static char *szMore4HashNew_begin = "Display structure for II";
static char *szMore4HashNew_end   = " HASH table:";
// parsing the new , hash table specific strings...
// There are 2048 buckets, 1 bucket(s) with overflow chains.
// There are 2048 empty hash bucket(s), 1 empty hash bucket(s) with overflow chains.
// There are 912 overflow pages of which 910 are empty.
// Longest overflow chain is 910 in bucket 1233
// Avg. length of overflow chain is : 910.000
static char *szthereare    = "There are ";
static char *szbuckets     = " buckets, ";
static char *szbuckwithovf = " bucket(s) with overflow chains.";
static char *szemptybuck   = " empty hash bucket(s), ";
static char *szemptywithovf= " empty hash bucket(s) with overflow chains.";
static char *szoverflow    = " overflow pages of which ";
static char *szareempty    = " are empty.";
static char *szlongestovfch= "Longest overflow chain is ";
static char *szinbucket    = " in bucket ";
static char *szavgovfchain = "Avg. length of overflow chain is : ";
static char *szDummy1      = "D - Non-empty hash buckets.";
static char *szDummy2      = "d - Empty hash buckets.";
static char *szDummy3      = "o - Non-empty overflow pages.";
static char *szDummy4      = "e - Empty overflow pages.";

static  long lpages0 ,lpages,lused,lfreed,lunused,loverflow,lgroupfactor;
static  long lposinblock,lusedinblock,loverflowinblock,lnbblocks, lunrecognisedpagetype;
static  BOOL bResult;
static  int mycallbackstate;
static  CByteArray *pBA;
static  BOOL bcallbackregistered = FALSE;
static  long  lovf1 , lovf2;
static  BOOL bHashNew;
static  long lbuckets,lbuck_with_ovf,lemptybuckets,lempty_with_ovf;
static  long  lhashoverflow,lemptyoverflow,llongest_ovf_chain,lbuck_longestovfchain;
static double favg_ovf_chain;
static long lposdatainarray, lposovfinarray, ldatapage, lovfpage;
static char c1; 

extern "C"
{
void mycallbackpages(void * p1, char * outbuf,int i)
{
   MEMORYSTATUS memstatus;
   int iret;
   char Line[1000];
   char buf[200];
   long  l1;
   mystring +=outbuf;
   char * CurLine  = mystring.GetBuffer(0);
   char * pc       = x_strchr(CurLine,TRACELINESEP);
   char *pc1;
   char c;
   
   if (!pc)
      return;
   *pc='\0';
   if (x_strlen(CurLine)>sizeof(Line)-1) {
      bResult=FALSE;
      mystring="";
      return;
   }
   x_strcpy(Line,CurLine);
   CurLine=Line;
   pc=Line;
   mystring="";
   while ( (pc1 = x_strgetlastcharptr(pc))!=NULL ) {
	  char c = *pc1;
      if (c=='\n' || c=='\r')
         *pc1 ='\0';
      else
         break;
   }
   suppspace((LPUCHAR)pc);
   if (pc[0]=='\0')
      return;
   if (!x_strncmp(pc,"----",4))
         return ;


   switch (mycallbackstate) {
      case 0 :
         if (x_strncmp(CurLine,szPage,x_strlen(szPage))) 
            return;

         pc=x_strstr(CurLine,szTotPages);
         if (!pc) {
            bResult=FALSE;
            break;
         }
         lpages0=atol(pc+x_strlen(szTotPages));
         if (lpages0<0L) {     /* header not found */
            myerror(ERR_INGRESPAGES);
            bResult=FALSE;
            return;
         }
         memstatus.dwLength = sizeof(MEMORYSTATUS);
#ifndef MAINWIN
         GlobalMemoryStatus( &memstatus);
         l1= min( (memstatus.dwTotalPhys/5) , (memstatus.dwAvailPhys/3) );
         while (l1<lpages0) {
           l1*=10L;
           lgroupfactor*=10L;
         }
#endif
         if (lgroupfactor>1L) {
            lovf1=1;
            while (lovf1*lovf1*lovf1<lgroupfactor)
               lovf1++;
           lovf2=lovf1 * lovf1;                              // of overflow "stars" displayed
         }
         pBA = new CByteArray();
         pBA->SetSize((lpages0/lgroupfactor) +1);
  
         lposinblock     = 0L;
         lusedinblock    = 0L;
         loverflowinblock= 0L;
		 lunrecognisedpagetype = 0L;
         bHashNew = FALSE;
         mycallbackstate = 1;
         break;
      case 1:
         if (!x_strncmp(CurLine,szMore4Hash,x_strlen(szMore4Hash))) {
            mycallbackstate = 2;
        //    IILQsthSvrTraceHdlr(mycallbackpages,(void *)NULL, FALSE, FALSE);
        //    bcallbackregistered = FALSE;
            break;   // extra info for Hash not displayed in this build
         }
         if (!x_strncmp(CurLine,szMore4HashNew_begin,x_strlen(szMore4HashNew_begin)) &&
              x_strstr(CurLine, szMore4HashNew_end)
            ) {
            mycallbackstate = 3;
			bHashNew = TRUE;
			lbuckets              = -1L;
			lbuck_with_ovf        = -1L;
			lemptybuckets         = -1L;
			lempty_with_ovf       = -1L;
			lhashoverflow         = -1L;
			lemptyoverflow        = -1L;
			llongest_ovf_chain    = -1L;
			lbuck_longestovfchain = -1L;
			favg_ovf_chain        = -1.;
			// positions in array, for updating the array with "empty", "with/without overflow" attributes
			lposdatainarray   = -1L;
			lposovfinarray    = -1L;
			ldatapage         =  0L;
			lovfpage          =  0L;
			break; 
         }

         x_strcpy(buf,CurLine);
         if (x_strlen(buf)<13) {
             myerror(ERR_INGRESPAGES);
             bResult = FALSE;
             return;
         }
         buf[12]='\0';
         iret=sscanf(buf,"%ld",&l1);
         if (iret!=1 /*||l1!=lpages */) {
            myerror(ERR_INGRESPAGES);
            bResult = FALSE;
            return;
         }
         pc=buf+13;
         while ( (c=*pc) !=NULL) {
            if (c!=' ') {
               switch (c) {
                  case 'H': // Free Header Page
                     lusedinblock++;
                     lused++;
                     break;
                  case 'M': // Free Map Page
                     lusedinblock++;
                     lused++;
                     break;
                  case 'F': // Free page, a page that has been used but has subsequently been made free
                     lfreed++;
                     break;
                  case 'r': // Root Page
                     lusedinblock++;
                     lused++;
                     break;
                  case 'i': // Index Page
                  case '1': // Index Page Level 1
                  case '2': // Index Page Level 2
                  case '3': // Index Page Level 3
                  case '4': // Index Page Level 4
                  case '5': // Index Page Level 5
                  case '6': // Index Page Level 6
                  case '7': // Index Page Level 7
                  case '8': // Index Page Level 8
                  case '9': // Index Page Level 9
                     lusedinblock++;
                     lused++;
                     break;
                  case 's': // Sprig Page
                     lusedinblock++;
                     lused++;
                     break;
                  case 'L': // Leaf Page
                     lusedinblock++;
                     lused++;
                     break;
                  case 'O': // Overflow Leaf Page
                     lused++;
                     lusedinblock++;
                     loverflowinblock++;
                     loverflow++;
                     break;
                  case 'o': // Overflow Data Page
                  case 'e': // Empty Overflow Data Page (hash specific)
                     lused++;
                     lusedinblock++;
                     loverflowinblock++;
                     loverflow++;
                     break;
                  case 'A': // Associated Data Page
                     lusedinblock++;
                     lused++;
                     break;
                  case 'D': // Data Page
                  case 'd': // empty Data Page (Hash Specific)
                     lusedinblock++;
                     lused++;
                     break;
                  case 'U': // Unused Page, a page that has never been written to
                     lunused++;
                     break;
                  default: //Note that we've seen an unrecognised pagetype.
                     lunrecognisedpagetype++;
                     break;
               }
               lposinblock++;
               if (lposinblock>=lgroupfactor) {
                  BYTE b;
                  if (lgroupfactor ==1L) 
                     b = ConvertPageCharToByte(c);
                  else {
                     long lmax =  ( 1 << FILLFACTORBITS) -1 ; // maximum 6 bits value
                     int i = MulDiv( lmax, lusedinblock, lgroupfactor);
                     if ( lusedinblock >lgroupfactor || (long)i > lmax) {
                        myerror(ERR_INGRESPAGES);
                        bResult=FALSE;
                     }      
                     b = (BYTE) i;
                     i=0;
                     if (loverflowinblock) {
                        i=1;
                        if (loverflowinblock>=lovf1) {
                           i=2;
                           if (loverflowinblock>=lovf2)
                              i=3;
                        }
                        b += (BYTE)  (i << FILLFACTORBITS );
                     }
                  }
                  pBA->SetAt(lnbblocks, b);
                  lnbblocks++;
                  lposinblock     = 0L;
                  lusedinblock    = 0L;
                  loverflowinblock= 0L;
               }
               lpages++;
            }
            pc++; /* the chars to be parsed here are normally hardcoded non-DBCS chars. 
			         if they were to be changed to DBCS, the logic should be revisited*/
         }
         
		 break;
      case 3:  // 2 is reserved for "old hash" (not managed), 3 is for Hash for Ingres 2.5 or more 
		if (!x_strncmp(CurLine,szthereare,x_strlen(szthereare))) {
			pc = x_strstr(CurLine,szbuckets);
			if (pc) {
				*pc='\0';
				lbuckets = atol(CurLine+x_strlen(szthereare));
				pc1 = x_strstr(pc+1,szbuckwithovf);
				if (!pc1) {
					lbuckets=-1;
					break;
				}
				*pc1='\0';
				lbuck_with_ovf = atol(pc+ x_strlen(szbuckets));
				break;
			}
			pc = x_strstr(CurLine,szemptybuck);
			if (pc) {
				*pc='\0';
				lemptybuckets = atol(CurLine+x_strlen(szthereare));
				pc1 = x_strstr(pc+1,szemptywithovf);
				if (!pc1) {
					lemptybuckets=-1;
					break;
				}
				*pc1='\0';
				lempty_with_ovf = atol(pc+ x_strlen(szemptybuck));
				break;
			}
			pc = x_strstr(CurLine,szoverflow);
			if (pc) {
				*pc='\0';
				lhashoverflow = atol(CurLine+x_strlen(szthereare));
				pc1 = x_strstr(pc+1,szareempty);
				if (!pc1) {
					lhashoverflow=-1;
					break;
				}
				*pc1='\0';
				lemptyoverflow = atol(pc+ x_strlen(szoverflow));
				break;
			}
		}
		if (!x_strncmp(CurLine,szlongestovfch,x_strlen(szlongestovfch))) {
			pc = x_strstr(CurLine,szinbucket);
			if (pc) 
				*pc='\0';
			llongest_ovf_chain  = atol(CurLine+x_strlen(szlongestovfch));
			if (pc) 
				lbuck_longestovfchain = atol(pc+ x_strlen(szinbucket));
			break;
		}
		if (!x_strncmp(CurLine,szavgovfchain,x_strlen(szavgovfchain))) {
			favg_ovf_chain = atof(CurLine+x_strlen(szavgovfchain));
			break;
		}

		if (!x_strncmp(CurLine,szDummy1,x_strlen(szDummy1)) ||
			!x_strncmp(CurLine,szDummy2,x_strlen(szDummy2)) ||
			!x_strncmp(CurLine,szDummy3,x_strlen(szDummy3)) ||
			!x_strncmp(CurLine,szDummy4,x_strlen(szDummy4))    ) 
			break;

		if (lgroupfactor !=1L) 
			break;
         x_strcpy(buf,CurLine);
         if (x_strlen(buf)<13) {
             myerror(ERR_INGRESPAGES);
             bResult = FALSE;
             return;
         }
         buf[12]='\0';
         iret=sscanf(buf,"%ld",&l1);
         if (iret!=1) {
            myerror(ERR_INGRESPAGES);
            bResult = FALSE;
            return;
         }
         pc=buf+13;
		while ( (c=*pc) !=NULL) {
			if (c!=' ') {
				BYTE b;

				switch (c) {
					case 'D': // Data Page
					case 'd': // Empty Data Page
						while (TRUE) {  // search next data page in array
							lposdatainarray++;
							if (lposdatainarray>=lnbblocks) {
								myerror(ERR_INGRESPAGES);
								bResult = FALSE;
								return;
							}
							b=pBA->GetAt(lposdatainarray);
							if (b==(BYTE) PAGE_DATA || b==(BYTE)PAGE_EMPTY_DATA_NO_OVF)
								break;
						}
						ldatapage++;
//						b = ConvertPageCharToByte(c);
						//assertion available only against next Ingres build!
//						ASSERT (b == pBA->GetAt(lposdatainarray) );
//						pBA->SetAt(lposdatainarray, b);
						c1 = c; 
						break;
					case 'o': // Overflow Page
					case 'e': // Empty Overflow Page
//						while (TRUE) {  // search next overflow page in array
//							lposovfinarray++;
//							if (lposovfinarray>=lnbblocks) {
//								myerror(ERR_INGRESPAGES);
//								bResult = FALSE;
//								return;
//							}
//							b=pBA->GetAt(lposovfinarray);
//							if (b==(BYTE) PAGE_OVERFLOW_DATA || b == (BYTE)PAGE_OVERFLOW_LEAF ||
//								b== (BYTE)PAGE_EMPTY_OVERFLOW)
//								break;
//						}
//						lovfpage++;
//						b = ConvertPageCharToByte(c);
// 						pBA->SetAt(lposovfinarray, b);
						switch (c1) { // update previous data page
							case 'D': // was "with data"
								b = ConvertPageCharToByte('x');
								break;
							case 'd': // was "empty data"
								b = ConvertPageCharToByte('y');
								break;
						}
						pBA->SetAt(lposdatainarray, b);
						break;
				}
			}
			pc++;  /* the chars to be parsed here are normally hardcoded non-DBCS chars. 
			         if they were to be changed to DBCS, the logic should be revisited*/
		}
		break;
	}
	return;
}

}  // extern "C"

extern "C" {


BOOL SQLGetPageTypesPerTableTypes(LPUCHAR pNodeName,int * p12ints)
{
	int isess;
	int ires;
	char buf[100];
	char * CurLine;

	BOOL bResult = TRUE;

	wsprintf(buf,"%s::iidbdb",(char *)pNodeName);
	ires = Getsession((LPUCHAR)buf,SESSION_TYPE_CACHENOREADLOCK, &isess);
	if (ires != RES_SUCCESS)
		return FALSE;

	StartSQLTrace();

	ires= ExecSQLImm ((LPUCHAR)"set trace point dm34", TRUE , NULL, NULL, NULL);

	StopSQLTrace();

	ReleaseSession(isess, RELEASE_COMMIT);

	if (ires!=RES_SUCCESS)
		return FALSE;
	
	CurLine=GetFirstTraceLine();

	ires = sscanf(CurLine, "CREATE TABLE page types 2k %d, 4k %d, 8k %d, 16k %d, 32k %d, 64k %d",
		p12ints,
		p12ints+1,
		p12ints+2,
		p12ints+3,
		p12ints+4,
		p12ints+5);
	if (ires!=6)
		return FALSE;

	CurLine = GetNextSignificantTraceLine();

	ires = sscanf(CurLine, "CREATE INDEX page types 2k %d, 4k %d, 8k %d, 16k %d, 32k %d, 64k %d",
		p12ints+6,
		p12ints+7,
		p12ints+8,
		p12ints+9,
		p12ints+10,
		p12ints+11);
	if (ires!=6)
		return FALSE;

	return TRUE;

}

}

BOOL GetIngresPageInfo(int nNodeHandle, LPUCHAR dbName, LPUCHAR tableName, LPUCHAR tableOwner, LPINGRESPAGEINFO lpPageInfo)
{
	int iret,ilocsession;
	char buf[400];
	UCHAR NodeName[MAXOBJECTNAME];
	UCHAR UserName[MAXOBJECTNAME], UserName1[MAXOBJECTNAME];
	UCHAR FullTableName[MAXOBJECTNAME*2];

	lstrcpy((char *)NodeName,(char *)GetVirtNodeName(nNodeHandle));
	DBAGetUserName( NodeName, UserName);

	lstrcpy((char *)FullTableName,QuoteIfNeeded((LPCTSTR)tableOwner));
	lstrcat((char *)FullTableName,".");
	lstrcat((char *)FullTableName,QuoteIfNeeded(RemoveDisplayQuotesIfAny((LPCTSTR)StringWithoutOwner(tableName))));
	if (x_stricmp((char *)UserName,(char *)tableOwner)) {
		BOOL bHasUserSuffix = GetConnectUserFromString(NodeName, UserName1);
		if (bHasUserSuffix) {
			//"You are connected through the %s username option .\n"
			//"Page information is not available for tables not owned by %s."
			wsprintf(buf,VDBA_MfcResourceString (IDS_F_USER_CONNECT),
						UserName1, UserName1);
			MessageBox(GetFocus(), buf, NULL, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
			memset(lpPageInfo, 0, sizeof(INGRESPAGEINFO));

			return TRUE; // TRUE avoids a second error message. the structure has been filled with
						    // zeroes so that the pane displays "N/A" in the appropriate controls
		}
		x_strcat((char *)NodeName,LPUSERPREFIXINNODENAME);
		x_strcat((char *)NodeName,(char *)tableOwner);
		x_strcat((char *)NodeName,LPUSERSUFFIXINNODENAME);
	}


   bResult=TRUE;
   pBA= NULL;
   mycallbackstate = 0;
   mystring="";
   memset (lpPageInfo,'\0',sizeof(INGRESPAGEINFO));
   lpages0     =-1L;
   lpages      =0L;
   lused       =0L;
   lfreed      =0L;
   lunused     =0L;
   loverflow   =0L;
   lgroupfactor=1L;
   lnbblocks   =0L;
   bcallbackregistered = FALSE;
   // connect
   wsprintf(buf, "%s::%s",NodeName, dbName);  
   iret = Getsession((LPUCHAR) buf, SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
   if (iret!=RES_SUCCESS)
    return FALSE;  

   // execute statement
   wsprintf(buf,"modify %s to table_debug with table_option=2",(char *)FullTableName);
   IILQsthSvrTraceHdlr(mycallbackpages,(void *)NULL, TRUE, FALSE);
   bcallbackregistered = TRUE;
   iret= ExecSQLImm ((LPUCHAR)buf,  TRUE , NULL, NULL, NULL);
   if (bcallbackregistered) {
      IILQsthSvrTraceHdlr(mycallbackpages,(void *)NULL, FALSE, FALSE);
      bcallbackregistered = FALSE;
   }
   ReleaseSession(ilocsession, RELEASE_COMMIT);
   if (iret!=RES_SUCCESS && iret!=RES_SQLNOEXEC)
      return FALSE;

   if (!bResult && pBA) {
     delete pBA;
     pBA=NULL;
   }

   if (lunrecognisedpagetype)
   {
	   char buf[256];
	   //"Unknown pagetype found for %u page%c in table %s.\n"
	   wsprintf(buf,VDBA_MfcResourceString (IDS_E_UNKNOWN_PAGETYPE),
		   lunrecognisedpagetype, (lunrecognisedpagetype == 1)? ' ':'s', (char *)FullTableName);
	   MessageBox(GetFocus(), buf, NULL, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
   }

   lpPageInfo->ltotal    = lpages;
   lpPageInfo->linuse    = lused;
   lpPageInfo->loverflow = loverflow;
   lpPageInfo->lfreed    = lfreed++;
   lpPageInfo->lneverused= lunused;

   lpPageInfo->pByteArray = pBA;
   lpPageInfo->lItemsPerByte = lgroupfactor;
   lpPageInfo->nbElements    = lnbblocks;

   
   ASSERT (lgroupfactor >1L || lnbblocks ==lpages0);
   ASSERT (lpages==lpages0);

   lpPageInfo->bNewHash               = bHashNew;
   lpPageInfo->lbuckets_no_ovf        = lbuckets - lemptybuckets -(lbuck_with_ovf - lempty_with_ovf);
   lpPageInfo->lemptybuckets_no_ovf   = lemptybuckets - lempty_with_ovf;
   lpPageInfo->lbuck_with_ovf         = lbuck_with_ovf - lempty_with_ovf;
   lpPageInfo->lemptybuckets_with_ovf = lempty_with_ovf;
   lpPageInfo->loverflow_not_empty    = lhashoverflow - lemptyoverflow;
   lpPageInfo->loverflow_empty        = lemptyoverflow;
   lpPageInfo->llongest_ovf_chain     = llongest_ovf_chain;
   lpPageInfo->lbuck_longestovfchain  = lbuck_longestovfchain;
   lpPageInfo->favg_ovf_chain         = favg_ovf_chain;

   return bResult;
}
#define RMCMD_MAXLINES 200

BOOL ExecRmcmdInBackground(char * lpVirtNode, char *lpCmd, CString InputLines)
{
	RMCMDPARAMS rmcmdparams;
	char *CurLine;
	int cpt,cpt2,ires,retval;
	int istatus;
	LPUCHAR feedbackLines[RMCMD_MAXLINES];
	BOOL NoCRs[RMCMD_MAXLINES];
	int iTimerCount = 0;
	int iCountLine = 0;
	bool bServerActive = FALSE;
   BOOL bHasTerminated = FALSE;
   time_t tim1,tim2;
   double d1;
   BOOL bResult=TRUE;
 
	ires=LaunchRemoteProcedure1( lpVirtNode, lpCmd, &rmcmdparams, FALSE);
	if (ires<0) {	// Failure to launch remote command
		ErrorMessage ((UINT)IDS_E_LAUNCHING_RMCMD, RES_ERR);
		return FALSE;
	}
	rmcmdparams.rmcmdhdl=ires;
	rmcmdparams.bSessionReleased = FALSE;

   // send input lines
   mystring=InputLines;
   for (CurLine=GetFirstTraceLine();CurLine;CurLine = GetNextTraceLine()) 
      	PutRmcmdInput(&rmcmdparams, (UCHAR *) CurLine, FALSE);

	// allocate space for future incoming lines
	memset(feedbackLines, 0, sizeof(feedbackLines));
	for (cpt=0; cpt<RMCMD_MAXLINES; cpt++) {
		feedbackLines[cpt] = (LPUCHAR)ESL_AllocMem(RMCMDLINELEN);
		if (!feedbackLines[cpt]) {
			for (cpt2=0; cpt2<cpt; cpt2++)
				ESL_FreeMem(feedbackLines[cpt2]);
         ErrorMessage ((UINT)IDS_ERR_MEM_REMOTECMD, RES_ERR);
         CleanupRemoteCommandTuples(&rmcmdparams);
			if (!rmcmdparams.bSessionReleased)
            ReleaseRemoteProcedureSession(&rmcmdparams);
			return FALSE;
		}
	}
   mystring="";
   time( &tim1 );
	while (TRUE)	{
      if (!bHasTerminated) 
         bHasTerminated = Wait4RmcmdEvent(&rmcmdparams, 2);
      retval = GetRmcmdOutput( &rmcmdparams,
								 RMCMD_MAXLINES,
								 feedbackLines,
								 NoCRs);
		//UpdateMessagePaint();

		if (retval < 0) 	// execution of the command is terminated!
			break;			  
		
		if (retval > 0) {
			for (int i=0;i<retval;i++)	{
				mystring+=(LPCSTR)feedbackLines[i];
				if (!NoCRs[i])
					 mystring+= _T("\r\n");
         }
			istatus = GetRmCmdStatus(&rmcmdparams);
			if ( istatus == RMCMDSTATUS_ERROR ) {
            bResult=FALSE;
				break;
         }
		}
		
		if (retval == 0 && !bServerActive) {
         time(&tim2);
         d1= difftime( tim2,tim1);
			if (d1>25) {
				tim1=tim2;
            istatus = GetRmCmdStatus(&rmcmdparams);
				if (istatus==RMCMDSTATUS_ERROR)  {
					MessageBox(GetFocus(),
						ResourceString ((UINT)IDS_REMOTECMD_NOTACCEPTED),
						NULL, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
               bResult=FALSE;
					break;
				}
				if (istatus!=RMCMDSTATUS_SENT)
					bServerActive=TRUE;
				else  {
					if (MessageBox(GetFocus(),
						ResourceString ((UINT)IDS_REMOTECMD_NOANSWER),
						ResourceString ((UINT)IDS_REMOTECMD_WARNING),
						MB_ICONQUESTION | MB_YESNO | MB_TASKMODAL) == IDNO){
                  bResult=FALSE;
						break;
					}
				}
			}
		}
	} // End while

	CleanupRemoteCommandTuples(&rmcmdparams);
	if (!rmcmdparams.bSessionReleased)
      ReleaseRemoteProcedureSession(&rmcmdparams);
	for (cpt=0; cpt<RMCMD_MAXLINES; cpt++)
		ESL_FreeMem(feedbackLines[cpt]);
	return bResult;
}

CString & GetTraceOutputBuf()
{
   return mystring;
}


static int floatfromIngString(TCHAR *pbuf, float * pf)
{
	TCHAR buf[100];
	fstrncpy((LPUCHAR)buf, (LPUCHAR)pbuf, sizeof(buf)-1);
	TCHAR * pc = buf;
	TCHAR * pc1;
	int n;
	while ( (pc1 = _tcschr(pc,_T(','))) != NULL) {
		*pc1=_T('.');
		pc   =pc1;
	}
	pc = _tcschr(buf,_T('.'));
	pc1= _tcsrchr(buf,_T('.'));
	if (pc!=pc1)
		return 0; /* more than one separator. return an error */
	/* locale is set to "C" within the scope where this function can be called*/
	n = sscanf(buf,"%f",pf); 
	return n;
}

//
// Query all the Table's columns either with statistic or not:
BOOL Table_QueryStatisticColumns (CaTableStatistic& ts)
{
	//
	// Get information of Table
	if (ts.m_nOT == OT_TABLE)
	{
		TABLEPARAMS tb;
		memset (&tb, 0, sizeof (tb));
		ts.m_nNumColumnKey = 0;

		lstrcpy ((char *)tb.DBName,     ts.m_strDBName);
		lstrcpy ((char *)tb.objectname, ts.m_strTable);
		lstrcpy ((char *)tb.szSchema,   ts.m_strTableOwner);
		int dummySesHndl;
		int iResult = GetDetailInfo ((LPUCHAR)(LPCTSTR)ts.m_strVNode, OT_TABLE, &tb, FALSE, &dummySesHndl);
		if (iResult != RES_SUCCESS)
			return FALSE;
		
		CString strCompositeCol = _T("");
		BOOL bOne = TRUE;
		BOOL bKey = FALSE; // If exist the key columns then add a special column COMPOSITE (...)
		CaTableStatisticColumn* pNewColumn = NULL;
		LPUCHAR lpucType;
		LPOBJECTLIST ls;
		LPCOLUMNPARAMS lpCol;
		ls = tb.lpColumns;
		while (ls) 
		{
			lpCol = (LPCOLUMNPARAMS)ls->lpObject;
			if (!lpCol)
				continue;
		
			pNewColumn = new CaTableStatisticColumn();
			pNewColumn->m_strColumn = lpCol->szColumn;        // Column's name

			if (lstrcmpi(lpCol->tchszInternalDataType, lpCol->tchszDataType) != 0)  {
				ASSERT (lpCol->tchszInternalDataType[0]);
				pNewColumn->m_strType = lpCol->tchszInternalDataType;  // UDTs
				if (lstrcmpi(lpCol->tchszInternalDataType,VDBA_MfcResourceString(IDS_OBJECT_KEY)) == 0 ||
					lstrcmpi(lpCol->tchszInternalDataType,VDBA_MfcResourceString(IDS_TABLE_KEY) ) == 0) {
					if (lpCol->bSystemMaintained)
						pNewColumn->m_strType += VDBA_MfcResourceString(IDS_SYSTEM_MAINTAINED);
					else
						pNewColumn->m_strType += VDBA_MfcResourceString(IDS_NO_SYSTEM_MAINTAINED);
				}
			}
			else
			{
				pNewColumn->m_strType   = lpCol->tchszDataType;   // Column's Data type

				lpucType = GetColTypeStr(lpCol);
				if (lpucType && lstrlen ((LPCTSTR)lpucType) > 0) 
					pNewColumn->m_strType = lpucType;
				if (lpucType)
					ESL_FreeMem(lpucType);
			}
			pNewColumn->m_strType.MakeLower();
			pNewColumn->m_nDataType = (int)lpCol->uDataType;
			//
			// Decimal data type ?
			if (lpCol->uDataType == INGTYPE_DECIMAL)
			{
				pNewColumn->m_nLength   = lpCol->lenInfo.decInfo.nPrecision;
				pNewColumn->m_nScale    = lpCol->lenInfo.decInfo.nScale;
			}
			else
			{
				pNewColumn->m_nScale    = -1;
				pNewColumn->m_nLength   = (int)lpCol->lenInfo.dwDataLen;
			}
			pNewColumn->m_bNullable     = lpCol->bNullable;     // Nullable
			pNewColumn->m_bWithDefault  = lpCol->bDefault;      // Use Default
			pNewColumn->m_strDefault    = (lpCol->lpszDefSpec)? lpCol->lpszDefSpec: _T("");
			pNewColumn->m_nPKey         = lpCol->nKeySequence ; // Key Sequence of the column
			pNewColumn->m_bHasStatistic = lpCol->bHasStatistics;
			
			ts.m_listColumn.AddTail (pNewColumn);
			ls = (LPOBJECTLIST)ls->lpNext;

			if (pNewColumn->m_nPKey > 0)
			{
				bKey = TRUE;
				if (!bOne)
					strCompositeCol += _T(", ");
				strCompositeCol += pNewColumn->m_strColumn;
				ts.m_nNumColumnKey++;
				bOne = FALSE;
			}
		}
		if (bKey && ts.m_AddCompositeForHistogram)
		{
			int ires, hdl = -1;
			CString strText;
			strText.Format (_T("%s::%s"), (LPCTSTR)ts.m_strVNode, (LPCTSTR)ts.m_strDBName);
			ires = Getsession ((LPUCHAR)(LPCTSTR)strText, SESSION_TYPE_CACHENOREADLOCK, &hdl);
			if (ires != RES_ERR)
			{
				pNewColumn = new CaTableStatisticColumn();
				pNewColumn->m_strColumn.Format(_T("composite(%s)"), (LPCTSTR)strCompositeCol);
				pNewColumn->m_bComposite = TRUE;
				ts.m_listColumn.AddTail (pNewColumn);
				pNewColumn->m_bHasStatistic = (INGRESII_llHasCompositeHistogram(OT_TABLE, ts.m_strTable, ts.m_strTableOwner) > 0);
				ReleaseSession(hdl, RELEASE_COMMIT);
			}
		}
		FreeAttachedPointers (&tb,  OT_TABLE);
	}
	else 
	if (ts.m_nOT == OT_INDEX)
	{
		INDEXPARAMS indexparams;
		memset (&indexparams, 0, sizeof (indexparams));
		ts.m_nNumColumnKey = 0;

		lstrcpy ((char *)indexparams.DBName,        ts.m_strDBName);
		lstrcpy ((char *)indexparams.TableName,     ts.m_strTable);
		lstrcpy ((char *)indexparams.TableOwner,    ts.m_strTableOwner);
		lstrcpy ((char *)indexparams.objectname,    ts.m_strIndex);
		lstrcpy ((char *)indexparams.objectowner,   ts.m_strTableOwner);

		int dummySesHndl;
		int iResult = GetDetailInfo ((LPUCHAR)(LPCTSTR)ts.m_strVNode, OT_INDEX, &indexparams, FALSE, &dummySesHndl);
		if (iResult != RES_SUCCESS)
			return FALSE;

		CaTableStatisticColumn* pNewColumn = NULL;
		LPOBJECTLIST ls;

		CString strCompositeCol = _T("");
		BOOL bOne = TRUE;
		BOOL bKey = FALSE;
		ls = indexparams.lpidxCols;
		while (ls) 
		{
			LPIDXCOLS lpIdxCols = (LPIDXCOLS)ls->lpObject;
			if (lpIdxCols->attr.bKey)
			{
				bKey = TRUE;
				if (!bOne)
					strCompositeCol += _T(", ");
				strCompositeCol += CString(lpIdxCols->szColName);
				ts.m_nNumColumnKey++;
				bOne = FALSE;
			}

			ls = (LPOBJECTLIST)ls->lpNext;
		}
		if (bKey && ts.m_AddCompositeForHistogram)
		{
			int ires, hdl = -1;
			CString strText;
			strText.Format (_T("%s::%s"), (LPCTSTR)ts.m_strVNode, (LPCTSTR)ts.m_strDBName);
			ires = Getsession ((LPUCHAR)(LPCTSTR)strText, SESSION_TYPE_CACHENOREADLOCK, &hdl);
			if (ires != RES_ERR)
			{
				pNewColumn = new CaTableStatisticColumn();
				pNewColumn->m_strColumn.Format(_T("composite(%s)"), (LPCTSTR)strCompositeCol);
				pNewColumn->m_bComposite = TRUE;
				ts.m_listColumn.AddTail (pNewColumn);
				pNewColumn->m_bHasStatistic = (INGRESII_llHasCompositeHistogram(OT_INDEX, ts.m_strIndex, ts.m_strTableOwner) > 0);
				ReleaseSession(hdl, RELEASE_COMMIT);
			}
		}

		FreeAttachedPointers (&indexparams,  OT_INDEX);
	}

	return TRUE;
}

static char *szdate        = "date:";
static char *szunique      = "unique values:";
static char *szrepet       = "repetition factor:";
static char *szuniqueflag  =" unique flag:";
static char *szcompleteflag=" complete flag:";
static char *szcell        = "cell:";
static char * szcount      = "count:";
static char * szrepf       = "repf:";
static char * szvalue      = "value:";

//
// Query the statistic for a given column <pColumn->m_strColumn>:
static BOOL Table_GetStatistics1 (CaTableStatisticColumn* pColumn, CaTableStatistic& ts)
{
   char szcmdbuf[200];
   char szusernamebuf[200];
   char buf[500];
   char * CurLine;
   char *pc, *pc1;
   int ires;
   float f,f1,f2;
   DBAGetUserName ((LPUCHAR)(LPCTSTR)ts.m_strVNode,(LPUCHAR) szusernamebuf);
   if (lstrcmpi(szusernamebuf,ts.m_strTableOwner)) {
       //"You must connect under the %s account \n"
       //"to display statistics of a table owned by %s."
       wsprintf(buf, VDBA_MfcResourceString (IDS_F_USER_CONNECT_STAT),
                     ts.m_strTableOwner, ts.m_strTableOwner);
       MessageBox(GetFocus(), buf, NULL, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
       return FALSE;
   }
   if (ts.m_nOT == OT_TABLE)
   {
     if (!pColumn->m_bComposite)
     {
       wsprintf(szcmdbuf,"statdump -u%s %s -r%s -a%s",
                szusernamebuf,
                (char *)(LPCTSTR)ts.m_strDBName,
                (char *)StringWithoutOwner((LPUCHAR)(LPCTSTR)ts.m_strTable),
                (char *)(LPCTSTR)pColumn->m_strColumn);
     }
     else
     {
       wsprintf(szcmdbuf,"statdump -zcpk -u%s %s -r%s ",
                szusernamebuf,
                (char *)(LPCTSTR)ts.m_strDBName,
                (char *)StringWithoutOwner((LPUCHAR)(LPCTSTR)ts.m_strTable),
                (char *)(LPCTSTR)pColumn->m_strColumn);
     }
   }
   else
   if (ts.m_nOT == OT_INDEX)
   {
       //
       // For an index: the command is expected to print out only the composite histogram.
       wsprintf(szcmdbuf,"statdump -u%s %s -r%s",
                szusernamebuf,
                (char *)(LPCTSTR)ts.m_strDBName,
                (char *)StringWithoutOwner((LPUCHAR)(LPCTSTR)ts.m_strIndex));
   }

	if (!ExecRmcmdInBackground((char *)(LPCTSTR)ts.m_strVNode, szcmdbuf, ""))
      return FALSE;

   CurLine=GetFirstTraceLine();
   if (!CurLine)
      return FALSE;
   while (CurLine && x_strnicmp(CurLine,szdate,x_strlen(szdate))) 
      CurLine = GetNextSignificantTraceLine();
   if (!CurLine)
      return FALSE;
   pc=x_strstr(CurLine, szunique);
   if (!pc)
      return FALSE;
   pc+=x_strlen(szunique);
   ires=floatfromIngString(pc,&f);
   if (ires!=1)
      return FALSE;
	ts.m_lUniqueValue    = (long) f;     // unique value

   while (CurLine && x_strnicmp(CurLine,szrepet,x_strlen(szrepet))) 
      CurLine = GetNextSignificantTraceLine();
   if (!CurLine)
      return FALSE;
   pc=x_strstr(CurLine, szuniqueflag);
   if (!pc)
      return FALSE;
   *pc='\0';
   ires=floatfromIngString(CurLine+x_strlen(szrepet),&f);
   if (ires!=1)
      return FALSE;
   ts.m_lRepetitionFlag = (long)f;     // repetition factor
   pc++;
   pc1=x_strstr(pc,szcompleteflag);
   if (!pc1)
      return FALSE;
   *pc1='\0';
   pc1+=x_strlen(szcompleteflag);
   pc+=x_strlen(szuniqueflag)-1;
   if ( (*pc)=='y' || (*pc)=='Y')   // unique flag
      ts.m_bUniqueFlag  = TRUE;
   else
      ts.m_bUniqueFlag  = FALSE;

   ires=atoi(pc1);                  // complete flag
   if (ires)
      ts.m_bCompleteFlag = TRUE;
   else
      ts.m_bCompleteFlag = FALSE;

   while ((CurLine = GetNextSignificantTraceLine())!=NULL) {
      char buf1[100],buf2[100],buf3[100],buf4[400];
      CString c1;

      if (x_strnicmp(CurLine,szcell,x_strlen(szcell)) )  
         continue;
      pc=x_strstr(CurLine,szcount);
      if (!pc) 
         return FALSE;
      *pc=0;
      x_strcpy(buf1,CurLine+x_strlen(szcell));
      pc1=x_strstr(pc+1,szrepf);
      if (!pc1)
         return FALSE;
      *pc1=0;
      x_strcpy(buf2,pc+x_strlen(szcount));
      pc=x_strstr(pc1+1,szvalue);
      if (!pc)
         return FALSE;
      *pc=0;
      x_strcpy(buf3,pc1+x_strlen(szrepf));
      fstrncpy((LPUCHAR)buf4, (LPUCHAR)(pc+x_strlen(szvalue)),sizeof(buf4));
      c1=buf4;
      ires=floatfromIngString(buf2,&f1);
      if (ires!=1)
         return FALSE;
      ires=floatfromIngString(buf3,&f2);
      if (ires!=1)
         return FALSE;
 
     	ts.m_listItem.AddTail (new CaTableStatisticItem(c1, (double) f1, (double) f2));
   }
   return TRUE;
}

BOOL Table_GetStatistics (CaTableStatisticColumn* pColumn, CaTableStatistic& ts)
{
	BOOL bResult;
	TCHAR buf[200];
	TCHAR * pc1;
	TCHAR * pc = _tsetlocale(LC_NUMERIC,NULL); /* save previous locale */
	if (pc) {
		_tcscpy(buf,pc);
		pc=buf;
	}
	_tsetlocale(LC_NUMERIC,"C"); /* set standard locale, the code will ensure that a dot is*/
	                             /* the numeric separator in sscanf calls*/
	bResult = Table_GetStatistics1 (pColumn, ts);
	pc1=_tsetlocale(LC_NUMERIC,pc); /* restore previous locale once complete */
	return bResult;
}

//
// Generate the statistic for a given column <pColumn->m_strColumn>:
BOOL Table_GenerateStatistics (HWND CurHwnd ,CaTableStatisticColumn* pColumn, CaTableStatistic& ts)
{
   char szusernamebuf[200];
   char buf[500];
   OPTIMIZEDBPARAMS optimizedb;

   DBAGetUserName ((LPUCHAR)(LPCTSTR)ts.m_strVNode,(LPUCHAR) szusernamebuf);

   if (IsSystemObject(OT_TABLE, (LPUCHAR)(LPCTSTR)ts.m_strTable, (LPUCHAR)(LPCTSTR)ts.m_strTableOwner)) {
      return FALSE;
   }

   if (lstrcmpi(szusernamebuf,ts.m_strTableOwner)) {
     //"You are connected under the %s account.\n"
     //"You cannot generate statistics for a table owned by %s."
     wsprintf(buf,VDBA_MfcResourceString (IDS_F_USER_GENERATE_STAT),
              szusernamebuf, ts.m_strTableOwner);
     MessageBox(GetFocus(), buf, NULL, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
     return FALSE;
   }
   // initialize structure
   if (ts.m_nOT == OT_TABLE)
   {
     memset(&optimizedb, 0, sizeof(optimizedb));
     optimizedb.m_nObjectType = OT_TABLE;
     optimizedb.bSelectTable =TRUE;
     optimizedb.bSelectColumn= TRUE;
     if (pColumn->m_bComposite)
         optimizedb.bComposite = TRUE;
     lstrcpy((char *)optimizedb.DBName    , ts.m_strDBName);
     lstrcpy((char *)optimizedb.TableName , (char *)StringWithoutOwner((LPUCHAR)(LPCTSTR)ts.m_strTable));
     lstrcpy((char *)optimizedb.TableOwner, ts.m_strTableOwner);
     lstrcpy((char *)optimizedb.ColumnName, pColumn->m_strColumn);
     DlgOptimizeDB ( CurHwnd, &optimizedb);
   }
   else
   if (ts.m_nOT == OT_INDEX)
   {
     memset(&optimizedb, 0, sizeof(optimizedb));
     optimizedb.m_nObjectType = OT_INDEX;
     optimizedb.bSelectIndex = TRUE;
     lstrcpy((char *)optimizedb.DBName    , ts.m_strDBName);
     lstrcpy((char *)optimizedb.TableName , (char *)StringWithoutOwner((LPUCHAR)(LPCTSTR)ts.m_strTable));
     lstrcpy((char *)optimizedb.TableOwner, ts.m_strTableOwner);
     lstrcpy((char *)optimizedb.tchszIndexName, ts.m_strIndex);
     lstrcpy((char *)optimizedb.tchszDisplayIndexName, ts.m_strIndex);
     DlgOptimizeDB ( CurHwnd, &optimizedb);
   }
   return TRUE;
}

//
// Remove the statistic for a given column <pColumn->m_strColumn>:
BOOL Table_RemoveStatistics (CaTableStatisticColumn* pColumn, CaTableStatistic& ts)
{
   char szcmdbuf[200];
   char szusernamebuf[200];
   char buf[500];
   char * CurLine;

  if (IsSystemObject(OT_TABLE, (LPUCHAR)(LPCTSTR)ts.m_strTable, (LPUCHAR)(LPCTSTR)ts.m_strTableOwner)) {
      return FALSE;
  }
  DBAGetUserName ((LPUCHAR)(LPCTSTR)ts.m_strVNode,(LPUCHAR) szusernamebuf);
  if (lstrcmpi(szusernamebuf,ts.m_strTableOwner)) {
      //"You are connected under the %s account.\n"
      //"You cannot remove statistics for a table owned by %s."
      wsprintf(buf, VDBA_MfcResourceString (IDS_F_USER_REMOVE_STAT),
               szusernamebuf, ts.m_strTableOwner);
      MessageBox(GetFocus(), buf, NULL, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
      return FALSE;
   }


   DBAGetUserName ((LPUCHAR)(LPCTSTR)ts.m_strVNode,(LPUCHAR) szusernamebuf);
   CString strZCPK = _T(" ");
   CString strObject;
   if (ts.m_nOT == OT_TABLE)
   {
       strObject = ts.m_strTable;
       strZCPK = _T(" -zcpk");
   }
   else
   if (ts.m_nOT == OT_INDEX)
       strObject = ts.m_strIndex;
   else
   {
       ASSERT(FALSE);
       return TRUE;
   }

   if (pColumn->m_bComposite) 
   {
       //
       // Table or Secondary index
       wsprintf(szcmdbuf,"statdump %s -zdl -u%s %s -r%s",
                (LPCTSTR)strZCPK,
                szusernamebuf,
                (char *)(LPCTSTR)ts.m_strDBName,
                (char *)StringWithoutOwner((LPUCHAR)(LPCTSTR)strObject));
   }
   else
   {
       //
       // Only the real column of Table
       wsprintf(szcmdbuf,"statdump -u%s -zdl %s -r%s -a%s",
                szusernamebuf,
                (char *)(LPCTSTR)ts.m_strDBName,
                (char *)StringWithoutOwner((LPUCHAR)(LPCTSTR)ts.m_strTable), // the -u stuff normally works around the fact that schema prefixes are not allowed 
                (char *)(LPCTSTR)pColumn->m_strColumn);
   }
   if (!ExecRmcmdInBackground((char *)(LPCTSTR)ts.m_strVNode, szcmdbuf, ""))
      return FALSE;

   CurLine=GetFirstTraceLine();
   while (CurLine)
      CurLine=CurLine = GetNextSignificantTraceLine();

   return TRUE;
}

extern "C" BOOL MfcGetRmcmdObjects_launcher(char *VnodeName, char *objName)
{
    return GetRmcmdObjects_launcher(VnodeName, objName);
}

BOOL GetRmcmdObjects_launcher ( char *nodeName, char *objName)
{
    char CommandLine[50];
    CString LocalDBNameOnServerNode;
    CString strVName;
    CString csAllLines,csUserName;

    strVName = nodeName;
    if (strVName.IsEmpty())
        return FALSE;

    wsprintf(CommandLine,"ddprocessuser");
    // retieve user executing the rmcmd process
    if (!ExecRmcmdInBackground((LPTSTR)(LPCTSTR) strVName, CommandLine, ""))
        return FALSE;

    csAllLines = GetTraceOutputBuf();
    if (csAllLines.IsEmpty() || csAllLines[0]==_T('*'))
        return FALSE;
    char * plastchar;
    char * presult = (LPTSTR)(LPCTSTR)csAllLines;

    while ( (plastchar = x_strgetlastcharptr(presult))!=NULL ) {
        char c = *plastchar;
            if (c=='\n' || c=='\r')
                *plastchar ='\0';
            else
                break;
    }
    _tcscpy(objName, presult);
    return TRUE;
}
