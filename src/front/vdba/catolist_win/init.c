#include "dll.h"

#include "catolist.h"
#include "resource.h"

HINSTANCE	hinst;

BOOL FRegisterControl(HINSTANCE hInstance);

#ifdef WIN32
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
			FRegisterControl(hInstance);
			hinst = hInstance;
			break;

		case DLL_PROCESS_DETACH:
			break;

		case DLL_THREAD_ATTACH:
			hinst = hInstance;
			break;

		case DLL_THREAD_DETACH:
			break;
	}

	return TRUE;
}
#else
HINSTANCE WINAPI LibMain(HINSTANCE hInstance, WORD wDataSeg,
                          WORD cbHeapSize, LPSTR lpCmdLine);
/*
 * LibMain
 *
 * Purpose:
 *  DLL-specific entry point called from LibEntry.  Initializes
 *  the DLL's heap and registers the CATOLIST custom control
 *  class.
 *
 * Parameters:
 *  hInstance       HANDLE instance of the DLL.
 *  wDataSeg        WORD segment selector of the DLL's data segment.
 *  wHeapSize       WORD byte count of the heap.
 *  lpCmdLine       LPSTR to command line used to start the module.
 *
 * Return Value:
 *  HANDLE          Instance handle of the DLL.
 *
 */

HINSTANCE WINAPI LibMain(HINSTANCE hInstance, WORD wDataSeg,
                          WORD cbHeapSize, LPSTR lpCmdLine)
{
	//Go register the control
	if (FRegisterControl(hInstance))
	{
		hinst = hInstance;

		if (cbHeapSize != 0)
			UnlockData(0);

		return hInstance;
	}

	hinst = NULL;
	return (HINSTANCE)NULL;
}
#endif


/*
 * FRegisterControl
 *
 * Purpose:
 *  Registers the CATOLIST control class, including CS_GLOBALCLASS
 *  to make the control available to all applications in the system.
 *
 * Parameters:
 *  hInstance       HANDLE Instance of the application or DLL that will
 *                  own this class.
 *
 * Return Value:
 *  BOOL            TRUE if the class is registered, FALSE otherwise.
 *                  TRUE is also returned if the class was already
 *                  registered.
 */

BOOL FRegisterControl(HINSTANCE hInstance)
{
	static BOOL     fRegistered=FALSE;
	WNDCLASS        wc;
	char            szClass[40];

	if (!fRegistered)
	{
		//Load the class name
		if (LoadString(hInstance, IDS_CLASSNAME, szClass, sizeof(szClass)) == 0)
			return FALSE;

		wc.lpfnWndProc   = CATOLISTWndProc;
		wc.cbClsExtra    = CBCLASSEXTRA;
		wc.cbWndExtra    = CBWINDOWEXTRA;
		wc.hInstance     = hInstance;
		wc.hIcon         = NULL;
		wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
		wc.lpszMenuName  = NULL;
		wc.lpszClassName = szClass;
		wc.style         = CS_DBLCLKS | CS_GLOBALCLASS |
                           CS_VREDRAW | CS_HREDRAW;

		fRegistered=RegisterClass(&wc);
	}

	return fRegistered;
}



