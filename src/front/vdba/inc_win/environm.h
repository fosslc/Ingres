/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : environm.h , Header File 
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Variable Environment Management
**
**
** History:
**
** 27-Aug-1999: (uk$so01) 
**    created
** 29-Feb-2000 (uk$so01)
**    Created. (SIR # 100634)
**    The "Add" button of Parameter Tab can be used to set 
**    Parameters for System and User Tab. 
** 23-Dec-2003: (uk$so01)
**    SIR #109221
**    This file is from the IVM project. Modify and put the file in the library
**    libingll.lib to be used by Visual Config Diff Analyzer (VCDA).
**    Rename the CaParameter to CaEnvironmentParameter.
** 04-Mar-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
** 27-Mar-2003 (uk$so01)
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
**    Enhance the library.
** 04-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
**/


#if !defined (ENVIRONM_FILE_HEADER)
#define ENVIRONM_FILE_HEADER

#define ST_MODIFY  0x0001 // Parameter can be changed
#define ST_UNSET   0x0002 // Can unset parameter
#define ST_RESTORE 0x0004 // Can be restore
#define ST_ADD     0x0008 // Can add extra perameter

typedef enum 
{
	P_TEXT = 0,
	P_NUMBER,
	P_NUMBER_SPIN,
	P_COMBO,
	P_PATH,
	P_PATH_FILE
} ParameterType;

typedef struct tagCHARxINTxTYPExMINxMAX
{
	TCHAR tchszName [128]; // VARIABLE NAME
	int   nV1;             // IDS_ of Description text of VARIABLE NAME
	ParameterType nType;   // Type of the value of VARIABLE NAME
	int  nMin;             // only if nType = P_NUMBER_SPIN
	int  nMax;             // only if nType = P_NUMBER_SPIN
	BOOL bLocallyResetable;// VARIABLE NAME can be redefined locally by the user
	BOOL bReadOnlyAsSystem;// edit and unset should not be available if TRUE
}CHARxINTxTYPExMINxMAX;


class CaNameValue: public CObject
{
	DECLARE_SERIAL (CaNameValue)
public:
	CaNameValue();
	CaNameValue(LPCTSTR lpszName, LPCTSTR lpszValue);
	~CaNameValue(){}
	virtual void Serialize (CArchive& ar);
	//
	// Manually manage the object version:
	// When you detect that you must manage the newer version (class members have been changed),
	// the derived class should override GetObjectVersion() and return
	// the value N same as IMPLEMENT_SERIAL (CaXyz, CaDBObject, N).
	virtual UINT GetObjectVersion(){return 1;}

	CString GetName(){return m_strParameter;}
	CString GetValue(){return m_strValue;}
	void SetName(LPCTSTR lpszName){m_strParameter = lpszName;}
	void SetValue(LPCTSTR lpszValue){m_strValue = lpszValue;}

protected:
	CString m_strParameter;
	CString m_strValue;

};



//
// CaEnvironmentParameter
// ************************************************************************************************
class CaEnvironmentParameter: public CaNameValue
{
	DECLARE_SERIAL (CaEnvironmentParameter)
public:
	CaEnvironmentParameter():
		m_strDescription(_T("")), 
		m_bLocallyResetable(FALSE),
		m_nType (P_TEXT),
		m_bLocalMachine(FALSE)
	{
		m_nMin = -1;
		m_nMax = -1;
		m_bReadOnly = FALSE;
		m_bSet = FALSE;
	}
	CaEnvironmentParameter(LPCTSTR lpszParameter, LPCTSTR lpszValue, LPCTSTR lpszDescription):
		CaNameValue(lpszParameter, lpszValue),
		m_strDescription(lpszDescription), 
		m_bLocallyResetable(FALSE),
		m_nType (P_TEXT),
		m_bLocalMachine(FALSE)
	{
		m_nMin = -1;
		m_nMax = -1;
		m_bReadOnly = FALSE;
		m_bSet = FALSE;
	}
	virtual ~CaEnvironmentParameter(){}
	virtual void Serialize (CArchive& ar);
	static CaEnvironmentParameter* Search (LPCTSTR lpszName, CTypedPtrList<CObList, CaEnvironmentParameter*>& lev);

	BOOL IsLocallyResetable(){return m_bLocallyResetable;}
	BOOL IsLocalMachine(){return m_bLocalMachine;}
	BOOL IsReadOnly(){return m_bReadOnly;} 
	BOOL IsSet(){return m_bSet;}
	ParameterType GetType(){return m_nType;}
	CString GetDescription(){return m_strDescription;}

	void SetReadOnly(BOOL bReadOnly){m_bReadOnly = bReadOnly;} 
	void Set(BOOL bSet){m_bSet = bSet;}
	void LocallyResetable(BOOL bResetable){m_bLocallyResetable = bResetable;}
	void SetLocalMachine(BOOL bLocal){m_bLocalMachine = bLocal;}
	void SetType(ParameterType type){m_nType = type;}
	void SetDescription(LPCTSTR lpszText){m_strDescription = lpszText;}
	BOOL LoadDescription(UINT nIDS);

	//
	// Only if m_nType = P_NUMBER_SPIN
	void SetMinMax(int  nMin, int  nMax){m_nMin = nMin; m_nMax = nMax;}
	void GetMinMax(int& nMin, int& nMax){nMin = m_nMin; nMax = m_nMax;}

protected:
	CString m_strDescription;
	BOOL m_bLocalMachine;
	BOOL m_bLocallyResetable;
	BOOL m_bReadOnly;
	BOOL m_bSet; // TRUE if Parameter has been Set;
	ParameterType m_nType;
	int m_nMin;
	int m_nMax;
};


//
// CaIngresVariable
// ************************************************************************************************
class CaIngresVariable: public CObject
{
public:
	CaIngresVariable();
	~CaIngresVariable();


	BOOL Init();
	BOOL Init(CHARxINTxTYPExMINxMAX* pTemplate, int nSize);
	BOOL QueryEnvironment     (CTypedPtrList<CObList, CaEnvironmentParameter*>& listParameter, BOOL bUnset = TRUE, BOOL bExtra = FALSE);
	BOOL QueryEnvironmentUser (CTypedPtrList<CObList, CaEnvironmentParameter*>& listParameter, BOOL bUnset = TRUE);
	BOOL SetIngresVariable (LPCTSTR lpszName, LPCTSTR lpszValue, BOOL bSystem, BOOL bUnset);

	void SetStringUnset (LPCTSTR lpszText = _T("<not set>")){m_strVariableNotSet = lpszText;}
	void SetStringNoDescription (LPCTSTR lpszText = __T("This environment variable is not documented")){m_strNoDescription = lpszText;}
	void SetStringNotSupport4Unix (LPCTSTR lpszText){m_strUnixNotSupported = lpszText;}

	static BOOL IsReadOnly(LPCTSTR lpszVariableName, LPCTSTR lpszII);
	static BOOL IsPath(LPCTSTR lpszVariableName);
	// IsIIDependent() returns
	//   0: not dependent
	//   1: name dependent
	//   2: value dependent
	static int  IsIIDependent(LPCTSTR lpszVariableName, LPCTSTR lpszII, CString& strFormatName);

protected:
	CaEnvironmentParameter* CaIngresVariable::IngresEnvValid(LPCTSTR lpszName);
	BOOL PreInitialize();

	//
	// List of ingres variable environments (hard coded):
	CTypedPtrList<CObList, CaEnvironmentParameter*> m_listEnvirenment;
	CString m_strVariableNotSet;
	CString m_strIngresBin;
	CString m_strNoDescription;
	CString m_strUnixNotSupported; // Originated from ivm: IDS_UNIX_OPERATION_NOT_SUPPORTED
};


//
// Check the values of parameters:
// Input : listEnvirenment contains the names of parameters with their values are empty.
// Output: listEnvirenment contains the couples (name, value). If the value is not empty
//         it means the parameter is not set or set to empty.
void INGRESII_CheckVariable (CTypedPtrList<CObList, CaNameValue*>& listEnvirenment);


#endif // ENVIRONM_FILE_HEADER
