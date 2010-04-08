#if !defined(AFX_256BMP_H__F3163F5E_E80C_11D0_9397_0020AF397415__INCLUDED_)
#define AFX_256BMP_H__F3163F5E_E80C_11D0_9397_0020AF397415__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif /* _MSC_VER >= 1000 */

/*
**
**  Name: 256bmp.h
**
**  Description:
**	This is the header file for the 256bmp.cpp file.
**
**  History:
**	??-???-???? (mcgem01)
**	    Created.
*/

/*
** C256bmp window
*/
class C256bmp : public CStatic
{
/* Construction */
public:
    C256bmp();

/* Attributes */
public:

/* Operations */
public:

/* Overrides */
    /* ClassWizard generated virtual function overrides */
    //{{AFX_VIRTUAL(C256bmp)
    //}}AFX_VIRTUAL

/* Implementation */
public:
    BITMAPINFO * GetBitmap();
    void GetBitmapDimension(CPoint & pt);
    virtual ~C256bmp();

    /* Generated message map functions */
protected:
    //{{AFX_MSG(C256bmp)
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_256BMP_H__F3163F5E_E80C_11D0_9397_0020AF397415__INCLUDED_)
