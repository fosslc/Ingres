/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : strlist.h : header file
**    Project  : Data Loader 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : list support
**
** History:
**
** xx-Dec-2003 (uk$so01)
**    Created.
*/

#if !defined (LISTCHAR_HEADER)
#define LISTCHAR_HEADER

char stack_top(char* stack);
void stack_PushState(char* stack, char state);
void stack_PopState(char* stack);
char PeekNextChar (const char* lpcText);


typedef struct tagLISTCHAR
{
	char* pItem;
	struct tagLISTCHAR* next;
	struct tagLISTCHAR* prev;

} LISTCHAR;

int ListChar_Count  (LISTCHAR* oldList);
LISTCHAR* ListChar_Add  (LISTCHAR* oldList, char* szString);
LISTCHAR* ListChar_Done (LISTCHAR* oldList);
LISTCHAR* AddToken(LISTCHAR* pListToken, char* strToken);

/*
** TrimLeft, TrimRight need buffer !!!
** You should not passe the string literal directly to the function !
** EX: char* t = TrimLeft("   456"); will result a GPF but
**     char buff[] = "   456";
**     char* t = TrimLeft(buff); OK.
*/
char* TrimLeft(char* strText);
char* TrimRight(char* strText);
/*
** ConcateStrings:
**
* The function that concates strings (ioBuffer, aString, ..) 
** 
** Parameter: 
**     1) ioBuffer: In Out Buffer 
**     2) aString : String to be concated.
**     3) ...     : Ellipse for other strings...with NULL terminated (char*)0 
** Return:
**     return  1 if Successful.
**     return  0 otherwise.
*/
int ConcateStrings (char** ioBuffer, char* aString, ...);

int ExtractValue (char* pToken, int nKeyLen, char* szValue);

/*
** Tokens are separated by a space or quote:
** Ex1: "ABC=WXX /BN""X"/CL --> 2 token (inside the quote, the "" is equivalent to \")
**      1) "ABC=WXX /BN""X"
**      2) /CL
** Ex2: MAKE IT -->
**      1) MAKE
**      2) IT
*/
LISTCHAR* MakeListTokens(char* zsText, int* pError);


#endif /* LISTCHAR_HEADER */

