/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : svrsqlas.cpp : Defines the initialization routines for the DLL.
**    Project  : SQL ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Main dll
**
** History:
**
** 10-Jul-2001 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 26-Mar-2004 (uk$so01)
**    SIR #111701, The default help page is available now!. 
**    Activate the default page if no specific ID help is specified.
** 15-Mar-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
**    Now the help context IDs are available.
**/


#include "stdafx.h"
#include <htmlhelp.h>
#include "svrsqlas.h"
#include "frysqlas.h"
#include "libguids.h"
#include "ingobdml.h"
#include "tkwait.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// values for context-sensitive help - extracted from hpj file
#define HELP_F_DT_BYTE        7000
#define HELP_F_DT_C           7001
#define HELP_F_DT_CHAR        7002
#define HELP_F_DT_DATE        7003
#define HELP_F_DT_DECIMAL     7004
#define HELP_F_DT_DOW         7005
#define HELP_F_DT_FLOAT4      7006
#define HELP_F_DT_FLOAT8      7007
#define HELP_F_DT_HEX         7009
#define HELP_F_DT_INT1        7010
#define HELP_F_DT_INT2        7011
#define HELP_F_DT_INT4        7012
#define HELP_F_DT_LONGBYTE    7013
#define HELP_F_DT_LONGVARC    7014
#define HELP_F_DT_MONEY       7015
#define HELP_F_DT_OBJKEY      7016
#define HELP_F_DT_TABKEY      7017
#define HELP_F_DT_TEXT        7018
#define HELP_F_DT_VARBYTE     7019
#define HELP_F_DT_VARCHAR     7020

#define HELP_F_NUM_ABS        7021
#define HELP_F_NUM_ATAN       7022
#define HELP_F_NUM_COS        7023
#define HELP_F_NUM_EXP        7024
#define HELP_F_NUM_LOG        7025
#define HELP_F_NUM_MOD        7026
#define HELP_F_NUM_SIN        7027
#define HELP_F_NUM_SQRT       7028

#define HELP_F_STR_CHAREXT    7029
#define HELP_F_STR_CONCAT     7030
#define HELP_F_STR_LEFT       7031
#define HELP_F_STR_LENGTH     7032
#define HELP_F_STR_LOCATE     7033
#define HELP_F_STR_LOWERCS    7034
#define HELP_F_STR_PAD        7035
#define HELP_F_STR_RIGHT      7036
#define HELP_F_STR_SHIFT      7037
#define HELP_F_STR_SIZE       7038
#define HELP_F_STR_SQUEEZE    7039
#define HELP_F_STR_TRIM       7040
#define HELP_F_STR_NOTRIM     7041
#define HELP_F_STR_UPPERCS    7042

#define HELP_F_DAT_DATETRUNC  7043
#define HELP_F_DAT_DATEPART   7044
#define HELP_F_DAT_GMT        7045
#define HELP_F_DAT_INTERVAL   7046
#define HELP_F_DAT_DATE       7047
#define HELP_F_DAT_TIME       7048

#define HELP_F_AGG_ANY        7049
#define HELP_F_AGG_COUNT      7050
#define HELP_F_AGG_SUM        7051
#define HELP_F_AGG_AVG        7052
#define HELP_F_AGG_MAX        7053
#define HELP_F_AGG_MIN        7054

#define HELP_F_IFN_IFN        7055

#define HELP_F_PRED_COMP      7056
#define HELP_F_PRED_LIKE      7057
#define HELP_F_PRED_BETWEEN   7058
#define HELP_F_PRED_IN        7059
#define HELP_F_PRED_ANY       7060
#define HELP_F_PRED_ALL       7061
#define HELP_F_PRED_EXIST     7062
#define HELP_F_PRED_ISNULL    7063

#define HELP_F_PREDCOMB_AND   7064
#define HELP_F_PREDCOMB_OR    7065
#define HELP_F_PREDCOMB_NOT   7066

#define HELP_F_DB_COLUMNS     7067
#define HELP_F_DB_TABLES      7068
#define HELP_F_DB_DATABASES   7069
#define HELP_F_DB_USERS       7070
#define HELP_F_DB_GROUPS      7071
#define HELP_F_DB_ROLES       7072
#define HELP_F_DB_PROFILES    7073
#define HELP_F_DB_LOCATIONS   7074

// Section Added 25/11/96 Emb : context help numbers for desktop

#define HELP_OF_MATH_ABS        7100
#define HELP_OF_MATH_ACOS       7101
#define HELP_OF_MATH_ASIN       7102
#define HELP_OF_MATH_ATAN       7103
#define HELP_OF_MATH_ATAN2      7104
#define HELP_OF_MATH_COS        7105
#define HELP_OF_MATH_EXP        7106
#define HELP_OF_MATH_FACTORIAL  7107
#define HELP_OF_MATH_INT        7108
#define HELP_OF_MATH_LN         7109
#define HELP_OF_MATH_LOG        7110
#define HELP_OF_MATH_MOD        7111
#define HELP_OF_MATH_PI         7112
#define HELP_OF_MATH_ROUND      7113
#define HELP_OF_MATH_SIN        7114
#define HELP_OF_MATH_SQRT       7115
#define HELP_OF_MATH_TAN        7116

#define HELP_OF_FIN_CTERM       7117
#define HELP_OF_FIN_FV          7118
#define HELP_OF_FIN_PMT         7119
#define HELP_OF_FIN_PV          7120
#define HELP_OF_FIN_RATE        7121
#define HELP_OF_FIN_SLN         7122
#define HELP_OF_FIN_SYD         7123
#define HELP_OF_FIN_TERM        7124

#define HELP_OF_DT_DATE         7125
#define HELP_OF_DT_DATETOCHAR   7126
#define HELP_OF_DT_DATEVALUE    7127
#define HELP_OF_DT_DAY          7128
#define HELP_OF_DT_HOUR         7129
#define HELP_OF_DT_MICROSECOND  7130
#define HELP_OF_DT_MINUTE       7131
#define HELP_OF_DT_MONTH        7132
#define HELP_OF_DT_MONTHBEG     7133
#define HELP_OF_DT_NOW          7134
#define HELP_OF_DT_QUARTER      7135
#define HELP_OF_DT_QUARTERBEG   7136
#define HELP_OF_DT_SECOND       7137
#define HELP_OF_DT_TIME         7138
#define HELP_OF_DT_TIMEVALUE    7139
#define HELP_OF_DT_WEEKBEG      7140
#define HELP_OF_DT_WEEKDAY      7141
#define HELP_OF_DT_YEAR         7142
#define HELP_OF_DT_YEARBEG      7143
#define HELP_OF_DT_YEARNO       7144

#define HELP_OF_STR_CHAR        7145
#define HELP_OF_STR_CODE        7146
#define HELP_OF_STR_DECODE      7147
#define HELP_OF_STR_EXACT       7148
#define HELP_OF_STR_FIND        7149
#define HELP_OF_STR_LEFT        7150
#define HELP_OF_STR_LENGTH      7151
#define HELP_OF_STR_LOWER       7152
#define HELP_OF_STR_MID         7153
#define HELP_OF_STR_NULLVALUE   7154
#define HELP_OF_STR_PROPER      7155
#define HELP_OF_STR_REPEAT      7156
#define HELP_OF_STR_REPLACE     7157
#define HELP_OF_STR_RIGHT       7158
#define HELP_OF_STR_SCAN        7159
#define HELP_OF_STR_STRING      7160
#define HELP_OF_STR_SUBSTRING   7161
#define HELP_OF_STR_TRIM        7162
#define HELP_OF_STR_UPPER       7163
#define HELP_OF_STR_VALUE       7164

#define HELP_OF_AGG_AVG         7165
#define HELP_OF_AGG_COUNT       7166
#define HELP_OF_AGG_MAX         7167
#define HELP_OF_AGG_MEDIAN      7168
#define HELP_OF_AGG_MIN         7169
#define HELP_OF_AGG_SUM         7170
#define HELP_OF_AGG_SDV         7171

#define HELP_OF_SPE_CHOOSE      7172
#define HELP_OF_SPE_DECIMAL     7173
#define HELP_OF_SPE_DECRYPT     7174
#define HELP_OF_SPE_DECODE      7175
#define HELP_OF_SPE_HEX         7176
#define HELP_OF_SPE_LICS        7177

#define HELP_OF_DB_DBAREA       7178
#define HELP_OF_DB_STOGROUP     7179

// End section added 25/11/96

#define HELP_F_SUBSEL_SUBSEL    7075    // ???
//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

static BOOL SetRegKeyValue(LPTSTR pszKey, LPTSTR pszSubkey, LPCTSTR pszValue);
static BOOL AddRegNamedValue(LPTSTR pszKey, LPTSTR pszSubkey, LPTSTR pszValueName, LPCTSTR pszValue);


//
// Animation Section:
// ************************************************************************************************
class CaSwitchSessionThread
{
public:
	CaSwitchSessionThread(BOOL bFirstEnable): m_bFirstEnable(bFirstEnable)
	{
		In();
		m_nOut = FALSE;
	}
	~CaSwitchSessionThread()
	{
		if (m_nOut)
			return;
		Out();
	}

	int In();
	int Out();

private:
	BOOL m_nOut;
	BOOL m_bFirstEnable;
};

int CaSwitchSessionThread::In()
{
	return 0;
}

int CaSwitchSessionThread::Out()
{
	return 0;
}


class CaExecParamQueryData: public CaExecParam
{
public:
	CaExecParamQueryData(CaLLQueryInfo* pInfo, CaSessionManager* pSmgr, CTypedPtrList<CObList, CaDBObject*>* pListObject);
	virtual ~CaExecParamQueryData(){}
	virtual int  Run(HWND hWndTaskDlg = NULL);
	virtual void OnTerminate(HWND hWndTaskDlg = NULL);
	virtual BOOL OnCancelled(HWND hWndTaskDlg = NULL);
	BOOL IsInterrupted(){return m_bInterrupted;}
	BOOL IsSucceeded(){return m_bSucceeded;}

	CString m_strError;
private:
	CaSessionManager* m_pSessionManager;
	CaLLQueryInfo* m_pQueryInfo;
	CTypedPtrList<CObList, CaDBObject*>* m_pListObject;
	BOOL m_bInterrupted;
	BOOL m_bSucceeded;
};




CaExecParamQueryData::CaExecParamQueryData(CaLLQueryInfo* pInfo, CaSessionManager* pSmgr, CTypedPtrList<CObList, CaDBObject*>* pListObject)
	:CaExecParam(INTERRUPT_NOT_ALLOWED), m_pSessionManager(pSmgr), m_pQueryInfo(pInfo), m_pListObject(pListObject)
{
	SetExecuteImmediately (FALSE);
	SetDelayExecution (2000);
	m_bInterrupted = FALSE;
	m_bSucceeded = TRUE;
	m_strError = _T("");
}

int CaExecParamQueryData::Run(HWND hWndTaskDlg)
{
	BOOL bOK = FALSE;
	try
	{
#if defined (_ANIMATION_)
		BOOL bEnableFirst = TRUE;
		CaSwitchSessionThread sw (bEnableFirst);
#endif
		CTypedPtrList<CObList, CaDBObject*>& listObj = *m_pListObject;
		bOK = INGRESII_llQueryObject (m_pQueryInfo, listObj, m_pSessionManager);
		if (!bOK)
			m_bSucceeded = FALSE;
#if defined (_ANIMATION_)
		sw.Out();
#endif
		return bOK? 0: 1;
	}
	catch (CeSqlException e)
	{
		m_strError = e.GetReason();
	}
	catch (...)
	{
		TRACE0("Exception in: CaExecParamQueryData::Run\n");
	}
	m_bSucceeded = FALSE;
	return 1;
}

void CaExecParamQueryData::OnTerminate(HWND hWndTaskDlg)
{

}

BOOL CaExecParamQueryData::OnCancelled(HWND hWndTaskDlg)
{


	return FALSE;
}


//
//  CSvrsqlasApp Section:
// ************************************************************************************************
BEGIN_MESSAGE_MAP(CSvrsqlasApp, CWinApp)
	//{{AFX_MSG_MAP(CSvrsqlasApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSvrsqlasApp construction

CSvrsqlasApp::CSvrsqlasApp()
{
	lstrcpy (m_tchszOutOfMemoryMessage, _T("Low of Memory ...\nCannot allocate memory, please close some applications !"));
	m_strHelpFile = _T("sqlassis.chm");
	m_bHelpFileAvailable = TRUE;
	m_strInstallationID = _T("");

	Init();
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CSvrsqlasApp object

CSvrsqlasApp theApp;

BOOL CSvrsqlasApp::InitInstance() 
{
	m_strInstallationID = INGRESII_QueryInstallationID();
	m_pServer = new CaComServer();
	m_pServer->m_hInstServer = m_hInstance;
	
	BOOL bOk = CWinApp::InitInstance();
	if (bOk)
	{
		bOk = TaskAnimateInitialize();
		if (!bOk)
		{
			//
			// Failed to load the dll %1.
			CString strMsg;
			AfxFormatString1(strMsg, IDS_FAIL_TO_LOAD_DLL, _T("<TkAnimate.dll>"));
			AfxMessageBox (strMsg);
			return FALSE;
		}
	}
	return bOk;
}

int CSvrsqlasApp::ExitInstance() 
{
	m_sessionManager.Cleanup();
	Done();

	if (m_pServer)
	{
		delete m_pServer;
		m_pServer = NULL;
	}
	return CWinApp::ExitInstance();
}

BOOL CSvrsqlasApp::UnicodeOk()
{
	BOOL bOk = TRUE;
	TCHAR tchszUserName[256];
	DWORD dwSize = 256;

	if (!GetUserName(tchszUserName, &dwSize))
		bOk = ERROR_CALL_NOT_IMPLEMENTED == GetLastError() ? FALSE : TRUE;
	return bOk;
}

void CSvrsqlasApp::Init()
{
	//
	// Data Conversion:
	m_listSQLDataConv.AddTail(new CaSQLComponent(TRUE, F_DT_BYTE,     _T("BYTE"),        _T("BYTE(expr,[,len])"), IDS_H_DATACONV1_BYTE));
	m_listSQLDataConv.AddTail(new CaSQLComponent(TRUE, F_DT_C,        _T("C"),           _T("C(expr,[,len])"),    IDS_H_DATACONV1_C));
	m_listSQLDataConv.AddTail(new CaSQLComponent(TRUE, F_DT_CHAR,     _T("CHAR"),        _T("CHAR(expr,[,len])"), IDS_H_DATACONV1_CHAR));
	m_listSQLDataConv.AddTail(new CaSQLComponent(TRUE, F_DT_DATE,     _T("DATE"),        _T("DATE(expr)"),        IDS_H_DATACONV1_DATE));
	m_listSQLDataConv.AddTail(new CaSQLComponent(TRUE, F_DT_DECIMAL,  _T("DECIMAL"),     _T("DECIMAL(expr,[,precision[,scale]])"), IDS_H_DATACONV1_DECIMAL));
	m_listSQLDataConv.AddTail(new CaSQLComponent(TRUE, F_DT_DOW,      _T("DOW"),         _T("DOW(expr)"),         IDS_H_DATACONV1_DOW));
	m_listSQLDataConv.AddTail(new CaSQLComponent(TRUE, F_DT_FLOAT4,   _T("FLOAT4"),      _T("FLOAT4(expr)"),      IDS_H_DATACONV1_FLOAT4));
	m_listSQLDataConv.AddTail(new CaSQLComponent(TRUE, F_DT_FLOAT8,   _T("FLOAT8"),      _T("FLOAT8(expr)"),      IDS_H_DATACONV1_FLOAT8));
	m_listSQLDataConv.AddTail(new CaSQLComponent(TRUE, F_DT_HEX,      _T("HEX"),         _T("HEX(expr)"),     IDS_H_DATACONV1_HEX));
	m_listSQLDataConv.AddTail(new CaSQLComponent(TRUE, F_DT_INT1,     _T("INT1"),        _T("INT1(expr)"),    IDS_H_DATACONV1_INT1));
	m_listSQLDataConv.AddTail(new CaSQLComponent(TRUE, F_DT_INT2,     _T("INT2"),        _T("INT2(expr)"),    IDS_H_DATACONV1_INT2));
	m_listSQLDataConv.AddTail(new CaSQLComponent(TRUE, F_DT_INT4,     _T("INT4"),        _T("INT4(expr)"),    IDS_H_DATACONV1_INT4));
	m_listSQLDataConv.AddTail(new CaSQLComponent(TRUE, F_DT_LONGBYTE, _T("LONG_BYTE"),   _T("LONG_BYTE(expr[,len])"), IDS_H_DATACONV1_LONGBYTE));
	m_listSQLDataConv.AddTail(new CaSQLComponent(TRUE, F_DT_LONGVARC, _T("LONG_VARCHAR"),_T("LONG_VARCHAR(expr)"),    IDS_H_DATACONV1_LONGVARCHAR));
	m_listSQLDataConv.AddTail(new CaSQLComponent(TRUE, F_DT_MONEY,    _T("MONEY"),       _T("MONEY(expr)"),           IDS_H_DATACONV1_MONEY));
	m_listSQLDataConv.AddTail(new CaSQLComponent(TRUE, F_DT_OBJKEY,   _T("OBJECT_KEY"),  _T("OBJECT_KEY(expr)"),      IDS_H_DATACONV1_OBJECTKEY));
	m_listSQLDataConv.AddTail(new CaSQLComponent(TRUE, F_DT_TABKEY,   _T("TABLE_KEY"),   _T("TABLE_KEY(expr)"),       IDS_H_DATACONV1_TABLEKEY));
	m_listSQLDataConv.AddTail(new CaSQLComponent(TRUE, F_DT_TEXT,     _T("TEXT"),        _T("TEXT(expr[,len])"),      IDS_H_DATACONV1_TEXT));
	m_listSQLDataConv.AddTail(new CaSQLComponent(TRUE, F_DT_VARBYTE,  _T("VARBYTE"),     _T("VARBYTE(expr[,len])"),   IDS_H_DATACONV1_VARBYTE));
	m_listSQLDataConv.AddTail(new CaSQLComponent(TRUE, F_DT_VARCHAR,  _T("VARCHAR"),     _T("VARCHAR(expr[,len])"),   IDS_H_DATACONV1_VARCHAR));

	//
	// Numeric Functions:
	m_listSQLNumFuncs.AddTail(new CaSQLComponent(TRUE, F_NUM_ABS,  _T("ABS"), _T("ABS(n)"),    IDS_H_NUMERIC_ABS));
	m_listSQLNumFuncs.AddTail(new CaSQLComponent(TRUE, F_NUM_ATAN, _T("ATAN"),_T("ATAN(n)"),   IDS_H_NUMERIC_ATAN));
	m_listSQLNumFuncs.AddTail(new CaSQLComponent(TRUE, F_NUM_COS,  _T("COS"), _T("COS(n)"),    IDS_H_NUMERIC_COS));
	m_listSQLNumFuncs.AddTail(new CaSQLComponent(TRUE, F_NUM_EXP,  _T("EXP"), _T("EXP(n)"),    IDS_H_NUMERIC_EXP));
	m_listSQLNumFuncs.AddTail(new CaSQLComponent(TRUE, F_NUM_LOG,  _T("LOG"), _T("LOG(n)"),    IDS_H_NUMERIC_LOG));
	m_listSQLNumFuncs.AddTail(new CaSQLComponent(TRUE, F_NUM_MOD,  _T("MOD"), _T("MOD(n, b)"), IDS_H_NUMERIC_MOD));
	m_listSQLNumFuncs.AddTail(new CaSQLComponent(TRUE, F_NUM_SIN,  _T("SIN"), _T("SIN(n)"),    IDS_H_NUMERIC_SIN));
	m_listSQLNumFuncs.AddTail(new CaSQLComponent(TRUE, F_NUM_SQRT, _T("SQRT"),_T("SQRT(n)"),   IDS_H_NUMERIC_SQRT));
	
	//
	// String Functions:
	m_listSQLStrFuncs.AddTail(new CaSQLComponent(TRUE, F_STR_CHAREXT,_T("CHAREXTRACT"),_T("CHAREXTRACT(c1,n)"),IDS_H_STRING_CHAREXTRACT));
	m_listSQLStrFuncs.AddTail(new CaSQLComponent(TRUE, F_STR_CONCAT, _T("CONCAT"),     _T("CONCAT(c1,c2)"),    IDS_H_STRING_CONCAT));
	m_listSQLStrFuncs.AddTail(new CaSQLComponent(TRUE, F_STR_LEFT,   _T("LEFT"),       _T("LEFT(c1,len)"),     IDS_H_STRING_LEFT));
	m_listSQLStrFuncs.AddTail(new CaSQLComponent(TRUE, F_STR_LENGTH, _T("LENGTH"),     _T("LENGTH(c1)"),       IDS_H_STRING_LENGTH));
	m_listSQLStrFuncs.AddTail(new CaSQLComponent(TRUE, F_STR_LOCATE, _T("LOCATE"),     _T("LOCATE(c1,c2)"),    IDS_H_STRING_LOCATE));
	m_listSQLStrFuncs.AddTail(new CaSQLComponent(TRUE, F_STR_LOWERCS,_T("LOWERCASE"),  _T("LOWERCASE(c1)"),    IDS_H_STRING_LOWERCASE));
	m_listSQLStrFuncs.AddTail(new CaSQLComponent(TRUE, F_STR_PAD,    _T("PAD"),        _T("PAD(c1)"),          IDS_H_STRING_PAD));
	m_listSQLStrFuncs.AddTail(new CaSQLComponent(TRUE, F_STR_RIGHT,  _T("RIGHT"),      _T("RIGHT(c1,len)"),    IDS_H_STRING_RIGHT));
	m_listSQLStrFuncs.AddTail(new CaSQLComponent(TRUE, F_STR_SHIFT,  _T("SHIFT"),      _T("SHIFT(c1,nshift)"), IDS_H_STRING_SHIFT));
	m_listSQLStrFuncs.AddTail(new CaSQLComponent(TRUE, F_STR_SIZE,   _T("SIZE"),       _T("SIZE(c1)"),         IDS_H_STRING_SIZE));
	m_listSQLStrFuncs.AddTail(new CaSQLComponent(TRUE, F_STR_SQUEEZE,_T("SQUEEZE"),    _T("SQUEEZE(c1)"),      IDS_H_STRING_SQUEEZE));
	m_listSQLStrFuncs.AddTail(new CaSQLComponent(TRUE, F_STR_TRIM,   _T("TRIM"),       _T("TRIM(c1)"),         IDS_H_STRING_TRIM));
	m_listSQLStrFuncs.AddTail(new CaSQLComponent(TRUE, F_STR_NOTRIM, _T("NOTRIM"),     _T("NOTRIM(c1)"),       IDS_H_STRING_NOTRIM));
	m_listSQLStrFuncs.AddTail(new CaSQLComponent(TRUE, F_STR_UPPERCS,_T("UPPERCS"),    _T("UPPERCS(c1)"),      IDS_H_STRING_UPPERCS));
	
	//
	// Date Functions:
	m_listSQLDateFuncs.AddTail(new CaSQLComponent(TRUE, F_DAT_DATETRUNC, _T("DATE_TRUNC"), _T("DATE_TRUNC(unit,date)"), IDS_H_DATE_DATETRUNC));
	m_listSQLDateFuncs.AddTail(new CaSQLComponent(TRUE, F_DAT_DATEPART,  _T("DATE_PART"),  _T("DATE_PART(unit,date)"),  IDS_H_DATE_DATEPART));
	m_listSQLDateFuncs.AddTail(new CaSQLComponent(TRUE, F_DAT_GMT,       _T("DATE_GMT"),   _T("DATE_GMT(date)"),        IDS_H_DATE_GMT));
	m_listSQLDateFuncs.AddTail(new CaSQLComponent(TRUE, F_DAT_INTERVAL,  _T("INTERVAL"),   _T("INTERVAL(unit,date_interval)"), IDS_H_DATE_INTERVAL));
	m_listSQLDateFuncs.AddTail(new CaSQLComponent(TRUE, F_DAT_DATE,      _T("_DATE"),      _T("_DATE(s)"),    IDS_H_DATE_DATE));
	m_listSQLDateFuncs.AddTail(new CaSQLComponent(TRUE, F_DAT_TIME,      _T("_TIME"),      _T("_TIME(s)"),    IDS_H_DATE_TIME));
	
	//
	// Aggregate Functions:
	m_listSQLAggFuncs.AddTail(new CaSQLComponent(TRUE, F_AGG_ANY,    _T("ANY"),  _T("ANY([distinct|all] expr)"),  IDS_H_AGG_ANY));
	m_listSQLAggFuncs.AddTail(new CaSQLComponent(TRUE, F_AGG_COUNT,  _T("COUNT"),_T("COUNT([distinct|all] expr)"),IDS_H_AGG_COUNT));
	m_listSQLAggFuncs.AddTail(new CaSQLComponent(TRUE, F_AGG_SUM,    _T("SUM"),  _T("SUM([distinct|all] expr)"),  IDS_H_AGG_SUM));
	m_listSQLAggFuncs.AddTail(new CaSQLComponent(TRUE, F_AGG_AVG,    _T("AVG"),  _T("AVG([distinct|all] expr)"),  IDS_H_AGG_AVG));
	m_listSQLAggFuncs.AddTail(new CaSQLComponent(TRUE, F_AGG_MAX,    _T("MAX"),  _T("MAX(expr)"), IDS_H_AGG_MAX));
	m_listSQLAggFuncs.AddTail(new CaSQLComponent(TRUE, F_AGG_MIN,    _T("MIN"),  _T("MIN(expr)"), IDS_H_AGG_MIN));
	
	//
	// If Functions:
	m_listSQLIfnFuncs.AddTail(new CaSQLComponent(TRUE, F_IFN_IFN,    _T("IFNULL"),_T("IFNULL(v1,v2)"), IDS_H_IF_IFNULL));

	//
	// Predicate Functions:
	m_listSQLPredFuncs.AddTail(new CaSQLComponent(TRUE, F_PRED_COMP,   _T("Comparison"),_T("expr1 comparison_operator expr2"),       IDS_H_PRED_COMPARISON));
	m_listSQLPredFuncs.AddTail(new CaSQLComponent(TRUE, F_PRED_LIKE,   _T("Like"),      _T("expr LIKE pattern [ESCAPE escape_char]"),IDS_H_PRED_LIKE));
	m_listSQLPredFuncs.AddTail(new CaSQLComponent(TRUE, F_PRED_BETWEEN,_T("Between"),   _T("y BETWEEN x AND z"),    -1));
	m_listSQLPredFuncs.AddTail(new CaSQLComponent(TRUE, F_PRED_IN,     _T("In"),        _T("y IN (x,...,z) or y IN (subquery)"), IDS_H_PRED_IN));
	m_listSQLPredFuncs.AddTail(new CaSQLComponent(TRUE, F_PRED_ANY,    _T("Any"),       _T("ANY(subquery)"),   IDS_H_PRED_ANY));
	m_listSQLPredFuncs.AddTail(new CaSQLComponent(TRUE, F_PRED_ALL,    _T("All"),       _T("ALL(subquery)"),   IDS_H_PRED_ALL));
	m_listSQLPredFuncs.AddTail(new CaSQLComponent(TRUE, F_PRED_EXIST,  _T("Exists"),    _T("EXISTS(subquery)"),IDS_H_PRED_EXISTS));
	m_listSQLPredFuncs.AddTail(new CaSQLComponent(TRUE, F_PRED_ISNULL, _T("Is null"),   _T("x is [not] null"), IDS_H_PRED_ISNULL));
	
	//
	// Predicate Combination Functions:
	m_listSQLPredCombFuncs.AddTail(new CaSQLComponent(TRUE, F_PREDCOMB_AND, _T("AND"), _T("Predicate AND Predicate"), IDS_H_PREDCOMB_AND));
	m_listSQLPredCombFuncs.AddTail(new CaSQLComponent(TRUE, F_PREDCOMB_OR,  _T("OR"),  _T("Predicate OR Predicate"),  IDS_H_PREDCOMB_OR));
	m_listSQLPredCombFuncs.AddTail(new CaSQLComponent(TRUE, F_PREDCOMB_NOT, _T("NOT"), _T("NOT Predicate"),           IDS_H_PREDCOMB_NOT));

	//
	// SQL Database Object Functions:
	m_listSQLDBObjects.AddTail(new CaSQLComponent(TRUE, F_DB_COLUMNS,    _T("COLUMN"),  _T("Column"),  IDS_H_DB_COLUMN));
	m_listSQLDBObjects.AddTail(new CaSQLComponent(TRUE, F_DB_TABLES,     _T("TABLE"),   _T("Table"),   IDS_H_DB_TABLE));
	m_listSQLDBObjects.AddTail(new CaSQLComponent(TRUE, F_DB_DATABASES , _T("DATABASE"),_T("Database"),IDS_H_DB_DATABASE));
	m_listSQLDBObjects.AddTail(new CaSQLComponent(TRUE, F_DB_USERS     , _T("USER"),    _T("User") ,   IDS_H_DB_USER));
	m_listSQLDBObjects.AddTail(new CaSQLComponent(TRUE, F_DB_GROUPS    , _T("GROUP"),   _T("Group") ,  IDS_H_DB_GROUP));
	m_listSQLDBObjects.AddTail(new CaSQLComponent(TRUE, F_DB_ROLES     , _T("ROLE"),    _T("Role") ,   IDS_H_DB_ROLE));
	m_listSQLDBObjects.AddTail(new CaSQLComponent(TRUE, F_DB_PROFILES  , _T("PROFILE"), _T("Profile") ,IDS_H_DB_PROFILE));
	m_listSQLDBObjects.AddTail(new CaSQLComponent(TRUE, F_DB_LOCATIONS , _T("LOCATION"),_T("Location"),IDS_H_DB_LOCATION));

	//
	// SQL Select Function:
	m_listSQLSelFuncs.AddTail(new CaSQLComponent(TRUE, F_SUBSEL_SUBSEL , _T("SELECT"), _T("Sub)Select Statement") , IDS_H_SUBSEL));

	//
	// SQL Familis:
	m_listSqlFamily.AddTail(new CaSQLFamily(_T("Data Type Conversion"),       FF_DTCONVERSION     , &m_listSQLDataConv     ));
	m_listSqlFamily.AddTail(new CaSQLFamily(_T("Numeric functions"),          FF_NUMFUNCTIONS     , &m_listSQLNumFuncs     ));
	m_listSqlFamily.AddTail(new CaSQLFamily(_T("String Functions"),           FF_STRFUNCTIONS     , &m_listSQLStrFuncs     ));
	m_listSqlFamily.AddTail(new CaSQLFamily(_T("Date Functions"),             FF_DATEFUNCTIONS    , &m_listSQLDateFuncs    ));
	m_listSqlFamily.AddTail(new CaSQLFamily(_T("Aggregate Functions"),        FF_AGGRFUNCTIONS    , &m_listSQLAggFuncs     ));
	m_listSqlFamily.AddTail(new CaSQLFamily(_T("IfNull Function"),            FF_IFNULLFUNC       , &m_listSQLIfnFuncs     ));
	m_listSqlFamily.AddTail(new CaSQLFamily(_T("Search Condition Predicate"), FF_PREDICATES       , &m_listSQLPredFuncs    ));
	m_listSqlFamily.AddTail(new CaSQLFamily(_T("Predicate Combinations"),     FF_PREDICCOMB       , &m_listSQLPredCombFuncs));
	m_listSqlFamily.AddTail(new CaSQLFamily(_T("Database Objects"),           FF_DATABASEOBJECTS  , &m_listSQLDBObjects    ));
	m_listSqlFamily.AddTail(new CaSQLFamily(_T("SubQueries"),                 FF_SUBSELECT        , &m_listSQLSelFuncs     ));
}

void CSvrsqlasApp::Done()
{
	while (!m_listSqlFamily.IsEmpty())
		delete m_listSqlFamily.RemoveHead();
	while (!m_listSQLDataConv.IsEmpty())
		delete m_listSQLDataConv.RemoveHead();
	while (!m_listSQLNumFuncs.IsEmpty())
		delete m_listSQLNumFuncs.RemoveHead();
	while (!m_listSQLStrFuncs.IsEmpty())
		delete m_listSQLStrFuncs.RemoveHead();
	while (!m_listSQLDateFuncs.IsEmpty())
		delete m_listSQLDateFuncs.RemoveHead();
	while (!m_listSQLAggFuncs.IsEmpty())
		delete m_listSQLAggFuncs.RemoveHead();
	while (!m_listSQLIfnFuncs.IsEmpty())
		delete m_listSQLIfnFuncs.RemoveHead();
	while (!m_listSQLPredFuncs.IsEmpty())
		delete m_listSQLPredFuncs.RemoveHead();
	while (!m_listSQLPredCombFuncs.IsEmpty())
		delete m_listSQLPredCombFuncs.RemoveHead();
	while (!m_listSQLDBObjects.IsEmpty())
		delete m_listSQLDBObjects.RemoveHead();
	while (!m_listSQLSelFuncs.IsEmpty())
		delete m_listSQLSelFuncs.RemoveHead();
}

//
// This function should call the query objects from the Static library ("INGRESII_llQueryObject") or
// from the COM Server ICAS.
BOOL CSvrsqlasApp::INGRESII_QueryObject(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	CaSessionManager& ssMgr = GetSessionManager();
	BOOL bOK = TRUE;
	CaExecParamQueryData* pExecParam = new CaExecParamQueryData (pInfo, &ssMgr, &listObject);
#if defined (_ANIMATION_)
	BOOL bEnableFirst = FALSE;
	CaSwitchSessionThread sw (bEnableFirst);

	CString strMsgAnimateTitle = _T("Querying objects...");
	strMsgAnimateTitle.LoadString (IDS_ANIMATE_TITLE_QUERYOBJECT);
	CxDlgWait dlg (strMsgAnimateTitle);
	dlg.SetUseAnimateAvi(AVI_FETCHF);
	dlg.SetExecParam (pExecParam);
	dlg.SetDeleteParam(FALSE);
	dlg.SetShowProgressBar(FALSE);
	dlg.SetHideCancelBottonAlways(TRUE);
	dlg.DoModal();

	sw.Out();
#else
	pExecParam->Run();
#endif

	if (!pExecParam->IsSucceeded())
	{
		while (!listObject.IsEmpty())
			delete listObject.RemoveHead();

		bOK = FALSE;
		//
		// Or throw exception:
		if (!pExecParam->m_strError.IsEmpty())
			AfxMessageBox (pExecParam->m_strError);
	}

	delete pExecParam;
	return bOK;
}

static UINT GetContextHelpId(UINT nId)
{
	switch(nId) 
	{
	case OF_DB_DBAREA:
		return HELP_OF_DB_DBAREA;
	case OF_DB_STOGROUP:
		return HELP_OF_DB_STOGROUP;

	case F_DT_BYTE: return HELP_F_DT_BYTE;
	case F_DT_C:return HELP_F_DT_C;
	case F_DT_CHAR:return HELP_F_DT_CHAR;
	case F_DT_DATE:return HELP_F_DT_DATE;
	case F_DT_DECIMAL:return HELP_F_DT_DECIMAL;
	case F_DT_DOW:
			return HELP_F_DT_DOW;
	case F_DT_FLOAT4:
			return HELP_F_DT_FLOAT4;
	case F_DT_FLOAT8:
		return HELP_F_DT_FLOAT8;
	case F_DT_HEX:
			return HELP_F_DT_HEX;
	case F_DT_INT1:
			return HELP_F_DT_INT1;
	case F_DT_INT2:
		return HELP_F_DT_INT2;
	case F_DT_INT4:
		return HELP_F_DT_INT4;
	case F_DT_LONGBYTE:
		return HELP_F_DT_LONGBYTE;
	case F_DT_LONGVARC:return HELP_F_DT_LONGVARC;
	case F_DT_MONEY:return HELP_F_DT_MONEY;
	case F_DT_OBJKEY:return HELP_F_DT_OBJKEY;
	case F_DT_TABKEY:return HELP_F_DT_TABKEY;
	case F_DT_TEXT:return HELP_F_DT_TEXT;
	case F_DT_VARBYTE:return HELP_F_DT_VARBYTE;
	case F_DT_VARCHAR:return HELP_F_DT_VARCHAR;

	case F_NUM_ABS:return HELP_F_NUM_ABS;
	case F_NUM_ATAN:return HELP_F_NUM_ATAN;
	case F_NUM_COS:return HELP_F_NUM_COS;
	case F_NUM_EXP:return HELP_F_NUM_EXP;
	case F_NUM_LOG:return HELP_F_NUM_LOG;
	case F_NUM_MOD:return HELP_F_NUM_MOD;
	case F_NUM_SIN:return HELP_F_NUM_SIN;
	case F_NUM_SQRT:return HELP_F_NUM_SQRT;

	case F_STR_CHAREXT:return HELP_F_STR_CHAREXT;
	case F_STR_CONCAT:return HELP_F_STR_CONCAT;
	case F_STR_LEFT:return HELP_F_STR_LEFT;
	case F_STR_LENGTH:return HELP_F_STR_LENGTH;
	case F_STR_LOCATE:return HELP_F_STR_LOCATE;
	case F_STR_LOWERCS:return HELP_F_STR_LOWERCS;
	case F_STR_PAD:return HELP_F_STR_PAD;
	case F_STR_RIGHT:return HELP_F_STR_RIGHT;
	case F_STR_SHIFT:return HELP_F_STR_SHIFT;
	case F_STR_SIZE:return HELP_F_STR_SIZE;
	case F_STR_SQUEEZE:return HELP_F_STR_SQUEEZE;
	case F_STR_TRIM:return HELP_F_STR_TRIM;
	case F_STR_NOTRIM:return HELP_F_STR_NOTRIM;
	case F_STR_UPPERCS:return HELP_F_STR_UPPERCS;

	case F_DAT_DATETRUNC:return HELP_F_DAT_DATETRUNC;
	case F_DAT_DATEPART:return HELP_F_DAT_DATEPART;
	case F_DAT_GMT:return HELP_F_DAT_GMT;
	case F_DAT_INTERVAL:return HELP_F_DAT_INTERVAL;
	case F_DAT_DATE:return HELP_F_DAT_DATE;
	case F_DAT_TIME:return HELP_F_DAT_TIME;

	case F_IFN_IFN:return HELP_F_IFN_IFN;

	case F_PRED_LIKE:return HELP_F_PRED_LIKE;
	case F_PRED_BETWEEN:return HELP_F_PRED_BETWEEN;
	case F_PRED_ISNULL:return HELP_F_PRED_ISNULL;

	case F_PREDCOMB_AND:return HELP_F_PREDCOMB_AND;
	case F_PREDCOMB_OR:return HELP_F_PREDCOMB_OR;
	case F_PREDCOMB_NOT:return HELP_F_PREDCOMB_NOT;

	case F_SUBSEL_SUBSEL:return HELP_F_SUBSEL_SUBSEL;

	case F_AGG_ANY:return HELP_F_AGG_ANY;
	case F_AGG_COUNT:return HELP_F_AGG_COUNT;
	case F_AGG_SUM:return HELP_F_AGG_SUM;
	case F_AGG_AVG:return HELP_F_AGG_AVG;
	case F_AGG_MAX:return HELP_F_AGG_MAX;
	case F_AGG_MIN:return HELP_F_AGG_MIN;

	case F_PRED_COMP:return HELP_F_PRED_COMP;
	case F_PRED_IN:return HELP_F_PRED_IN;
	case F_PRED_ANY:return HELP_F_PRED_ANY;
	case F_PRED_ALL:return HELP_F_PRED_ALL;
	case F_PRED_EXIST:return HELP_F_PRED_EXIST;

	case F_DB_COLUMNS:return HELP_F_DB_COLUMNS;
	case F_DB_TABLES:return HELP_F_DB_TABLES;
	case F_DB_DATABASES:return HELP_F_DB_DATABASES;
	case F_DB_USERS:return HELP_F_DB_USERS;
	case F_DB_GROUPS:return HELP_F_DB_GROUPS;
	case F_DB_ROLES:return HELP_F_DB_ROLES;
	case F_DB_PROFILES:return HELP_F_DB_PROFILES;
	case F_DB_LOCATIONS:return HELP_F_DB_LOCATIONS;
	default:
		return nId;    // unknown ---> generic
	}
}

BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID)
{
	int  nPos = 0;
	CString strHelpFilePath;
	CString strTemp;
	strTemp = theApp.m_pszHelpFilePath;
#if defined (MAINWIN)
	nPos = strTemp.ReverseFind( '/'); 
#else
	nPos = strTemp.ReverseFind( '\\'); 
#endif
	if (nPos != -1) 
	{
		strHelpFilePath = strTemp.Left( nPos +1 );
		strHelpFilePath += theApp.m_strHelpFile;
	}
	else
		strHelpFilePath = theApp.m_strHelpFile;
	UINT nHelp = GetContextHelpId(nHelpID);
	TRACE1 ("APP_HelpInfo, Help Context ID = %d\n", nHelp);
	if (theApp.m_bHelpFileAvailable)
	{
		if (nHelp == 0)
			HtmlHelp(hWnd, strHelpFilePath, HH_DISPLAY_TOPIC, NULL);
		else
			HtmlHelp(hWnd, strHelpFilePath, HH_HELP_CONTEXT, nHelp);
	}
	else
	{
		TRACE0 ("Help file is not available: <theApp.m_bHelpFileAvailable = FALSE>\n");
		return FALSE;
	}

	return TRUE;
}


//
//  Function: DllRegisterServer
//
//  Summary:  The standard exported function that can be called to command
//            this DLL server to register itself in the system registry.
//
//  Args:
//
//  Returns:  HRESULT
//              NOERROR
//  ***********************************************************************************************
STDAPI DllRegisterServer()
{
	TCHAR tchszCLSID[128+1];
	TCHAR tchszModulePath[MAX_PATH];

	//
	// Obtain the path to this module's executable file for later use.
	GetModuleFileName(theApp.m_hInstance, tchszModulePath, sizeof(tchszModulePath)/sizeof(TCHAR));

	//
	// Create some base key strings.
	lstrcpy(tchszCLSID, TEXT("CLSID\\"));
	lstrcat(tchszCLSID, cstrCLSID_SQLxASSISTANCT);

	//
	// Create ProgID keys.
	if (!SetRegKeyValue(TEXT("SqlAssistant.1"), NULL, TEXT("SqlAssistant - Wizard Assistant")))
		return E_FAIL;
	if (!SetRegKeyValue(TEXT("SqlAssistant.1"), TEXT("CLSID"), cstrCLSID_SQLxASSISTANCT))
		return E_FAIL;

	//
	// Create VersionIndependentProgID keys.
	if (!SetRegKeyValue(TEXT("SqlAssistant"), NULL, TEXT("SqlAssistant - Wizard Assistant")))
		return E_FAIL;
	if (!SetRegKeyValue(TEXT("SqlAssistant"), TEXT("CurVer"), TEXT("SqlAssistant.1")))
		return E_FAIL;
	if (!SetRegKeyValue(TEXT("SqlAssistant"), TEXT("CLSID"), cstrCLSID_SQLxASSISTANCT))
		return E_FAIL;

	//
	// Create entries under CLSID.
	if (!SetRegKeyValue(tchszCLSID, NULL, TEXT("SqlAssistant - Wizard Assistant")))
		return E_FAIL;
	if (!SetRegKeyValue(tchszCLSID, TEXT("ProgID"), TEXT("SqlAssistant.1")))
		return E_FAIL;
	if (!SetRegKeyValue(tchszCLSID, TEXT("VersionIndependentProgID"), TEXT("SqlAssistant")))
		return E_FAIL;
	if (!SetRegKeyValue(tchszCLSID, TEXT("NotInsertable"), NULL))
		return E_FAIL;
	if (!SetRegKeyValue(tchszCLSID, TEXT("InprocServer32"), tchszModulePath))
		return E_FAIL;
	if (!AddRegNamedValue(tchszCLSID, TEXT("InprocServer32"), TEXT("ThreadingModel"), TEXT("Both")))
		return E_FAIL;

	return NOERROR;
}



//
//  Function: DllUnregisterServer
//
//  Summary:  The standard exported function that can be called to command
//            this DLL server to unregister itself from the system Registry.
//
//  Args: 
//  Returns:  HRESULT
//              NOERROR
//  ***********************************************************************************************
STDAPI DllUnregisterServer()
{
	TCHAR tchszCLSID[128+1];
	TCHAR tchszTemp[512];

	lstrcpy(tchszCLSID, TEXT("CLSID\\"));
	lstrcat(tchszCLSID, cstrCLSID_SQLxASSISTANCT);

	if (RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("SqlAssistant\\CurVer")) != ERROR_SUCCESS)
		return E_FAIL;
	if (RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("SqlAssistant\\CLSID")) != ERROR_SUCCESS)
		return E_FAIL;
	if (RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("SqlAssistant")) != ERROR_SUCCESS)
		return E_FAIL;

	if (RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("SqlAssistant.1\\CLSID")) != ERROR_SUCCESS)
		return E_FAIL;
	if (RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("SqlAssistant.1")) != ERROR_SUCCESS)
		return E_FAIL;

	lstrcpy(tchszTemp, tchszCLSID);
	lstrcat(tchszTemp, TEXT("\\ProgID"));
	if (RegDeleteKey(HKEY_CLASSES_ROOT, tchszTemp) != ERROR_SUCCESS)
		return E_FAIL;

	lstrcpy(tchszTemp, tchszCLSID);
	lstrcat(tchszTemp, TEXT("\\VersionIndependentProgID"));
	if (RegDeleteKey(HKEY_CLASSES_ROOT, tchszTemp) != ERROR_SUCCESS)
		return E_FAIL;

	lstrcpy(tchszTemp, tchszCLSID);
	lstrcat(tchszTemp, TEXT("\\NotInsertable"));
	if (RegDeleteKey(HKEY_CLASSES_ROOT, tchszTemp) != ERROR_SUCCESS)
		return E_FAIL;

	lstrcpy(tchszTemp, tchszCLSID);
	lstrcat(tchszTemp, TEXT("\\InprocServer32"));
	if (RegDeleteKey(HKEY_CLASSES_ROOT, tchszTemp) != ERROR_SUCCESS)
		return E_FAIL;

	if (RegDeleteKey(HKEY_CLASSES_ROOT, tchszCLSID) != ERROR_SUCCESS)
		return E_FAIL;

	return NOERROR;
}












//
//  Function: DllGetClassObject
//
//  Summary:  The standard exported function that the COM service library
//            uses to obtain an object class of the class factory for a
//            specified component provided by this server DLL.
//
//  Args:     REFCLSID rclsid,
//              [in] The CLSID of the requested Component.
//            REFIID riid,
//              [in] GUID of the requested interface on the Class Factory.
//            LPVOID* ppv)
//              [out] Address of the caller's pointer variable that will
//              receive the requested interface pointer.
//
//  Returns:  HRESULT
//              E_FAIL if requested component isn't supported.
//              E_OUTOFMEMORY if out of memory.
//              Error code out of the QueryInterface.
//  ***********************************************************************************************
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	HRESULT hError = CLASS_E_CLASSNOTAVAILABLE;
	IUnknown* pCob = NULL;

	CaComServer* pServer = theApp.GetServer();

	if (CLSID_SQLxASSISTANCT == rclsid)
	{
		hError = E_OUTOFMEMORY;
		pCob = new CaFactorySqlAssistant(NULL, pServer);
	}

	if (NULL != pCob)
	{
		hError = pCob->QueryInterface(riid, ppv);
		if (FAILED(hError))
		{
			delete pCob;
			pCob = NULL;
		}
	}

	return hError;
}


//
//  Function: DllCanUnloadNow
//
//  Summary:  The standard exported function that the COM service library
//            uses to determine if this server DLL can be unloaded.
//
//  Args:
//
//  Returns:  HRESULT
//              S_OK if this DLL server can be unloaded.
//              S_FALSE if this DLL can not be unloaded.
//  ***********************************************************************************************
STDAPI DllCanUnloadNow()
{
	HRESULT hError;

	CaComServer* pServer = theApp.GetServer();

	// We return S_OK of there are no longer any living objects AND
	// there are no outstanding client locks on this server.
	hError = (0L==pServer->m_cObjects && 0L==pServer->m_cLocks) ? S_OK : S_FALSE;

	return hError;
}



//
//  Function: SetRegKeyValue
//
//  Summary:  Internal utility function to set a Key, Subkey, and value
//            in the system Registry.
//
//  Args:     LPTSTR pszKey,
//            LPTSTR pszSubkey,
//            LPTSTR pszValue)
//
//  Returns:  BOOL
//              TRUE if success; FALSE if not.
//  ***********************************************************************************************
BOOL SetRegKeyValue(LPTSTR pszKey, LPTSTR pszSubkey, LPCTSTR pszValue)
{
	BOOL bOk = FALSE;

	LONG ec;
	HKEY hKey;
	TCHAR szKey[256];

	lstrcpy(szKey, pszKey);

	if (NULL != pszSubkey)
	{
		lstrcat(szKey, TEXT("\\"));
		lstrcat(szKey, pszSubkey);
	}

	ec = RegCreateKeyEx(
	    HKEY_CLASSES_ROOT,
	    szKey,
	    0,
	    NULL,
	    REG_OPTION_NON_VOLATILE,
	    KEY_ALL_ACCESS,
	    NULL,
	    &hKey,
	    NULL);

	if (ERROR_SUCCESS == ec)
	{
		if (NULL != pszValue)
		{
			ec = RegSetValueEx(
				hKey,
				NULL,
				0,
				REG_SZ,
				(BYTE *)pszValue,
				(lstrlen(pszValue)+1)*sizeof(TCHAR));
			if (ERROR_SUCCESS == ec)
				bOk = TRUE;
			RegCloseKey(hKey);
		}

		if (ERROR_SUCCESS == ec)
			bOk = TRUE;
	}
	return bOk;
}

//
//  Function: AddRegNamedValue
//
//  Summary:  Internal utility function to add a named data value to an
//            existing Key (with optional Subkey) in the system Registry
//            under HKEY_CLASSES_ROOT.
//
//  Args:     LPTSTR pszKey,
//            LPTSTR pszSubkey,
//            LPTSTR pszValueName,
//            LPTSTR pszValue)
//
//  Returns:  BOOL
//              TRUE if success; FALSE if not.
// ************************************************************************************************
BOOL AddRegNamedValue(LPTSTR pszKey, LPTSTR pszSubkey, LPTSTR pszValueName, LPCTSTR pszValue)
{
	BOOL bOk = FALSE;
	LONG ec;
	HKEY hKey;
	TCHAR szKey[256];

	lstrcpy(szKey, pszKey);

	if (NULL != pszSubkey)
	{
		lstrcat(szKey, TEXT("\\"));
		lstrcat(szKey, pszSubkey);
	}

	ec = RegOpenKeyEx(
	    HKEY_CLASSES_ROOT,
	    szKey,
	    0,
	    KEY_ALL_ACCESS,
	    &hKey);

	if (NULL != pszValue && ERROR_SUCCESS == ec)
	{
		ec = RegSetValueEx(
		    hKey,
		    pszValueName,
		    0,
		    REG_SZ,
		    (BYTE *)pszValue,
		    (lstrlen(pszValue)+1)*sizeof(TCHAR));
		if (ERROR_SUCCESS == ec)
		  bOk = TRUE;
		RegCloseKey(hKey);
	}

	return bOk;
}



CaSychronizeInterrupt::CaSychronizeInterrupt()
{
	m_hEventWorkerThread  = CreateEvent(NULL, TRUE, TRUE, NULL);
	m_hEventUserInterface = CreateEvent(NULL, TRUE, TRUE, NULL);

}

CaSychronizeInterrupt::~CaSychronizeInterrupt()
{
	m_bRequestCansel = FALSE;
	if (m_hEventWorkerThread)
		CloseHandle(m_hEventWorkerThread);
	if (m_hEventUserInterface)
		CloseHandle(m_hEventUserInterface);
}

void CaSychronizeInterrupt::BlockUserInterface(BOOL bLock)
{
	if (bLock)
		ResetEvent(m_hEventUserInterface);
	else
		SetEvent(m_hEventUserInterface);
}

void CaSychronizeInterrupt::BlockWorkerThread (BOOL bLock)
{
	if (bLock)
		ResetEvent(m_hEventWorkerThread);
	else
		SetEvent(m_hEventWorkerThread);
}

void CaSychronizeInterrupt::WaitUserInterface()
{
	WaitForSingleObject (m_hEventUserInterface, INFINITE);
}

void CaSychronizeInterrupt::WaitWorkerThread()
{
	WaitForSingleObject (m_hEventWorkerThread, INFINITE);
}
