/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlcolum.h: interface for the CaColumn class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Column object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
** 17-May-2004 (uk$so01)
**    SIR #109220, Fix some bugs introduced by change #462417
**    (Manage compare "User-Defined Data Types" in VDDA)
**/


#if !defined (DMLCOLUMNS_HEADER)
#define DMLCOLUMNS_HEADER
#include "dmlbase.h"

//
// Object: Column 
// **************
class CaColumn: public CaDBObject
{
	DECLARE_SERIAL (CaColumn)
public:
	enum {COLUMN_NOLENGTH = 0, COLUMN_LENGTH, COLUMN_PRECISION, COLUMN_PRECISIONxSCALE};
	CaColumn(): CaDBObject(), m_strLength(_T("")), m_strDefault(_T("")), m_bColumnText(TRUE)
	{
		Initialize();
	}

	//
	// m_strItemOwner of CaDBObject will receive the Column type string !
	CaColumn(LPCTSTR lpszItem, LPCTSTR lpszType, int nDataType = -1)
		: CaDBObject(lpszItem, lpszType, FALSE), m_strLength(_T("")), m_strDefault(_T("")), m_bColumnText(TRUE)
	{
		Initialize();

		m_strDataType = lpszType;
		m_iDataType = nDataType;
	}

	CaColumn(const CaColumn& c);
	CaColumn operator = (const CaColumn& c);

	void Initialize()
	{
		m_iDataType = -1;
		m_nLength = 0;
		m_nScale  = 0;
		m_bNullable = FALSE;
		m_bDefault = FALSE;
		m_nHasDefault = 0;
		m_bShortDisplay = TRUE;
		m_nKeySequence = 0;
		m_bSystemMaintained = FALSE;
	}
	
	virtual ~CaColumn(){}
	virtual void Serialize (CArchive& ar);
	BOOL Matched (CaColumn* pObj, MatchObjectFlag nFlag = MATCHED_NAME);
	int  NeedLength(); // return COLUMN_NOLENGTH if length is not required.
	void SetNullable(BOOL bSet){m_bNullable = bSet;}
	void SetDefault (BOOL bSet){m_bDefault = bSet;}
	void SetColumnText(BOOL bSet){m_bColumnText= bSet;}
	void SetDefaultValue (LPCTSTR lpszValue){m_strDefault = lpszValue;}
	void SetTypeStr(LPCTSTR lpszType){m_strDataType = lpszType;}
	void SetLength(int nVal){m_nLength = nVal;}
	void SetScale (int nVal){m_nScale = nVal;}
	void SetDefaultDescription (int nVal){m_nHasDefault = nVal;}
	void SetKeySequence(int nSet){m_nKeySequence = nSet;}
	void SetSystemMaintained (BOOL bSet){m_bSystemMaintained = bSet;}
	void SetInternalTypeStr(LPCTSTR lpszType){m_strInternalDataType = lpszType;}

	CString GetTypeStr(){return m_strDataType;}
	CString GetDefaultValue(){ASSERT(m_bDefault);  return m_strDefault;}
	BOOL IsNullable(){return m_bNullable;}
	BOOL IsDefault(){return m_bDefault;}
	BOOL IsColumnText(){return m_bColumnText;}
	BOOL IsSystemMaintained (){return m_bSystemMaintained;}
	BOOL IsUserDefinedDataTypes ();

	int GetType(){return m_iDataType;}
	int GetLength(){return m_nLength;}
	int GetScale (){return m_nScale;}
	int GetDefaultDescription (){return m_nHasDefault;}
	int GetKeySequence(){return m_nKeySequence;}

	void GenerateStatement (CString& strColumText);
	void SetShortDisplay(BOOL bSet){m_bShortDisplay = bSet;}

	BOOL CanBeInWhereClause();           // Used in Ija control
	BOOL RoundingErrorsPossibleInText(); // Used in Ija control
protected:
	void Copy (const CaColumn& c);

	int     m_iDataType;    // Type code. For example IISQ_VCH_TYPE is 21, 
	CString m_strDataType;  // Type code. For example IISQ_VCH_TYPE is VARCHAR
	CString m_strInternalDataType; // Type code. For example OBJECT_KEY or TABLE_KEY
	                               // or PNUM, COMPLEX for User defined Data Type
	CString m_strLength;
	BOOL    m_bNullable;    // TRUE if column can contain null value 
	BOOL    m_bDefault ;    // TRUE if column is given a default value when a row is inserted. 
	BOOL    m_bColumnText;  // The data of this column is Text;
	CString m_strDefault;   // Default value.
	int     m_nLength;      // Lenght if data type requires
	int     m_nScale;       // If data type requires. Ex for dacimal, then DECIMAL (m_nLength, m_nScale)
	int     m_nHasDefault;  // 1=Y (column is defined as with default or default value) 
	                        // 2=N (column is defined as not default)
	                        // 0=U (column is defined without a default, or unknown)
	BOOL    m_bShortDisplay;// Display in short format;
	int     m_nKeySequence; // The order of column in the PRIMARY or UNIQUE key. numbered from 1.
	BOOL    m_bSystemMaintained; // TRUE if the column is system-maintained, FALSE if not system-maintained.
public:
	static CaColumn* FindColumn(LPCTSTR lpszName, CTypedPtrList< CObList, CaColumn* >& listColumn);

};


#endif // DMLCOLUMNS_HEADER