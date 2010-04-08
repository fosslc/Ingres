/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
**
**  Project : libmon library for the Ingres Monitor ActiveX control
**
**    Source : tools.c
**    misc utility functions
**
** History
**
**	22-Nov-2001 (noifr01)
**      extracted and adapted from the winmain\tools.c source
**  24-Mar-2004 (shah03)
**      (sir 112040) removed the usage of "_tcsinc" added "pointer+1" in the same place
**  25-Oct-2004 (noifr01)
**     (bug 113305) x_strpbrk() wasn't handling correctly effective DBCS
**     characters
**  02-apr-2009 (drivi01)
**    In efforts to port to Visual Studio 2008, clean up warnings.
********************************************************************/

#include <stdio.h>
#include "libmon.h"
#include "error.h"
#include "tchar.h"
#include "dbaset.h"

UCHAR *fstrncpy(pu1,pu2,n)
UCHAR *pu1;
UCHAR *pu2;
int n;
{	// don't use STlcopy (may split the last char)
	char * bufret = pu1;
	while ( n >= CMbytecnt(pu2) +1 ) { // copy char only if it fits including the trailing \0
		n -=CMbytecnt(pu2);
		if (*pu2 == EOS) 
			break;
		CMcpyinc(pu2,pu1);
	}
	*pu1=EOS;
	return bufret;

}

/* suppspace: returns TRUE if trailing blanks have been found and removed */
BOOL suppspace(pu)
UCHAR *pu;
{
	int iprevlen = STlength(pu);
	int inewlen  = STtrmwhite(pu);
	if  (inewlen<iprevlen)
		return TRUE;
	else
		return FALSE;
}

/* suppspacebutleavefirst():
   the purpose of this function is to remove trailing blanks except the
   leading one in the case the string only contains blanks.
   This is useful specifically for getting the real name of a database object
   (see the "Object Names" section of the SQL reference guide) */
BOOL suppspacebutleavefirst(UCHAR *pu)
{
    int iprevlen = STlength(pu);
    int inewlen  = STtrmwhite(pu);

    if (iprevlen > 0 && inewlen == 0) {
        STcopy((" "),pu);
		inewlen=x_strlen(pu);
	}
    if  (inewlen<iprevlen)
        return TRUE;
    else
        return FALSE;
}

BOOL isempty(pchar)
LPUCHAR pchar;
{
	while (*pchar != EOS) {
		if (!CMwhite(pchar))
			return FALSE;
		CMnext(pchar);
	}
   return TRUE;
}

void ourString2Upper(LPUCHAR pc)
{
	while (*pc != EOS) {
		int ilen = CMbytecnt(pc);
	    CMtoupper(pc, pc);
		if (CMbytecnt(pc)!=ilen) {// the case where the uppercase bytes count for a char is different from
								  // the lower case one is normally impossible (according indirectly to the
								  // CL spec, that specifies that the copy can be done in place ('src' = 'dst')..)
								  // however, this is verified here and a system error is provided if this
								  // is found not to be true (runtime test)
			myerror(ERR_DBCS);  
			*pc=EOS;
			return;
		}
		CMnext(pc);
	}
}

char * x_strgetlastcharptr(char * pstring)
{
	int ilen = x_strlen(pstring);
	int ipos = 0;
	while (ipos<ilen) {
		int cb = CMbytecnt(pstring+ipos);
		if (ipos+cb>=ilen) 
			return (pstring+ipos);
		ipos+=cb;
	}
	return NULL; /* case where ilen==0 returns here */
}

void ourString2Lower(LPUCHAR pc)
{
	while (*pc != EOS) {
		int ilen = CMbytecnt(pc);
	    CMtolower(pc, pc);
		if (CMbytecnt(pc)!=ilen) {// the case where the uppercase bytes count for a char is different from
								  // the lower case one is normally impossible (according indirectly to the
								  // CL spec, that specifies that the copy can be done in place ('src' = 'dst')..)
								  // however, this is verified here and a system error is provided if this
								  // is found not to be true (runtime test)
			myerror(ERR_DBCS);  
			*pc=EOS;
			return;
		}
		CMnext(pc);
	}
}

static char bufresu[100];


LPUCHAR getsubstring(pustring,pusubstring,lenresult)
LPUCHAR pustring;
LPUCHAR pusubstring;
int lenresult;
{
   LPUCHAR pc = pustring;
   pc = x_strstr(pustring,pusubstring);
   if (pc== NULL) {
		bufresu[0]='\0';
		return bufresu;
   }
   fstrncpy(bufresu,pc+x_strlen(pusubstring),lenresult);
   return bufresu;
}

void storeint(lpstring,i)
LPUCHAR lpstring;
int i;
{
#ifdef MAINWIN
#if defined(su4_us5) || defined(hp8_us5) || defined(hpb_us5) || \
    defined(rs4_us5)
  // Bus alignment problem!
  memcpy(lpstring,(char *)&i,sizeof(int));
#else
#error platform-specific code missing
#endif  /* su4_us5 hp8_us5 hpb_us5 rs4_us5 */
#else   /* MAINWIN */
   * ((int *) lpstring) = i;
#endif  // MAINWIN
}

int getint(lpstring)
LPUCHAR lpstring;
{
#ifdef MAINWIN
#if defined(su4_us5) || defined(hp8_us5) || defined(hpb_us5) || \
    defined(rs4_us5)
  // Bus alignment problem!
  int i;
  memcpy((char*)&i,lpstring,sizeof(int));
  return i;
#else
#error platform-specific code missing
#endif  /* su4_us5 hp8_us5 hpb_us5 rs4_us5 */
#else   /* MAINWIN */
   return * ((int *)lpstring) ;
#endif  // MAINWIN
}


LPTSTR GetSchemaDot (LPCTSTR lpstring)
{
   int nbDoubleQuotes;
   LPCTSTR lpCurString;

   /*if (!_tcschr(lpstring,'.'))*/
   if (!STchr(lpstring,'.'))
      return NULL;

   lpCurString = lpstring;
   nbDoubleQuotes = 0;
   while (*lpCurString)
   {
      if (*lpCurString == '"')
      {
         nbDoubleQuotes++;
         if (nbDoubleQuotes == 2)
            nbDoubleQuotes=0;
      }
      if (*lpCurString == '.' && nbDoubleQuotes == 0)
         return (char *)lpCurString;
      CMnext(lpCurString);
   }
   return NULL;
}

LPUCHAR StringWithOwner(LPUCHAR lpstring, LPUCHAR lpowner, LPUCHAR bufresult)
{
   if ( GetSchemaDot(lpstring) )
      x_strcpy(bufresult,lpstring);
   else
      sprintf(bufresult,"%s.%s",Quote4DisplayIfNeeded(lpowner),lpstring);
   return bufresult;
}

LPUCHAR StringWithoutOwner(LPUCHAR lpstring)
{
   LPUCHAR pc = GetSchemaDot(lpstring);
   if (pc)
     return (pc+1);
   return lpstring;
}

LPUCHAR OwnerFromString(LPUCHAR lpstring, LPUCHAR bufresult)
{
   LPUCHAR pc;
   x_strcpy(bufresult,lpstring);
   //pc = _tcschr(bufresult,'.');
   pc = GetSchemaDot(bufresult);
   if (pc) {
      *pc='\0';
	  strcpy(bufresult, RemoveDisplayQuotesIfAny(bufresult));
   }
   else
      *bufresult='\0';
   return bufresult;
}


int GetIntFromStartofString(LPUCHAR lpstring, LPUCHAR bufresult )
{
   sscanf (lpstring,"%s",bufresult);
   return atoi(bufresult);
}

// x_strchr maps strchr to CL functions (for DBMS compatibility)
// the (STindex((x), &(y), 0) macro doesn't work because the second parameter is usuall

// x_strchr is specified for a one byte character, otherwise provides a system error
// (used only for searching for specific keywords)
char * x_strchr(const char *buf, int ichar)
{
	char bufchar[4];
	bufchar[0] = ichar;
	if (CMbytecnt(bufchar)!=1)// ':' and ' ' are normally one byte. In case it wouldn't,
	   myerror(ERR_DBCS); // provide a system error
	return STindex(buf,bufchar,0);
}

// specified only for 1 byte (special) character to be searched
char * x_strrchr( const char *buf, int ichar)
{
	char * p1;
	char * p2;
	p1 = x_strchr(buf, ichar);
	if (p1==NULL)
		return NULL;
	while ( (p2 = x_strchr(p1+CMbytecnt(p1),ichar)) !=NULL)
		p1=p2;
	return p1;
}


char * x_strncat( char *pdest, const char *psrc, int n )
{
	fstrncpy((LPUCHAR) (pdest+x_strlen(pdest)),(LPUCHAR) psrc,n+1); // +1 because fstrncpy is specified  to include
	                                          // the trailing \0 in n, but strncat apparently not
	return pdest;
}
void FillVnodeandDbtargetFromString(LPUCHAR TargetNode,LPUCHAR TargetDB,LPUCHAR szName)
{
   LPUCHAR ptemp;
   UCHAR buf[4*MAXOBJECTNAME];
   UCHAR temp1[MAXOBJECTNAME];
   fstrncpy(buf,szName,sizeof(buf));
   szName=buf;

   ptemp=x_strchr(szName,':');
   if (ptemp)
     {
	   if (CMbytecnt(ptemp)!=1 || CMbytecnt((" "))!=1) // ':' and ' ' are normally one byte. In case it wouldn't,
													 // provide a system error
		   myerror(ERR_DBCS);
	   else {
		*ptemp    =' ';
		*(++ptemp)=' ';
	   }
     }
   sscanf (szName,"%s %s %s",temp1,TargetNode,TargetDB);
}

LPUCHAR x_stristr(LPUCHAR lpstring, LPUCHAR lpString2Find)
{
   int len=x_strlen(lpString2Find);
   while(*lpstring !=EOS) {
      if (!x_strnicmp(lpstring,lpString2Find,len))
         return lpstring;
      CMnext(lpstring);
   }
   return NULL;
}

char * x_strstr(const char * lpstring, const char * lpString2Find)
{
   int len=x_strlen(lpString2Find);
   while(*lpstring !=EOS) {
      if (!x_strncmp(lpstring,lpString2Find,len))
         return (char *) lpstring;
      CMnext(lpstring);
   }
   return NULL;
}

char *  x_strpbrk( const char *string, const char *strCharSet )
{
#ifdef _DEBUG
	/* strCharSet is supposed to be a constant with only */
	/* single byte chars such as *  ; . etc...           */
	const char *ptemp = strCharSet;
	while ( (*ptemp)!=EOS) {
		if (CMbytecnt(ptemp)!=1)
		{
			myerror(ERR_DBCS);
			return (NULL);
		}
		CMnext(ptemp);
	}
#endif
	while ( (*string)!=EOS) {
		if (CMbytecnt(string)==1 &&
			/* only one byte chars supported in strCharSet */
			x_strchr(strCharSet, (int)(*string))!=NULL) {
			return (char *)string;
		}
		CMnext(string);
	}
	return NULL;
}

int x_strnicmp( const char *str1, const char *str2, int len)
{
	int ires;
	while ( len > 1 ) {
		if (*str1 == EOS) {
			if (*str2 == EOS)
				return 0;
			else 
				return (-1);
		}
		if (*str2 == EOS)
			return 1;
		ires= CMcmpnocase(str1, str2);
		if (ires!=0)
			return ires;
		CMbytedec(len,str1);	/* one less character to compare (double or single byte) */
		CMnext(str1);
		CMnext(str2);
	}
	if (len==1) {
		// COMPARE 1 BYTE (not CHARACTER) (even if it is the first of a 2 bytes
		// character) is the best we can do there, given the ambiguity of the
		// functionality in this case
		return/* DON'T ADD x_PREFIX HERE*/ strnicmp( str1,str2,1);
	}
	return 0;
}

int x_strncmp ( const char *str1, const char *str2, int len)
{
	int ires;
	while ( len > 1 ) {
		if (*str1 == EOS) {
			if (*str2 == EOS)
				return 0;
			else 
				return (-1);
		}
		if (*str2 == EOS)
			return 1;
		ires= CMcmpcase(str1, str2);
		if (ires!=0)
			return ires;
		CMbytedec(len,str1);	/* one less character to compare (double or single byte) */
		CMnext(str1);
		CMnext(str2);
	}
	if (len==1) {
		// COMPARE 1 BYTE (not CHARACTER) (even if it is the first of a 2 bytes
		// character) is the best we can do there, given the ambiguity of the
		// functionality in this case
		return/* DON'T ADD x_PREFIX HERE*/ strncmp( str1,str2,1);
	}
	return 0;

}

int StartLinesWithTabs(LPUCHAR buffer)
{

   char buf[4];
   buf[0]=0x0D;
   buf[1]=0x0A;
   buf[2]=' ';
   buf[3]='\0';
   if (*buffer==' ') {
     *buffer=0x09;
     buffer++;
     while (CMspace(buffer))
       STcopy(buffer+CMbytecnt(buffer),buffer);
   }

   while (buffer=x_strstr(buffer,buf))   {
     buffer[2]=0x09;
     buffer+=3;
     while (CMspace(buffer))
       STcopy(buffer+CMbytecnt(buffer),buffer);
   }
   return 1;
}

static int iquoted   = 0;
static int iunquoted = 0;
#define MAXQUOTEDSTRINGS         8
#define MAXQUOTEDSTRINGLEN       512
#define MAXUNQUOTEDSTRINGS       8
#define MAXUNQUOTEDSTRINGLEN     512

static TCHAR quotedstrings[MAXQUOTEDSTRINGS][MAXQUOTEDSTRINGLEN];
static TCHAR unquotedstrings[MAXUNQUOTEDSTRINGS][MAXUNQUOTEDSTRINGLEN];
static TCHAR tchQuote [] = {'"', EOS};
static LPCTSTR quoted(LPCTSTR lpstring)
{
   LPTSTR  lpresult, lpquote,lptemp;
   LPCTSTR lpbuf;

   iquoted++;
   if (iquoted>=MAXQUOTEDSTRINGS)
      iquoted=0;
   lpresult = quotedstrings[iquoted];

   lpbuf = lpstring;
   lpquote = x_strchr(lpbuf,*tchQuote);
   if (lpquote) {
      // Copy string and repeat the quotes.
      // Add the quotes at the begin and at the end of the string.
      lptemp = lpresult;

      CMcpychar(tchQuote,lptemp);
      CMnext(lptemp);
      while (*lpbuf != EOS)
      {
        CMcpychar(lpbuf,lptemp);
        if (!CMcmpcase(lpbuf, tchQuote))
        {
            CMnext(lptemp);
            CMcpychar(tchQuote,lptemp);
        }
        CMnext(lptemp);
        CMnext(lpbuf);
      }
      CMcpychar(tchQuote,lptemp);
      CMnext(lptemp);
      *lptemp = EOS;
   }
   else
   {
      lstrcpy(lpresult ,tchQuote);
      lstrcat(lpresult ,lpstring);
      lstrcat(lpresult ,tchQuote);
   }
   return lpresult;
}

LPCTSTR Quote4DisplayIfNeeded(LPCTSTR lpstring)
{
   /*if (_tcschr(lpstring,'.') || _tcschr(lpstring,'\"'))*/
	if (STchr(lpstring,'.') || STchr(lpstring,'\"'))
     return quoted (lpstring);
   else
      return lpstring;
}


LPCTSTR RemoveDisplayQuotesIfAny(LPCTSTR lpstring)
{
   LPTSTR lpresult,lpquote,lptemp;
   TCHAR lpcharPrev[2],lpcharCurrent[2];
   LPCTSTR lpbuf;
   int iLenString,nByteCntFirstChar,nByteCntLastChar;

   /*if (!_tcschr(lpstring,'.')  && !_tcschr(lpstring,'\"'))*/
   if (!STchr(lpstring,'.')  && !STchr(lpstring,'\"'))
      return lpstring;

   iunquoted++;
   if (iunquoted>=MAXUNQUOTEDSTRINGS)
      iunquoted=0;
   lpresult = unquotedstrings[iunquoted];
   // remove the quotes at the begin and at the end of the string.
   iLenString = lstrlen(lpstring);
   nByteCntFirstChar = CMbytecnt(lpstring);
   nByteCntLastChar  = CMbytecnt(lpstring+iLenString);

   if (!CMcmpcase(lpstring,tchQuote) && !CMcmpcase(lpstring+iLenString-nByteCntLastChar,tchQuote))
   {
      lpbuf = lpstring;
      fstrncpy(lpresult,(LPTSTR)lpbuf+nByteCntFirstChar,iLenString-nByteCntLastChar);

      lpquote = strchr(lpstring,*tchQuote);
      if (lpquote) {
          lpbuf = lpresult;
          lptemp = lpresult;
          while (*lpbuf != EOS) // copy each caracter and remove the double quote was add
          {                     // for display the object name
            CMcpychar(lpbuf,lptemp);
            CMcpychar(lpbuf,lpcharPrev);
            CMnext(lpbuf);
            CMcpychar(lpbuf,lpcharCurrent);
            CMnext(lptemp);
            if (!CMcmpcase(lpcharPrev, tchQuote) && !CMcmpcase(lpcharCurrent, tchQuote))
                CMnext(lpbuf); // don't copy this quote
          }
          *lptemp = EOS;
          return lpresult;
      }
      else
          return lpresult;
   }
   else
      return lpstring;
}


LPCTSTR QuoteIfNeeded(LPCTSTR lpName)
{
   LPTSTR lpDelimitedNecessary;
   lpDelimitedNecessary = x_strpbrk( lpName, LIST_SPECIAL_CHARACTERS_SPECIFIED_IN_DOUBLE_QUOTE);
   if ( !lpDelimitedNecessary )
      return lpName;
   return quoted (lpName);
}

LPCTSTR RemoveSQLQuotes(LPCTSTR lpstring)
{
   LPTSTR lpresult,lpquote,lptemp;
   TCHAR lpcharPrev[2],lpcharCurrent[2];
   LPCTSTR lpbuf;
   int iLenString,nByteCntFirstChar,nByteCntLastChar;

   iunquoted++;
   if (iunquoted>=MAXUNQUOTEDSTRINGS)
      iunquoted=0;
   lpresult = unquotedstrings[iunquoted];
   // remove the quotes at the begin and at the end of the string.
   iLenString = lstrlen(lpstring);
   nByteCntFirstChar = CMbytecnt(lpstring);
   nByteCntLastChar  = CMbytecnt(lpstring+iLenString);

   if (!CMcmpcase(lpstring,tchQuote) && !CMcmpcase(lpstring+iLenString-nByteCntLastChar,tchQuote))
   {
      lpbuf = lpstring;
      fstrncpy(lpresult,(LPTSTR)lpbuf+nByteCntFirstChar,iLenString-nByteCntLastChar);

      lpquote = strchr(lpstring,*tchQuote);
      if (lpquote) {
          lpbuf = lpresult;
          lptemp = lpresult;
          while (*lpbuf != EOS) // copy each caracter and remove the double quote was add
          {                     // for display the object name
            CMcpychar(lpbuf,lptemp);
            CMcpychar(lpbuf,lpcharPrev);
            CMnext(lpbuf);
            CMcpychar(lpbuf,lpcharCurrent);
            CMnext(lptemp);
            if (!CMcmpcase(lpcharPrev, tchQuote) && !CMcmpcase(lpcharCurrent, tchQuote))
                CMnext(lpbuf); // don't copy this quote
          }
          *lptemp = EOS;
          return lpresult;
      }
      else
          return lpresult;
   }
   else
      return lpstring;
}

/******************************************************************
* Function :   Escaped all simple quote in string                 *
*                                                                 *
*   Parameters :                                                  *
*       lpDBEventText : Address of string to be analyzed          *
*                                                                 *
*   Returns :                                                     *
*       RES_SUCCESS                                               *
*       RES_ERR when memory allocation failed                     *
******************************************************************/
int EscapedSimpleQuote(TCHAR **lpDBEventText)
{
    int nbQuote = 0;
    TCHAR *tcCurrQuote, *tcCurEventText, *tcNewBuffer,*tcOldEventText,*tcNewEventText;

    if (!lpDBEventText)
        return RES_SUCCESS;

    tcCurEventText = *lpDBEventText;
    /*while (tcCurrQuote = STchr(tcCurEventText,('\'')))*/
	while (tcCurrQuote = STchr(tcCurEventText,('\'')))
    {
        tcCurEventText  = tcCurrQuote + sizeof(TCHAR) ;
        nbQuote++;
    }

    if (!nbQuote)
        return RES_SUCCESS;

    tcNewBuffer = ESL_AllocMem (((STlength(*lpDBEventText) + nbQuote) * sizeof(TCHAR)) + sizeof(TCHAR));
    if (!tcNewBuffer)
        return RES_ERR;

    tcOldEventText = *lpDBEventText;
    tcNewEventText = tcNewBuffer;

    while(*tcOldEventText != ('\0'))
    {
        /*_tccpy(tcNewEventText,tcOldEventText);*/
		x_strcpy(tcNewEventText,tcOldEventText);
        if (*tcNewEventText == ('\''))
        {
            /*tcNewEventText = _tcsinc(tcNewEventText);*/
			tcNewEventText = tcNewEventText+1;
            /*_tccpy(tcNewEventText,("'"));*/
			x_strcpy(tcNewEventText,("'"));
        }

        /*tcNewEventText = _tcsinc(tcNewEventText);
        tcOldEventText = _tcsinc(tcOldEventText);*/
		tcNewEventText = tcNewEventText+1;
        tcOldEventText = tcOldEventText+1;
    }
    *tcNewEventText = ('\0');

    ESL_FreeMem (*lpDBEventText);
    *lpDBEventText = tcNewBuffer;
    return RES_SUCCESS;
}

LPVOID ESL_AllocMem(uisize) /* specified to fill allocated area with zeroes */
UINT uisize;
{
   LPVOID lpv;

   lpv = malloc(uisize);
   if (lpv)
   {
      memset(lpv,'\0',uisize);
   }
   else
      myerror(ERR_ALLOCMEM);
   return lpv;

}
void   ESL_FreeMem (lpArea)
LPVOID lpArea;
{
   free(lpArea);
}

LPVOID ESL_ReAllocMem(lpArea, uinewsize, uioldsize)
LPVOID lpArea;
UINT uinewsize;
UINT uioldsize;
{
   LPVOID lpv = realloc(lpArea, uinewsize);
   if ( lpv && uioldsize>0 && (uinewsize>uioldsize) )
         memset((LPUCHAR)lpv + uioldsize, '\0', (uinewsize-uioldsize));
   return lpv;
}

time_t ESL_time()
{
   return time(NULL);
}

static LPUCHAR lpstaticdisppublic= "(public)";
static LPUCHAR lpstaticsyspublic = "public";

LPUCHAR lppublicdispstring()
{
   return lpstaticdisppublic;
}

LPUCHAR lppublicsysstring()
{
   return lpstaticsyspublic;
}


int myerror(ierr)
int ierr;
{
   char buf[80];
   wsprintf(buf,"error %d", ierr);
   MessageBox( GetFocus(), buf, NULL, MB_OK | MB_TASKMODAL);
   return RES_ERR;
}

BOOL AskIfKeepOldState(iobjecttype,errtype,parentstrings)
int iobjecttype;
int errtype;
LPUCHAR * parentstrings;
{
   return FALSE;  /* if error while refreshing data, the error should now appear in the tree*/
}

BOOL HasOwner(iobjecttype)
int iobjecttype;
{
	if (iobjecttype == OT_DATABASE) {
		if (GetOIVers() == OIVERS_NOTOI )
			return FALSE;
	}

   switch (iobjecttype) {
      case OT_DATABASE        :
      case OT_TABLE           :
      case OT_VIEW            :
      case OT_INDEX           :
      case OT_PROCEDURE       :
      case OT_RULE            :

      case OT_REPLIC_REGTABLE:    // Added Emb Sep. 11, 95

         return TRUE;
      default:
         return FALSE;
   }
}


BOOL IsSystemObject(iobjecttype, lpobjectname, lpobjectowner)
int     iobjecttype;
LPUCHAR lpobjectname;
LPUCHAR lpobjectowner;
{
   TCHAR   tchQuoteDollar [] = {'"', '$', EOS};
   if (HasOwner(iobjecttype)) {
      if (!x_stricmp(lpobjectowner,"$ingres"))
         return TRUE;
   }

   if (iobjecttype==OT_GROUPUSER && !x_stricmp(lpobjectname,"$ingres"))
      return FALSE;        // $ingres in a group should appear
                           // but not be expandable

   if (lpobjectname[0]=='$')
      return TRUE;
   if (! x_strncmp(lpobjectname,tchQuoteDollar,2)) // quoted (system) object name
       return TRUE; 

   switch (iobjecttype) {
      case OT_USER  :
      case OT_SCHEMAUSER  :
         if (!x_stricmp(lpobjectname,"$ingres"))
            return TRUE;
         else
            return FALSE;
         break;
      case OT_TABLE:
      case OT_VIEW:
      case OT_DBEVENT:
         if (!x_strnicmp(lpobjectname,"ii",2)) /* ii tables always considered as system objects */
            return TRUE;
         if (GetOIVers() == OIVERS_NOTOI) 
            return FALSE; /* generic gateways: don't change old logic where dd_ tables are not system objects */

         if (!x_strnicmp(lpobjectname,"dd_",3))  // TO BE DISCUSSED
            return TRUE;
         else
            return FALSE;
         break;
         
      case OT_DATABASE:
		if (!x_stricmp(lpobjectname,"iidbdb"))
			return TRUE;
		break;
	  default:
		break;
   }

   if (!x_strnicmp(lpobjectname,"ii_",3))
      return TRUE;

   return FALSE;
}

BOOL CanObjectHaveSchema(iobjecttype)
int iobjecttype;
{
   switch (iobjecttype) {
      case OT_TABLE:
      case OT_VIEW:
      case OT_INDEX:
      case OT_PROCEDURE:
      case OT_RULE:
      case OT_DBEVENT:
         return TRUE;
         break;
      default:
         return FALSE;
         break;
   }
   return FALSE;
}

static int strcmpignoringquotes(LPCTSTR pS1,LPCTSTR pS2)
{
   LPCTSTR ps11 = RemoveDisplayQuotesIfAny(pS1);
   LPCTSTR ps21 = RemoveDisplayQuotesIfAny(pS2);
   return lstrcmp(ps11,ps21);
}

int DOMTreeCmpDoubleStr(p11,p12,p21,p22)
LPUCHAR p11;
LPUCHAR p12;
LPUCHAR p21;
LPUCHAR p22;
{
  int resutemp = strcmpignoringquotes(p11,p21);
  if (!resutemp) 
    resutemp = strcmpignoringquotes(p12,p22);
  return resutemp;
}

int DOMTreeCmpTableNames(p1,powner1,p2,powner2)
LPUCHAR p1;
LPUCHAR powner1;
LPUCHAR p2;
LPUCHAR powner2;
{
  char *pchar1,*pchar2;
  int icmp;
  int icmp2;
  char buf[MAXOBJECTNAME];

  pchar1=GetSchemaDot(p1);
  pchar2=GetSchemaDot(p2);
  if (pchar1) {
    if (!pchar2) {
      icmp2=strcmpignoringquotes(pchar1+1,p2);
      if (icmp2)
        return icmp2;
      else {
        x_strcpy(buf,p1);
        pchar1=GetSchemaDot(buf);
        *pchar1='\0';
        return strcmpignoringquotes(RemoveDisplayQuotesIfAny(buf),powner2);
      }
    }
    else /* both have a dot */
      return DOMTreeCmpDoubleStr(pchar1+1,p1,pchar2+1,p2);
  }
  else {  /* pchar1 null */
    if (pchar2) {
      icmp2=strcmpignoringquotes(p1,pchar2+1);
      if (icmp2)
        return icmp2;
      else {
        x_strcpy(buf,p2);
        pchar2=GetSchemaDot(buf);
        *pchar2='\0';
        return strcmpignoringquotes(powner1,RemoveDisplayQuotesIfAny(buf));
      }
    }
  }
  icmp = strcmpignoringquotes(p1,p2);
  if (icmp)              /* both are NULL */
    return icmp;
  return strcmpignoringquotes(powner1,powner2);

}

int DOMTreeCmpStr(p1,powner1,p2,powner2,iobjecttype, bEqualIfMixed)
LPUCHAR p1;
LPUCHAR powner1;
LPUCHAR p2;
LPUCHAR powner2;
int iobjecttype;
BOOL bEqualIfMixed;/* TRUE if wish return 0 if one has dot, and other not*/
{
  char *pchar1,*pchar2;
  int icmp ;
  int icmp2;
  if (!CanObjectHaveSchema(iobjecttype))
    return x_strcmp(p1,p2);

  pchar1=GetSchemaDot(p1); //pchar1=strchr(p1,'.');
  pchar2=GetSchemaDot(p2); //pchar2=strchr(p2,'.');
  
  if (pchar1) {
    if (!pchar2) {
      icmp2=strcmpignoringquotes(pchar1+1,p2);
      if (bEqualIfMixed || icmp2)
        return icmp2;
      else 
        return strcmpignoringquotes(powner1,powner2);
    }
    else /* both have a dot */
      //return DOMTreeCmpDoubleStr(pchar1+1,p1,pchar2+1,p2);
      return DOMTreeCmpDoubleStr(pchar1+1,powner1,pchar2+1,powner2);
  }
  else {  /* pchar1 null */
    if (pchar2) {
      icmp2=strcmpignoringquotes(p1,pchar2+1);
      if (bEqualIfMixed || icmp2)
        return icmp2;
    }
  }
  icmp = strcmpignoringquotes(p1,p2);
  if (bEqualIfMixed || icmp)              /* both are NULL */
    return icmp;
  return strcmpignoringquotes(powner1,powner2); 

}

int ESL_GetTempFileName(LPUCHAR buffer, int buflen)
{
   UCHAR szFileId[_MAX_PATH];
#ifdef WIN32
   UCHAR szTempPath[_MAX_PATH];
   if (GetTempPath(sizeof(szTempPath), szTempPath) == 0)
     return RES_ERR;
   if (GetTempFileName(szTempPath, "dba", 0, szFileId) == 0)
     return RES_ERR;
#else
   GetTempFileName(0, "dba", 0, szFileId);
#endif
   if ( (int) x_strlen(szFileId) > buflen-1 )
     return RES_ERR;
   fstrncpy(buffer,szFileId,buflen);
   return RES_SUCCESS;
}

int ESL_FillBufferFromFile(LPUCHAR filename,LPUCHAR buffer,int buflen, int * pretlen, BOOL bAppend0)
{
   HFILE hf;
   UINT uiresult;
   int result = RES_SUCCESS;

   if (bAppend0) {
      buflen--;
      if (buflen<0)
         return RES_ERR;
   }

   hf = _lopen (filename, OF_READ);
   if (hf == HFILE_ERROR)
      return RES_ERR;

   uiresult =_lread(hf, buffer, buflen);
   if (uiresult==HFILE_ERROR)
      result = RES_ERR;

   *pretlen= (int) uiresult;

   if (bAppend0 && result == RES_SUCCESS)
      buffer[uiresult]='\0';
   
   hf =_lclose(hf);
   if (hf == HFILE_ERROR)
        result = RES_ERR;

   return result;
}

int ESL_RemoveFile(LPUCHAR filename)
{
   OFSTRUCT ofFileStruct;
   HFILE hf;
   hf = OpenFile((LPCSTR)filename, &ofFileStruct, OF_DELETE);
   if (hf==HFILE_ERROR)
      return RES_ERR;
   return RES_SUCCESS;
}

