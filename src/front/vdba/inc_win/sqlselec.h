/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlselec.h, Header file
**    Project  : VDBA
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : class CaCursor (using cursor to select data from table)
**
** History:
**
** xx-Jan-1998 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 30-Jan-2002 (uk$so01)
**    SIR #106648, Enhance the library.
** 14-Fev-2002 (uk$so01)
**    SIR #106648, Enhance library (select loop)
** 29-Apr-2003 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM 
**    Handle special unicode only for Ingres Version 2.6
** 18-Jan-2005 (komve01)
**    BUG #113723,Issue#13853323.Corrected the minimum scale to 1
**    for Float4 and Float8 values.
** 13-Sep-2005 (gupsh01,uk$so01)
**    Added functions SetReleaseSessionOnClose and GetReleaseSessionOnClose
**    in CaCursor.
**/


#if !defined (SQLSELECT_HEADER)
#define SQLSELECT_HEADER
#include "qryinfo.h"

class CaDBObject;
class CaColumn;
class CaBufferColumn:public CObject
{
public:
	CaBufferColumn();
	CaBufferColumn(int nSize);
	virtual ~CaBufferColumn();
	
	int    GetSize(){return m_nSize;}
	TCHAR* GetBuffer(){return m_pBuffer;}
	short* GetIndicatorVariable(){return &m_sIndicator;}
	void   SetBufferSize (int nSize);

protected:
	int    m_nSize;
	TCHAR* m_pBuffer;
	short  m_sIndicator;
};


//
// Money Format:
// ************************************************************************************************
#define MAX_MONEY_FORMAT 4*sizeof(TCHAR)
class CaMoneyFormat
{
public:
	enum {MONEY_LEAD=0, MONEY_TRAIL, MONEY_NONE};
	CaMoneyFormat();
	~CaMoneyFormat(){}

	LPCTSTR GetSymbol(){return m_tchIIMoneyFormat;}
	int GetPrecision(){return m_nIIMoneyPrec;}
	int GetSymbolPosition(){return m_nIILeadOrTrail;}
	BOOL IsUsedMoneySymbol(){return m_bFormatMoney;}
	void SetUsedMoneySymbol(BOOL bSet){m_bFormatMoney = bSet;}


protected:
	int     m_nIIMoneyPrec;
	int     m_nIILeadOrTrail;
	TCHAR   m_tchIIMoneyFormat[MAX_MONEY_FORMAT + sizeof(TCHAR)];
	BOOL    m_bFormatMoney; // TRUE: handle the money symbol (trailing or leading), FALSE: ignore symbol
private:
	BOOL Initialize();
};

//
// Decimal Format:
// ************************************************************************************************
class CaDecimalFormat
{
public:
	CaDecimalFormat();
	~CaDecimalFormat(){}

	TCHAR GetSeparator(){return m_tchDecimalSeparator;}

protected:
	TCHAR m_tchDecimalSeparator;
private:
	BOOL Initialize();

};


//
// Float4 precision scale extrema:
#define MINF4_PREC          1
#define MAXF4_PREC        255
#define MINF4_SCALE         MINF4_PREC
#define MAXF4_SCALE         MAXF4_PREC
#define F4_DEFAULT_PREC    11
#define F4_DEFAULT_SCALE    3
#define F4_DEFAULT_DISPLAY _T('n')
//
// Float8 precision scale extrema:
#define MINF8_PREC          1
#define MAXF8_PREC        255
#define MINF8_SCALE         MINF8_PREC
#define MAXF8_SCALE         MAXF8_PREC
#define F8_DEFAULT_PREC    11
#define F8_DEFAULT_SCALE    3
#define F8_DEFAULT_DISPLAY _T('n')
//
// Utility functions for float preferences:
void F4_OnChangeEditPrecision(CEdit* pCtrl);
void F4_OnChangeEditScale(CEdit* pCtrl);
void F8_OnChangeEditPrecision(CEdit* pCtrl);
void F8_OnChangeEditScale(CEdit* pCtrl);

//
// Cursor Info:
// ************************************************************************************************
class CaCursorInfo: public CaConnectInfo
{
	DECLARE_SERIAL (CaCursorInfo)
public:
	CaCursorInfo();
	CaCursorInfo(const CaCursorInfo& c);
	CaCursorInfo operator = (const CaCursorInfo& c);
	void Copy (const CaCursorInfo& c);
	virtual ~CaCursorInfo(){}
	virtual void Serialize (CArchive& ar);

	void SetFloat8Format(int nWidth = 10, int nDecimal = 3, TCHAR tchMode = _T('n'))
	{
		m_nFloat8Width     = nWidth;
		m_nFloat8Decimal   = nDecimal; 
		m_ntchDisplayMode8 = tchMode;
	}
	void SetFloat4Format(int nWidth = 10, int nDecimal = 3, TCHAR tchMode = _T('n'))
	{
		m_nFloat4Width     = nWidth;
		m_nFloat4Decimal   = nDecimal; 
		m_ntchDisplayMode4 = tchMode;
	}

	int GetFloat4Width(){return m_nFloat4Width;}
	int GetFloat4Decimal(){return m_nFloat4Decimal;}
	TCHAR GetFloat4DisplayMode(){return m_ntchDisplayMode4;}
	int GetFloat8Width(){return m_nFloat8Width;}
	int GetFloat8Decimal(){return m_nFloat8Decimal;}
	TCHAR GetFloat8DisplayMode(){return m_ntchDisplayMode8;}
 


protected:
	//
	// Data format:
	int m_nFloat4Width;       // Default = 10
	int m_nFloat4Decimal;     // Default = 3
	TCHAR m_ntchDisplayMode4; // Default = 'n' (can be 'f' = float or 'e' = exponential)

	int m_nFloat8Width;       // Default = 10
	int m_nFloat8Decimal;     // Default = 3
	TCHAR m_ntchDisplayMode8; // Default = 'n' (can be 'f' = float or 'e' = exponential)
};

class CaFloatFormat: public CaCursorInfo
{
	DECLARE_SERIAL (CaFloatFormat)
public:
	CaFloatFormat():CaCursorInfo(){}
	virtual ~CaFloatFormat(){}

};

//
// CaRowTransfer:
// ************************************************************************************************
#define CAROW_FIELD_NULL             0x00000001
#define CAROW_FIELD_NOTDISPLAYABLE  (CAROW_FIELD_NULL << 1)
class CaRowTransfer: public CObject
{
public:
	CaRowTransfer(){}
	virtual ~CaRowTransfer()
	{
		m_strRecord.RemoveAll();
	}
	CStringList& GetRecord(){return m_strRecord;}
	CArray <UINT, UINT>& GetArrayFlag(){return m_arrayFlag;}

protected:
	CStringList m_strRecord;
	//
	// The element of m_arrayFlag is one or more of (CAROW_FIELD_NULL, CAROW_FIELD_NOTDISPLAYABLE)
	CArray <UINT, UINT> m_arrayFlag;
};

//
// CaCursor:
// ************************************************************************************************
class CaCursor: public CObject
{
public:
	//
	// nCounter must be generated and should be distict from one to another
	// while cursors are alive.
	CaCursor (int nCounter, LPCTSTR lpszRawStatement);
	CaCursor (int nCounter, LPCTSTR lpszRawStatement, int nIngresVersion);
	~CaCursor();
	CString GetCursorName(){return m_strCursorName;}
	CaCursorInfo& GetCursorInfo(){return m_cursorInfo;}
	CaMoneyFormat& GetMoneyFormat(){return m_moneyFormat;}
	CTypedPtrList< CObList, CaColumn* >& GetListHeader(){return m_listHeader;}
	void SetReleaseSessionOnClose(BOOL bReleaseSessionOnClose) {m_bReleaseSessionOnClose=bReleaseSessionOnClose;}
	BOOL GetReleaseSessionOnClose(){return m_bReleaseSessionOnClose;}

	//
	// Fetch one row, the result is stored in the string list. Return FALSE and close the
	// cursor when try to fetch past the last row.
	// It might throw the 'CeSQLException' when error occured.
	BOOL Fetch(CStringList& tuple, int& nCount);
	BOOL Fetch(CaRowTransfer* pRow, int& nCount);

	BOOL Close();
	//
	// After the cursor is constructed, you must first call open to enable the 
	// fetch operation.
	BOOL Open();

protected:
	//
	// Construct the descriptor (SQLDA). It initialize the column headers.
	// The result is stored in the member 'm_listHeader'.
	BOOL ConstructCursor(LPCTSTR lpszCursorName, LPCTSTR lpszStatement, LPCTSTR lpszRawStatement, LPVOID& pSqlda);
	BOOL DoFetch(CStringList& tuple, int& nCount, CArray <UINT, UINT>* pArrayNullField);

	BOOL m_bReleaseSessionOnClose;
	CaCursorInfo    m_cursorInfo;
	CaMoneyFormat   m_moneyFormat;
	CaDecimalFormat m_decimalFormat;
	CTypedPtrList< CObList, CaColumn* > m_listHeader;
	CTypedPtrList< CObList, CaBufferColumn* > m_listBufferColumn;
	LPVOID  m_pSqlda;
	CString m_strCursorName;
	CString m_strStatement;
	CString m_strRawStatement;

	int m_nIngresVersion;
};


//
// MANAGEMENT OF SORT ROWS:
// ************************
typedef struct tagSORTROW
{
	BOOL bAsc;
	int  nDisplayType;

} SORTROW;


//
// Constant returned by 'INGRESII_GetSqlDisplayType'

#define SQLACT_NUMBER       1
#define SQLACT_NUMBER_FLOAT 2
#define SQLACT_TEXT         3
#define SQLACT_DATE         4
int INGRESII_GetSqlDisplayType(int nInternalIngresDataType);
int CALLBACK SelectResultCompareSubItem (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

//
// Compile the Statements (return the time (ms) taken to compile the statements)
long INGRESII_CompileStatement(CStringList& listStatement, int nIngresVersion);





#endif // SQLSELECT_HEADER
