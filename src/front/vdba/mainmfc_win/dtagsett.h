/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
** 
**    Source   : dtagsett.h, Header File
**    Project  : Ingres II / Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Data defintion for General Display Settings.
**
** History:
**
** xx-Nov-1998 (uk$so01)
**    Created
** 26-Apr-2000 (uk$so01)
**    SIR #101328
**    Set the autocomit option to FALSE in the SQL Test Preference Box.
** 04-Aug-2000 (noifr01)
**    SIR #102274
**    restored the initial autocommit option to TRUE for EDBC specifically
** 05-Jul-2001 (uk$so01)
**    Integrate & Generate XML (Setting the XML Preview Mode).
** 24-Jul-2001 (schph01)
**    SIR #105273 
**    Change the default float display mode by "n" instead of "f".
**    Manage default precision and scale, according to IEEE_FLOAT define.
** 21-Mar-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
**/

#if !defined (DTAGSETT_HEADER)
#define DTAGSETT_HEADER

//
// General Display Setting:
class CaGeneralDisplaySetting: public CObject
{
	DECLARE_SERIAL (CaGeneralDisplaySetting)
public:
	CaGeneralDisplaySetting();
	CaGeneralDisplaySetting(const CaGeneralDisplaySetting& c);
	CaGeneralDisplaySetting operator = (const CaGeneralDisplaySetting& c);

	virtual ~CaGeneralDisplaySetting(){}
	virtual void Serialize (CArchive& ar);

	BOOL Save();
	BOOL Load();
	BOOL LoadDefault();
	void NotifyChanges();

public:
	//
	// 'm_nDlgWaitDelayExecution' is a delay (in seconds) between the execution of the Thread and
	// the display of Dialog with animation:
	long m_nDlgWaitDelayExecution;
	//
	// The list control will have the line seperators (grids)
	BOOL m_bListCtrlGrid;

	// Do we maximize documents when created?
	BOOL m_bMaximizeDocs;

	//
	// Preference Dialog Display:
	// BIG ICON, SMALL ICON, ....
	int  m_nGeneralDisplayView;
	int  m_nSortColumn;
	BOOL m_bAsc;
protected:
	void Copy (const CaGeneralDisplaySetting& c);
};




#endif // DTAGSETT_HEADER
