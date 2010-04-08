/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : cmdargs.h , header File
**    Project  : Ingres II / vdbamon
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Parse the arguments
**
** History:
**
** 22-Mar-2002 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 23-Dec-2003: (uk$so01)
**    SIR #109221
** 26-Feb-2003: (uk$so01)
**    SIR #106952
**/

#if !defined (CMDARGS_PARSEARG_HEADER)
#define CMDARGS_PARSEARG_HEADER
#include "constdef.h"

typedef enum 
{
	K_UNKNOWN = 0,
	K_NODE,
	K_SERVER,
	K_USER,
	K_DATABASE,
	K_TABLE,
	K_FILE
} KeywordType;
#define OBT_OBJECTWITHOWNER  (-2)
void ExtractNameAndOwner (LPCTSTR lpszObject, CString& strOwner, CString& strName, BOOL bRemoveTextQualifier = TRUE);


//
// The following are predefined key names: (they use the key=value format)
//     -node
//     -server
//     -user
//     -database
//     -table
//     -file
// 
class CaKeyValueArg: public CObject
{
	DECLARE_DYNCREATE(CaKeyValueArg)
public:
	CaKeyValueArg();
	CaKeyValueArg(LPCTSTR lpszKey);
	virtual ~CaKeyValueArg(){}
	
	BOOL IsMatched(){return m_Matched;}
	CString GetKey(){return m_strKey;}
	CString GetValue(){return m_strValue;}
	void SetKey(LPCTSTR lpszText){m_strKey = lpszText;}
	void SetValue(LPCTSTR lpszText){m_strValue = lpszText; m_Matched = TRUE;}

	BOOL GetEqualSign(){return m_bEqualSign ;}
	BOOL GetNeedValue(){return m_bValue ;}
	void SetEqualSign(BOOL bSet){m_bEqualSign = bSet;}
	void SetNeedValue(BOOL bSet){m_bValue = bSet;}
	int  GetArgType(){return m_nObjType;} // -1 OR OBT_XXX

protected:
	void SetArgType(int nObjType){m_nObjType = nObjType;} // OBT_XXX
	void Init()
	{
		m_Matched = FALSE;
		m_bEqualSign = FALSE;
		m_bValue = FALSE;
		m_nObjType = -1;
	}
	CString m_strKey;
	CString m_strValue;
	BOOL m_Matched;
private:
	BOOL m_bEqualSign;
	BOOL m_bValue;
	int  m_nObjType;
};

class CaKeyValueArgObjectWithOwner: public CaKeyValueArg
{
	DECLARE_DYNCREATE(CaKeyValueArgObjectWithOwner)
public:
	CaKeyValueArgObjectWithOwner();
	CaKeyValueArgObjectWithOwner(LPCTSTR lpszKey , int nOBjType);
	virtual ~CaKeyValueArgObjectWithOwner(){}
	void Init()
	{
		m_strOwner = _T("");
		m_strObject = _T("");
	}

	void SetOwner(LPCTSTR lpszText){m_strOwner = lpszText; m_Matched = TRUE;}
	void SetObject(LPCTSTR lpszText){m_strObject = lpszText; m_Matched = TRUE;}
	CString GetOwner(){return m_strOwner;}
	CString GetObject(){return m_strObject;}

protected:
	CString m_strOwner;
	CString m_strObject;

};



class CaArgumentLine
{
public:
	CaArgumentLine():m_strCmdLine(_T("")), m_nArgs(0){}
	CaArgumentLine(LPCTSTR lpszCmdLine);
	~CaArgumentLine();
	static CString RemoveQuote (LPCTSTR lpszText, TCHAR tchQuote);
	CString GetKeyWord(LPCTSTR lpszString, int& nLen);
	void SetCmdLine(LPCTSTR lpszCmdLine){m_strCmdLine = lpszCmdLine;}
	//
	// lpszKey    : key value.   Ex: -node
	// bEqualSign : TRUE|FALSE   Ex: if TRUE then use the lpszKey=value format (bValue must be TRUE)
	// bValue     : TRUE|FALSE   Ex: if TRUE the key has value, else the key has no value for input
	//                               but the output will be "TRUE" or "FALSE"
	// bObjectWithOwner: if bValue = TRUE then bOjectWithOwner means (if true) the value can be
	//                               inputed as [owner.]object. The program add the item of class
	//                               CaKeyValueArgObjectWithOwner instead of CaKeyValueArg as the Key Object.
	void AddKey (LPCTSTR lpszKey, BOOL bEqualSign, BOOL bValue, BOOL bOjectWithOwner = FALSE);
	CTypedPtrList <CObList, CaKeyValueArg*>& GetKeys(){return m_listKeyValue;}

	//
	// Return Value:
	//    -1: error, the program continues like no command lines are specified
	//     0: OK
	virtual int Parse ();
	static BOOL MakeListTokens(LPCTSTR lpzsText, CStringArray& arrayToken, LPCTSTR lpzsSep = NULL);
	BOOL GetStatus(){return (m_nArgs == 0);}
	CString m_strFile;
protected:
	struct StateStack
	{
		BOOL IsEmpty(){return m_stack.IsEmpty();}
		TCHAR Top()
		{
			TCHAR state;
			if (IsEmpty())
				return _T('\0');
			state = m_stack.GetHead();
			return state;
		}
		void PushState(TCHAR state){ m_stack.AddHead(state); };
		void PopState() { m_stack.RemoveHead(); }
	
		CList< TCHAR, TCHAR > m_stack;
	};
	
	BOOL AnalyseNonKey    (CStringArray& arryToken, int& nCurrentIndex);
	BOOL AnalyseKey       (CStringArray& arryToken, int& nCurrentIndex, CaKeyValueArg* pKey);
	BOOL AnalyseKeyValue  (CStringArray& arryToken, int& nCurrentIndex, CaKeyValueArg* pKey);
	BOOL AnalyseKeyEqValue(CStringArray& arryToken, int& nCurrentIndex, CaKeyValueArg* pKey);


	CTypedPtrList <CObList, CaKeyValueArg*> m_listKeyValue;
	int m_nArgs;
	KeywordType m_nCurrentKey;
	CString m_strCmdLine;
	CStringList m_listKey;

};


#endif // CMDARGS_PARSEARG_HEADER