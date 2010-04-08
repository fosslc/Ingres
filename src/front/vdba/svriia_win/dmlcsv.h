/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlcsv.h: header file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Manipulation of data of csv, txt file 
**
** History:
**
** 15-Feb-2001 (uk$so01)
**    Created
** 09-Jul-2001 (hanje04)
**    Made CaDetectLineInfo public for MAINWIN builds becuase of compile 
**    problems.
** 21-Jan-2002 (uk$so01)
**    (bug 106844). Additional fix: Replace ifstream and some of CFile.by FILE*.
** 21-Nov-2003 (uk$so01)
**    SIR  #111320, Construct default headers from the "skipped first line"
**/

#if !defined (DMLCSV_HEADER)
#define DMLCSV_HEADER
#include "formdata.h"
class CaIIAInfo;

//
// Field Separator:
typedef enum
{
	FMT_UNKNOWN = 0,
	FMT_FIELDSEPARATOR,
	FMT_FIXEDWIDTH,
	FMT_DBF
} ColumnSeparatorType;


//
// The separator:
// ************************************************************************************************
class CaFailureInfo: public CObject
{
public:
	CaFailureInfo()
	{
		m_strSeparator = _T("");
		m_tchTQ = _T('\0');
		m_bCS = FALSE;
		m_bTS = FALSE;
		m_nLine = -1;
		m_strLine = _T("");
	}
	virtual ~CaFailureInfo(){}

	CString m_strSeparator; // separate each token by a space.
	TCHAR   m_tchTQ;        // Text Qualifier;
	BOOL    m_bCS;          // Consecutive separators considered as one.
	BOOL    m_bTS;          // TRUE => Ignore Trailing Separators .

	int     m_nLine;        // Line number.
	CString m_strLine;      // Text line.
};


//
// The separator:
// ************************************************************************************************
class CaSeparator: public CObject
{
public:
	enum {SEP_SEPARATOR = 0, SEP_BEGINLINE, SEP_ENDLINE};

	CaSeparator(TCHAR tchChar, int nCount = 1, int nType = SEP_SEPARATOR)
		:m_nCount(nCount), m_strSeparator(tchChar), m_nType(nType){}
	CaSeparator(LPCTSTR lpSep, int nCount = 1, int nType = SEP_SEPARATOR)
		:m_nCount(nCount), m_strSeparator(lpSep), m_nType(nType){}
	virtual ~CaSeparator(){}
	CaSeparator(const CaSeparator& c){Copy(c);}
	CaSeparator operator = (const CaSeparator& c)
	{
		ASSERT (&c != this); // A=A;
		if (&c == this)
			return *this;
		Copy(c);
		return *this;
	};
	int& GetCount() {return m_nCount;}
	CString& GetSeparator(){return m_strSeparator;}
	void SetType(int nType){m_nType=nType;}
	int GetType(){return m_nType;}
protected:
	CString m_strSeparator;
	int     m_nCount;
	int     m_nType;
	void Copy(const CaSeparator& c)
	{
		m_strSeparator = c.m_strSeparator;
		m_nCount = c.m_nCount;
		m_nType = c.m_nType;
	};
};

//
// The set of field separators:
// ************************************************************************************************
class CaSeparatorSet: public CObject
{
public:
	CaSeparatorSet();
	virtual ~CaSeparatorSet();
	CaSeparatorSet(const CaSeparatorSet& c);
	CaSeparatorSet operator = (const CaSeparatorSet& c);

	void NewSeparator (CString& strSep);
	void RemoveSeparator (CaSeparator* pObj);
	void SetIgnoreTrailingSeparator(BOOL bSet){m_bIgnoreTrailingSeparator = bSet;}
	void SetTextQualifier(TCHAR tchszChar){m_tchszTextQualifier = tchszChar;}
	BOOL GetIgnoreTrailingSeparator(){return m_bIgnoreTrailingSeparator;}
	TCHAR GetTextQualifier(){return m_tchszTextQualifier;}
	//
	// Match all kind of separators (single & multiple caracters)
	CaSeparator* CanBeSeparator (LPTSTR lpszText);

	void SetConsecutiveSeparatorsAsOne(BOOL bSet){m_bConsecutiveSeparatorsAsOne = bSet;}
	BOOL GetConsecutiveSeparatorsAsOne(){return m_bConsecutiveSeparatorsAsOne;}
	void AddNewConsecutiveSeparators (LPCTSTR strPrev, LPCTSTR strCurrent);
	void RemoveConsecutiveSeparators (LPCTSTR strSep);
	BOOL IsConsecutiveSeparator (LPCTSTR strSep);

	//
	// Initialize the set of separators from the first line text of row record:
	//
	// Initialize the user defined set of separators from the input string:
	void InitUserSet(LPCTSTR lpszInputSeparators);

	//
	// Return NULL if not a separator:
	CaSeparator* GetSeparator(TCHAR tchChar);
	CaSeparator* GetSeparator(LPCTSTR lpSep);

	CTypedPtrList< CObList, CaSeparator* >& GetListCharSeparator(){return m_listCharSeparator;}
	CTypedPtrList< CObList, CaSeparator* >& GetListStrSeparator() {return m_listStrSeparator;}

protected:
	class CaSeparatorConsecutive: public CObject
	{
	public:
		CaSeparatorConsecutive(LPCTSTR lpszPreceded, LPCTSTR lpszFollowed)
			:m_first(lpszPreceded), m_next(lpszFollowed){}
		~CaSeparatorConsecutive(){}

	
		CaSeparator m_first; // the preceded separator
		CaSeparator m_next;  // the followed separator
	};

	BOOL m_bConsecutiveSeparatorsAsOne; // TRUE means treats consecutive separators as one.
	//
	// TRUE if InitUserSet() has been called.
	BOOL m_bUserSet;
	// TRUE to indicate that trailing separators are ignored
	BOOL  m_bIgnoreTrailingSeparator;
	TCHAR m_tchszTextQualifier;

	CTypedPtrList< CObList, CaSeparator* > m_listCharSeparator; // Single charactor as a separator
	CTypedPtrList< CObList, CaSeparator* > m_listStrSeparator;  // Multiple charactors as a separator
	CTypedPtrList< CObList, CaSeparatorConsecutive* > m_listSeparatorConsecutive;


	BOOL TextQualifier(TCHAR tchszChar);
	void Copy(const CaSeparatorSet& c);
	void Cleanup();
};

//
//  Class CaTreeSolution:
// ************************************************************************************************
class CaTreeSolution
{
public:
	CaTreeSolution(LPCTSTR lpszSeparator, int nColumn = -1)
		:m_strOtherSeparator(lpszSeparator), m_nColumn(nColumn){}
	virtual ~CaTreeSolution(){}

	void FindSolution (CaIIAInfo* pData);

#ifdef MAINWIN
public:
#else
private:
#endif
	class CaDetectLineInfo
	{
	public:
		CaDetectLineInfo(int nLine, BOOL bSQ, BOOL bDQ, BOOL bCS, BOOL bTS)
		{
			m_bDetectSQ = bSQ;
			m_bDetectDQ = bDQ;
			m_bDetectCS = bCS;
			m_bDetectTS = bTS;
			m_nLine     = nLine;
		}
		~CaDetectLineInfo()
		{
		}
		int  GetLine(){return m_nLine;}
		BOOL GetDetectSQ(){return m_bDetectSQ;}
		BOOL GetDetectDQ(){return m_bDetectDQ;}
		BOOL GetDetectCS(){return m_bDetectCS;}
		BOOL GetDetectTS(){return m_bDetectTS;}
		void SetDetectSQ(BOOL bSet){m_bDetectSQ = bSet;}
		void SetDetectDQ(BOOL bSet){m_bDetectDQ = bSet;}
		void SetDetectCS(BOOL bSet){m_bDetectCS = bSet;}
		void SetDetectTS(BOOL bSet){m_bDetectTS = bSet;}
	protected:
		int  m_nLine;      // Line number;
		BOOL m_bDetectSQ;  // TRUE : must detect Simple Quote
		BOOL m_bDetectDQ;  // TRUE : must detect Double Quote
		BOOL m_bDetectCS;  // TRUE : must detect consecutive separator
		BOOL m_bDetectTS;  // TRUE : must detect trailing separator
	};

	class CaExploredInfo
	{
	public:
		CaExploredInfo(CaIIAInfo* pData, LPCTSTR lpszOtherSeparator = NULL, int nColumn = 0)
			:m_nLineDQ(-1), m_nLineSQ(-1)
		{
			m_nLastScannedLine0yz = -1;
			m_nLastScannedLine1yz = -1;
			m_nLastScannedLine2yz = -1;
			m_nLineCS0yz = -1;
			m_nLineCS1yz = -1;
			m_nLineCS2yz = -1;
			m_nLineTS0yz = -1;
			m_nLineTS1yz = -1;
			m_nLineTS2yz = -1;
			m_nColumn   = nColumn;
			m_pIIAInfo = pData;

			if (lpszOtherSeparator)
				m_strOtherSeparator = lpszOtherSeparator;
			else
				m_strOtherSeparator = _T("");
		}
		~CaExploredInfo();
		void SetLineSQ(int nLine){m_nLineSQ = nLine;}
		void SetLineDQ(int nLine){m_nLineDQ = nLine;}
		void SetLineCS0yz(int nLine){m_nLineCS0yz= nLine;}
		void SetLineCS1yz(int nLine){m_nLineCS1yz= nLine;}
		void SetLineCS2yz(int nLine){m_nLineCS2yz= nLine;}
		void SetLineTS0yz(int nLine){m_nLineTS0yz= nLine;}
		void SetLineTS1yz(int nLine){m_nLineTS1yz= nLine;}
		void SetLineTS2yz(int nLine){m_nLineTS2yz= nLine;}

		void SetLastScannedLine0yz(int nLine){m_nLastScannedLine0yz= nLine;}
		void SetLastScannedLine1yz(int nLine){m_nLastScannedLine1yz= nLine;}
		void SetLastScannedLine2yz(int nLine){m_nLastScannedLine2yz= nLine;}

		int GetLineSQ(){return m_nLineSQ;}
		int GetLineDQ(){return m_nLineDQ;}
		int GetLineCS0yz(){return m_nLineCS0yz;}
		int GetLineCS1yz(){return m_nLineCS1yz;}
		int GetLineCS2yz(){return m_nLineCS2yz;}
		int GetLineTS0yz(){return m_nLineTS0yz;}
		int GetLineTS1yz(){return m_nLineTS1yz;}
		int GetLineTS2yz(){return m_nLineTS2yz;}

		int  GetLastScannedLine0yz(){return m_nLastScannedLine0yz;}
		int  GetLastScannedLine1yz(){return m_nLastScannedLine1yz;}
		int  GetLastScannedLine2yz(){return m_nLastScannedLine2yz;}
	
		CString GetOtherSeparator() {return m_strOtherSeparator;}
		int GetColumnCount(){return m_nColumn;}
		CaIIAInfo* GetIIAInfo() {return m_pIIAInfo;}

		//
		// N = 0: for node (0, y, z)
		// N = 1: for node (1, y, z)
		// N = 2: for node (2, y, z)
		void NewTQBranchNyz (int N, TCHAR tchTQ, CString& strSep1, CString& strSep2);
		void NewCSBranchNyz (int N, CString& strSep1, CString& strSep2);
		void NewTSBranchNyz (int N, CString& strTS);

		//
		// nCheck = 0: for node (0, y, z)
		// nCheck = 1: for node (1, y, z)
		// nCheck = 2: for node (2, y, z)
		BOOL ExistTQ (TCHAR tchTQ, CString& strSeparator, int nCheck);
		BOOL ExistCS (CString& strSeparator, int nCheck);
		BOOL ExistTS (CString& strSeparator, int nCheck);

		//
		// The list m_lSQ0y & m_lDQ0y are updated
		// when exploring the first branch (0, 0) and continue to explore
		// in the next branches if the first exploration has not completed
		// due to the failure.
		CTypedPtrList< CObList, CaSeparator* > m_lSQ0yz; // Outer separators for Single Quote; Node (0, y).
		CTypedPtrList< CObList, CaSeparator* > m_lDQ0yz; // Outer separators for Double Quote; Node (0, y).

		CTypedPtrList< CObList, CaSeparator* > m_lSQ1yz; // Outer separators for Single Quote; Node (1, y).
		CTypedPtrList< CObList, CaSeparator* > m_lDQ1yz; // Outer separators for Double Quote; Node (1, y).

		CTypedPtrList< CObList, CaSeparator* > m_lSQ2yz; // Outer separators for Single Quote; Node (2, y).
		CTypedPtrList< CObList, CaSeparator* > m_lDQ2yz; // Outer separators for Double Quote; Node (2, y).

		CTypedPtrList< CObList, CaSeparator* > m_lCS0yz; // Cocsecutive separator for branch (0,y,z).
		CTypedPtrList< CObList, CaSeparator* > m_lCS1yz; // Cocsecutive separator for branch (1,y,z).
		CTypedPtrList< CObList, CaSeparator* > m_lCS2yz; // Cocsecutive separator for branch (2,y,z).
		CTypedPtrList< CObList, CaSeparator* > m_lTS0yz; // Cocsecutive separator for branch (0,y,z).
		CTypedPtrList< CObList, CaSeparator* > m_lTS1yz; // Cocsecutive separator for branch (1,y,z).
		CTypedPtrList< CObList, CaSeparator* > m_lTS2yz; // Cocsecutive separator for branch (2,y,z).


	protected:
		CString m_strOtherSeparator;
		int m_nColumn;
		CaIIAInfo* m_pIIAInfo;

		int m_nLastScannedLine0yz ;// The last line scanned.
		int m_nLastScannedLine1yz ;// The last line scanned.
		int m_nLastScannedLine2yz ;// The last line scanned.

		int m_nLineSQ;    // With ambiguity, Line number where the first simple quote (SQ) has been detected (-1: none)
		int m_nLineDQ;    // With ambiguity, Line number where the first double quote (DQ) has been detected (-1: none)
		int m_nLineCS0yz; // Line number where the first consecutive separator (CS) has been detected; node (0, y)
		int m_nLineCS1yz; // Line number where the first consecutive separator (CS) has been detected; node (1, y)
		int m_nLineCS2yz; // Line number where the first consecutive separator (CS) has been detected; node (2, y)
		int m_nLineTS0yz; // Line number where the first trailing separator (TS) has been detected; node (0, y)
		int m_nLineTS1yz; // Line number where the first trailing separator (TS) has been detected; node (1, y)
		int m_nLineTS2yz; // Line number where the first trailing separator (TS) has been detected; node (2, y)
#ifdef MAINWIN
		friend class CaTreeNodeDetection;
#endif
	};

	class CaTreeNodeDetection:public CObject
	{
	public:
		enum {BRANCH_NOTEXPLORED=0, BRANCH_FAILED, BRANCH_SUCCEEDED};
		CaTreeNodeDetection(CStringArray* pArrayLine, int nTQ = -1, int nCSO = -1, int nTS = -1)
		{
			m_nTQ  = nTQ;
			m_nCSO = nCSO;
			m_nTS  = nTS;
			m_pArrayLine=pArrayLine;
			m_nIndex = 0;
			m_nNodeType = BRANCH_NOTEXPLORED;
			m_pListFailureInfo = NULL;
		}
		virtual ~CaTreeNodeDetection();
		void InitializeRoot();
		void Detection(CaExploredInfo* pExploredInfo = NULL);
		void Traverse (CaIIAInfo* pData, CTypedPtrList < CObList, CaSeparatorSet* >& listSolution);

		int GetTextQualifier(){return m_nTQ;}
		int GetConsecutiveSeparator(){return m_nCSO;}
		int GetTrailingSeparator(){return m_nTS;}
		CTypedPtrList< CObList, CaTreeNodeDetection* > m_listChildren;
		CaSeparatorSet m_separatorSet;

		CTypedPtrList< CObList, CaFailureInfo* >* m_pListFailureInfo;

	protected:
		void CountSeparator(
			CaSeparatorSet* pSeparatorSet, 
			CaSeparatorSet* pTemplate,
			CaExploredInfo* pExploredInfo, 
			CString& strLine, 
			CaDetectLineInfo* pInfo);
		//
		// The m_nTQ members below, indicate the possible choices of combination.
		// The value  0 means nothing is not consedered as.Text Qualifier (and can no longer be changed)
		// The value  1 means simple quote <'> is consedered as. as Text Qualifier (and can no longer be changed)
		// The value  2 means double quote <"> is consedered as. as Text Qualifier (and can no longer be changed)
		// The value -1 means you must set to 0|1|2.
		int m_nTQ;     // Text qualifier is a double quote character <">

		// The m_nCSO members below, indicate the possible choices of combination.
		// The value  0 means consecutive separators are not consedered as.one. (and can no longer be changed)
		// The value  1 means consecutive separators are consedered as.one. (and can no longer be changed)
		// The value -1 means you must set to 0|1.
		int m_nCSO;    // Consecutive separators are considered as one.

		// The m_nTS  members below, indicate the possible choices of combination.
		// The value  0 Not ignore the Trailing Separators
		// The value  1 Ignore the Trailing Separators
		// The value -1 means you must set to 0|1.
		int m_nTS;    // Trailing Separators.
		//
		// Zero-based index for the text line to scaned.
		// From that index this node begins to scan to find the solution.
		// For the root node, the m_nIndex will be zero.
		int  m_nIndex;
	private:
		int  m_nNodeType;
		CStringArray* m_pArrayLine;
		void CreateRoot();

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
	};
	friend class CaTreeNodeDetection;
	CString m_strOtherSeparator;
	int  m_nColumn;

};


void CSV_GetHeaderLines(LPCTSTR lpszFile, int nAnsiOem, CString& strLine);
void File_GetLines(CaIIAInfo* pData);
void GetFieldsFromLine (CString& strLine, CStringArray& arrayFields, CaSeparatorSet* pSeparatorSet);
void GetFieldsFromLineFixedWidth (CString& strLine, CStringArray& arrayFields, int* pArrayDividerPos, int nSize);


#endif //DMLCSV_HEADER



