#ifndef SAVELOAD_INCLUDED
#define SAVELOAD_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

BOOL SaveNewMonitorWindow(HWND hWnd);
BOOL SaveNewDbeventWindow(HWND hWnd);
BOOL SaveNewNodeselWindow(HWND hWnd);
BOOL SaveNewSqlactWindow(HWND hWnd);

BOOL OpenNewMonitorWindow(void);
BOOL OpenNewDbeventWindow(void);
BOOL OpenNewNodeselWindow(void);
BOOL OpenNewSqlactWindow(void);

BOOL SaveExNonMfcSpecialData(HWND hWnd);

#ifdef __cplusplus
};
#endif

#endif  // SAVELOAD_INCLUDED
