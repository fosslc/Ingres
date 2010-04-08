//
// DgErrH.h: 
// C/C++ interface for custom message box with button to launch errors history
//
#ifndef DGERR_HEADER
#define DGERR_HEADER

//
// First Section : "extern "C" functions
//
#ifdef __cplusplus
extern "C"
{
#endif

// Defined in DgErrDet.cpp
void MessageWithHistoryButton(HWND CurrentFocus, LPCTSTR Message);

// Defined in main.c
void DisplayStatementsErrors(HWND hWnd);

#ifdef __cplusplus
}
#endif


#endif  // DGERR_HEADER
