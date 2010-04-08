/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : msghandl.c
//
//    TOOLS,   handle the message box
//
**  09-Mar-2010 (drivi01)
**	SIR 123397
**	Add function ErrorMessage2 which does the same thing
**	as ErrorMessage except takes an error in a form of a
**	string instead of an integer.
********************************************************************/


#include "dll.h"
#include "msghandl.h"
#include "windows.h"
#include "dba.h"
#include "main.h"
#include "resource.h"
#include "er.h" //for ER_MAX_LEN define



void WarningMessage  (unsigned int message_id)
{
   HWND CurrentFocus;
   char MessageString [400];
   int  nByte, resp;

   nByte = LoadString (hResource, (unsigned int)message_id, MessageString, sizeof (MessageString));

   if (nByte <= 0)
   {
       wsprintf (MessageString, "Message not defined !. id = %u ", message_id);
   }

   CurrentFocus = GetFocus ();
   resp = MessageBox (NULL, MessageString, 
                            ResourceString(IDS_TITLE_WARNING), MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
   SetFocus (CurrentFocus);
}

int ConfirmedMessage (unsigned int message_id)
{
   HWND CurrentFocus;
   char MessageString [400];
   int  nByte, resp;

   nByte = LoadString (hResource, (unsigned int)message_id, MessageString, sizeof (MessageString));

   if (nByte <= 0)
   {
       wsprintf (MessageString, "Message not defined !. id = %u ", message_id);
   }

   CurrentFocus = GetFocus ();
   resp = MessageBox (NULL, MessageString, ResourceString(IDS_TITLE_CONFIRMATION), MB_ICONQUESTION | MB_YESNO | MB_TASKMODAL);
   SetFocus (CurrentFocus);
  
   return (resp);
}


int ConfirmedDrop (unsigned int message_id, unsigned char* object)
{
   HWND CurrentFocus;

   char szFormat [100];
   char MessageString [400];
   int  nByte, resp;

   nByte = LoadString (hResource, (unsigned int)message_id, szFormat, sizeof (szFormat));

   if (nByte > 0 && object )
   {
       wsprintf (MessageString, szFormat, object);

   }
   else
   {
       if (object)
           wsprintf (MessageString, "Message not defined !. id = %u ", message_id);
   }

   CurrentFocus = GetFocus ();
   resp = MessageBox (CurrentFocus, MessageString, NULL, MB_ICONQUESTION |MB_OKCANCEL);
   SetFocus (CurrentFocus);
   return (resp);
}


void   InformedMessage (unsigned int message_id)
{
   char MessageString [400];
   HWND CurrentFocus;

   LoadString (hResource, message_id, MessageString, sizeof (MessageString));
   CurrentFocus = GetFocus ();
   MessageBox (NULL, MessageString, NULL, MB_ICONEXCLAMATION | MB_TASKMODAL);
   SetFocus (CurrentFocus);

}

//#define DISPLAY_SQLERR_MSG
#ifdef DISPLAY_SQLERR_MSG
# include "C:\INGRES\FILES\EQSQLCA.H"
    extern IISQLCA sqlca;   /* SQL Communications Area */
#endif // DISPLAY_SQLERR_MSG

#include "dgerrh.h"     // MessageWithHistoryButton

void ErrorMessage (UINT message_id, int reason)
{
   char MessageString [200];
   char Reason [80];
   char Message[200];
   HWND CurrentFocus;

   switch (reason)
   {
       case RES_ERR:
           x_strcpy (Reason, "");

           // Trial EMB
           #ifdef DISPLAY_SQLERR_MSG
           // Note : under debugger, we see that sqlcode has been reset
           // in the ReleaseSession/Rollback call
           // Also, the error text can be truncated - need full length
           if (sqlca.sqlcode < 0)
           wsprintf ( Reason, "error %ld : %s",
                      sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
           #endif
           break;
       case RES_TIMEOUT:
           LoadString (hResource, (UINT)IDS_I_TIMEOUT, (LPSTR)Reason, sizeof (Reason));
           break;
       case RES_NOGRANT:
           LoadString (hResource, (UINT)IDS_I_NOGRANT, (LPSTR)Reason, sizeof (Reason));
           break;
       case RES_ALREADYEXIST:
           LoadString (hResource, (UINT)IDS_I_OBJECT_ALREADY_EXIST, (LPSTR)Reason, sizeof (Reason));
           break;
       default:
           x_strcpy (Reason, "");
           break;
   }

   ZEROINIT (MessageString);
   if (message_id == (UINT)IDS_E_CANNOT_ALLOCATE_MEMORY)
       x_strcpy (MessageString, "Cannot allocate memory.");
   else
       LoadString (hResource, (UINT)message_id, MessageString, sizeof (MessageString));

   if (x_strlen (Reason) > 0)
       wsprintf (Message, "%s\n%s", Reason, MessageString);
   else
       wsprintf (Message, "%s", MessageString);

   CurrentFocus = GetFocus ();
   if (message_id == (UINT)IDS_E_CANNOT_ALLOCATE_MEMORY)
       MessageBox (NULL, Message, NULL, MB_ICONHAND|MB_SYSTEMMODAL|MB_OK);
   else {
       // MessageBox (NULL, Message, NULL, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
       MessageWithHistoryButton(CurrentFocus, Message);
   }
   SetFocus   (CurrentFocus);
}

#include "dbaset.h"
void ErrorMessage2 (char* message, char* buf)
{
   char Message[ER_MAX_LEN];
   HWND CurrentFocus;

   wsprintf (Message, "%s", message);

   CurrentFocus = GetFocus ();
   AddSqlError(message, buf, -1);
   MessageWithHistoryButton(CurrentFocus, Message);
   SetFocus   (CurrentFocus);
}



void   ErrorMessagr4Object  (unsigned int error_type)
{



}


void   MsgInvalidObject (unsigned int message_id)
{
   char MessageString [400];
   HWND CurrentFocus;

   LoadString (hResource, message_id, MessageString, sizeof (MessageString));
   CurrentFocus = GetFocus ();
   MessageBox (NULL, MessageString, NULL, MB_ICONEXCLAMATION | MB_TASKMODAL | MB_OK);
   SetFocus (CurrentFocus);
}

