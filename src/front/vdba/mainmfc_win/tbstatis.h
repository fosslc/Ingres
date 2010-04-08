/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : tbstatis.h, Header File 
**    Project  : CA-OpenIngres/Visual DBA.
**    Author   : UK Sotheavut.(uk$so01)
**    Purpose  : The Statistics of the table 
**
** History:
** xx-Mar-1998 (uk$so01)
**    Created.
** 03-May-2001 (uk$so01)
**    SIR #102678
**    Support the composite histogram of base table and secondary index.
** 08-Oct-2001 (schph01)
**    SIR #105881
**    Add variable member m_nNumColumnKey in CaTableStatistic class
*/

#if !defined (DOM_RIGHTPANE_TABLE_STATISTIC_HEADER)
#define DOM_RIGHTPANE_TABLE_STATISTIC_HEADER

class CaTableStatisticColumn: public CObject
{
	DECLARE_SERIAL (CaTableStatisticColumn)
public:
	enum
	{
		COLUMN_STATISTIC,
		COLUMN_NAME,
		COLUMN_TYPE,
		COLUMN_PKEY,
		COLUMN_NULLABLE,
		COLUMN_WITHDEFAULT,
		COLUMN_DEFAULT
	};

	CaTableStatisticColumn();
	CaTableStatisticColumn (const CaTableStatisticColumn& c);
	CaTableStatisticColumn operator = (const CaTableStatisticColumn& c);
	virtual ~CaTableStatisticColumn(){}
	virtual void Serialize (CArchive& ar);
	void Copy(const CaTableStatisticColumn& c);

	//
	// lParamSort is a pointer to the structure SORTPARAMS
	// m_nItem member specify wich memer to compare.
	static int CALLBACK Compare (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	//
	// Data members
	BOOL    m_bHasStatistic; // If column has the statistic 
	BOOL    m_bNullable;     // Nullable
	BOOL    m_bWithDefault;  // Use default
	CString m_strColumn;     // Column Name
	CString m_strType;       // Data type
	CString m_strDefault;    // Default string;
	int     m_nLength;       // Length
	int     m_nScale;        // -1: not use. For the decimal, m_nLength = Precision 
	int     m_nPKey;         // Primary Key order
	int     m_nDataType;     // Data type.
	BOOL    m_bComposite;    // Composite column (for the composite histogram)
};



class CaTableStatisticItem: public CObject
{
	DECLARE_SERIAL (CaTableStatisticItem)
public:
	CaTableStatisticItem()
		:m_strValue(_T("")), m_dPercentage(0.0), m_dRepetitionFactor(0.0){};
	CaTableStatisticItem(LPCTSTR lpszValue, double dPercent, double dRepFact)
		:m_strValue(lpszValue), m_dPercentage(dPercent), m_dRepetitionFactor(dRepFact){};
	CaTableStatisticItem (const CaTableStatisticItem& c);
	CaTableStatisticItem operator = (const CaTableStatisticItem& c);

	virtual ~CaTableStatisticItem(){}
	virtual void Serialize (CArchive& ar);

	//
	// Data members:

	CString m_strValue;          // Statistic value
	double  m_dPercentage;       // Percentage
	double  m_dRepetitionFactor; // Frequency

};

class CaTableStatistic: public CObject
{
	DECLARE_SERIAL (CaTableStatistic)
public:
	CaTableStatistic();
	CaTableStatistic (const CaTableStatistic& c);
	CaTableStatistic operator = (const CaTableStatistic& c);

	virtual ~CaTableStatistic();
	virtual void Serialize (CArchive& ar);
	void Cleanup();
	void MaxValues (double& maxPercent, double& maxRepFactor);

	//
	// Data members
	CString m_strVNode;
	CString m_strDBName;
	CString m_strTable;
	CString m_strTableOwner;
	CString m_strIndex;
	BOOL    m_bAsc;         // Sort direction
	int     m_nCurrentSort; // Which column is sorted ?
	int     m_nOT;
	int     m_nNumColumnKey;
	long m_lUniqueValue;
	long m_lRepetitionFlag;

	BOOL m_bUniqueFlag;
	BOOL m_bCompleteFlag;
	BOOL m_AddCompositeForHistogram;
	//
	// List of columns for statistic's use:
	CTypedPtrList<CObList, CaTableStatisticColumn*> m_listColumn;

	//
	// List of statistic of a column in <m_listColumn>
	// if the column has the statistic.
	CTypedPtrList<CObList, CaTableStatisticItem*> m_listItem;
protected:
	void Copy (const CaTableStatistic& c);
};


#endif
