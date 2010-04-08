/*	CustCtrl.h

	Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
	

	Messages and flags for Custom Control servers.

	Date	   Who	Description
	---------  ---	----------------------------------------------------------
	25-Aug-94  PJM	Add SHOWHIDE support for custom controls.
	16-Nov-93  PJM	Fix OS/2 IF_STATIC compile problems.
	15-Nov-93  PJM	Add support for STATIC-capable custom controls.

*****************************************************************************/


/* User-item messages */
#define CCM_QDEFAULTSIZE	(WM_USER + 0x100)
#define CCM_GETFLAGS		(WM_USER + 0x101)
#define CCM_GETNUM			(WM_USER + 0x102)
#define CCM_GETSTRCOUNT		(WM_USER + 0x103)
#define CCM_GETSTRLEN		(WM_USER + 0x104)
#define CCM_GETSTR			(WM_USER + 0x105)
#define CCM_FORMSIZED		(WM_USER + 0x106)
#define CCM_SETNUM			(WM_USER + 0x107)
#define CCM_SETSTR			(WM_USER + 0x108)
#define CCM_RESETCONTENT	(WM_USER + 0x109)

#define CCM_CUSTOM			(WM_USER + 0x2000)
#define CCM_CUSTOM_NOW		(WM_USER + 0x2001)


/*	CCF_* are bit masks for responding to the CCM_GETFLAGS message.
	Return a sum of these to the CCM_GETFLAGS message, or don't
	process CCM_GETFLAGS at all to get the default settings.
*/
#define CCF_FOCUS		0x0001
	/* By default, custom control will not receive focus. */
	/* If the custom control processes key events, it should set CCF_FOCUS */
	/* and process WM_SETFOCUS, WM_KILLFOCUS, AND visibly show the focus. */

#define CCF_INITGRAY	0x0002
	/* By default, custom controls are initially _Normal (not _Gray). */

#define CCF_NOENABLES	0x0004
	/* By default, the item will get a WM_ENABLE message every time the */
	/* form becomes front or leaves the front.  If this flag is set, the */
	/* item will get enables only when its _Normal/_Gray-state changes. */


/*	By convention, the first numeric modifier in the FormSetObject
	may modify the style of the custom control.  If the control ordinarily
	takes integer-range modifiers, this constant can distinguish
	ordinary modifiers from style modifiers as in:
		FormSetObject(rsID, "cc_class", ...; _CCSTYLE + flag1 + flag2 ...)
*/
#undef  CCSTYLE    // this is defined elsewhere so undefine it
#define CCSTYLE		65536				/* matches Realizer's _CCStyle */



/*	User-defined messages should be small positive offsets to CCMBASE.
*/
#define CCMBASE		(WM_USER + 0x110)	/* matches Realizer's _CCM */


//////////////////////////////////////////////////////////////////////////////
// Start of post-2.0 changes /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#define HPAINTCTX HDC

typedef struct tag_STATIC_PAINTSTRUCT {
	HWND			hw;			// HWND corresponding to the hpc
	HWND			hFormW;		// HWND of the form (same as hw if static)
	HPAINTCTX		hpc;
	RECT			rect;
	COLORREF		fgColor;
	HBRUSH			bkBrush;
	HFONT			hFont;
	LPSTR			lpText;
	LPVOID			lpRetainedInfo;
} STATIC_PAINTSTRUCT, FAR *SPS_PTR;

#define CCM_STATIC_QINFOSIZE	(WM_USER + 0x2010)
	// wP: unused
	// lP: unused
	// Return the number of bytes the control needs for retained storage.
	// Remember, once a control is STATIC-ized, it cannot access window words!
	// If the control returns a value < 0, no storage will be allocated, and
	// CCM_STATIC_SETINFO will not be invoked.
	// If the control returns 0, the control will NOT be STATIC-ized.
	// If the control returns > 0, that many bytes will be allocated,
	// and CCM_STATIC_SETINFO will be called with lP pointing to the memory.

#define CCM_STATIC_SETINFO		(WM_USER + 0x2011)
	// wP: unused
	// lP: pointer to control's retained storage.
	// The control must unload everything from window words/longs into this
	// retained storage buffer, because the window itself will be destroyed
	// immediately hereafter.

#define CCM_STATIC_PAINT		(WM_USER + 0x2012)
	// wP: unused
	// lP: pointer to STATIC_PAINTSTRUCT

#define CCM_STATIC_DESTROY		(WM_USER + 0x2014)
	// wP: unused
	// lP: pointer to control's retained storage.
	// The control must destroy anything it would have destroyed during
	// WM_DESTROY if it hadn't been made static.


//////////////////////////////////////////////////////////////////////////////
// SHOWHIDE messages for custom controls /////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


#define CCM_SHOWHIDE_QINFOSIZE	(WM_USER + 0x2020)
	// wP: unused
	// lP: unused
	// Return the number of bytes the control needs for retained storage.
	// Remember, once a control is 'hidden', it cannot access window words!
	// If the control returns a value < 0, no storage will be allocated, but
	// the control will still receive CCM_SHOWHIDE_HIDE and CCM_SHOWHIDE_SHOW.
	// If the control returns 0, the control will NOT be SHOWHIDE-hidden.
	// If the control returns > 0, that many bytes will be allocated,
	// and CCM_SHOWHIDE_HIDE will be called with lP pointing to the memory.

#define CCM_SHOWHIDE_HIDE		(WM_USER + 0x2021)
	// wP: unused
	// lP: pointer to control's (just-allocated) retained storage.
	// The control must unload everything from window words/longs into the
	// retained storage buffer, because the window itself will be destroyed
	// immediately hereafter.  Be careful in the control's WM_DESTROY code to
	// avoid destroying something which is referenced by the retained storage.

#define CCM_SHOWHIDE_SHOW		(WM_USER + 0x2022)
	// wP: unused
	// lP: pointer to control's (about-to-be-freed) retained storage.
	// The control must restore everything it needs from the retained storage
	// saved during CCM_SHOWHIDE_HIDE.  The retained storage buffer will be
	// freed immediately hereafter.

#define CCM_SHOWHIDE_DESTROY	(WM_USER + 0x2024)
	// wP: unused
	// lP: pointer to control's (about-to-be-freed) retained storage.
	// The control must destroy anything it would have destroyed during
	// WM_DESTROY if it hadn't been SHOWHIDE-hidden.  This occurs if the form
	// or control is detroyed while the control is in the SHOWHIDE-hidden state.
	// In this case, the control will not be recreated and it won't receive
	// the usual WM_DESTROY.


//////////////////////////////////////////////////////////////////////////////
// Message crackers and packers for custom control messages. /////////////////
//////////////////////////////////////////////////////////////////////////////


#ifdef WIN16
#define FORWARD_CCM_CUSTOM(hwnd, id, hwndCtl, codeNotify, fn) \
    (void)(fn)((hwnd), CCM_CUSTOM, (WPARAM)(int)(id), MAKELPARAM((UINT)(hwndCtl), (codeNotify)))
#else
#define FORWARD_CCM_CUSTOM(hwnd, id, hwndCtl, codeNotify, fn) \
    (void)(fn)((hwnd), CCM_CUSTOM, MAKEWPARAM((UINT)(id),(UINT)(codeNotify)), (LPARAM)(HWND)(hwndCtl))
#endif
