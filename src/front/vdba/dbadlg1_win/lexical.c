/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project  : CA/OpenIngres Visual DBA
**
**    Source   : lexical.c
**
**    TOOLS
**
**    history after 01-May-2002
**
**  14-May-2002 (noifr01)
**  (bug 107773) don't refuse any more international characters (such as
**  accented characters) for the object names (and indirectly leave the
**  DBMS return its own error if invalid characters are found)
********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <malloc.h>
#include "lexical.h"


#define 	DEAD_LOCK_STATE -1
#define 	FINAL_STATE      1

#ifdef DOUBLEBYTE
#include <windows.h>
BOOL	bLastCharLeadByte=FALSE;
#endif

#if 0
static int State_0 (char c)
{
	switch (c)
	{
	case '_' :
	case '$' :
		return(2);
	case '&' :
	case '*' :
	case '@' :
	case ':' :
	case ',' :
	case '#' :
	case '"' :
	case '=' :
	case '/' :
	case '>' :
	case '<' :
	case '(' :
	case ')' :
	case '-' :
	case '%' :
	case '.' :
	case '+' :
	case '?' :
	case ';' :
	case '\'' :
	case ' ' :
	case '|' :
		return (1);

	default  :
#ifdef DOUBLEBYTE
		if (bLastCharLeadByte)
		{
			bLastCharLeadByte=FALSE;
			return(1);
		}
		else if ((bLastCharLeadByte=IsDBCSLeadByte(c)) || (IsCharAlpha(c)))
			return(1);
#else
		if (isalpha ((int)(unsigned char)c))
			return (1);
#endif
		else
			return DEAD_LOCK_STATE;
	}
}



static int State_1 (char c)
{
	switch (c)
	{
	case '#' :
	case '$' :
	case '@' :
	case '_' :
	case '&' :
	case '*' :
	case ':' :
	case ',' :
	case '"' :
	case '=' :
	case '/' :
	case '>' :
	case '<' :
	case '(' :
	case ')' :
	case '-' :
	case '%' :
	case '.' :
	case '+' :
	case '?' :
	case ';' :
	case '\'' :
	case ' ' :
	case '|' :
		return (1);

	default  :
#ifdef DOUBLEBYTE
		if (bLastCharLeadByte)
		{
			bLastCharLeadByte=FALSE;
			return(1);
		}
		else if ((bLastCharLeadByte=IsDBCSLeadByte(c)) || (IsCharAlphaNumeric(c)))
			return(1);
#else
		if (isalnum ((int)(unsigned char)c))
			return (1);
#endif
		else
			return DEAD_LOCK_STATE;
	}
}




static int State_2 (char c) 
{
	switch (c)
	{
	case '_' :
	case '$' :
		return(2);
	case '#' :
	case '@' :
	case '&' :
	case '*' :
	case ':' :
	case ',' :
	case '"' :
	case '=' :
	case '/' :
	case '>' :
	case '<' :
	case '(' :
	case ')' :
	case '-' :
	case '%' :
	case '.' :
	case '+' :
	case '?' :
	case ';' :
	case '\'' :
	case ' ' :
	case '|' :
		return (1);

	default  :
#ifdef DOUBLEBYTE
		if (bLastCharLeadByte)
		{
			bLastCharLeadByte=FALSE;
			return(1);
		}
		else if ((bLastCharLeadByte=IsDBCSLeadByte(c)) || (IsCharAlphaNumeric(c)))
			return(1);
#else
		if (isalnum ((int)(unsigned char)c))
			return (1);
#endif
		else
			return DEAD_LOCK_STATE;
	}
}

#endif


BOOL IsObjectNameValid (const char* object, int object_type)
{
   return TRUE;
#if 0
   int   len   = x_strlen (object);
   int   next  = 0;
   int   state = 0;

#ifdef DOUBLEBYTE
   bLastCharLeadByte=FALSE;
#endif

   while (next <len)
   {
       switch (state)
       {
           case 0: 
               state = State_0 (object [next]);
               break;
           
           case 1:
               state = State_1 (object [next]);
               break;

           case 2:
               state = State_2 (object [next]);
               break;

           case DEAD_LOCK_STATE:
               return FALSE;
       }
       next++;
   }

   if(state == DEAD_LOCK_STATE) // The last caracter is not valid.
       return FALSE;

   switch (object_type)
   {
       case OT_UNKNOWN:
           if (state == FINAL_STATE)
                return (TRUE);
           else
               return (FALSE);

       case OT_DATABASE:
           if (object [0] == '_')
               return (FALSE);
           else
               return (TRUE);

       case OT_TABLE:
           if (((object [0] == 'i') || (object [0] == 'I')) &&
               ((object [1] == 'i') || (object [1] == 'I')))
               return (FALSE);
           else
               return (TRUE);

       case OT_USER:
           if (object [0] == '$')
               return (FALSE);
           else
               return (TRUE);
       default:
           return (FALSE);

   }
#endif
}

