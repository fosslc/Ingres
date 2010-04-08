/*
**  Copyright (c) 2004 Ingres Corporation
*/

/*
**  History:
**	29-apr-2004 (wonst02)
**	    Created.
*/

#pragma once
#include "256bmp.h"


// WcFinal dialog

class WcFinal : public CPropertyPage
{
	DECLARE_DYNAMIC(WcFinal)

public:
	WcFinal();
	virtual ~WcFinal();

// Dialog Data
	enum { IDD = IDD_FINALPROP };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnSetActive();
	C256bmp m_image;
	virtual BOOL OnWizardFinish();
};
