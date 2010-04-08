/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Project : Ingres Visual DBA
**
**  Source : tools.h
**  environment independant layer tools
**
**  History after 15-May-2000
**
**  20 -May-2000 (noifr01)
**    (bug 99242) added prototype for new  x_strgetlastcharptr function 
**    (provides a pointer on last non \0 character in a string. specified
**     to be DBCS compliant)
**  09-Oct-2000 (schph01)
**    bug 102845 Added the $ sign to the list of characters requiring to quote
**    an object name when generating SQL syntax
**  15-Mars-2001 (schph01)
**    bug 104091 added prototype EscapedSimpleQuote() function.
**  23-Nov-2004 (schph01)
**    bug 113511 Change the define for x_strcmp and x_stricmp,
**    used _tcscoll and _stricoll instead of STCompare for resolves
**    sort problem with Chinese characters.
********************************************************************/
#include <tchar.h>

UCHAR *fstrncpy(UCHAR * puchardest, UCHAR * pucharsrc, int n);
BOOL suppspace(UCHAR *pu);
BOOL suppspacebutleavefirst(UCHAR *pu);
LPUCHAR getsubstring(LPUCHAR pustrings, LPUCHAR pssubstring, int lenresult);
BOOL isempty(LPUCHAR pchar);

#define x_strlen(x) (STlength(x))

// x_strchr is specified for a one byte character, otherwise provides a system error
// (used only for searching for specific keywords)
extern char * x_strchr(const char *buf, int ichar);
extern char * x_strrchr( const char *buf, int ichar);

extern LPUCHAR x_stristr(LPUCHAR lpstring, LPUCHAR lpString2Find);
extern char * x_strstr (const char * lpstring, const char * lpString2Find);
extern char * x_strpbrk( const char *string, const char *strCharSet );
extern char * x_strncat( char *pdest, const char *psrc, int n );
extern char * x_strgetlastcharptr(char * pstring);



#define x_strcmp(x,y)	  ( _tcscoll((x), (y)))
#define x_stricmp(x,y)  ( _tcsicoll((x), (y)))

int x_strnicmp( const char *str1, const char *str2, int len);
int x_strncmp ( const char *str1, const char *str2, int len);

/* TO BE FINISHED in second step of DBCS porting: remove remaining calls to x_strncpy,
   because of the ambiguity of whether a trailing \0 is inserted or not within the
   result string. Temporary, the function is mapped back to strncpy because of a couple
   of places where it assumed the behavior without the \0. All such places will need a
   fix for DBCS support */
#define x_strncpy(x,y,l) (strncpy((x),(y),(l)))
//#define x_strncpy(x,y,l) (lstrcpyn((x),(y),(l)))
#define x_strcpy(dest,src) (lstrcpy((dest),(src)))
#define x_strcat(x,y) (STcat( (x), (y) ))

void storeint(LPUCHAR lpstring, int i);
int getint(LPUCHAR lpstring);

LPUCHAR StringWithOwner(LPUCHAR lpstring, LPUCHAR lpowner, LPUCHAR bufresult);
LPUCHAR StringWithoutOwner(LPUCHAR lpstring);
LPUCHAR OwnerFromString(LPUCHAR lpstring, LPUCHAR bufresult);

int GetIntFromStartofString(LPUCHAR lpstring, LPUCHAR bufresult);
void FillVnodeandDbtargetFromString(LPUCHAR TargetNode,LPUCHAR TargetDB,LPUCHAR szName);

BOOL IsStringExpressionCompatible(LPUCHAR lpstring, LPUCHAR lpexpression);

void ourString2Upper(LPUCHAR pc);
void ourString2Lower(LPUCHAR pc);

int StartLinesWithTabs(LPUCHAR buffer);
LPTSTR  GetSchemaDot (LPCTSTR lpstring);

#define LIST_VALID_SPECIAL_CHARACTERS_4_OBJ_NAME "#@$_&*:,\"=/<>()-%.+?;' |"
#define LIST_SPECIAL_CHARACTERS_SPECIFIED_IN_DOUBLE_QUOTE "$&*:,\"=/<>()-%.+?;' |"

LPCTSTR QuoteIfNeeded(LPCTSTR lpstring);

LPCTSTR Quote4DisplayIfNeeded(LPCTSTR lpstring);
LPCTSTR RemoveDisplayQuotesIfAny(LPCTSTR lpstring);
LPCTSTR RemoveSQLQuotes(LPCTSTR lpstring);

int EscapedSimpleQuote(TCHAR **lpDBEventText);

