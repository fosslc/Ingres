/*
** Copyright (c) 1998, 2007 Ingres Corporation
*/

/*
** Name: XADTC.C
**
** Description:
**	Ingres ODBC driver XA TM to MS Distributed Transaction
**	Coordinator (MS DTC) recovery Interface.
**
** History:
**	25-jul-1998 (thoda04)
**	    Initial coding
**	11-oct-2001 (thoda04)
**	    Integrate into 2.8
**	05-jul-2001 (somsa01)
**	    Use xa.h from the normal include directory. Also, cleaned
**	    up compiler warnings.
**      14-feb-2003 (loera01) SIR 109643
**          Minimize platform-dependency in code.  Make backward-compatible
**          with earlier Ingres versions.
**      07-mar-2003 (loera01)
**          Use common definition for ODBC_INI.          
**      05-jun-2003 (loera01)
**          Removed duplicate include of odbcinst.h.
**      22-sep-2004 (loera01)
**          Load "II" DLL's instead of "OI".
**       3-dec-2004 (thoda04) Bug 113565
**          Load "IILIB" DLLs for r3 instead of "II".
**      10-jul-2007 (thoda04)
**          Use Windows Event Logging instead of ERlog.
*/

#include <compat.h>
#include <sql.h>                    /* ODBC Core definitions */
#include <sqlext.h>                 /* ODBC extensions */
#include <sqlstate.h>               /* ODBC driver sqlstate codes */
#include <sqlcmd.h>


#include <cv.h>
#include <er.h>
#include <st.h>
#include <tm.h>
#include <xa.h>

#include <sqlca.h>
#include <idmsxsq.h>                /* control block typedefs */
#include <idmsutil.h>               /* utility stuff */
#include "caidoopt.h"              /* connect options */
#include "idmsoinf.h"               /* information definitions */
#include "idmsoids.h"               /* string ids */
#include <idmseini.h>               /* ini file keys */
#include "idmsodbc.h"               /* ODBC driver definitions             */
#include <odbcinst.h>               /* ODBC configuration defs */

typedef struct  xa_switch_t  IIXA_SWITCH;
IIXA_SWITCH * piixa_switch = NULL;

typedef DWORD XA_SWITCH_FLAGS;
HRESULT __cdecl GetXaSwitch( 
    /* [in] */ XA_SWITCH_FLAGS XaSwitchFlags,
    /* [out] */ struct xa_switch_t __RPC_FAR *__RPC_FAR *ppXaSwitch);

extern
   CRITICAL_SECTION csLibqxa; /* Use to single thread calls to Ingres XA routines
                                since they are not thread=safe yet */
static char * szFuncName="xa_recover";
static char   errlog_xa_info[256]="";

#define IILQshSetHandler          (*lpfnIILQshSetHandler)
#define IILQisInqSqlio            (*lpfnIILQisInqSqlio)

void    WriteDTCerrToEventLog(HRESULT hr, char * t);
void    WriteToEventLog(LPCSTR pszSrcName, LPCSTR message);
void    IILQshSetHandler(int hdlr,int (*funcptr)())=NULL;
void    IILQisInqSqlio(short *indaddr,int isvar, 
                      int type,int len,void * addr, int attrib)=NULL;
int     odxa_errhandler(void);
void    Prologue(char * xafuncname);
void    Epilogue(void);
void    SetErrorHandler(void);
void    UnsetErrorHandler(void);
void    ReformatOpenString(char * old_xa_info, char * new_xa_info, int bPasswords);
char *  GetKeywordAndValue(char *s, char** pkey, char** pvalue);
static HRESULT LoadLibqxaAndLibqLibraries (void);

/*  external functions */
int pwcrypt (LPCSTR, LPSTR);

extern HINSTANCE   hOixantModule; /* Saved module instance handle of iixant.dll  */ 
extern HINSTANCE   hLibqModule;   /* Saved module instance handle of iiembdnt.dll  */ 

/* local XA functions */
int xa_open    (char *, int, long);       /* xa_open function pointer */
int xa_close   (char *, int, long);       /* xa_close function pointer*/
int xa_start   (XID *, int, long);        /* xa_start function pointer */
int xa_end     (XID *, int, long);        /* xa_end function pointer */
int xa_rollback(XID *, int, long);        /* xa_rollback function pointer */
int xa_prepare (XID *, int, long);        /* xa_prepare function pointer */
int xa_commit  (XID *, int, long);        /* xa_commit function pointer */
int xa_recover (XID *, long, int, long);  /* xa_recover function pointer*/
int xa_forget  (XID *, int, long);        /* xa_forget function pointer */
int xa_complete(int *, int *, int, long); /* xa_complete function pointer */

struct xa_switch_t xa_switch= {
    "INGRES VIA ODBC (CAIIOD35)",  /* name of resource manager */
    TMNOFLAGS | TMNOMIGRATE,       /* resource manager specific options */
    0,                             /* version; must be 0       */
    xa_open,
    xa_close,
    xa_start,
    xa_end,
    xa_rollback,
    xa_prepare,
    xa_commit,
    xa_recover,
    xa_forget,
    xa_complete
};

/*
**  GetXaSwitch
**
**  Return pointer to the XA switch.
**
**  This function is implemented and exported by the ODBC XA TM Interface.
**  After the MS XA transaction manager (TM) LoadLibrary's the Ingres ODBC driver,
**  it obtains the function by calling GetProcAddress.  The TM then
**  invokes this function to obtain a pointer to the XA switch.
**
**  On entry: XaSwitchFlags      Any flags that provide information about
**                               the TM making the call.  Currently
**                               XA_SWITCH_F_DTC is the only defined flag.
**            ppXaSwitch      -->to the pointer to XA Switch to be returned.
**
**  Returns:  S_OK            Success
**            E_FAIL          Unable to provide the XA switch
**            E_INVALIDARG    One or more of the parameters are not valid
*/

HRESULT GetXaSwitch(XA_SWITCH_FLAGS XaSwitchFlags,  struct xa_switch_t** ppXaSwitch)
{
    HRESULT hr;

    if (!ppXaSwitch)
       return(E_INVALIDARG);

    hr = LoadLibqxaAndLibqLibraries();
    if FAILED(hr)
       return(hr);

    *ppXaSwitch = &xa_switch;
    return S_OK;
}

int xa_open    (char * xa_info, int rmid, long flags)
{   int rc;
    char new_xa_info[512];

    ReformatOpenString(xa_info, new_xa_info, TRUE);
       /* convert ODBC DSN and info to Ingres format XA OpenString */
    
    Prologue("xa_open");  /* single thread LIBQXA  and set error handler */     
    ReformatOpenString(xa_info, errlog_xa_info, FALSE);  /* xa_info w/o passwords*/
    rc=piixa_switch->xa_open_entry(new_xa_info, rmid, flags);
    Epilogue();
    return(rc);


//    char s[512];
//    memset(s,'\0',sizeof(s));
//    wsprintf(s,"rmid=0x%8.8x   flags=0x%8.8x\n",(long)rmid, flags);
//    strcpy(s,"xa_info=");
//    if (xa_info)
//       strncat(s,xa_info,sizeof(s)-10);
//    else 
//       strcat(s,"<null>");
//    MessageBox(NULL, s, "xa_open", MB_ICONINFORMATION|MB_OK);
//    return XA_OK;
}

int xa_close   (char * xa_info, int rmid, long flags)
{   int rc;

    Prologue("xa_close");  /* single thread LIBQXA  and set error handler */     
    rc=piixa_switch->xa_close_entry(xa_info, rmid, flags);
    Epilogue();
    return(rc);

}

int xa_start   (XID * xid, int rmid, long flags)
{   int rc;

    Prologue("xa_start");  /* single thread LIBQXA  and set error handler */     
    rc=piixa_switch->xa_start_entry(xid, rmid, flags);
    Epilogue();
    return(rc);
}

int xa_end     (XID * xid, int rmid, long flags)
{   int rc;

    Prologue("xa_end");  /* single thread LIBQXA  and set error handler */     
    rc=piixa_switch->xa_end_entry(xid, rmid, flags);
    Epilogue();
    return(rc);
}

int xa_rollback(XID * xid, int rmid, long flags)
{   int rc;

    Prologue("xa_rollback");  /* single thread LIBQXA  and set error handler */     
    rc=piixa_switch->xa_rollback_entry(xid, rmid, flags);
    Epilogue();
    return(rc);
}

int xa_prepare (XID * xid, int rmid, long flags)
{   int rc;

    Prologue("xa_prepare");  /* single thread LIBQXA  and set error handler */     
    rc=piixa_switch->xa_prepare_entry(xid, rmid, flags);
    Epilogue();
    return(rc);
}

int xa_commit  (XID * xid, int rmid, long flags)
{   int rc;

    Prologue("xa_commit");  /* single thread LIBQXA  and set error handler */     
    rc=piixa_switch->xa_commit_entry(xid, rmid, flags);
    Epilogue();
    return(rc);
}

int xa_recover (XID * xids, long count, int rmid, long flags)
{   int rc;

    Prologue("xa_recover");  /* single thread LIBQXA  and set error handler */     
    rc=piixa_switch->xa_recover_entry(xids, count, rmid, flags);
    Epilogue();
    return(rc);
}

int xa_forget  (XID * xid, int rmid, long flags)
{   int rc;

    Prologue("xa_forget");  /* single thread LIBQXA  and set error handler */     
    rc=piixa_switch->xa_forget_entry(xid, rmid, flags);
    Epilogue();
    return(rc);
}

int xa_complete(int * handle, int * retval, int rmid, long flags)
{   int rc;

    Prologue("xa_complete");  /* single thread LIBQXA  and set error handler */     
    rc=piixa_switch->xa_complete_entry(handle, retval, rmid, flags);
    Epilogue();
    return(rc);
}


void Prologue(char * xafuncname)
{
    //MessageBox(NULL, "MSDTC calling Prologue", "XASwitch", MB_ICONSTOP|MB_OK|MB_SETFOREGROUND);

    EnterCriticalSection (&csLibqxa);  /* single thread LIBQXA */
    szFuncName=xafuncname;
    SetErrorHandler();
}

void Epilogue(void)
{
    UnsetErrorHandler();
    LeaveCriticalSection (&csLibqxa);
}

void SetErrorHandler(void)
{
//  EXEC SQL SET_SQL(errorhandler = odxa_errhandler);
       IILQshSetHandler(1, odxa_errhandler);
}

void UnsetErrorHandler(void)
{
//  EXEC SQL SET_SQL(errorhandler = odxa_errhandler);
       IILQshSetHandler(1, NULL);
}

int  odxa_errhandler(void)
{
    char buf[257]="";
    char message[513]="        ::[                , 00000000]: ";
    long error_no = 0;
    char error_text[257];
    SYSTIME     time_now;
    i4  len;

//  EXEC SQL INQUIRE_SQL(:error_no = ERRORNO,:error_text = ERRORTEXT);
        IILQisInqSqlio((short *)0,1,30,4,&error_no,2);
        IILQisInqSqlio((short *)0,1,32,256,error_text,63);

    CVlx((u_i4)error_no, buf);
    STprintf(message, ERx("        ::[%-16s, %s]: "), szFuncName, buf);
    TMnow( &time_now );
    TMstr( &time_now, buf );
    STcat(message, buf);
    STcat(message, " MS DTC Ingres XA Interface: ");

    if (STequal(szFuncName,"xa_open"))  /* dump out xa_open's xa_info string */
       {
         len = (i4)STlength(message);
         STcat(message, errlog_xa_info);
         WriteDTCerrToEventLog(S_OK, message);
         message[len] = '\0';   /* chop back to the message header */
       }

    STcat(message, error_text);

    WriteDTCerrToEventLog(S_OK, message);
    return 0;
}

void    ReformatOpenString(char * old_xa_info, char * new_xa_info, int bPasswords)
{
    char   xa_info_wkbuf[300];  /* hold the old_xa_info here as null-termed tokens */
    char   szVNODEbuf[64];
    char   szDATABASEbuf[64];
    char   szSERVERTYPEbuf[64];
    char * s;
    char * szDSN       =NULL;
    char * szDATABASE  =NULL;
    char * szVNODE     =NULL;
    char * szSERVERTYPE=NULL;
    char * szROLE      =NULL;
    char * szROLEPWD   =NULL;
    char * szUID       =NULL;
    char * szPWD       =NULL;
    char * szGROUP     =NULL;
    char * szOPTIONS   =NULL;
    char * key, * value;      /* pointers to key and value tokens in xa_info_wkbuf*/ 

    memset(xa_info_wkbuf,'\0',sizeof(xa_info_wkbuf));  /* clear background to '\0' */
    strcpy(xa_info_wkbuf,old_xa_info);  /* copy orig xa_info into token buffer */

    s=xa_info_wkbuf;
    while (*s)
       { s=GetKeywordAndValue(s,&key,&value);
         if (key==NULL)
            break;
         if      (strcmp(key,"DSN")==0)             szDSN       =value;
         else if (strcmp(key,"SERVER")==0)          szVNODE     =value;
         else if (strcmp(key,"DATABASE")==0)        szDATABASE  =value;
         else if (strcmp(key,"SERVERTYPE")==0)      szSERVERTYPE=value;
         else if (strcmp(key,"ROLE")==0)            szROLE      =value;
         else if (strcmp(key,"ROLEPWD")==0)         szROLEPWD   =value;
         else if (strcmp(key,"UID")==0)             szUID       =value;
         else if (strcmp(key,"PWD")==0)             szPWD       =value;
         else if (strcmp(key,"GROUP")==0)           szGROUP     =value;
         else if (strcmp(key,"OPTIONS")==0)         szOPTIONS   =value;
       }  /* end while loop */

    if (szDSN)
       { SQLGetPrivateProfileString (szDSN,"Server","",szVNODE=szVNODEbuf, 
                               sizeof(szVNODEbuf),ODBC_INI);
         SQLGetPrivateProfileString (szDSN,"Database","",szDATABASE=szDATABASEbuf, 
                               sizeof(szDATABASEbuf),ODBC_INI);
         SQLGetPrivateProfileString (szDSN,"ServerType","",szSERVERTYPE=szSERVERTYPEbuf, 
                               sizeof(szSERVERTYPEbuf),ODBC_INI);
       }

#ifdef CAIIODXA_PASSWORDS_ARE_ENCRYPTED
    if (szPWD)
		pwcrypt(szUID, szPWD);      /* decrpts the szPWD     */  
    if (szROLEPWD)
		pwcrypt(szROLE, szROLEPWD); /* decrpts the szROLEPWD */
#endif
        
    strcpy(new_xa_info, "INGRES ");  /* start building the new Ingres Open String */

    if (szVNODE  &&  *szVNODE  &&  stricmp(szVNODE,"(local)") != 0)
       {strcat(new_xa_info, szVNODE);
        strcat(new_xa_info, "::");
       }
    if (szDATABASE)
       {strcat(new_xa_info, szDATABASE);
       }
    if (szSERVERTYPE  &&  *szSERVERTYPE)
       {strcat(new_xa_info, "/");
        strcat(new_xa_info, szSERVERTYPE);
       }

    if (!szROLE  &&  !szROLEPWD  &&  !szUID  &&  !szPWD  &&  !szOPTIONS)
       return;   /* if no OPTIONS, all done! */

    strcat(new_xa_info, " options = ");

    if (szROLE)
       {strcat(new_xa_info,"'-R"); 
        strcat(new_xa_info, szROLE);
        if (szROLEPWD)
           {strcat(new_xa_info,"/");
            if (bPasswords)
               strcat(new_xa_info, szROLEPWD);
            else
               strcat(new_xa_info, "********");
           }
        strcat(new_xa_info,"' ");
       }

    if (szUID)
       {strcat(new_xa_info,"'-u"); 
        strcat(new_xa_info, szUID);
        if (szPWD)
           {strcat(new_xa_info,"/");
            if (bPasswords)
               strcat(new_xa_info, szPWD);
            else
               strcat(new_xa_info, "********");
           }
        strcat(new_xa_info,"' ");
       }

    if (szGROUP)
       {strcat(new_xa_info,"'-G"); 
        strcat(new_xa_info, szGROUP);
        strcat(new_xa_info,"' ");
       }

    if (szOPTIONS)
       {if (*szOPTIONS!='\''  &&  *szOPTIONS!='\"')   /* don't wrap in quotes if already quoted */
           strcat(new_xa_info,"'"); 
        strcat(new_xa_info, szOPTIONS);
        if (*szOPTIONS!='\''  &&  *szOPTIONS!='\"')
           strcat(new_xa_info,"' ");
       }

    return;
}

char *  GetKeywordAndValue(char *s, char** pkey, char** pvalue)
{   *pkey=NULL;
    while (*s==' '  ||  *s==',')  /* skip any leading commas or spaces */
       { s++;
         s=ScanPastSpaces(s);
       }
    if (*s=='\0')   return(s);

    *pkey=s;                   /* pkey->"key" in key=value pair */

    while(*s  &&  *s != '=')   /* scan for "=" in "key=value," pair */
       s++;
    *s='\0';                   /* null term the "key" */
    s++;                       /* s->char after the "=" */

    s=ScanPastSpaces(s);
    *pvalue=s;                 /* pvalue->"value" in "key=value, pair" */
    while(*s  &&  *s != ',')   /* scan for "," in "key=value," pair */
       s++;
    *s='\0';                   /* null term the "value" */
    s++;                       /* s->char after the "," */

    return(s);    
}

/*
**  LoadLibqxaAndLibqLibraries
**
**  LoadLibrary iixant.dll or iiembdnt.dll Ingres libraries.
**  Fill in the proc addresses
**
**  On entry: no parms
**
**  Returns:  S_OK if success, E_NOINTERFACE if failure
*/
static HRESULT LoadLibqxaAndLibqLibraries (void)
{ 
    HRESULT hr = S_OK;
    char s[256];
    char es[80];
    DWORD dw;
#define INGODBCDRIVERDTCXAINTERFACE "Ingres ODBC Driver DTC/XA interface "

    if (piixa_switch          &&   /* if already ready resolve, return OK */
        lpfnIILQshSetHandler  &&
        lpfnIILQisInqSqlio)
            return(S_OK);

    if (!hOixantModule)      /* if iixant.dll not loaded yet */
       {hOixantModule=LoadLibrary("iilibxa.dll");  /* libqxa module */
        if (hOixantModule==NULL)         /* if LoadLibrary failed */
           hr = E_NOINTERFACE;
       }

    if (!hLibqModule  &&  SUCCEEDED(hr)) /* if iiembdnt.dll not loaded yet */
       {hLibqModule=LoadLibrary("iilibembed.dll");  /* libq module */
        if (hLibqModule==NULL)           /* if LoadLibrary failed */
           hr = E_NOINTERFACE;
       }

    if (FAILED(hr))
       {dw = GetLastError();
        wsprintf(es, "LoadLibrary error code = %d", dw);
        if (dw == 1154)  strcat(es, " (DLL is not a valid 32-bit DLL)");
        if (dw == 1157)  strcat(es, " (dependent DLL could not be found in the path)");
        if (dw == 126 )  strcat(es, " (DLL could not be found in the path)");
 
        if (!hOixantModule)    /* iixant.dll not found */
           strcpy(s, INGODBCDRIVERDTCXAINTERFACE "could not load iixant.dll. ");
        else
        if (!hLibqModule)      /* iiembdnt.dll not found */
           strcpy(s, INGODBCDRIVERDTCXAINTERFACE "could not load iiembdnt.dll. ");

        strcat(s,es);    /* strcat on the LoadLibrary error code and explanation) */
        WriteDTCerrToEventLog(hr, s);  /* write message to log */

        return(hr); 
       }

    /* LoadLibrary OK.  Resolve items */
    piixa_switch   = (void*) GetProcAddress(hOixantModule,"iixa_switch");
    if (!piixa_switch)
       {WriteDTCerrToEventLog(hr=E_NOINTERFACE,
           INGODBCDRIVERDTCXAINTERFACE "could not resolve iixa_switch");
        return(hr);
       }

    lpfnIILQshSetHandler   = (void*) GetProcAddress(hLibqModule,"IILQshSetHandler");
    if (!lpfnIILQshSetHandler)
       {WriteDTCerrToEventLog(hr=E_NOINTERFACE,
           INGODBCDRIVERDTCXAINTERFACE "could not resolve IILQshSetHandler");
        return(hr);
       }

    lpfnIILQisInqSqlio     = (void*) GetProcAddress(hLibqModule,"IILQisInqSqlio");
    if (!lpfnIILQisInqSqlio)
       {WriteDTCerrToEventLog(hr=E_NOINTERFACE,
           INGODBCDRIVERDTCXAINTERFACE "could not resolve IILQisInqSqlio");
        return(hr);
       }

    return(S_OK);
}


/*
** Name: WriteDTCerrToEventLog
**
** Description:
**      Decode the error return code into a message and
**      report to the Windows Event Log the error message list. 
**
*/
void WriteDTCerrToEventLog(HRESULT hr, char * t)
{
 char    message[512];
 char    workarea[256];
 char *  s;

 switch (hr)
 {
   case E_FAIL:
      s="rc=E_FAIL (General failure; Check the Windows Event Viewer for more information)";
      break;
   case E_INVALIDARG:
      s="rc=E_INVALIDARG (One or more of the parameters are NULL or invalid)";
      break;
   case E_NOINTERFACE:
      s="rc=E_NOINTERFACE (Unable to provide the requested interface)";
      break;
   case E_OUTOFMEMORY:
      s="rc=E_OUTOFMEMORY (Unable to allocate resources for the object)";
      break;
   case E_UNEXPECTED:
      s="rc=E_UNEXPECTED (An unexpected error occurred)";
      break;
   case E_POINTER:
      s="rc=E_POINTER (Invalid handle)";
      break;
   case XACT_E_CONNECTION_DOWN:
      s="rc=XACT_E_CONNECTION_DOWN (Lost connection with DTC Transaction Manager)";
      break;
   case XACT_E_TMNOTAVAILABLE:
      s="rc=XACT_E_TMNOTAVAILABLE (Unable to establish a connection with DTC (MS DTC service probably not started))";
      break;
   case XACT_E_NOTRANSACTION:
      s="rc=XACT_E_NOTRANSACTION (No transaction corresponding to ITRANSACTION pointer)";
      break;
   case XACT_E_CONNECTION_DENIED:
      s="rc=XACT_E_CONNECTION_DENIED (Transaction manager refused to accept a connection)";
      break;
   case XACT_E_LOGFULL:
      s="rc=XACT_E_LOGFULL (Transaction Manager has run out of log space)";
      break;
   case XACT_E_XA_TX_DISABLED:
      s="rc=XACT_E_XA_TX_DISABLED (Transaction Manager has disabled its support "
        "for XA transactions.  See MS KB article 817066 to enable XA and register "
        "\"caiiod35.dll\".)";
      break;
   default:
      s=workarea;
      wsprintf(s,"0x%8.8lX",(long)hr);
 }

 strcpy(message, t);
 if (hr != S_OK)
    {
     strcat(message, ";  ");
     strcat(message,s);   /* write return code message to errlog */
    }

 WriteToEventLog("Ingres ODBC", message);

 return;
}

/*
** Name: WriteToEventLog
**
** Description:
**      Write the message string to the specified event source in the Event Log.
**
*/
void WriteToEventLog(LPCSTR pszSrcName, LPCSTR message)
{
    HANDLE hEventLog;

    hEventLog = RegisterEventSourceA(NULL, pszSrcName);  /* register on local machine */
    if (hEventLog == NULL)  /* could not register the event source for some reason */
        return;

    ReportEventA(hEventLog,    /* event log handle */
        EVENTLOG_ERROR_TYPE,   /* event type */
        0,                     /* event category */
        0,                     /* event identifier */
        NULL,                  /* no security identifier */
        1,                     /* number of substitution strings */
        0,                     /* no raw data */
        &message,              /* message strings */
        NULL);                 /* no raw data */

    DeregisterEventSource(hEventLog);
}


