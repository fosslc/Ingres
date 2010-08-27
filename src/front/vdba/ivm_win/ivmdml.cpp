/*
**  Copyright (C) 2005-2010 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : ivmdml.cpp , Implementation File
**
**  Project  : Ingres II / Visual Manager
**
**  Author   : Sotheavut UK (uk$so01)
**
**
**  Purpose  : Data Manipulation Langage of IVM
**
**  History:
**	21-Dec-1998 (uk$so01)
**	    Created.
**	??-???-???? (noifr01)
**	    Implemented the code connecting to real data:
**	    (to be moved to a new source)
**	01-Feb-2000 (noifr01)
**	    (bug 100284) if IVM_GetFileSize() returns a value of (-1), which
**	    normally can happen only if the path or file doesn't exist,
**	    consider the lenght as 0, i.e consider (in terms of Ingres
**	    messages) as if the file was existing but empty.
**	02-Feb-2000 (noifr01)
**	    (bug 100309) under Windows 98, the file size returned by the OS,
**	    gotten in IVM through the _stat function, is not significant of
**	    the data that are really there. Replaced the IVM_GetFileSize()
**	    funtion call, in 95/98 environmnents, by a call to a new
**	    function that opens the file, and seeks to the end to get the
**	    size of available data.
**	02-Feb-2000 (noifr01)
**	    (bug 100315) added the IsRmcmdServer() function.
**	02-Feb-2000 (noifr01)
**	    (bug 100315) removed a test whether the name server is started
**	    (in the "external access" dialog) since the dialog is not
**	    invoked if it is stopped.
**	04-feb-2000 (somsa01)
**	    Removed extra define of 'result' in GetFileStatus(). Also,
**	    in the case of MAINWIN use open() instead of sopen(). Also,
**	    in IVM_RunProcess(), removed the "#ifdef MAINWIN" stuff.
**  31-Jul-2000 (noifr01)
**      -(bug 102225) get rid of garbage empty error messages that were
**        added (between 2 real messages) in the case where the 0D 0A
**        string terminating sequence (under NT) was positioned with the
**        0D at the end of a 8192 characters block (in errlog.log), and the
**        0A at the first position of the next block. 
**      -Misc related cleanup:
**       - changed an internal test that was done incorrectly , resulting
**         in the minor, internal only side effect of having the (internal) 
**         number of messages to be read at once to be 2501 instead of 2500
**         (this fix makes debugging easier).
**       - increased by 1 the size of the tchbuf buffer, and explicitly null
**         terminate it (although in theory not strictly required (static
**         buffer)). This buffer (receiving the raw data from errlog.log) 
**         is scanned through the strpbrk function for searching
**         messages ends, and was probably previously null terminated 
**         "by chance" by the presence of the static tchprevbuf buffer that 
**         immediatly followed it and was unused (data that would have been
**         found after the end of tchbuf would not have been taken into account
**         given the code, but the possibility of scanning after the end was to
**         be removed).
**         Also removed that unused tchprevbuf buffer and the code that filled
**         it (the data was not used)
**  02-Aug-2000 (uk$so01)
**       bug 99242 Handle DBCS
**	06-oct-2000 (somsa01)
**	    In AddEventToList(), we were not correctly copying over the
**	    server id from the errlog.log error message.
**	20-oct-2000 (somsa01)
**	    In AddEventToList(), we were not checking for 'I_' and 'S_'
**	    messages.
**  18-Jan-2001 (noifr01)
**      (SIR 103727) manage JDBC server
**  18-Jan-2001 (uk$so01)
**    SIR #103727 (Make IVM support JDBC Server)
**    Create the tree Item data JDBC Folder
**  12-Fev-2001 (noifr01)
**    bug 102974 (manage multiline errlog.log messages)
**  05-Apr-2001 (uk$so01)
**    Add new function INGRESII_QueryInstallationID() whose body codes are moved from
**    mainview.cpp.
**  19-Jun-2001 (noifr01)
**    (sir 105046) figure out the server class name from the corresponding
**    E_CL2530_CS_PARAM line, and add it in the "special instances list"
**    information. 
**    Don't provide the "dependencies" error messages upon shutdown of a DBMS
**    server if the server doesn't have the INGRES server class.
**  01-nov-2001 (somsa01)
**    CLeaned up 64-bit compiler warnings.
**  20-Mar-2002 (noifr01)
**    (buf 107366) in the special management for the transaction log tree branches, 
**    the name of the branch was not updated in sztypesingle in the COMPONENTINFO
**    structure
**  28-May-2002 (noifr01)
**   (sir 107879) moved the Get95FileSize() function (and have it called directly by
**   IVM_GetFileSize() )
**  03-Oct-2002 (noifr01)
**    (SIR 107587) have IVM manage DB2UDB gateway (mapping the GUI to
**    oracle gateway code until this is moved to generic management).
**  20-Nov-2002 (noifr01)
**    (bug 109143) management of gateway messages
** 27-Feb-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
** 26-Jun-2003 (schph01)
**    SIR #110471 Add II_HALF_DUPLEX, II_STAR_LOG, DD_RSERVERS parameters in
**    CHARxINTxTYPExMINxMAX structure.
** 02-Feb-2004 (noifr01)
**    removed any unneeded reference to iostream libraries, including now
**    unused internal test code 
** 09-Mar-2004 (uk$so01)
**    SIR #110013 (Handle the DAS Server)
** 10-Nov-2004 (noifr01)
**    (bug 113412) replaced usage of hardcoded strings (such as "(default)"
**    for a configuration name), with that of data from the message file
** 14-Apr-2007 (drivi01)
**    Fixed a bug in AddEventToList function.  The function looks at certain
**    server types to establish if server can have additional instances
**    the server type for DAS server was referenced incorrectly, the function
**    was looking for DASVR server type instead of GCD.
** 25-Jul-2005 (drivi01)
**    For Vista, update code to use winstart to start/stop servers
**    whenever start/stop menu is activated.  IVM will run as a user
**    application and will elevate privileges to start/stop by
**    calling ShellExecuteEx function, since ShellExecuteEx can not
**    subscribe to standard input/output, we must use winstart to
**    stop/start ingres.  Added checks to make sure that individual 
**    servers can not be started if service isn't running.
** 05-May-2008 (drivi01)
**    Add IVM_IsDBATools function that will analyze the installation
**    and determine if it's a DBA Tools installation.
** 07-May-2008 (drivi01)
**    Remove Message boxes used for debugging purposes from the code.
** 18-Aug-2010 (lunbr01)  bug 124216
**    Remove code that depended on the local server listen addresses
**    being in the format used for named pipes, and instead use the
**    code for Unix that doesn't depend on the format of the listen
**    addresses. This is needed on Windows to support using protocol
**    "tcp_ip" as the local IPC rather than the default named pipes.
**    IVM code should not depend on the format of the listen addresses.
**    "#ifdef MAINWIN" had been used to generate the Unix version of
**    the code.  Hence, the fix is to keep the MAINWIN code and strip
**    out the non-MAINWIN code that was used to detect and parse pipe
**    names.
*/

#include "stdafx.h"
#include "mainfrm.h"
#include "maindoc.h"
#include "compcat.h"
#include "ivmdisp.h"
#include "ivmdml.h"
#include "ivm.h"
#include "ll.h"
#include "regsvice.h"
#include "wmusrmsg.h"
#include "getinst.h"
#include "readflg.h"
#include "evtcats.h"
#include "vnodedml.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>

extern "C"
{
# include <compat.h>
# include <cm.h>
# include <st.h>
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/* "(default)" not localized in errlog.log.use English string here*/
static char *EngDefConfname = "(Default)";
char *GetEngDefConfName() {	return EngDefConfname; }

static UINT CountSize (CTypedPtrList<CObList, CaLoggedEvent*>* pListEvent)
{
	if (!pListEvent)
		return 0;
	if (pListEvent->GetCount() == 0)
		return 0;

	UINT nSize = 0;
	CaLoggedEvent* pEvent = NULL;
	POSITION pos = pListEvent->GetHeadPosition();
	while (pos != NULL)
	{
		pEvent = pListEvent->GetNext (pos);
		nSize += pEvent->GetSize();
	}
	return nSize;
}

//
// The second one must fill up:
//   pListEvent, the list of events that match the categories specified in the settings:
//   pQueryEventInfo, the information about the query of events (See detail of CaQueryEventInformation)
static void ReadSampleFile2 (
	CTypedPtrList<CObList, CaLoggedEvent*>* pListEvent, 
	CaQueryEventInformation* pQueryEventInfo)
{
}



static BOOL GetFileStatus (CString& fileName, struct _stat* buffer)
{
	int result = _tstat( fileName, buffer);
	if (result != 0)
		return FALSE;

	return TRUE;
}



BOOL IVM_llinit()
{
	//
	// Simulation mode:
	if (theApp.m_nSimulate & SIMULATE_COMPONENT)
		return TRUE;
	//
	// Real mode:
	return VCBFllinit();
}

BOOL IVM_llterminate()
{
	//
	// Simulation mode:
	if (theApp.m_nSimulate & SIMULATE_COMPONENT)
		return TRUE;
	//
	// Real mode:
	return VCBFllterminate();
}

//
// Dummy function !!!
BOOL VCBFRequestExit()
{
	return TRUE;
}
// End Dummy function !!!


//
// Return TRUE if the Component Tree need to be updated:
BOOL IVM_llIsComponentChanged()
{
	//
	// TODO: Implement the code !

	return TRUE;
}

#define MAXRETURNED          2500
#define PREVBUFFERSIZE       1000
#define EACHBUFFER           8192
static long lbufferstartpos = 0L;
static long buf0or1         = 0L;
static long lnextpos2read   = 0L;
static long lnextpos2handle = 0L;
static long llastposread=0L;
static long llastfilesize = 0L;
static BOOL bMoreInMemory = FALSE;
TCHAR  tchbuf[2 * EACHBUFFER +1];
long  lLastEvent = -1;
long  lFirstEvent= -1;
long  lExceed    =  0;
BOOL  bLimitReached = FALSE;
static long lID0 = -1l;
static long lprevFileSize = 0L;
static BOOL blogfilemessageboxdislayed = FALSE;
MSGCLASSANDID lastMessageClassAndID = { CLASS_OTHERS,  MSG_NOCODE };
TCHAR bufLastCompType[200]; // "installation", "dbms", etc...
TCHAR bufLastCompName[200]; // "default", etc...
TCHAR bufLastInstance[100];
TCHAR bufLastClass[100];    // server class



// file consistency check information
#define FILECHANGE_CHECK_BYTES 4
static BOOL bCheckPreviousFileState = FALSE;
static BOOL bcheckinfoready = FALSE;
static long lposcheckbuffer;
static TCHAR checkbuffer[FILECHANGE_CHECK_BYTES];

void IVM_ll_RestartLFScanFromStart() {
	lbufferstartpos = 0L;
	buf0or1         = 0L;
	lnextpos2read   = 0L;
	lnextpos2handle = 0L;
	llastposread=0L;
	llastfilesize = 0L;
	bMoreInMemory = FALSE;
	lLastEvent = -1;
	lFirstEvent= -1;
	lExceed    =  0;
	bLimitReached = FALSE;
	lID0 = -1l;
	lprevFileSize = 0L;
	bcheckinfoready = FALSE;
	blogfilemessageboxdislayed = FALSE;
	bGetInstMsgSpecialInstanceDisplayed = FALSE;
	lastMessageClassAndID.lMsgClass  = CLASS_OTHERS;
	lastMessageClassAndID.lMsgFullID = MSG_NOCODE;
	_tcscpy(bufLastCompType,_T(""));
	_tcscpy(bufLastCompName,_T(""));
	_tcscpy(bufLastInstance,_T(""));
	_tcscpy(bufLastClass,_T(""));
	theApp.GetSpecialInstanceInfo().CleanUp(); // reset special instance list since we are rescanning the errlog.log
}

// TO BE DONE: try to see if file could remain open between the tries


#define READERRLOGERROR (-1L)
#define READERRLOGINCONSISTENT (-2L)
static long ReadErrLogBuffer( char * buffer, long lstartpos, long ilen)
{
	char filename[256];
	long lres;
	int inumread,i;
	strcpy(filename, (char *) (LPCTSTR) theApp.m_strLoggedEventFile);
#ifdef MAINWIN  // mainsoft
	int ihdl= _open (filename,_O_RDONLY | _O_BINARY );
#else
	int ihdl= _sopen (filename,_O_RDONLY | _O_BINARY  ,_SH_DENYNO );
#endif
	if (ihdl<0)
		return READERRLOGERROR;

	if (bCheckPreviousFileState && bcheckinfoready) {
		char bufloc[FILECHANGE_CHECK_BYTES];
		bCheckPreviousFileState = FALSE;
		bcheckinfoready = FALSE;

		lres =_lseek( ihdl, lposcheckbuffer, SEEK_SET );
		if (lres<0 || lres!=lposcheckbuffer) {
			_close( ihdl );
			return READERRLOGINCONSISTENT; // should normally happen only if the file has been changed
		}
		inumread = _read( ihdl, bufloc, FILECHANGE_CHECK_BYTES );
		if (inumread != FILECHANGE_CHECK_BYTES) {
			_close( ihdl );
			return READERRLOGINCONSISTENT; // should normally happen only if the file has been changed
		}
		if ( memcmp(bufloc,checkbuffer,FILECHANGE_CHECK_BYTES)!=0) {
			_close( ihdl );
			return READERRLOGINCONSISTENT; // should normally happen only if the file has been changed
		}
	}

	lres =_lseek( ihdl, lstartpos, SEEK_SET );
	if (lres<0 || lres!=lstartpos) {
		_close( ihdl );
		return READERRLOGERROR;
	}
	inumread = _read( ihdl, buffer, ilen );
	_close( ihdl );

	if (!bcheckinfoready && inumread>FILECHANGE_CHECK_BYTES) {
		lposcheckbuffer = lstartpos;
		memcpy(checkbuffer,buffer,FILECHANGE_CHECK_BYTES);
		bcheckinfoready = TRUE;

	}
	for (i=0;i<inumread;i++) { // NULL chars will be displayed as spaces. allows to handle '\0' terminated strings
		if (buffer[i] =='\0' || buffer[i] == '\t') // DBCS OK given the ranges of leading and trailing bytes
			buffer[i]=' ';
	}
	return (long)inumread;
	
}

BOOL SerializeLogFileCheck(CArchive& ar)
{
	if (ar.IsStoring()) {
		ar << bcheckinfoready;
		if (bcheckinfoready) {
			ar << lposcheckbuffer;
			ar.Write( (void*) checkbuffer, FILECHANGE_CHECK_BYTES );
		}
		return TRUE;
	}
	else {  // read and check if file still OK
		BOOL bShouldTest;
		long lpos1;
		char buf_arch[FILECHANGE_CHECK_BYTES];
		UINT uiread;

		theApp.SetMarkAllAsUnread_flag(FALSE);

		ar >> bShouldTest;
		if (!bShouldTest)
			return TRUE;

		ar >>lpos1;
		uiread = ar.Read( buf_arch, FILECHANGE_CHECK_BYTES );

		BOOL bResult = FALSE;
		while (TRUE) {
			if (uiread < FILECHANGE_CHECK_BYTES)
				break;

			char filename[256];
			long lres;
			int inumread;
			strcpy(filename, (char *) (LPCTSTR) theApp.m_strLoggedEventFile);
#ifdef MAINWIN  // mainsoft
			int ihdl= _open (filename,_O_RDONLY | _O_BINARY);
#else
			int ihdl= _sopen (filename,_O_RDONLY | _O_BINARY  ,_SH_DENYNO );
#endif
			if (ihdl<0)
				break;

			lres =_lseek( ihdl, lpos1, SEEK_SET );
			if (lres<0 || lres!=lpos1) {
				_close( ihdl );
				break;
			}

			char buf2[FILECHANGE_CHECK_BYTES];
			inumread = _read( ihdl, buf2, FILECHANGE_CHECK_BYTES );
			if (inumread != FILECHANGE_CHECK_BYTES) {
				_close( ihdl );
				break;
			}
			if ( memcmp(buf_arch,buf2,FILECHANGE_CHECK_BYTES)!=0) {
				_close( ihdl );
				break;
			}
			bResult = TRUE;
			break;
		}
		if (!bResult)
			theApp.SetMarkAllAsUnread_flag(TRUE);
		return bResult;
	}
}

static char *fstrncpy(char *pu1, char *pu2, int n)
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

static BOOL suppspace(char *pu)
{
	SIZE_TYPE iprevlen = STlength(pu);
	SIZE_TYPE inewlen  = STtrmwhite(pu);
	if  (inewlen<iprevlen)
		return TRUE;
	else
		return FALSE;
}

static void AddEventToList (char * lpstring, CTypedPtrList<CObList, CaLoggedEvent*>*  plistLoggedEvent)
{
	CaLoggedEvent* pEvent = NULL;

	TCHAR bufdate[200];
	TCHAR buftime[200];
	TCHAR GWbufdate[200];

	TCHAR bufInstance[100];
	
	TCHAR bufText[2000];

	TCHAR bufComp[100]; // dummy for now
	TCHAR bufCat[100]; // dummy for now
	
	char * pc, *pc1,*pc2, *pc3, *pc4, *pc5;
	TCHAR buf[2000], buf2[2000];
	BOOL bErrCodeFound = FALSE;
	BOOL bExtraLineofPrevMessage = FALSE;

	BOOL bSetAlert = FALSE;

	MSGCLASSANDID msgclassAndId;

	fstrncpy(buf, lpstring, sizeof(buf)-1);
	//FRPAGP58::[II\INGRES\6d    , ffffffff]: Tue Jan 26 15:32:23 1999 E_SC0129_SERVER_UP	Ingres Release II 2.5/00 (int.w32/99) Server -- Normal Startup.
	strcpy(bufdate,"");
	strcpy(buftime,"");
	strcpy(bufInstance,"");
	strcpy(bufText,buf);	// default puts the full string into the text area
	strcpy(bufComp,"");
	strcpy(bufCat,"");

	while (TRUE) {
		pc1 = _tcsstr(buf,"::[");
		if (!pc1) 
			break;
		pc2 = _tcschr(pc1,']');
		if (!pc2)
			break;
		fstrncpy(buf2,pc1+3,(pc2-pc1)-2);
		pc = _tcschr(buf2,',');
		if (pc) {
			fstrncpy(bufInstance,buf2,pc-buf2+1);
			suppspace(bufInstance);
			if (!stricmp(bufInstance,"ingres") )
				strcpy(bufInstance,""); // was not a real instance
			// (all others seem to be real instances)
		}
		if (bufInstance[0]) {
			// Remove trailing string such as IIGCC, IIGCB, ... that come after
			// spaces, and don't belong to the instance name. The server type will be identified
			// through the preceeding 0x9051D or == 0xC2A10 message information
			char * pcloc = _tcschr(bufInstance,' ');
			if (pcloc)
				*pcloc='\0';
			strcpy(bufText, pc2+1);
		}
		else
			strcpy(bufText,pc1+2);
		pc = _tcsinc(pc2); // pc points after the ']' sign
		while (*pc==':')
			pc=_tcsinc(pc);
		while (*pc==' ')
			pc=_tcsinc(pc);
		pc2=_tcsstr(pc," E_");
		pc3=_tcsstr(pc," W_");
		pc4=_tcsstr(pc," I_");
		pc5=_tcsstr(pc," S_");
		if (pc2 || pc3 || pc4 || pc5) {
			bErrCodeFound = TRUE;
			if (!pc2)
			{
			    if (pc3)
				pc2=pc3;
			    else if (pc4)
				pc2=pc4;
			    else
				pc2=pc5;
			}
			else {
				if (pc3 && pc3<pc2)
					pc2=pc3;
				else if (pc4 && pc4<pc2)
				    pc2=pc4;
				else if (pc5 && pc5<pc2)
				    pc2=pc5;
			}
		}
		else {
			lstrcpy(bufText, pc);
			break;
		}
		fstrncpy(bufdate,pc,(pc2-pc)+1);
		suppspace(bufdate);
		strcpy(bufText,pc2+1);
		break;
	}
	msgclassAndId =getMsgStringClassAndId(bufText,GWbufdate,sizeof(GWbufdate));

	// update in memory special server class and config names, to be taken into
	// account in later messages

	if (msgclassAndId.lMsgFullID == 0x12530) { // E_CL2530_CS_PARAM. search for the server class
		char *pt1 = _tcschr(bufText,_T(' ')); // searches for the space after "E_CL2530_CS_PARAM"
		if (pt1) {
			while (*pt1==' ')
				pt1=_tcsinc(pt1);
			char *pstring2search = "server_class =";
			if (_tcsstr(pt1,pstring2search) == pt1) {
				pt1+=_tcslen(pstring2search);
				while (*pt1==' ')
					pt1=_tcsinc(pt1);
				_tcscpy(bufLastClass,pt1);
				suppspace(bufLastClass);
			}
		}
	}

	// manage "Finished loading configuration parameters for configuration 'xyz' of server type 'xyz2'.
	while (!bExtraLineofPrevMessage && (msgclassAndId.lMsgFullID == 0x9051D || msgclassAndId.lMsgFullID == 0xC2A10 
															|| msgclassAndId.lMsgFullID == 0xC4802)) {
		char ConfigName[100];
		char CompType[100];
		char * pc11 = _tcschr(bufText,'\'');
		if (!pc11)
			break;
		char * pc12 = _tcschr(pc11+1,'\'');
		if (!pc12)
			break;
		fstrncpy(ConfigName, pc11+1,pc12-pc11);

		/* "(Default)" is not localized in errlog.log...*/
		if (!_tcsicmp(ConfigName,GetEngDefConfName())) 
			_tcscpy(ConfigName,GetLocDefConfName());

		pc11 = _tcschr(pc12+1,'\'');
		if (!pc11)
			break;
		pc12 = _tcschr(pc11+1,'\'');
		if (!pc12)
			break;
		fstrncpy(CompType, pc11+1,pc12-pc11);

		// if the instance has non standard characteristics, add it to the 
		// "special instances" list

		int isvrtype = COMP_TYPE_SETTINGS;

		if (!stricmp(CompType,"DBMS"))
			isvrtype = COMP_TYPE_DBMS;
		else if (!stricmp(CompType,"STAR"))
			isvrtype = COMP_TYPE_STAR;
		else if (!stricmp(CompType,"GCC"))
			isvrtype = COMP_TYPE_NET;
		else if (!stricmp(CompType,"JDBC"))
			isvrtype = COMP_TYPE_JDBC;
		else if (!stricmp(CompType,"GCD"))
			isvrtype = COMP_TYPE_DASVR;
		else if (!stricmp(CompType,"GCB"))
			isvrtype = COMP_TYPE_BRIDGE;
		else if (!stricmp(CompType,"ICE"))
			isvrtype = COMP_TYPE_INTERNET_COM;
		else 
			break;

		// Put all such servers in the "special list", because there is
		// no way otherwise to figure out their server type, the server class not 
		// being part of the instance name in the general case.
		BOOL bSpecialInstance = TRUE;
		if (bSpecialInstance) {
			if ( !_tcsicmp(ConfigName,GetLocDefConfName()))
			   strcpy(ConfigName,GetLocDefConfName()); // to be consistent with what is seen in the IVM/VCBF branches
			CaParseInstanceInfo *pInstance = new CaParseInstanceInfo(bufInstance,ConfigName,bufLastClass,isvrtype);
			theApp.GetSpecialInstanceInfo().GetSpecialInstancesList().AddTail(pInstance);
		}
		_tcscpy(bufLastClass,_T(""));
		break;
		// end of management of "Finished loading configuration parameters [...] messages
	}

	// special management when E_GC0151_GCN_STARTUP is encountered:
	// - reset the "special instance list", since at this point its elements are normally no
	//   more useful
	// - special management for the "since last name server startup" message filter setting
	if (!bExtraLineofPrevMessage && msgclassAndId.lMsgFullID == 0xC0151) {
		// TO BE FINISHED: detect special case where name server would be re-registered
		// after a crash.... if even significant
			theApp.GetSpecialInstanceInfo().CleanUp();
			_tcscpy(bufLastClass,_T(""));
			// Store the name server instance in the "special list", since the server
			// type otherwise cannot be deducted from the instance name in the general case (it is
			// not part of the instance name under anything other than named pipes on Windows)
			CaParseInstanceInfo *pInstance = new CaParseInstanceInfo(bufInstance,"","",COMP_TYPE_NAME);
			theApp.GetSpecialInstanceInfo().GetSpecialInstancesList().AddTail(pInstance);
		// TO BE DONE: special management for the "since last name server startup" message filter setting
	}

	if (msgclassAndId.lMsgClass == CLASS_OTHERS && msgclassAndId.lMsgFullID == MSG_NOCODE) {
		msgclassAndId.lMsgClass =  lastMessageClassAndID.lMsgClass ;
		msgclassAndId.lMsgFullID = lastMessageClassAndID.lMsgFullID;
		bExtraLineofPrevMessage = TRUE;
	}
	else {
		lastMessageClassAndID.lMsgClass =  msgclassAndId.lMsgClass ;
		lastMessageClassAndID.lMsgFullID = msgclassAndId.lMsgFullID;
		bExtraLineofPrevMessage = FALSE;
	}

	BOOL bAdd2NormalList = TRUE;
	BOOL bAddToStartStopList = FALSE;

	BOOL bIsClassified = FALSE;

	if (IVM_IsStartStopMessage(msgclassAndId.lMsgFullID))
		bAddToStartStopList = TRUE;

	// get A/N/D setting for this message
	CaMessage* pmsgsettings = NULL;
	CaMessageEntry* pIngresCat = theApp.m_setting.m_messageManager.FindEntry (msgclassAndId.lMsgClass);
	if (pIngresCat) {
		pmsgsettings = pIngresCat->Search(msgclassAndId.lMsgFullID);
		if (pmsgsettings) {
			bIsClassified = TRUE;
		}
		else
			pmsgsettings = pIngresCat;
	}
	//pmsgsettings = theApp.m_setting.m_messageManager.Search(msgclassAndId.lMsgClass,
	//															   msgclassAndId.lMsgFullID);
	if (pmsgsettings) {
		switch (pmsgsettings->GetClass()) {
			case IMSG_DISCARD:
				bAdd2NormalList = FALSE;
				bSetAlert = FALSE;
				break;
			case IMSG_ALERT:
				bSetAlert = TRUE;
			// default to no alert was done at variable initialization time
		}
	}

	lID0++; //increased regarless of the filters, since this number is designed to map the position of the message
            // within the errlog.log file

	if (!bAdd2NormalList && !bAddToStartStopList)
		return;

	pEvent = new CaLoggedEvent (bufdate, buftime, bufComp, bufCat, bufText);
	pEvent->SetCatType(msgclassAndId.lMsgClass);
	pEvent->SetCode(msgclassAndId.lMsgFullID);
	pEvent->SetClassify(bIsClassified);

	while (!bExtraLineofPrevMessage) {
		if (bufInstance[0]){
			_tcscpy(bufLastInstance,bufInstance);
			if (GetCompTypeAndNameFromInstanceName(bufInstance,bufLastCompType, bufLastCompName)) 
				break;
		}
		_tcscpy(bufLastInstance,bufInstance);
		_tcscpy(bufLastCompType,_T(""));
		_tcscpy(bufLastCompName,_T(""));
		switch (msgclassAndId.lMsgFullID) {

			// to be finished with actual codes
			case 0xC0151 : // EVT_CAT_PRIMARY_GC_E_GC0151_GCN_STARTUP
			case 0xC0152 : //EVT_CAT_PRIMARY_GC_E_GC0152_GCN_SHUTDOWN
				_tcscpy(bufLastCompType, GetNameSvrCompName());
				if (!lstrcmp(bufLastInstance,"")) // on new versions, the instance is there
					strcpy(bufLastInstance, _T("II\\NMSVR"));
				break;
			case 0x90129 : //EVT_CAT_PRIMARY_SC_E_SC0129_SERVER_UP
			case 0x90128 : //EVT_CAT_PRIMARY_SC_E_SC0128_SERVER_DOWN
			case 0x12518 : //EVT_CAT_SVRONLY_CS_E_CL2518_CS_NORMAL_SHUTDOWN
				break;  // instance normally already set: to be checked for all server types
			case 0x39815 : //EVT_CAT_PRIMARY_DM_E_DM9815_ARCH_SHUTDOWN
				_tcscpy(bufLastCompType,GetArchProcessCompName());
				strcpy(bufLastInstance, _T("II_ACP"));
				break;
			case 0xC2815 : //EVT_CAT_PRIMARY_GC_E_GC2815_NTWK_OPEN
				break; // doesn't distinguish between NET and STAR for example
			case 0xC2006 : //EVT_CAT_PRIMARY_GC_E_GC2006_STARTUP:
			case 0xC2002 : //EVT_CAT_PRIMARY_GC_E_GC2002_SHUTDOWN
				_tcscpy(bufLastCompType, GetNetSvrCompName());
				strcpy(bufLastInstance, _T(""));
				break;
			case 0xC2A03 : // EVT_CAT_PRIMARY_GC_E_GC2A03_STARTUP
			case 0xC2A04 : //EVT_CAT_PRIMARY_GC_E_GC2A04_SHUTDOWN
				_tcscpy(bufLastCompType, GetBridgeSvrCompName());
				strcpy(bufLastInstance, _T(""));
				break;
			case 0xEC0001:  // RE0001
			case 0xEC0002:  // RE0002
				_tcscpy(bufLastCompType, GetRmcmdCompName());
				break;
			case GW_ORA_START:
			case GW_ORA_START_CONNECTLIMIT:
			case GW_ORA_STOP:
				_tcscpy(bufLastInstance, _T(""));
				_tcscpy(bufLastCompType, oneGW());
				_tcscpy(bufLastCompName, GetOracleGWName());
				pEvent->m_strDate= (LPSTR) GWbufdate;
				break;
			case GW_MSSQL_START:
			case GW_MSSQL_START_CONNECTLIMIT:
			case GW_MSSQL_STOP:
				_tcscpy(bufLastInstance, _T(""));
				_tcscpy(bufLastCompType, oneGW());
				_tcscpy(bufLastCompName, GetMSSQLGWName());
				pEvent->m_strDate= (LPSTR) GWbufdate;
				break;
			case GW_INF_START:
			case GW_INF_START_CONNECTLIMIT:
			case GW_INF_STOP:
				_tcscpy(bufLastInstance, _T(""));
				_tcscpy(bufLastCompType, oneGW());
				_tcscpy(bufLastCompName, GetInformixGWName());
				pEvent->m_strDate= (LPSTR) GWbufdate;
				break;
			case GW_SYB_START:
			case GW_SYB_START_CONNECTLIMIT:
			case GW_SYB_STOP:
				_tcscpy(bufLastInstance, _T(""));
				_tcscpy(bufLastCompType, oneGW());
				_tcscpy(bufLastCompName, GetSybaseGWName());
				pEvent->m_strDate= (LPSTR) GWbufdate;
				break;
			case GW_DB2UDB_START:
			case GW_DB2UDB_START_CONNECTLIMIT:
			case GW_DB2UDB_STOP:
				_tcscpy(bufLastInstance, _T(""));
				_tcscpy(bufLastCompType, oneGW());
				_tcscpy(bufLastCompName, GetDB2UDBGWName());
				pEvent->m_strDate= (LPSTR) GWbufdate;
				break;
			case GW_OTHER_STARTSTOP_MSG:
				_tcscpy(bufLastInstance, _T(""));
				_tcscpy(bufLastCompType, oneGW());
				_tcscpy(bufLastCompName, _T("")); /* empty string because unknown message */
				break;

			default:
				break;
		}
		break;
	}
	pEvent->m_strInstance      = bufLastInstance;
	pEvent->m_strComponentType = bufLastCompType;
	pEvent->m_strComponentName = bufLastCompName;

	pEvent->SetNotFirstLine( bExtraLineofPrevMessage);
//	pEvent->SetAlert (bSetAlert); included in new, next step
	pEvent->SetClass (bSetAlert? IMSG_ALERT : IMSG_NOTIFY);
	pEvent->SetIdentifier (lID0);
	//
	// Notify the main thread that this is the last name server started up.
	// The main thread will take action only if it has finished its query event cycle.
	// So, the subsequent call of 'theApp.SetLastNameStartedup(lID0)' will have effect
	// only for the last one:
	if (pEvent->IsNameServerStartedup() && !bExtraLineofPrevMessage)
		theApp.SetLastNameStartedup(lID0);

	if (bAdd2NormalList) {
		plistLoggedEvent->AddTail (pEvent);
		if (bAddToStartStopList)
			pEvent = pEvent->Clone();
	}
	if (bAddToStartStopList) {
		// add to start stop message list
		CaIvmEventStartStop& startstoplist = theApp.GetEventStartStopData();
		startstoplist.Add(pEvent);
	}
}

//
// Return TRUE if the size of file 'errlog.log' has been changed since the last called.
int IVM_llGetNewEventsStatus()
{
	int  iResult = RET_NO_NEW_EVENT;

	//
	// Simulation mode:
	if (theApp.m_nSimulate & SIMULATE_LOGGEDEVENT)
	{
#if defined (_DEBUG) && defined (SIMULATION)
		if (theApp.GetLastReadEventId() == -1)
			return RET_NEW_EVENTS;
		//
		// Checked to see if the 'errlog.log' has been modified since
		// the last access:
		struct  _stat buf; 
		if (!GetFileStatus (theApp.m_strLoggedEventFile, &buf))
			return RET_NO_NEW_EVENT;
		
		if (theApp.m_lfileSize == 0)
			iResult   = RET_NEW_EVENTS;
		else
		if (theApp.m_lfileSize != (UINT)buf.st_size)
			iResult   = RET_NEW_EVENTS;
		theApp.m_lfileSize = (UINT)buf.st_size;
#endif
	}
	//
	// Real mode:
	else
	{
		if (bMoreInMemory)
			return RET_NEW_EVENTS;
		long lnewlogfilesize;
		lnewlogfilesize = IVM_GetFileSize((char *) (LPCTSTR) theApp.m_strLoggedEventFile);

		if (lnewlogfilesize == -1L) /* _the only case where IVM_GetFileSize() returns -1 is assumed to be if the file */
			lnewlogfilesize = 0L;   /* doesn't exist, in which case we consider it (in terms of Ingres messages) as empty*/
		if (lnewlogfilesize>lbufferstartpos+lnextpos2read)
			iResult = RET_NEW_EVENTS;
		if ( lnewlogfilesize < lprevFileSize) {
			iResult = RET_LOGFILE_CHANGED_BY_HAND;
		}
		lprevFileSize = lnewlogfilesize;
	}
	return iResult;
}



//
// The second one must fill up:
//   pListEvent, the list of events that match the categories specified in the settings:
//   pQueryEventInfo, the information about the query of events (See detail of CaQueryEventInformation)
int IVM_llQueryLoggedEvent (
	CTypedPtrList<CObList, CaLoggedEvent*>* pListEvent,
	CaQueryEventInformation* pQueryEventInfo)

{
	try
	{
		//
		// Simulation mode:
		if (theApp.m_nSimulate & SIMULATE_LOGGEDEVENT)
		{
			//
			// Open file and Query the new Events:
			ReadSampleFile2 (pListEvent, pQueryEventInfo);
		}
		//
		// Real mode:
		else
		{
			int iaddedlines =0;
			TCHAR str0D0A[] = {0x0D, 0x0A, '\0'};
			lLastEvent = -1;
			lFirstEvent= -1;
			lExceed    =  0;
			bLimitReached = FALSE;

			bCheckPreviousFileState = TRUE; // asks for 1 test whether the file has been changed externally

			while (TRUE) {
				long lr;
				bMoreInMemory=FALSE;
				while (lnextpos2handle < lnextpos2read) {
					char * pc =strpbrk ((char *)(tchbuf+lnextpos2handle),str0D0A);
					TCHAR bufline[2 * EACHBUFFER];
					if (!pc || pc>=(char *)(tchbuf+lnextpos2read)) 
						break;
					if (iaddedlines>=MAXRETURNED) {
						bMoreInMemory =TRUE;
						break;
					}
					*pc='\0';
					strcpy(bufline, tchbuf+lnextpos2handle);
					pc++;
					while (*pc && _tcschr(str0D0A,*pc) && pc < (char *)(tchbuf+lnextpos2read))
						pc++;
					lnextpos2handle = (pc-tchbuf);
					if (bufline[0]=='\0') //bug 102225. Empty lines not displayed anyhow by design. Removing the garbage
						continue;         // line at this place covers the case of 0D 0A with the 0D at the end of a 8192
						                  // block  (messages across 2 blocks were already managed, what was not managed
						                  // was the "0D 0A" block across 2 blocks)

					AddEventToList (bufline, pListEvent);
					if (lFirstEvent == -1L) 
						lFirstEvent= lID0;
					lLastEvent=lID0;
					iaddedlines++;
				}
				if (bMoreInMemory)
					break;

				int iEvtsStatus =  IVM_llGetNewEventsStatus();
				if  (iEvtsStatus == RET_LOGFILE_CHANGED_BY_HAND)
					return iEvtsStatus;
				if  (iEvtsStatus == RET_NO_NEW_EVENT)
					break;


				// Read buffer From Disk
				if (lnextpos2read== 2 * EACHBUFFER) {
					if (lnextpos2handle<EACHBUFFER) {
						ASSERT(FALSE);
						if (!blogfilemessageboxdislayed) {
							// TO BE DONE: possibly exit IVM in this case
							IVM_PostModelessMessageBox(IDS_E_MORE8000CHARSLINES);
							blogfilemessageboxdislayed = TRUE;
						}
						return RET_ERROR;
						// COULD BE IMPROVED, if event size >EACHBUFFER, by splitting into 2
					}
					memcpy(tchbuf,tchbuf+EACHBUFFER,EACHBUFFER);
					lnextpos2handle -=EACHBUFFER;
					lnextpos2read   -=EACHBUFFER;
					lbufferstartpos +=EACHBUFFER;
				}

				buf0or1 = lnextpos2read/EACHBUFFER;
				lr = ReadErrLogBuffer(tchbuf+(buf0or1 * EACHBUFFER),
										   lbufferstartpos+(buf0or1 * EACHBUFFER),
										   EACHBUFFER);
				ASSERT (sizeof(tchbuf) > 2 * EACHBUFFER); /* should be 2*EACHBUFFER+1 minimum*/
				tchbuf[sizeof(tchbuf)-1]='\0'; // cleaner, although the static buffer is normally already initialized to 0
				if (lr<0L) {
					if (lr == READERRLOGINCONSISTENT) 
						return RET_LOGFILE_CHANGED_BY_HAND;
					return RET_ERROR;
				}

				long lnextpos2readNEW = buf0or1*EACHBUFFER+lr;
				if (lnextpos2readNEW<lnextpos2read) {
					ASSERT(FALSE);
					return RET_ERROR;
				}
				lnextpos2read = lnextpos2readNEW;
			}
			pQueryEventInfo->SetFirstEventId(lFirstEvent);
			pQueryEventInfo->SetLastEventId(lLastEvent);

			long lCurrentCount = 0;
			long lTotal  = 0;
			long lExceed = 0L;
 			BOOL bLimitReached = FALSE;

			int nEventMaxType = theApp.m_setting.GetEventMaxType();
			switch (nEventMaxType)	{
				case EVMAX_COUNT:
					lCurrentCount = (long) theApp.GetEventCount();
					lTotal = lCurrentCount + (long) pListEvent->GetCount();
					if (lTotal < (long) theApp.m_setting.GetMaxCount())
						bLimitReached = FALSE;
					else
					{
						bLimitReached = TRUE;
						lExceed = lTotal - (long)theApp.m_setting.GetMaxCount();
					}
					break;
				case EVMAX_MEMORY:
					lCurrentCount = (long)theApp.GetEventSize();
					lTotal = lCurrentCount + CountSize (pListEvent);
					if (lTotal < (long) theApp.m_setting.GetMaxSize() * 1024L*1024L)
						bLimitReached = FALSE;
					else
					{
						bLimitReached = TRUE;
						lExceed = lTotal - (long) (theApp.m_setting.GetMaxSize()) * 1024L*1024L;
						lExceed = (-1)*lExceed;  // by convention, <0 means a size, not a count
					}
					break;

				case EVMAX_NOLIMIT:
				case EVMAX_SINCEDAY:
				case EVMAX_SINCENAME:
					break;
				default:
					ASSERT (FALSE);
					break;
			}
			pQueryEventInfo->SetLimitReach(bLimitReached);
			pQueryEventInfo->SetExceed(lExceed);


		}
	}
	catch (...)
	{
		MessageBeep (-1);
		TRACE0 ("System error(raise exception): IVM_llQueryLoggedEvent failed)\n");
		return RET_ERROR;
	}
	return RET_OK;
}


BOOL IVM_InitializeInstance()
{
	if (theApp.m_nSimulate & SIMULATE_INSTANCE)
		return TRUE;
	UpdInstanceLists();
	return TRUE;
}


BOOL IVM_llQueryInstance (CaTreeComponentItemData* pComponent, CTypedPtrList<CObList, CaTreeComponentItemData*>& listInstance)
{
	BOOL bOK = FALSE;
	//
	// Simulation mode:
	if (theApp.m_nSimulate & SIMULATE_INSTANCE)
	{
	}
	//
	// Real mode:
	else
	{
		bOK = LLGetComponentInstances (pComponent, listInstance);
	}

	return bOK;
}


static BOOL IVM_RunProcess (LPCTSTR lpszProcessName)
{
	HANDLE m_hChildStdoutRd, m_hChildStdoutWr;
	HANDLE m_hChildStdinRd, m_hChildStdinWr;

	STARTUPINFO m_siStartInfo;
	PROCESS_INFORMATION m_piProcInfo;
	

	HANDLE hChildStdinWr;
	DWORD  dwExitCode;
	DWORD  dwWaitResult = 0;
	UINT   uRet = 1;
	BOOL   bTimeOut = FALSE;      // If the delay of 15 minutes taken to execute ingstart is exceeded.
	long   lTimeOut = 1000*60*15; // 15 mins (Max time taken to finish execution of ingstart)
	CString strText;

	// Init security attributes
	SECURITY_ATTRIBUTES sa;
	memset (&sa, 0, sizeof (sa));
	sa.nLength              = sizeof(sa);
	sa.bInheritHandle       = TRUE; // make pipe handles inheritable
	sa.lpSecurityDescriptor = NULL;
	
	try
	{
		//
		// Create a pipe for the child's STDOUT
		if (!CreatePipe(&m_hChildStdoutRd, &m_hChildStdoutWr, &sa, 0)) 
			throw lpszProcessName;
		//
		// Create a pipe for the child's STDIN
		if (!CreatePipe(&m_hChildStdinRd, &hChildStdinWr, &sa, 0)) 
			throw lpszProcessName;
		
		//
		// Duplicate the write handle to the STDIN pipe so it's not inherited
		if (!DuplicateHandle(
			 GetCurrentProcess(), 
			 hChildStdinWr, 
			 GetCurrentProcess(), 
			 &m_hChildStdinWr, 
			 0, FALSE, 
			 DUPLICATE_SAME_ACCESS)) 
		{
			throw lpszProcessName;
		}
		//
		// Don't need it any more
		CloseHandle(hChildStdinWr);

		//
		// Create the child process
		m_siStartInfo.cb = sizeof(STARTUPINFO);
		m_siStartInfo.lpReserved = NULL;
		m_siStartInfo.lpReserved2 = NULL;
		m_siStartInfo.cbReserved2 = 0;
		m_siStartInfo.lpDesktop = NULL;
		m_siStartInfo.lpTitle = NULL;
		m_siStartInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		m_siStartInfo.wShowWindow = SW_HIDE;
		m_siStartInfo.hStdInput = m_hChildStdinRd;
		m_siStartInfo.hStdOutput = m_hChildStdoutWr;
		m_siStartInfo.hStdError = m_hChildStdoutWr;
		if (!CreateProcess (
			 NULL, 
			 (LPTSTR)lpszProcessName, 
			 NULL, 
			 NULL, 
			 TRUE, 
			 NORMAL_PRIORITY_CLASS, 
			 NULL, 
			 NULL, 
			 &m_siStartInfo, 
			 &m_piProcInfo)) 
		{
			throw lpszProcessName;
		}
		CloseHandle(m_hChildStdinRd);
		CloseHandle(m_hChildStdoutWr);
		
		WaitForSingleObject (m_piProcInfo.hProcess, lTimeOut);
		if (!GetExitCodeProcess(m_piProcInfo.hProcess, &dwExitCode))
			throw lpszProcessName;

		switch (dwExitCode)
		{
		case STILL_ACTIVE:
			//
			// Time out
			bTimeOut = TRUE;
			uRet = 1;
			break;
		default:
			//
			// Execution complete successfully
			uRet = 0;
		}
		//
		// Normal termination of Thread if (uRet = 0):
	}
	catch (...)
	{
		uRet = 1;
	}
	return (uRet == 0)? TRUE: FALSE;

	return TRUE;
}


BOOL IVM_StartComponent (LPCTSTR lpszStartString)
{
	return IVM_RunProcess (lpszStartString);
}

BOOL IVM_StopComponent  (LPCTSTR lpszStopString)
{
	return IVM_RunProcess (lpszStopString);
}


CaTreeComponentItemData* IVM_MatchInstance (LPCTSTR lpszInstance)
{
	CaTreeDataMutex treeMutex;
	CaTreeComponentItemData* pItem = NULL;
	try
	{
		CaIvmTree& treeData = theApp.GetTreeData();
		CaTreeComponentItemData* pComp = treeData.MatchInstance (lpszInstance);
		if (pComp)
		{
			pItem = new CaTreeComponentItemData();

			pItem->m_nComponentType   = pComp->m_nComponentType;
			pItem->m_strComponentType = pComp->m_strComponentType;
			pItem->m_strComponentName = pComp->m_strComponentName;
			pItem->m_strComponentInstance = pComp->m_strComponentInstance;
		}
	}
	catch (...)
	{
		pItem = NULL;
	}
	return pItem;
}

BOOL IVM_IsReinitializeEventsRequired()
{
	return FALSE;
}


static void IVM_RestartReadLogEvent()
{
	theApp.ResetReadEventId();
	theApp.m_lfileSize = 0;
}


BOOL IVM_GetHostInfo (CString& strInstall, CString& strHost, CString& strIISystem)
{
	SHOSTINFO curInfo;
	BOOL bOK = FALSE;
	if (!(theApp.m_nSimulate & SIMULATE_COMPONENT))
	{
		bOK = VCBFGetHostInfos(&curInfo);
	}
	if (bOK)
	{
		strInstall = curInfo.ii_installation;
		strHost = curInfo.host;
		strIISystem = curInfo.ii_system;
	}
	else
	{
		strInstall = _T("n/a");
		strHost = _T("n/a");
		strIISystem = _T("n/a");
	}
	return TRUE;
}


long IVM_NewEvent (long nEventNumber, BOOL bRead)
{
	long nCount = 0;
	try
	{
		CaReadFlag* pRFlag = theApp.GetReadFlag();
		nCount = pRFlag->Add (nEventNumber, bRead);
	}
	catch (...)
	{
		AfxMessageBox (_T("System error (IVM_NewEvent): \nCannot add new flag for read/not read event"));
		nCount = 0;
	}
	return nCount;
}

BOOL IVM_IsReadEvent (long nEventNumber)
{
	CaReadFlag* pRFlag = theApp.GetReadFlag();
	if (pRFlag && (pRFlag->GetAt(nEventNumber) == TRUE))
		return TRUE;
	return FALSE;
}

void IVM_SetReadEvent(long nEventNumber, BOOL bRead)
{
	CaReadFlag* pRFlag = theApp.GetReadFlag();
	if (pRFlag)
		pRFlag->SetAt(nEventNumber, bRead);
}



//
// Query the list of Components,
// if bQueryInstance = TRUE, this function will query the list of instances for each component.
BOOL IVM_llQueryComponent (CTypedPtrList<CObList, CaTreeComponentItemData*>* pListComponent, BOOL bQueryInstance)
{
	TCHAR buf [256];
	int nUnderScore;
	CString strItem;

	BOOL bOk = FALSE;
	BOOL bAllowDuplicate = FALSE;
	COMPONENTINFO comp;
	CaTreeComponentItemData* pItem = NULL;
	memset (&comp, 0, sizeof (comp));
	buf [0] = _T('\0');
	nUnderScore = -1;


	bOk = VCBFllGetFirstComp(&comp);

	while (bOk)
	{
		bAllowDuplicate = VCBFllCanCompBeDuplicated(&comp);
		switch (comp.itype) 
		{
		case COMP_TYPE_NAME:
			// Name Server
			pItem = new CaItemDataName();
			break;
		case COMP_TYPE_DBMS:
			// DBMS Server
			pItem = new CaItemDataDbms();
			break;
		case COMP_TYPE_INTERNET_COM:
			// ICE Server
			pItem = new CaItemDataIce();
			break;
		case COMP_TYPE_NET:
			// NET Server
			pItem = new CaItemDataNet();
			break;
		case COMP_TYPE_BRIDGE:
			// BRIDGE Server
			pItem = new CaItemDataBridge();
			break;
		case COMP_TYPE_STAR:
			// STAR Server
			pItem = new CaItemDataStar();
			break;
		case COMP_TYPE_JDBC:
			// JDBC Server
			pItem = new CaItemDataJdbc();
			break;
		case COMP_TYPE_DASVR:
			// DASVR Server
			pItem = new CaItemDataDasvr();
			break;
		case COMP_TYPE_SECURITY:
			// SECaRITY
			pItem = new CaItemDataSecurity();
			break;
		case COMP_TYPE_LOCKING_SYSTEM:
			// LOCKING SYSTEM
			pItem = new CaItemDataLocking();
			break;
		case COMP_TYPE_LOGGING_SYSTEM:
			// LOGGING SYSTEM
			pItem = new CaItemDataLogging();
			break;
		case COMP_TYPE_TRANSACTION_LOG:
			// LOGFILE 
			pItem = new CaItemDataLogFile();
			if (lstrcmpi ((LPCTSTR)comp.sztype, GetTransLogCompName()) == 0 && lstrcmpi ((LPCTSTR)comp.szname, _T("II_LOG_FILE")) == 0)
			{
				_tcscpy ((LPTSTR)comp.sztype, _T("Primary Transaction Log"));
				_tcscpy ((LPTSTR)comp.sztypesingle, (LPTSTR)comp.sztype);
				pItem->SetOrder (ORDER_TRANSACTION_PRIMARY_LOG);
			}
			else
			if (lstrcmpi ((LPCTSTR)comp.sztype,GetTransLogCompName()) == 0 && lstrcmpi ((LPCTSTR)comp.szname, _T("II_DUAL_LOG")) == 0)
			{
				_tcscpy ((LPTSTR)comp.sztype, _T("Dual Transaction Log"));
				_tcscpy ((LPTSTR)comp.sztypesingle, (LPTSTR)comp.sztype);
				pItem->SetOrder (ORDER_TRANSACTION_DUAL_LOG);
			}

			break;
		case COMP_TYPE_RECOVERY:
			// RECOVERY Server
			pItem = new CaItemDataRecovery();
			break;
		case COMP_TYPE_ARCHIVER:
			// ARCHIVER
			pItem = new CaItemDataArchiver();
			break;
		case COMP_TYPE_OIDSK_DBMS:
			// Desktop DBMS
			pItem = new CaItemDataDbmsOidt();
			break;
		case COMP_TYPE_RMCMD:
			// RMCMD
			pItem = new CaItemDataRmcmd();
			break;
		case COMP_TYPE_GW_ORACLE:
			// Oracle Gateway
			pItem = new CaItemDataOracle();
			pItem->SetStartStopArg(_T("oracle"));
			break;
		case COMP_TYPE_GW_DB2UDB:
			// db2 udb gateway mapped to oracle GUI
			pItem = new CaItemDataOracle();
			pItem->SetStartStopArg(_T("db2udb"));
			break;
		case COMP_TYPE_GW_INFORMIX:
			// Informix Gateway
			pItem = new CaItemDataInformix();
			break;
		case COMP_TYPE_GW_SYBASE:
			// Sybase Gateway
			pItem = new CaItemDataSybase();
			break;
		case COMP_TYPE_GW_MSSQL:
			// MSSQL Gateway
			pItem = new CaItemDataMsSql();
			break;
		case COMP_TYPE_GW_ODBC:
			// ODBC Gateway
			pItem = new CaItemDataOdbc();
			break;
		default:
			//
			// Type of component is unknown !
			ASSERT (FALSE);
			return FALSE;
		}

		pItem->m_bAllowDuplicate  = bAllowDuplicate;
		pItem->m_strComponentName = comp.szname;
		pItem->m_strComponentTypes= comp.sztype;
		pItem->m_strComponentType = comp.sztypesingle;
		pItem->m_nComponentType   = comp.itype;
		pItem->m_strComponentName.TrimRight();
		pItem->m_strComponentType.TrimRight();
		pItem->m_nStartupCount = _ttol ((LPCTSTR)comp.szcount);
		if (bQueryInstance)
			pItem->QueryInstance();

		ASSERT (pListComponent);
		if (pListComponent && pItem)
			pListComponent->AddTail (pItem);

		//
		// Simulation mode:
		if (theApp.m_nSimulate & SIMULATE_COMPONENT)
		{
#if defined (_DEBUG) && defined (SIMULATION)
			if (in.peek() != _TEOF)
			{
				in >> comp.itype;
				in >> comp.szcount;
				in >> comp.sztype;
				in.getline (buf, 63); 
				strItem = buf;
				strItem.TrimLeft();
				strItem.TrimRight();
				lstrcpy (comp.szname, strItem);

				strItem = comp.sztype;
				nUnderScore = strItem.Find (_T('_'));
				while (nUnderScore != -1)
				{
					strItem.SetAt (nUnderScore, _T(' '));
					nUnderScore = strItem.Find (_T('_'));
				}
				lstrcpy (comp.sztype, strItem);
				//
				// Use the same string:
				lstrcpy (comp.sztypesingle, comp.sztype);
			}
			else
				bOk = FALSE;
#endif // defined (_DEBUG) && defined (SIMULATION)
		}
		//
		// Real mode:
		else
		{
			bOk = VCBFllGetNextComp(&comp);
		}
	}
	return TRUE;
}


CString IVM_GetTimeSinceNDays(UINT nDays)
{
	CTime t  = CTime::GetCurrentTime();
	CTime tm = t - CTimeSpan (nDays, 0, 0, 0);
	return tm.Format (theApp.m_strTimeFormat);
}


BOOL IVM_IsStartMessage(long lmessageID)
{
	switch (lmessageID) {
		case 0xC0151 : //EVT_CAT_PRIMARY_GC_E_GC0151_GCN_STARTUP
		case 0x90129 : //EVT_CAT_PRIMARY_SC_E_SC0129_SERVER_UP
		case 0xC2815 : //EVT_CAT_PRIMARY_GC_E_GC2815_NTWK_OPEN
		case 0xC2006 : //EVT_CAT_PRIMARY_GC_E_GC2006_STARTUP
		case 0xC2A03 : //EVT_CAT_PRIMARY_GC_E_GC2A03_STARTUP
		case 0xEC0001: // E_RE0001_RMCMD_UP
		case 0xED0100: // E_JD0100_STARTUP	JDBC Server normal startup.
		case 0xED0106: // E_JD0106_NTWK_OPEN	Network open complete for protocol WINTCP [...]
		case 0xC4800 : //EVT_CAT_PRIMARY_GC_E_GC4800 DAS Server startup
		case 0xC4803 : //GC_E_GC4803 DAS server Network Open

		case GW_ORA_START:
		case GW_ORA_START_CONNECTLIMIT:
		case GW_MSSQL_START:
		case GW_MSSQL_START_CONNECTLIMIT:
		case GW_INF_START:
		case GW_INF_START_CONNECTLIMIT:
		case GW_SYB_START:
		case GW_SYB_START_CONNECTLIMIT:
		case GW_DB2UDB_START:
		case GW_DB2UDB_START_CONNECTLIMIT:

				return TRUE;
			default:
				return FALSE;
	}
	return FALSE;
}

BOOL IVM_IsStopMessage(long lmessageID)
{
	switch (lmessageID) {
		case 0xC0152 : //EVT_CAT_PRIMARY_GC_E_GC0152_GCN_SHUTDOWN
		case 0x90128 : //EVT_CAT_PRIMARY_SC_E_SC0128_SERVER_DOWN
		case 0x12518 : //EVT_CAT_SVRONLY_CS_E_CL2518_CS_NORMAL_SHUTDOWN
		case 0x39815 : //EVT_CAT_PRIMARY_DM_E_DM9815_ARCH_SHUTDOWN
		case 0xC2002 : //EVT_CAT_PRIMARY_GC_E_GC2002_SHUTDOWN
		case 0xC2A04 : //EVT_CAT_PRIMARY_GC_E_GC2A04_SHUTDOWN
		case 0xEC0002: // E_RE0002_RMCMD_DOWN
		case 0xED0101: // E_JD0101_SHUTDOWN	JDBC Server normal shutdown
		case 0xC4801 : //EVT_CAT_PRIMARY_GC_E_GC4801 DAS Server shutdown
		case GW_ORA_STOP:
		case GW_MSSQL_STOP:
		case GW_INF_STOP:
		case GW_SYB_STOP:
		case GW_DB2UDB_STOP:
				return TRUE;
			default:
				return FALSE;
	}
	return FALSE;
}

BOOL IVM_IsLoggingSystemMessage(long lmsgcat, long lmessageID) 
{
	if (lmsgcat == 0x10F00)
		return TRUE;
	return FALSE;
}

BOOL IVM_IsLockingSystemMessage(long lmsgcat, long lmessageID) 
{
	if (lmsgcat == 0x11000)
		return TRUE;
	return FALSE;
}

BOOL IVM_IsStartStopMessage(long lmessageID)
{
	if ( IVM_IsStartMessage(lmessageID) ||
		 IVM_IsStopMessage (lmessageID) ||
		 lmessageID== GW_OTHER_STARTSTOP_MSG
	   )
	   return TRUE;

	return FALSE;
}


static BOOL XStartedAt(CTreeCtrl* pTree, HTREEITEM hItem, int nComponentType)
{
	if (!hItem)
		return FALSE;
	CaTreeComponentItemData* pItem = (CaTreeComponentItemData*)pTree->GetItemData(hItem);
	if (!pItem)
		return FALSE;
	if (pItem->m_nComponentType == nComponentType && !pItem->IsFolder())
	{
		CTypedPtrList<CObList, CaTreeComponentInstanceItemData*>* pListInstance = pItem->GetInstances();
		if (pListInstance && pListInstance->GetCount() > 0)
			return TRUE;
	}
	else
	{
		if (!pTree->ItemHasChildren(hItem))
			return FALSE;
		HTREEITEM hChild = pTree->GetChildItem (hItem);
		while (hChild)
		{
			BOOL bFound = XStartedAt(pTree, hChild, nComponentType);
			if (bFound)
				return TRUE;
			hChild = pTree->GetNextSiblingItem(hChild);
		}
	}
	return FALSE;
}

//
// Check to see if exist at least an instance running of a component
// given by it type <nComponentType>
BOOL IVM_IsStarted (CTreeCtrl* pTree, int nComponentType)
{
	BOOL bStarted = FALSE;
	if (!pTree)
		return FALSE;
	if (nComponentType == -1)
		return FALSE;
	HTREEITEM hRoot = pTree->GetRootItem();
	if (!hRoot)
		return FALSE;

	bStarted = XStartedAt (pTree, hRoot, nComponentType);
	return bStarted;
}


//
// Return the generic Message Text of a given message id:
CString IVM_IngresGenericMessage (long lCode, LPCTSTR lpszOriginMsg)
{
	TCHAR tchszBuffer [512];
	tchszBuffer[0] = _T('0');
	
	return GetAndFormatIngMsg (lCode, tchszBuffer, 510)? tchszBuffer: lpszOriginMsg;
}

//
// Return the empty string if allowed, otherwise
// the string contains the message of the reason why it is not allowed.

// TO BE DONE: new strings to be put into resources

CString IVM_AllowedStart (CTreeCtrl* pTree, CaTreeComponentItemData* pItem)
{
	CString strMsg = _T("System error (IVM_AllowedStart): Cannot start this component");
	BOOL bInstallation = FALSE;
	ASSERT (pItem);
	if (!pItem)
		return strMsg;
	CaPageInformation* pPageInfo = pItem->GetPageInformation();
	ASSERT (pPageInfo);
	if (!pPageInfo)
		return strMsg;
	bInstallation = (pPageInfo->GetClassName().CompareNoCase (_T("INSTALLATION")) == 0);


	if (bInstallation)
	{
		//
		// Only if Name server is not started.
		if (IVM_IsStarted (pTree, COMP_TYPE_NAME))
			//_T("The Name Server is already running.");
			strMsg.LoadString (IDS_MSG_NAME_SERVER_ALREADY_RUNNING); 
		else
			strMsg = _T("");
		return strMsg;
	}

	switch (pItem->m_nComponentType)
	{
	case COMP_TYPE_NAME:
		// Name Server: Only if it is not started.
		if (IVM_IsStarted(pTree, COMP_TYPE_NAME))
			//_T("The Name Server is already running.");
			strMsg.LoadString (IDS_MSG_NAME_SERVER_ALREADY_RUNNING); 
		else if(theApp.m_bVista && !IVM_IsServiceRunning(theApp.m_strIngresServiceName))
			strMsg.LoadString (IDS_MSG_REQUIRE_SERVICE );
		else
			strMsg = _T("");
		break;
	case COMP_TYPE_DBMS:
		// DBMS Server: Only if 
		//              Name Server has been started
		//              Recovery has been started
		//              Archiver has been started
		if (!IVM_IsStarted(pTree, COMP_TYPE_NAME))
			//_T("Unable to start DBMS server, since the Name Server process is not running.");
			strMsg.LoadString (IDS_MSG_UNABLE_RUN_DBMSxIIGCN);
		else
		if (!IVM_IsStarted(pTree, COMP_TYPE_RECOVERY))
			//_T("Unable to start DBMS server, since the Recovery Server process is not running.");
			strMsg.LoadString (IDS_MSG_UNABLE_RUN_DBMSxIIRCP);
		else if(theApp.m_bVista && !IVM_IsServiceRunning(theApp.m_strIngresServiceName))
			strMsg.LoadString (IDS_MSG_REQUIRE_SERVICE );
		else
			strMsg = _T("");
		break;
	case COMP_TYPE_INTERNET_COM:
		// ICE Server
		if (IVM_IsStarted(pTree, COMP_TYPE_DBMS))
			strMsg = _T("");
		else if(theApp.m_bVista && !IVM_IsServiceRunning(theApp.m_strIngresServiceName))
			strMsg.LoadString (IDS_MSG_REQUIRE_SERVICE );
		else
			//_T("Unable to start ICE Server, since no DBMS Server processes could be found in this installation.");
			strMsg.LoadString (IDS_MSG_UNABLE_RUN_ICExIIDBMS);
		break;
	case COMP_TYPE_NET:
		// NET Server
		if (!IVM_IsStarted(pTree, COMP_TYPE_NAME))
			//_T("Unable to start Net Server, since the Name Server process is not running.");
			strMsg.LoadString (IDS_MSG_UNABLE_RUN_IIGCCxIIGCN);
		else if(theApp.m_bVista && !IVM_IsServiceRunning(theApp.m_strIngresServiceName))
			strMsg.LoadString (IDS_MSG_REQUIRE_SERVICE );
		else
			strMsg = _T("");
		break;
	case COMP_TYPE_JDBC:
		// JDBC Server
		if (!IVM_IsStarted(pTree, COMP_TYPE_NAME))
			//_T("Unable to start a JDBC Server, since the Name Server process is not running.");
			strMsg.LoadString (IDS_MSG_UNABLE_RUN_IIJDBCxIIGCN);
		else if(theApp.m_bVista && !IVM_IsServiceRunning(theApp.m_strIngresServiceName))
			strMsg.LoadString (IDS_MSG_REQUIRE_SERVICE );
		else
			strMsg = _T("");
		break;
	case COMP_TYPE_DASVR:
		// DASVR Server
		if (!IVM_IsStarted(pTree, COMP_TYPE_NAME))
			//_T("Unable to start a DASVR Server, since the Name Server process is not running.");
			strMsg.LoadString (IDS_MSG_UNABLE_RUN_IIDASVRxIIGCN);
		else if(theApp.m_bVista && !IVM_IsServiceRunning(theApp.m_strIngresServiceName))
			strMsg.LoadString (IDS_MSG_REQUIRE_SERVICE );
		else
			strMsg = _T("");
		break;
	case COMP_TYPE_BRIDGE:
		// BRIDGE Server: Only if one or more Instances of Net Server
		//                have been started.
		if (!IVM_IsStarted(pTree, COMP_TYPE_NET))
			//_T("Unable to start Bridge Server, since no Net Server processes could be found in this installation.");
			strMsg.LoadString (IDS_MSG_UNABLE_RUN_BRIDGExIIGCC);
		else if(theApp.m_bVista && !IVM_IsServiceRunning(theApp.m_strIngresServiceName))
			strMsg.LoadString (IDS_MSG_REQUIRE_SERVICE );
		else
			strMsg = _T("");
		break;
	case COMP_TYPE_STAR:
		// STAR Server: (DBMS Instance)+, (NET Instance)+
		if (!IVM_IsStarted(pTree, COMP_TYPE_DBMS))
			//_T("Unable to start Star Server, since no DBMS Server processes could be found in this installation.");
			strMsg.LoadString (IDS_MSG_UNABLE_RUN_STARxIIDBMS);
		else
		if (!IVM_IsStarted(pTree, COMP_TYPE_NET))
			//_T("Unable to start Star Server, since no Net Server processes could be found in this installation.");
			strMsg.LoadString (IDS_MSG_UNABLE_RUN_STARxIIGCC);
		else if(theApp.m_bVista && !IVM_IsServiceRunning(theApp.m_strIngresServiceName))
			strMsg.LoadString (IDS_MSG_REQUIRE_SERVICE );
		else
			strMsg = _T("");
		break;
	case COMP_TYPE_SECURITY:
		// SECaRITY
	case COMP_TYPE_LOCKING_SYSTEM:
		// LOCKING SYSTEM
	case COMP_TYPE_LOGGING_SYSTEM:
		// LOGGING SYSTEM
	case COMP_TYPE_TRANSACTION_LOG:
		// LOGFILE 
		break;

	case COMP_TYPE_RECOVERY:
		// RECOVERY Server
		if (!IVM_IsStarted(pTree, COMP_TYPE_NAME))
			//_T("Unable to start Recovery Server, since the Name Server process is not running.");
			strMsg.LoadString (IDS_MSG_UNABLE_RUN_RECOVERYxIIGCN);
		else
		if (IVM_IsStarted(pTree, COMP_TYPE_RECOVERY))
			//_T("The Recovery Server is already running.");
			strMsg.LoadString (IDS_MSG_RECOVER_ALREADY_RUNNING);
		else if(theApp.m_bVista && !IVM_IsServiceRunning(theApp.m_strIngresServiceName))
			strMsg.LoadString (IDS_MSG_REQUIRE_SERVICE );
		else
			strMsg = _T("");
		break;
	case COMP_TYPE_ARCHIVER:
		// ARCHIVER
		if (!IVM_IsStarted(pTree, COMP_TYPE_NAME))
			//_T("Unable to start Archiver Server, since the Name Server process is not running.");
			strMsg.LoadString (IDS_MSG_UNABLE_RUN_ARCHIVERxIIGCN);
		else
		if (IVM_IsStarted(pTree, COMP_TYPE_ARCHIVER))
			//_T("The Archiver server is already running.");
			strMsg.LoadString (IDS_MSG_ARCHIVER_ALREADY_RUNNING);
		else if(theApp.m_bVista && !IVM_IsServiceRunning(theApp.m_strIngresServiceName))
			strMsg.LoadString (IDS_MSG_REQUIRE_SERVICE );
		else
			strMsg = _T("");
		break;

	case COMP_TYPE_OIDSK_DBMS:
		// Desktop DBMS
		break;

	case COMP_TYPE_RMCMD:
		// RMCMD
		if (!IVM_IsStarted(pTree, COMP_TYPE_DBMS))
			//_T("Unable to start a Remote Command Server, since no DBMS Server processes could be found in this installation.");
			strMsg.LoadString (IDS_MSG_UNABLE_RUN_RMCMDxIIDBMS);
		else
		if (IVM_IsStarted(pTree, COMP_TYPE_RMCMD))
			//_T("An instance of the Remote Command Server is already running.");
			strMsg.LoadString (IDS_MSG_RMCMD_ALREADY_RUNNING);
		else if(theApp.m_bVista && !IVM_IsServiceRunning(theApp.m_strIngresServiceName))
			strMsg.LoadString (IDS_MSG_REQUIRE_SERVICE );
		else
			strMsg = _T("");
		break;

	case COMP_TYPE_GW_ORACLE:
		// Oracle Gateway
		if (!IVM_IsStarted(pTree, COMP_TYPE_NAME))
			//_T("Unable to start the ORACLE Gateway, since the Name Server process is not running.");
			strMsg.LoadString (IDS_MSG_UNABLE_RUN_ORACLExIIGCN);
		else
		if (IVM_IsStarted(pTree, pItem->m_nComponentType))
			//_T("An instance of the ORACLE Gateway is already running.");
			strMsg.LoadString (IDS_MSG_ORACLE_ALREADY_RUNNING);
		else if(theApp.m_bVista && !IVM_IsServiceRunning(theApp.m_strIngresServiceName))
			strMsg.LoadString (IDS_MSG_REQUIRE_SERVICE );
		else
			strMsg = _T("");
		break;
	case COMP_TYPE_GW_DB2UDB:
		// db2udb Gateway
		if (!IVM_IsStarted(pTree, COMP_TYPE_NAME))
			strMsg.Format(IDS_MSG_UNABLE_RUN_GATEWAYxIIGCN,_T("DB2 UDB"));
		else
		if (IVM_IsStarted(pTree, pItem->m_nComponentType))
			strMsg.Format(IDS_MSG_GATEWAY_ALREADY_RUNNING,_T("DB2 UDB"));
		else if(theApp.m_bVista && !IVM_IsServiceRunning(theApp.m_strIngresServiceName))
			strMsg.LoadString (IDS_MSG_REQUIRE_SERVICE );
		else
			strMsg = _T("");
		break;
	case COMP_TYPE_GW_INFORMIX:
		// Informix Gateway
		if (!IVM_IsStarted(pTree, COMP_TYPE_NAME))
			//_T("Unable to start the INFORMIX Gateway, since the Name Server process is not running.");
			strMsg.LoadString (IDS_MSG_UNABLE_RUN_INFORMIXxIIGCN);
		else
		if (IVM_IsStarted(pTree, pItem->m_nComponentType))
			//_T("An instance of the INFORMIX Gateway is already running.");
			strMsg.LoadString (IDS_MSG_INFORMIX_ALREADY_RUNNING);
		else if(theApp.m_bVista && !IVM_IsServiceRunning(theApp.m_strIngresServiceName))
			strMsg.LoadString (IDS_MSG_REQUIRE_SERVICE );
		else
			strMsg = _T("");
		break;
	case COMP_TYPE_GW_SYBASE:
		// Sybase Gateway
		if (!IVM_IsStarted(pTree, COMP_TYPE_NAME))
			//_T("Unable to start the SYBASE Gateway, since the Name Server process is not running.");
			strMsg.LoadString (IDS_MSG_UNABLE_RUN_SYBASExIIGCN);
		else
		if (IVM_IsStarted(pTree, pItem->m_nComponentType))
			//_T("An instance of the SYBASE Gateway is already running.");
			strMsg.LoadString (IDS_MSG_SYBASE_ALREADY_RUNNING);
		else if(theApp.m_bVista && !IVM_IsServiceRunning(theApp.m_strIngresServiceName))
			strMsg.LoadString (IDS_MSG_REQUIRE_SERVICE );
		else
			strMsg = _T("");
		break;
	case COMP_TYPE_GW_MSSQL:
		// MSSQL Gateway
		if (!IVM_IsStarted(pTree, COMP_TYPE_NAME))
			//_T("Unable to start the MSSQL Gateway, since the Name Server process is not running.");
			strMsg.LoadString (IDS_MSG_UNABLE_RUN_MSSQLxIIGCN);
		else
		if (IVM_IsStarted(pTree, pItem->m_nComponentType))
			//_T("An instance of the MSSQL Gateway is already running.");
			strMsg.LoadString (IDS_MSG_MSSQL_ALREADY_RUNNING);
		else if(theApp.m_bVista && !IVM_IsServiceRunning(theApp.m_strIngresServiceName))
			strMsg.LoadString (IDS_MSG_REQUIRE_SERVICE );
		else
			strMsg = _T("");
		break;
	case COMP_TYPE_GW_ODBC:
		// ODBC Gateway
		if (!IVM_IsStarted(pTree, COMP_TYPE_NAME))
			//_T("Unable to start the ODBC Gateway, since the Name Server process is not running.");
			strMsg.LoadString (IDS_MSG_UNABLE_RUN_ODBCxIIGCN);
		else
		if (IVM_IsStarted(pTree, pItem->m_nComponentType))
			//_T("An instance of the ODBC Gateway is already running.");
			strMsg.LoadString (IDS_MSG_ODBC_ALREADY_RUNNING);
		else if(theApp.m_bVista && !IVM_IsServiceRunning(theApp.m_strIngresServiceName))
			strMsg.LoadString (IDS_MSG_REQUIRE_SERVICE );
		else
			strMsg = _T("");
		break;
	default:
		//
		// Type of component is unknown !
		return strMsg;
	}

	return strMsg;
}


//
// Return the empty string if allowed, otherwise
// the string contains the message of the reason why it is not allowed.
CString IVM_AllowedStop (CTreeCtrl* pTree, CaTreeComponentItemData* pItem)
{
	CString strMsg = _T("System error (IVM_AllowedStop): Cannot stop this component");
	BOOL bInstallation = FALSE;
	ASSERT (pItem);
	if (!pItem)
		return strMsg;
	CaPageInformation* pPageInfo = pItem->GetPageInformation();
	ASSERT (pPageInfo);
	if (!pPageInfo)
		return strMsg;
	bInstallation = (pPageInfo->GetClassName().CompareNoCase (_T("INSTALLATION")) == 0);

	BOOL bSpecialServerClass = FALSE;

	if (pItem->m_nComponentType == COMP_TYPE_DBMS) {
		// loop on the "special instances" for detecting if the instance to be stopped has its
		// "normal" server class: if not, don't look for dependencies of other components
		// on the server and just return with no error message
		CaParseInstanceInfo * pInstance = NULL;
		POSITION  pos = theApp.GetSpecialInstanceInfo().GetSpecialInstancesList().GetHeadPosition();
		while (pos != NULL)
		{
			pInstance = theApp.GetSpecialInstanceInfo().GetSpecialInstancesList().GetNext(pos);
			if (pInstance->m_iservertype == pItem->m_nComponentType &&
				pInstance->m_csInstance.CompareNoCase(pItem->m_strComponentInstance)==0
			   ) {
				if (pInstance->m_csServerClass.CompareNoCase(_T("INGRES"))!=0) {
					bSpecialServerClass = TRUE;
					strMsg = _T("");
					return strMsg;
				}
			}
		}
	}


	//
	// Whole installation:
	if (bInstallation)
	{
		if (IVM_IsStarted(pTree, COMP_TYPE_NAME))
			strMsg = _T("");
		else
			//_T("Stopping the installation is meaningless, since the Name Server is not running");
			strMsg.LoadString(IDS_MSG_UNABLE_STOP_INSTALLATION);
		return strMsg;
	}

	switch (pItem->m_nComponentType)
	{
	case COMP_TYPE_NAME:
		// Name Server:
		if (IVM_IsStarted(pTree, COMP_TYPE_DBMS))
			//_T("Unable to stop the Name Server, since some DBMS Server processes are still running.");
			strMsg.LoadString(IDS_MSG_UNABLE_STOP_IIGCNxIIDBMS);
		else
		if (IVM_IsStarted(pTree, COMP_TYPE_NET))
			//_T("Unable to stop the Name Server, since some Net Server processes are still running.");
			strMsg.LoadString(IDS_MSG_UNABLE_STOP_IIGCNxIIGCC);
		else
		if (IVM_IsStarted(pTree, COMP_TYPE_JDBC))
			//_T("Unable to stop the Name Server, since some JDBC Server processes are still running.");
			strMsg.LoadString(IDS_MSG_UNABLE_STOP_IIGCNxIIJDBC);
		else
		if (IVM_IsStarted(pTree, COMP_TYPE_DASVR))
			//_T("Unable to stop the Name Server, since some DASVR Server processes are still running.");
			strMsg.LoadString(IDS_MSG_UNABLE_STOP_IIGCNxIIDASVR);
		else
		if (IVM_IsStarted(pTree, COMP_TYPE_GW_ORACLE)   ||
			IVM_IsStarted(pTree, COMP_TYPE_GW_DB2UDB)   ||
			IVM_IsStarted(pTree, COMP_TYPE_GW_INFORMIX) ||
			IVM_IsStarted(pTree, COMP_TYPE_GW_SYBASE)   ||
			IVM_IsStarted(pTree, COMP_TYPE_GW_MSSQL)    ||
			IVM_IsStarted(pTree, COMP_TYPE_GW_ODBC) 
		   )
			//_T("Unable to stop the Name Server, since some Gateways are still running.");
			strMsg.LoadString(IDS_MSG_UNABLE_STOP_IIGCNxGATEWAY);
		else
		if (IVM_IsStarted(pTree, pItem->m_nComponentType))
			strMsg = _T("");
		else
			//_T("The Name Server is not running");
			strMsg.LoadString(IDS_MSG_IIGCN_NOT_RUNNING);
		break;
	case COMP_TYPE_DBMS:
		// DBMS Server: Only if no instance of RMCMD, Net, Ice
		if (IVM_IsStarted(pTree, COMP_TYPE_RMCMD))
			//_T("Unable to stop the DBMS Server, since the Remote Command Server is still running.");
			strMsg.LoadString(IDS_MSG_UNABLE_STOP_IIDBMSxRMCMD);
		else
		if (IVM_IsStarted(pTree, COMP_TYPE_INTERNET_COM))
			//_T("Unable to stop the DBMS Server, since an ICE Server is still running.");
			strMsg.LoadString(IDS_MSG_UNABLE_STOP_IIDBMSxICE);
		else
		if (IVM_IsStarted(pTree, COMP_TYPE_STAR))
			//_T("Unable to stop the DBMS Server, since a Star Server is still running.");
			strMsg.LoadString(IDS_MSG_UNABLE_STOP_IIDBMSxSTAR);
		else
		if (IVM_IsStarted(pTree, pItem->m_nComponentType))
			strMsg = _T("");
		else
			//_T("No DBMS Server is running.");
			strMsg.LoadString(IDS_MSG_IIDBMS_NOT_RUNNING);
		break;

	case COMP_TYPE_INTERNET_COM:
		// ICE Server
		if (IVM_IsStarted(pTree, pItem->m_nComponentType))
			strMsg = _T("");
		else
			//_T("No ICE server is currently running.");
			strMsg.LoadString(IDS_MSG_ICE_NOT_RUNNING);
		break;
	case COMP_TYPE_NET:
		// NET Server: Only if no instance of STAR
		//                     no instance of Bridge
		if (IVM_IsStarted(pTree, COMP_TYPE_BRIDGE))
			//_T("Unable to stop the Net Server, since a Bridge Server is still running.");
			strMsg.LoadString(IDS_MSG_UNABLE_STOP_NETxBRIDGE);
		else
		if (IVM_IsStarted(pTree, COMP_TYPE_STAR))
			//_T("Unable to stop the Net Server, since a Star Server is still running.");
			strMsg.LoadString(IDS_MSG_UNABLE_STOP_NETxSTAR);
		else
		if (IVM_IsStarted(pTree, pItem->m_nComponentType))
			strMsg = _T("");
		else
			//_T("No NET Server is currently running.");
			strMsg.LoadString(IDS_MSG_IIGCC_NOT_RUNNING);
		break;
	case COMP_TYPE_JDBC:
		// JDBC Server: no special dependency here
		if (IVM_IsStarted(pTree, pItem->m_nComponentType))
			strMsg = _T("");
		else
			//_T("No NET Server is currently running.");
			strMsg.LoadString(IDS_MSG_IIJDBC_NOT_RUNNING);
		break;
	case COMP_TYPE_DASVR:
		// DASVR Server: no special dependency here
		if (IVM_IsStarted(pTree, pItem->m_nComponentType))
			strMsg = _T("");
		else
			//_T("No NET Server is currently running.");
			strMsg.LoadString(IDS_MSG_IIDASVR_NOT_RUNNING);
		break;


	case COMP_TYPE_BRIDGE:
		// BRIDGE Server: Only if it is running
		if (IVM_IsStarted(pTree, pItem->m_nComponentType))
			strMsg = _T("");
		else
			//_T("No BRIDGE Server is currently running.");
			strMsg.LoadString(IDS_MSG_BRIDGE_NOT_RUNNING);
		break;
	case COMP_TYPE_STAR:
		// STAR Server: Only if it is running
		if (IVM_IsStarted(pTree, pItem->m_nComponentType))
			strMsg = _T("");
		else
			//_T("No STAR Server is currently running.");
			strMsg.LoadString(IDS_MSG_STAR_NOT_RUNNING);
		break;
	case COMP_TYPE_SECURITY:
	case COMP_TYPE_LOCKING_SYSTEM:
	case COMP_TYPE_LOGGING_SYSTEM:
	case COMP_TYPE_TRANSACTION_LOG:
		break;

	case COMP_TYPE_RECOVERY:
		// RECOVERY Server: Only if no instance of DBMS
		if (IVM_IsStarted(pTree, COMP_TYPE_DBMS))
			//_T("Unable to Stop the Recovery Server, since a DBMS Server is still running.");
			strMsg.LoadString(IDS_MSG_UNABLE_STOP_RECOVERYxIIDBMS);
		else
			strMsg = _T("");
		break;
	case COMP_TYPE_ARCHIVER:
		// ARCHIVER Server: Only if no instance of DBMS
		if (IVM_IsStarted(pTree, COMP_TYPE_DBMS))
			//_T("Unable to stop the Archiver Server, since a DBMS Server is still running.");
			strMsg.LoadString(IDS_MSG_UNABLE_STOP_ARCHIVERxIIDBMS);
		else
			strMsg = _T("");
		break;

	case COMP_TYPE_OIDSK_DBMS:
		// Desktop DBMS
		break;

	case COMP_TYPE_RMCMD:
		// RMCMD: Only if it is running
		if (! IVM_IsStarted(pTree, pItem->m_nComponentType))
			//_T("The Remote Command Server is not running");
			strMsg.LoadString(IDS_MSG_RMCMD_NOT_RUNNING);
		else
			strMsg = _T("");
		break;

	case COMP_TYPE_GW_ORACLE:
		// Oracle Gateway
		if (!IVM_IsStarted(pTree, pItem->m_nComponentType))
			//_T("No ORACLE gateway is running");
			strMsg.LoadString(IDS_MSG_ORACLE_NOT_RUNNING);
		else
			strMsg = _T("");
		break;
	case COMP_TYPE_GW_DB2UDB:
		// db2udb Gateway
		if (!IVM_IsStarted(pTree, pItem->m_nComponentType))
			strMsg.Format(IDS_MSG_GATEWAY_NOT_RUNNING,_T("DB2 UDB"));
		else
			strMsg = _T("");
		break;
	case COMP_TYPE_GW_INFORMIX:
		// Informix Gateway: Only if it is running
		if (!IVM_IsStarted(pTree, pItem->m_nComponentType))
			//_T("No INFORMIX gateway is running");
			strMsg.LoadString(IDS_MSG_INFORMIX_NOT_RUNNING);
		else
			strMsg = _T("");
		break;
	case COMP_TYPE_GW_SYBASE:
		// Sybase Gateway: Only if it is running
		if (!IVM_IsStarted(pTree, pItem->m_nComponentType))
			//_T("No SYBASE gateway is running");
			strMsg.LoadString(IDS_MSG_SYBASE_NOT_RUNNING);
		else
			strMsg = _T("");
		break;
	case COMP_TYPE_GW_MSSQL:
		// MSSQL Gateway: Only if it is running
		if (!IVM_IsStarted(pTree, pItem->m_nComponentType))
			//_T("No MSSQL gateway is running");
			strMsg.LoadString(IDS_MSG_MSSQL_NOT_RUNNING);
		else
			strMsg = _T("");
		break;
	case COMP_TYPE_GW_ODBC:
		// ODBC Gateway: Only if it is running
		if (!IVM_IsStarted(pTree, pItem->m_nComponentType))
			//_T("No ODBC gateway is running");
			strMsg.LoadString(IDS_MSG_ODBC_NOT_RUNNING);
		else
			strMsg = _T("");
		break;
	default:
		//
		// Type of component is unknown !
		return strMsg;
	}

	return strMsg;
}

BOOL IVM_IsNameServerStarted()
{
	BOOL bStarted = FALSE;
	CdMainDoc* pMainDoc = theApp.GetMainDocument();
	ASSERT (pMainDoc);
	if (!pMainDoc)
		return bStarted;
	HWND hLeftView = pMainDoc->GetLeftViewHandle();
	if (hLeftView && ::IsWindow (hLeftView))
		bStarted = (BOOL)::SendMessage (hLeftView, WMUSRMSG_GET_STATE, (WPARAM)COMP_TYPE_NAME, 0);

	return bStarted;
}

void IVM_ShutDownIngres(CTreeCtrl* pTree)
{
	if (!IVM_IsStarted(pTree, COMP_TYPE_NAME))
		return;
	//
	// Check to see if Ingres is running as service:
	BOOL bService = FALSE;
#if !defined (MAINWIN)
	bService = IVM_IsServiceInstalled (theApp.m_strIngresServiceName);
	if (bService)
		bService = IVM_IsServiceRunning (theApp.m_strIngresServiceName);
#endif
	//
	// Shut down Ingres:
	if (!bService)
	{
		if (theApp.m_bVista)
			IVM_RunProcess(_T("winstart /param=\"-kill\""));
		else
			IVM_RunProcess (_T("ingstop -kill"));
	}
}


BOOL IsNameServer (CaPageInformation* pPageInfo)
{
	return pPageInfo->GetIvmItem()->m_nComponentType == COMP_TYPE_NAME;
}

BOOL IsRmcmdServer (CaPageInformation* pPageInfo)
{
	return pPageInfo->GetIvmItem()->m_nComponentType == COMP_TYPE_RMCMD;
}


void IVM_InitializeIngresVariable()
{
	CHARxINTxTYPExMINxMAX IEV[] = 
	{
		//
		// To set/unset -> use ingsetenv/ingunset:
		{_T("II_BIND_SVC_xx"),          IDS_II_BIND_SVC_xx,          P_TEXT,          -1,    -1, FALSE, FALSE},
		{_T("II_CHARSETxx"),            IDS_II_CHARSETxx,            P_TEXT,          -1,    -1, FALSE, TRUE},
		{_T("II_CLIENT"),               IDS_II_CLIENT,               P_TEXT,          -1,    -1, FALSE, FALSE},
		{_T("II_COLLATION"),            IDS_II_COLLATION,            P_TEXT,          -1,    -1, FALSE, FALSE},
		{_T("II_CONFIG"),               IDS_II_CONFIG,               P_TEXT,          -1,    -1, FALSE, TRUE},
		{_T("II_CONNECT_RETRIES"),      IDS_II_CONNECT_RETRIES,      P_TEXT,          -1,    -1, FALSE, FALSE},

		{_T("II_DATABASE"),             IDS_II_DATABASE,             P_PATH,          -1,    -1, FALSE, TRUE},
		{_T("II_CHECKPOINT"),           IDS_II_CHECKPOINT,           P_PATH,          -1,    -1, 0,     TRUE},
		{_T("II_DUMP"),                 IDS_II_DUMP,                 P_PATH,          -1,    -1, FALSE, TRUE},
		{_T("II_JOURNAL"),              IDS_II_JOURNAL,              P_PATH,          -1,    -1, FALSE, TRUE},
		{_T("II_WORK"),                 IDS_II_WORK,                 P_PATH,          -1,    -1, 0,     FALSE},

		{_T("II_DBMS_LOG"),             IDS_II_DBMS_LOG,             P_PATH_FILE,     -1,    -1, FALSE, FALSE},
		{_T("II_DUAL_LOG"),             IDS_II_DUAL_LOG,             P_PATH,          -1,    -1, FALSE, FALSE},
		{_T("II_DUAL_LOG_NAME"),        IDS_II_DUAL_LOG_NAME,        P_TEXT,          -1,    -1, FALSE, FALSE},
		{_T("II_LOG_FILE"),             IDS_II_LOG_FILE,             P_PATH,          -1,    -1, FALSE, FALSE},
		{_T("II_LOG_FILE_NAME"),        IDS_II_LOG_FILE_NAME,        P_TEXT,          -1,    -1, FALSE, FALSE},
		{_T("II_GCNxx_PORT"),           IDS_II_GCNxx_PORT,           P_TEXT,          -1,    -1, FALSE, TRUE},
		{_T("II_INSTALLATION"),         IDS_II_INSTALLATION,         P_TEXT,          -1,    -1, FALSE, TRUE},
		{_T("II_MSGDIR"),               IDS_II_MSGDIR,               P_TEXT,          -1,    -1, FALSE, TRUE},
		{_T("II_TIMEZONE_NAME"),        IDS_II_TIMEZONE_NAME,        P_TEXT,          -1,    -1, FALSE, FALSE},
		{_T("ING_SYSTEM_SET"),          IDS_ING_SYSTEM_SET,          P_TEXT,          -1,    -1, FALSE, FALSE},
//		{_T("VEC"),                     IDS_VEC,                     P_TEXT ,         -1,    -1, FALSE, FALSE},

	#if defined (MAINWIN)
		{_T("II_DIRECT_IO"),            IDS_II_DIRECT_IO,            P_TEXT,          -1,    -1, FALSE, FALSE},
		{_T("II_ERSEND"),               IDS_II_ERSEND,               P_TEXT,          -1,    -1, FALSE, FALSE},
		{_T("II_NUM_SLAVES"),           IDS_II_NUM_SLAVES,           P_TEXT,          -1,    -1, FALSE, FALSE},
	#endif
	#if defined (VMS) // VMS specific
		{_T("II_C_COMPILER"),           IDS_II_C_COMPILER,           P_TEXT,          -1,    -1, FALSE, FALSE},
		{_T("II_LOG_DEVICE"),           IDS_II_LOG_DEVICE,           P_TEXT,          -1,    -1, FALSE, FALSE},
	#endif
		{_T("II_SYSTEM"),               IDS_II_SYSTEM,               P_TEXT,          -1,    -1, FALSE, TRUE },
		{_T("II_CKPTMPL_FILE"),         IDS_II_CKPTMPL_FILE,         P_TEXT,          -1,    -1, FALSE, FALSE},
		

		//
		// To set/unset:
		//   1): Use ingsetenv/ingunset  (for Ingres System wide available for all users)
		//   2): Use the system registry (for user locally redefined)
	//	{_T("DBNAME_ING"),              IDS_DBNAME_ING,              P_TEXT,          -1,    -1, TRUE, FALSE },
	//	{_T("DBNAME_SQL_INIT"),         IDS_DBNAME_SQL_INIT,         P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_ABF_RUNOPT"),           IDS_II_ABF_RUNOPT,           P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_AFD_TIMEOUT"),          IDS_II_AFD_TIMEOUT,          P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_APPLICATION_LANGUAGE"), IDS_II_APPLICATION_LANGUAGE, P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_DATE_CENTURY_BOUNDARY"),IDS_II_DATE_CENTURY_BOUNDARY,P_NUMBER_SPIN,    0,   100, TRUE, FALSE },
		{_T("II_DATE_FORMAT"),          IDS_II_DATE_FORMAT,          P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_DBMS_SERVER"),          IDS_II_DBMS_SERVER,          P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_DECIMAL"),              IDS_II_DECIMAL,              P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_DML_DEF"),              IDS_II_DML_DEF,              P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_EMBED_SET"),            IDS_II_EMBED_SET,            P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_FRS_KEYFIND"),          IDS_II_FRS_KEYFIND,          P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_GCA_LOG"),              IDS_II_GCA_LOG,              P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_GCx_TRACE"),            IDS_II_GCx_TRACE,            P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_HELP_EDIT"),            IDS_II_HELP_EDIT,            P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_LANGUAGE"),             IDS_II_LANGUAGE,             P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_MONEY_FORMAT"),         IDS_II_MONEY_FORMAT,         P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_MONEY_PREC"),           IDS_II_MONEY_PREC,           P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_NULL_STRING"),          IDS_II_NULL_STRING,          P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_PATTERN_MATCH"),        IDS_II_PATTERN_MATCH,        P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_POST_4GLGEN"),          IDS_II_POST_4GLGEN,          P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_PRINTSCREEN_FILE"),     IDS_II_PRINTSCREEN_FILE,     P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_SQL_INIT"),             IDS_II_SQL_INIT,             P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_TEMPORARY"),            IDS_II_TEMPORARY,            P_PATH,          -1,    -1, TRUE, FALSE },
		{_T("II_TERMCAP_FILE"),         IDS_II_TERMCAP_FILE,         P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_TFDIR"),                IDS_II_TFDIR,                P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_TM_ON_ERROR"),          IDS_II_TM_ON_ERROR,          P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_VNODE_PATH"),           IDS_II_VNODE_PATH,           P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("ING_ABFDIR"),              IDS_ING_ABFDIR,              P_PATH,          -1,    -1, TRUE, FALSE },
		{_T("ING_ABFOPT1"),             IDS_ING_ABFOPT1,             P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("ING_EDIT"),                IDS_ING_EDIT,                P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("ING_PRINT"),               IDS_ING_PRINT,               P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("ING_SET"),                 IDS_ING_SET,                 P_TEXT,          -1,    -1, TRUE, FALSE },
//		{_T("ING_SET_DBNAME"),          IDS_ING_SET_DBNAME,          P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("INGRES_KEYS"),             IDS_INGRES_KEYS,             P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("INIT_INGRES"),             IDS_INIT_INGRES,             P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("TERM_INGRES"),             IDS_TERM_INGRES,             P_TEXT,          -1,    -1, TRUE, FALSE },
	#if defined (MAINWIN)
		{_T("TERM"),                    IDS_TERM,                    P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("ING_SHELL"),               IDS_ING_SHELL,               P_TEXT,          -1,    -1, TRUE, FALSE },
	#endif
		{_T("FORCEDITHER"),             IDS_FORCEDITHER,             P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("IIDLDIR"),                 IDS_IIDLDIR,                 P_PATH,          -1,    -1, TRUE, FALSE },
		{_T("IIW4GL_DEBUG_3GL"),        IDS_IIW4GL_DEBUG_3GL,        P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_4GL_DECIMAL"),          IDS_II_4GL_DECIMAL,          P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_COLORTABLE"),           IDS_II_COLORTABLE,           P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_COLORTABLE_FILE"),      IDS_II_COLORTABLE_FILE,      P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_DEBUG_W4GL"),           IDS_II_DEBUG_W4GL,           P_NUMBER_SPIN,    0,    32, TRUE, FALSE },
		{_T("II_DEFINE_COLORMAP"),      IDS_II_DEFINE_COLORMAP,      P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_DOUBLE_CLICK"),         IDS_II_DOUBLE_CLICK,         P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_FONT_CONVENTION"),      IDS_II_FONT_CONVENTION,      P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_FONT_FILE"),            IDS_II_FONT_FILE,            P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_FORCE_C_CONVENTION"),   IDS_II_FORCE_C_CONVENTION,   P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_KEYBOARD"),             IDS_II_KEYBOARD,             P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_KEYBOARD_FILE"),        IDS_II_KEYBOARD_FILE,        P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_LIBU3GL"),              IDS_II_LIBU3GL,              P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_MATCH_EMPTY_BLANK"),    IDS_II_MATCH_EMPTY_BLANK,    P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_MONOCHROME_DISPLAY"),   IDS_II_MONOCHROME_DISPLAY,   P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_SCREEN_HEIGHT_INCHES"), IDS_II_SCREEN_HEIGHT_INCHES, P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_SCREEN_HEIGHT_MMS"),    IDS_II_SCREEN_HEIGHT_MMS,    P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_SCREEN_WIDTH_INCHES"),  IDS_II_SCREEN_WIDTH_INCHES,  P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_SCREEN_WIDTH_MMS"),     IDS_II_SCREEN_WIDTH_MMS,     P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_VIEW"),                 IDS_II_VIEW,                 P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_W4GLAPPS_DIR"),         IDS_II_W4GLAPPS_DIR,         P_PATH,          -1,    -1, TRUE, FALSE },
		{_T("II_W4GLAPPS_SYS"),         IDS_II_W4GLAPPS_SYS,         P_PATH,          -1,    -1, TRUE, FALSE },
		{_T("II_W4GL_CACHE_LIMIT"),     IDS_II_W4GL_CACHE_LIMIT,     P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_W4GL_COLOR_TABLE_METHOD"),  IDS_II_W4GL_COLOR_TABLE_METHOD, P_NUMBER_SPIN, 222,    999, TRUE, FALSE },
		{_T("II_W4GL_LINEWIDTHS"),      IDS_II_W4GL_LINEWIDTHS,      P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_W4GL_LOCKS"),           IDS_II_W4GL_LOCKS,           P_NUMBER_SPIN,    0, 16000, TRUE, FALSE },
		{_T("II_W4GL_OPEN_IMAGES"),     IDS_II_W4GL_OPEN_IMAGES,     P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_W4GL_USE_256_COLORS"),  IDS_II_W4GL_USE_256_COLORS,  P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_W4GL_USE_PURE_COLORS"), IDS_II_W4GL_USE_PURE_COLORS, P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_WINDOWEDIT"),           IDS_II_WINDOWEDIT,           P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_WINDOWVIEW"),           IDS_II_WINDOWVIEW,           P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_PF_NODE"),              IDS_II_PF_NODE,              P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_HALF_DUPLEX"),          IDS_II_HALF_DUPLEX,          P_TEXT,          -1,    -1, TRUE, FALSE },
		{_T("II_STAR_LOG"),             IDS_II_STAR_LOG,             P_PATH,          -1,    -1, TRUE, FALSE },
		{_T("DD_RSERVERS"),             IDS_DD_RSERVERS,             P_PATH,          -1,    -1, TRUE, FALSE },

		{NULL, -1, P_TEXT, -1, -1, 0, 0}
	};

	int nSize = sizeof (IEV)/sizeof(CHARxINTxTYPExMINxMAX);
	theApp.m_ingresVariable.Init(IEV, nSize);
}


BOOL IVM_SetIngresVariable (LPCTSTR lpszName, LPCTSTR lpszValue, BOOL bSystem, BOOL bUnset)
{
	return theApp.m_ingresVariable.SetIngresVariable(lpszName, lpszValue, bSystem, bUnset);
}


BOOL IVM_IsDBATools(CString strII)
{
     HKEY hKey;
     char szKey[256];
     static int isDBATools=-1;
     char szData[1024];
     DWORD dwSize;

     if (isDBATools < 0)
     {
	isDBATools=0;
	sprintf(szKey, "SOFTWARE\\IngresCorporation\\Ingres\\%s_Installation", strII);
	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_QUERY_VALUE, &hKey))
	{
		dwSize=sizeof(szData);
		if (RegQueryValueEx(hKey, "dbatools", 0, 0, (BYTE*)szData, &dwSize)==ERROR_SUCCESS)
		{
			if (stricmp(szData, "1")==0)
			{
				isDBATools=1;
			}
		}
	}
     }	
     return isDBATools;				
}
