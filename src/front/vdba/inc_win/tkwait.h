/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : tkwait.h, Header file 
**    Project  : Extension DLL (Task Animation).
**    Author   : Sotheavut UK (uk$so01)
**
**    Purpose  : Exported Classes for Dialog that shows the progression of Task (Interruptible)
**
** History:
**
** 07-Feb-1999 (uk$so01)
**    created
** 16-Mar-2001 (uk$so01)
**    Changed to be an Extension DLL
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
*/


#if !defined(AFX_TKWAIT_H__3CB4766D_EC09_11D2_A2E0_00609725DDAF__INCLUDED_)
#define AFX_TKWAIT_H__3CB4766D_EC09_11D2_A2E0_00609725DDAF__INCLUDED_
#undef AFX_DATA
#define AFX_DATA AFX_EXT_DATA
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//
// CONSTANT FOR INTERRUPT MODE:
// ---------------------------
#define INTERRUPT_NOT_ALLOWED 0
#define INTERRUPT_USERPRECIFY 0x0001
#define INTERRUPT_KILLTHREAD  0x0002

//
// Make sure that the application that uses this message WM_EXECUTE_TASK
// is not in conflict with its own user defined message:
#define WM_EXECUTE_TASK (WM_USER + 6001)
//
// Enumeration for WPARAM of WM_EXECUTE_TASK
enum
{
	W_EXTRA_TEXTINFO = 0,  // LPARAM is (LPTSTR)
	W_RUN_TASK,            // LPARAM is 0, Start the Thread funtion
	W_STARTSTOP_ANIMATION, // LPARAM is 0, Stop playing the animation, 1 Start playing the animation
	W_TASK_TERMINATE,      // LPARAM is m_nReturnCode,  The Thread funtion has terminate
	W_TASK_PROGRESS,       // LPARAM is current_job * (100/total_job)
	W_KILLTASK             // LPARAM is 0 (kill the thread through TerminateThread(handle).
};



//
// EXPORT CLASS CaExecParam:
// ************************************************************************************************
class AFX_EXT_CLASS CaExecParam: public CObject
{
public:
	//
	// Default constructor: not interuptible !!
	CaExecParam();
	CaExecParam(const CaExecParam& c);
	//
	// Second constructor, allow to choose interrupt of execution:
	// For the possible values of 'nType'
	// Please see the #define INTERRUPT_XXX above.
	CaExecParam(UINT nInteruptType);
	virtual ~CaExecParam();

	//
	// OVERWRITE:
	// The derived classes must overwrite these member functions:
	// NOTE: You should not attempt to manipulate the RESOURCES, i.e LoadString(),
	//       LoadIcon(), ... in these bellow members. Instead you manimpulate
	//       when the object is constructed. This is because the member functions
	//       bellow are called in the context of the Extension DLL and not in the context
	//       of the caller.

	// int Run(): A thread will call this member to execute your job.
	// Should return 0 if succeeded.
	virtual int Run(HWND hWndTaskDlg = NULL){ASSERT(FALSE); return 0;}
	// int OnTerminate(): The dialog box will call this member when the Thread 
	// that calls Run() has terminated.
	virtual void OnTerminate(HWND hWndTaskDlg = NULL){}
	// BOOL OnCancelled(): The dialog box will call this member when user cancels
	// the operation. (Only if the object is interruptible)
	// You should return FALSE if you do not want to cancel.
	virtual BOOL OnCancelled(HWND hWndTaskDlg = NULL){return TRUE;}


	//
	// Execute immediately:
	// If m_bExecuteImmediately is TRUE then the animate dialog will be shown up
	// immediately before running the Task.
	// Otherwise the task is executed fist and the dialog waits for an elapse of time
	// before becoming visible.
	// (Default m_bExecuteImmediately = TRUE)
	void SetExecuteImmediately(BOOL bSet){m_bExecuteImmediately = bSet;}
	BOOL IsExecuteImmediately(){return m_bExecuteImmediately;}

	//
	// Delay time (in ms) after that we display the dialog box if the Thread is still alive 
	// The member is used only if IsExecuteImmediately() return FALSE.
	UINT GetDelayExecution(){return m_nDelayExecution;}
	void SetDelayExecution(UINT nDelay){m_nDelayExecution = nDelay;}

	//
	// The dialog will call this function to see if the task can be interrupted.
	UINT GetInterruptType(){return m_nInterruptType;}

	BOOL IsInterrupted(){return m_bInterrupted;}
	void SetInterrupted(BOOL bSet){m_bInterrupted = bSet;}

protected:
	//
	// 'm_bExecuteImmediately' Indicates to launch the Dialog immediately
	//  before running the Thread T1 that call the member Run()
	BOOL m_bExecuteImmediately;
	void Copy(const CaExecParam& c);

protected:
	HANDLE m_hMutex;
	BOOL m_bInterrupted;
	UINT m_nInterruptType;
	UINT m_nDelayExecution;

	void Init(UINT nInteruptType);
};


enum 
{
	AVI_CLOCK,
	AVI_CONNECT,
	AVI_DELETE,
	AVI_DISCONNECT,
	AVI_DRAGDROP,
	AVI_EXAMINE,
	AVI_FETCHF,
	AVI_FETCHR,
	AVI_REFRESH
};


//
// EXPORT CLASS CxDlgWait:
// ************************************************************************************************
class AFX_EXT_CLASS CxDlgWait
{
public:
	enum CenterType {OPEN_AT_MOUSE, OPEN_CENTER_AT_MOUSE};
	//
	// The second constructor must be called with lpszCaption not null.
	// NOTE: IDD_XTASK_WAIT_WITH_TITLE is a duplicated DLG ID from IDD_XTASK_WAIT.
	//       It has title, ... (I canot use neither ModifyStyle() nor SetWindowLong()
	//       to change the dialog style on creation !!!.
	CxDlgWait(LPCTSTR lpszCaption, CWnd* pParent = NULL);

	virtual ~CxDlgWait();
	int DoModal();

	//
	// SetExecParam + GetExecParam
	// First of all, you must call SetExecParam to set the object of your 
	// derived class from CaExecParam.
	void SetExecParam (CaExecParam* pExecParam);
	CaExecParam* GetExecParam();

	//
	// SetDeleteParam()
	// Indicate the animate dalog if it must delete object CaExecParam on destroy. 
	// bDelete = TRUE, the animate dalog deletes object CaExecParam on destroy. 
	// (The default is FALSE)
	void SetDeleteParam(BOOL bDelete);

	//
	// SetUseAnimateAvi()
	// Show the animation by playing the avi file.
	// (The dafult is AVI_CLOCK)
	void SetUseAnimateAvi (int nAviType);

	//
	// Show the static control of two lines of text under the animate control:
	// Default (hide)
	void SetUseExtraInfo();

	//
	// Show or hide the progress control:
	// Default (hide, and has the range in [0 100]
	void SetShowProgressBar(BOOL bShow);

	//
	// Hide animate dialog when the thread that calls  CaExecParam::Run() has finished:
	// Default (hide)
	void SetHideDialogWhenTerminate(BOOL bHide);

	//
	// If bHide = TRUE, then even if the interruption is allowed
	// the cancel button is always hide. But the user can press the ESC key to cancel.
	void SetHideCancelBottonAlways(BOOL bHide);

	//
	// Force the dialog to open and center at the mouse pointer:
	void CenterDialog(CenterType nType = OPEN_AT_MOUSE);

private:
	void* m_pDlg;

};

//
// Using this DLL from another Regular DLL (ActiveX control, In-Process DLL) must
// call explicitely this function TaskAnimateInitialize()
extern "C" BOOL WINAPI TaskAnimateInitialize();

#undef AFX_DATA
#define AFX_DATA
#endif // !defined(AFX_TKWAIT_H__3CB4766D_EC09_11D2_A2E0_00609725DDAF__INCLUDED_)
