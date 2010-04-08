// DgErrDet.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "dgerrdet.h"

#include "dgerrh.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// used in dbadlg1\msghandl.c
extern "C" void MessageWithHistoryButton(HWND CurrentFocus, LPCTSTR Message)
{
  /* TEST ONLY: 
  MessageBox(CurrentFocus, Message, NULL, MB_ICONEXCLAMATION | MB_OKCANCEL );
  */

  CuDlgErrorWitnDetailBtn dlg;

  dlg.m_errText = Message;
  dlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CuDlgErrorWitnDetailBtn dialog


CuDlgErrorWitnDetailBtn::CuDlgErrorWitnDetailBtn(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgErrorWitnDetailBtn::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgErrorWitnDetailBtn)
	m_errText = _T("");
	//}}AFX_DATA_INIT
}


void CuDlgErrorWitnDetailBtn::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgErrorWitnDetailBtn)
	DDX_Control(pDX, IDOK, m_btnOk);
	DDX_Control(pDX, IDC_ERRTXT, m_edErrTxt);
	DDX_Control(pDX, IDC_DETAIL, m_btnDetail);
	DDX_Control(pDX, IDC_STATIC_ICON, m_stIcon);
	DDX_Text(pDX, IDC_ERRTXT, m_errText);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgErrorWitnDetailBtn, CDialog)
	//{{AFX_MSG_MAP(CuDlgErrorWitnDetailBtn)
	ON_BN_CLICKED(IDC_DETAIL, OnDetail)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


// Adjust ratio for text size calculation - in percentage
#define ADJUSTRATIO 20
static int AdjustedRatio(int originalWidth)
{
  int extra = (long)originalWidth * ADJUSTRATIO / 100L;

  int newWidth = originalWidth + extra;
  return newWidth;
}


// window too high if less than VERT_RATIO/HORZ_RATIO times more wide than high
#define VERT_RATIO 4L
#define HORZ_RATIO 1L
static BOOL WouldBeTooHigh(int pixWidth, int pixHeight)
{
  // if pixHeight * VertRatio > pixWidth * HorzRatio, would be too high
  long VertRatio = VERT_RATIO;
  long HorzRatio = HORZ_RATIO;
  long lW = pixWidth * HorzRatio;
  long lH = pixHeight * VertRatio;
  ASSERT (lW > 0);
  ASSERT (lH > 0);
  if (lH > lW)
    return TRUE;
  return FALSE;
}

BOOL CuDlgErrorWitnDetailBtn::CalculateTxtRect(CRect *prcNewTxt)
{
  CRect rcTxt;
  m_edErrTxt.GetWindowRect(rcTxt);  // For minimums
  int refWidth = rcTxt.Width();
  int refHeight = rcTxt.Height();

  // Select the dialog font
  CClientDC dc(this);
  CFont* pFont = GetFont();
  CFont* oldFont = dc.SelectObject(pFont);

  // Calculate reference line
  CSize refSize;
  refSize = dc.GetTextExtent("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", 52);
  int OneLineHeight = refSize.cy;

  // Calculate size if only one line
  CSize size;
  size = dc.GetTextExtent(m_errText);
  // manage "new lines"
  ASSERT (size.cy % OneLineHeight == 0);
  int nbForcedNewLines = size.cy / OneLineHeight - 1;
  if (nbForcedNewLines > 0) {
    // HARD JOB HERE! TO BE ENHANCED - ANY IDEA ?
  }

  int pixWidth = refWidth;
  int pixHeight = refHeight;

  int pixFullWidth = size.cx;

  // pre-test : less than one line: use rcNew "as is"
  if (pixFullWidth > refWidth) {
    // iterative process: starting from current control width,
    // estimate number of lines (calculation assumes total consumed width
    // will be increased by ratio adjustment due to space left because of new lines
    // Stop when 2/3 of display width reached, whatever the height is
    int screenWidth = GetSystemMetrics(SM_CXFULLSCREEN);
    int limitWidth = screenWidth * 2 / 3;
    for (pixWidth = refWidth; pixWidth < limitWidth; pixWidth += pixWidth/10) {
      int nbLines = ( (AdjustedRatio(pixFullWidth)) + pixWidth -1)  / pixWidth;
      // adjust if forced new lines in text
      if (nbForcedNewLines) {
        if (nbLines < nbForcedNewLines + 1)
          nbLines = nbForcedNewLines + 1;
      }
      pixHeight = OneLineHeight * nbLines;

      // result becomes less and less high, and more and more wide
      // To avoid infinite loop, we act as follows:
      // if "would be too high", continue loop
      // in any other case, break the loop
      if (WouldBeTooHigh(pixWidth, pixHeight))
        continue;

      // acceptable
      break;
    }
  }

  // select the old font
  dc.SelectObject(oldFont);

  // Update result rect
  CRect rcNew(rcTxt);
  rcNew.right = rcNew.left + pixWidth;
  rcNew.bottom = rcNew.top + pixHeight;

  // Ensure minimum size
  if (rcNew.Width() < rcTxt.Width())
    rcNew.right = rcNew.left + rcTxt.Width();
  if (rcNew.Height() < rcTxt.Height())
    rcNew.bottom = rcNew.top + rcTxt.Height();

  if (rcNew == rcTxt)
    return FALSE;   // size not modified

  *prcNewTxt = rcNew;
  return TRUE;
}




/////////////////////////////////////////////////////////////////////////////
// CuDlgErrorWitnDetailBtn message handlers

BOOL CuDlgErrorWitnDetailBtn::OnInitDialog() 
{
	CDialog::OnInitDialog();

  // Set icon
  HICON hIcon = LoadIcon(NULL, IDI_EXCLAMATION);
  ASSERT (hIcon);
  m_stIcon.SetIcon(hIcon);

  // Get Rectangles of all relevant controls
  CRect rcDlg;
  GetWindowRect(rcDlg);

  // Calculate the minimum rectangle to display entirely the error text
  CRect rcNewTxt;
  BOOL bInflated = CalculateTxtRect(&rcNewTxt);
  if (bInflated) {
    // Calculate pixel inflate X/Y of error text control
    CRect rcTxt;
    m_edErrTxt.GetWindowRect(rcTxt);
    int inflateX, inflateY;
    inflateX = rcNewTxt.Width() - rcTxt.Width();
    inflateY = rcNewTxt.Height() - rcTxt.Height();

    // Adjust size of dialog
    // No need to call Screen To Client since no move
    SetWindowPos(0, 0, 0,
                 rcDlg.Width() + inflateX, rcDlg.Height() + inflateY,
                 SWP_NOZORDER | SWP_NOMOVE);

    // Adjust Error Text
    // No need to call Screen To Client since no move
    m_edErrTxt.SetWindowPos(0, 0, 0,
                            rcNewTxt.Width(), rcNewTxt.Height(),
                            SWP_NOZORDER | SWP_NOMOVE);

    // Adjust Detail Button
    CRect rcDetail;
    m_btnDetail.GetWindowRect(rcDetail);
    ScreenToClient (rcDetail);
    m_btnDetail.SetWindowPos(0,
                             rcDetail.left, rcDetail.top + inflateY,
                             0, 0,
                             SWP_NOZORDER | SWP_NOSIZE);

    // Adjust Ok Button
    CRect rcOk;
    m_btnOk.GetWindowRect(rcOk);
    ScreenToClient (rcOk);
    m_btnOk.SetWindowPos(0,
                         rcOk.left + inflateX / 2, rcOk.top + inflateY,
                         0, 0,
                         SWP_NOZORDER | SWP_NOSIZE);
  }
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgErrorWitnDetailBtn::OnDetail() 
{
  ASSERT (m_hWnd);
  DisplayStatementsErrors(m_hWnd);
}
