/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Project : CA/OpenIngres Visual DBA
**
**  Source : tools.c
**  environment independant layer tools
**
**  Author : Francois Noirot-Nerin
**
**  History:
**	14-Feb-2000 (somsa01)
**	    Added HP to the list of machines where alignment on a
**	    short boundary is required.
**	02-Oct-2000 (hanch04)
**	    Use a string instead of a char
**  19-May-2000 (noifr01)
**     (bug 99242) created x_strgetlastcharptr function (gets pointer
**     on last non-\0 character in a string. specified to be DBCS compliant)
**  23-May-2000 (schph01)
**     (bug 99242)  cleanup for DBCS compliance
**	21-Feb-2001 (ahaal01)
**	    Added AIX (rs4_us5) to the list of machines where alignment
**	    on a short boundary is required.
**  15-Mars-2001 (schph01)
**     bug 104091 Created EscapedSimpleQuote() function : escaped all the
**     simple quotes find in parameter string. (used for the dbevent text)
**  01-Nov-2001 (hanje04)
**      Add Linux to list of platforms that require 'short' buffer alignment.
**  25-Oct-2004 (noifr01)
**     (bug 113305) x_strpbrk() wasn't handling correctly effective DBCS
**     characters
**  07-Aug-2009 (drivi01)
**	Add pragma to disable deprecated POSIX functions warning
**	4996.
*/
/* Turn off POSIX warning for this file until Microsoft fixes this bug */
#pragma warning(disable: 4996)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dba.h"
#include "main.h"
#include "dbafile.h"
#include "error.h"
#include "compress.h"
#include "assert.h"
#include "tchar.h"

# include <compat.h>
# include <cm.h>
# include <st.h>


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
        STcopy(_T(" "),pu);
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
    defined(rs4_us5) || defined(int_lnx)
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
    defined(rs4_us5) || defined(int_lnx)
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

//
// Low Level Functions for Save/Load environment - Created 27/3/95 Fnn+Emb
//
void InitLLFile4Save()
{
  bSaveCompressed = FALSE;
  bReadCompressed = FALSE;
}

int LLDBAAppend4Save(FILEIDENT fident, void *buf, UINT cb)
{
  size_t  count;
  int     error;

#ifndef NOCOMPRESS
  if (bSaveCompressed)
  {
     return DBACompress4Save(fident, buf, cb);
  }
#endif

  count = (UINT)fwrite(buf, 1, cb, fident);
  if (count < cb) {
    error = ferror(fident);
    // TO BE FINISHED - QUESTIONABLE : use error ???
    return RES_ERR;
  }
  else
    return RES_SUCCESS;
}

int LLDBAAppend4SaveNC(FILEIDENT fident, void *buf, UINT cb)
{
  size_t  count;
  int     error;

  count = (UINT)fwrite(buf, 1, cb, fident);
  if (count < cb) {
    error = ferror(fident);
    // TO BE FINISHED - QUESTIONABLE : use error ???
    return RES_ERR;
  }
  else
    return RES_SUCCESS;
}

int LLDBAAppendChars4Save(FILEIDENT fident, char c, UINT cb)
{
#define LOCSIZE 400
  char buf[LOCSIZE];
  int  ires;

  char * pc = buf;

  if (cb>LOCSIZE) {
    pc = ESL_AllocMem(cb);
    if (!pc)
      return RES_ERR;
  }

  memset(pc,c,cb);
  ires = DBAAppend4Save(fident,pc,cb);
  if (cb>LOCSIZE) {
    ESL_FreeMem(pc);
  }
  return ires;
}

void InitLLFile4Read()
{
  bReadCompressed = FALSE;
  bSaveCompressed = FALSE;
}

BOOL LLDBAisCmprsActive()
{
#ifndef NOCOMPRESS
  if (bSaveCompressed || bReadCompressed)   
    return TRUE;
#endif
  return FALSE;
}


static char * lpHistData = (char *) 0;
static BOOL   bReadInHistData = FALSE;
static int    pos4read;
static int    FirstAvailPos;
static int    histsize =  ENDOFFILEEMPTYCHARS;
static long   posinfile=0;

long LLDBASeekRel(FILEIDENT fident,long cb)
{
   if (cb>0) {
      myerror(ERR_FILE);
      return -1;
   }
   if (!cb)
      return posinfile;
   if (!bReadInHistData) {
      if (cb>histsize-FirstAvailPos) {
         myerror(ERR_FILE);
         return -1;
      }
      bReadInHistData=TRUE;
      pos4read=histsize+(int)cb; // cb is <0
   }
   else {
      if (cb>pos4read-FirstAvailPos) {
         myerror(ERR_FILE);
         return -1;
      }
      pos4read+=(int)cb; // cb is <0

   }
   posinfile+=cb;  // cb is <0
   return posinfile;
}

int LLDBAReadData(FILEIDENT fident, void *bufresu, UINT cb, UINT *pActualCb)
{
  size_t  count;
  int     error;
  int     ires = RES_SUCCESS;

#ifndef NOCOMPRESS
  if (bReadCompressed) {
     if (pActualCb)
        *pActualCb=cb;
     if (!lpHistData) {
       lpHistData= ESL_AllocMem(histsize);
       if (!lpHistData)
          return RES_ERR;
       FirstAvailPos=histsize;
       bReadInHistData = FALSE;
       posinfile=0;
     }
     if (bReadInHistData) {
        int c1;
        if (cb<(UINT)(histsize-pos4read)) 
           c1=cb;
        else {
           c1= histsize-pos4read;
           bReadInHistData=FALSE;
        }
        memcpy(bufresu,lpHistData+pos4read,c1);
        pos4read+=c1;
        bufresu= (char *)bufresu+c1;
        cb-=c1;
        posinfile+=c1;
     }
     if (cb) {
        ires=DBAReadCompressedData(fident, bufresu, cb);
        posinfile+=cb;
        if (cb>=(UINT)histsize) {
           memcpy(lpHistData,(char *)bufresu+(cb-histsize),histsize);
           FirstAvailPos=0;
        }
        else {
           if (FirstAvailPos<(int) cb) {
             memmove(lpHistData,lpHistData+cb,histsize-cb);
             memmove(lpHistData+histsize-cb,bufresu,cb),
             FirstAvailPos=0;
           }
           else { 
             memmove(lpHistData+FirstAvailPos-cb,lpHistData+FirstAvailPos,histsize-FirstAvailPos);
             memmove(lpHistData+histsize-cb,bufresu,cb);
             FirstAvailPos-=cb;
           }
        }
     }
     return ires;
  }
#endif

  count = (UINT)fread(bufresu, 1, cb, fident);
  if (pActualCb)
    *pActualCb = count;   // actual number of bytes read
  if (count < cb) {
    // Replaced emb
    memset (((char *)bufresu) + count, '\0', cb-count);
    // old code : memset(bufresu,'\0',cb);

    if (feof(fident))
      return RES_ENDOFDATA;   // No more data
    else 
      error = ferror(fident);
    // TO BE FINISHED - QUESTIONABLE : use error ???
    return RES_ERR;
  }
  else
    return RES_SUCCESS;
}

int LLDBATerminateFile(FILEIDENT fident)
{
#ifndef NOCOMPRESS
  if (bSaveCompressed || bReadCompressed)  
  {
    CompressFlush(fident);
    CompressFree();
  }
  if (lpHistData) {
    ESL_FreeMem(lpHistData);
    lpHistData = (char *)0;
  }
#endif
    return RES_SUCCESS;
}


LPTSTR GetSchemaDot (LPCTSTR lpstring)
{
   int nbDoubleQuotes;
   LPCTSTR lpCurString;

   if (!_tcschr(lpstring,'.'))
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
	   if (CMbytecnt(ptemp)!=1 || CMbytecnt(_T(" "))!=1) // ':' and ' ' are normally one byte. In case it wouldn't,
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
	char * ptemp=strCharSet;
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




static char locbuf[4][MAXOBJECTNAME];
static int ilocbufpos=0;
static LPUCHAR wildcardsubstring(LPUCHAR lpstring, int pos) /* pos is the string "subsection" number (not an byte index within the string) */
{
   int i;
   LPUCHAR lpresult,lploc;
   ilocbufpos++;
   if (ilocbufpos>=3)
      ilocbufpos=0;
   lpresult=locbuf[ilocbufpos];
   for (i=0;;i++) {
      switch (*lpstring) {
         case '*':
         case '?':
            if (i==pos) {
               fstrncpy(lpresult,lpstring,CMbytecnt(lpstring)+sizeof(TCHAR));
               return lpresult;
            }
            CMnext(lpstring);
            continue;
            break;
         case '\0':
            lpresult[0]='\0';
            return lpresult;
         default:
            lploc= x_strpbrk(lpstring,"*?");
            if (!lploc) {
               if (i==pos) {
                  fstrncpy(lpresult,lpstring,MAXOBJECTNAME);
                  return lpresult;
               }
               else {
                  lpresult[0]='\0';
                  return lpresult;
               }
            }
            if (i==pos) {
               fstrncpy(lpresult,lpstring,(lploc-lpstring)+sizeof(TCHAR));
               return lpresult;
            }
            lpstring=lploc;
      }
   }
   myerror(ERR_WILDCARD);
   return (LPUCHAR)0;
}

BOOL IsStringExpressionCompatible(LPUCHAR lpstring, LPUCHAR lpexpression)
{                        
   LPUCHAR pc1,pcnext;
   int i;

   if (!lpexpression[0])
      return TRUE;

   lpstring=StringWithoutOwner(lpstring); // don't deal with the schema !

   for (i=0;;i++) {
      pc1    = wildcardsubstring(lpexpression, i);
      pcnext = wildcardsubstring(lpexpression, i+1);
      switch (*pc1) {
         case '*':
            switch (*pcnext){
               case '\0':
                  return TRUE;
                  break;
               case '*':
                  break;
               case '?':
                  break;
               default:
                  lpstring=x_stristr(lpstring,pcnext);
                  if (!lpstring)
                     return FALSE;
                  break;
            }
            break;
         case '?':
            if (!(*lpstring))
               return FALSE;
            CMnext(lpstring);
            break;
         case '\0':
            if (*lpstring)
               return FALSE;
            else
               return TRUE;
            break;
         default:
            if (x_strnicmp(pc1,lpstring,x_strlen(pc1)))
               return FALSE;
            lpstring+=x_strlen(pc1);
            break;
      }
   }
   myerror(ERR_WILDCARD);
   return FALSE;
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
   if (_tcschr(lpstring,'.') || _tcschr(lpstring,'\"'))
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

   if (!_tcschr(lpstring,'.')  && !_tcschr(lpstring,'\"'))
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
    while (tcCurrQuote = _tcschr(tcCurEventText,_T('\'')))
    {
        tcCurEventText  = tcCurrQuote + sizeof(TCHAR) ;
        nbQuote++;
    }

    if (!nbQuote)
        return RES_SUCCESS;

    tcNewBuffer = ESL_AllocMem (((_tcslen(*lpDBEventText) + nbQuote) * sizeof(TCHAR)) + sizeof(TCHAR));
    if (!tcNewBuffer)
        return RES_ERR;

    tcOldEventText = *lpDBEventText;
    tcNewEventText = tcNewBuffer;

    while(*tcOldEventText != _T('\0'))
    {
        _tccpy(tcNewEventText,tcOldEventText);
        if (*tcNewEventText == _T('\''))
        {
            tcNewEventText = _tcsinc(tcNewEventText);
            _tccpy(tcNewEventText,_T("'"));
        }
        tcNewEventText = _tcsinc(tcNewEventText);
        tcOldEventText = _tcsinc(tcOldEventText);
    }
    *tcNewEventText = _T('\0');

    ESL_FreeMem (*lpDBEventText);
    *lpDBEventText = tcNewBuffer;
    return RES_SUCCESS;
}
