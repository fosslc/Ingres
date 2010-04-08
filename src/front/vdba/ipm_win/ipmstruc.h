/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
** Source   : ipmstruc.h, Header File 
** Project  : Ingres II/ (Vdba Monitoring)
** Author   : UK Sotheavut (uk$so01)
** Purpose  : Declaration of Constants and Structures 
**
** History:
**
** 12-Nov-2001 (uk$so01)
**    Created
*/

#if !defined (_IPMDML_HEADER)
#define _IPM_CONSTANTxSTRUCTURE_HEADER

// constants
#define DEFAULT_IMAGE_ID    0
#define IMAGE_WIDTH        16   // ex-12
#define IMAGE_HEIGHT       15   // ex-12

// Sub branch type
typedef enum
{
	SUBBRANCH_KIND_NONE = 1,
	SUBBRANCH_KIND_STATIC,
	SUBBRANCH_KIND_OBJ,
	SUBBRANCH_KIND_MIX
}	SubBK;

// action for display
typedef enum
{
	DISPLAY_ACT_EXPAND = 1,
	DISPLAY_ACT_REFRESH,
	DISPLAY_ACT_CHANGEFILTER
} DispAction;

// cause for refresh
typedef enum
{
	REFRESH_ADD = 1,
	REFRESH_ALTER,
	REFRESH_DROP,
	REFRESH_ACTIVATEWIN,
	REFRESH_CLOSEWIN,
	REFRESH_CONNECT,
	REFRESH_DBEVENT,
	REFRESH_DISCONNECT,
	REFRESH_MONITOR,
	REFRESH_SQLTEST,
	REFRESH_FORCE,
	REFRESH_KILLSHUTDOWN
} RefreshCause;

// cause for change filter
typedef enum
{
	FILTER_NOTHING = 1,
	FILTER_RESOURCE_TYPE,
	FILTER_NULL_RESOURCES,
	FILTER_INTERNAL_SESSIONS,
	FILTER_SYSTEM_LOCK_LISTS,
	FILTER_INACTIVE_TRANSACTIONS,
	FILTER_SEVERAL_ACTIONS,

	FILTER_DOM_SYSTEMOBJECTS,
	FILTER_DOM_BASEOWNER,
	FILTER_DOM_OTHEROWNER,

	// New as of May 25, 98: for detail window of type list: update if necessary
	// if used, need to test new field "UpdateType" of LPIPMUPDATEPARAMS against
	// type used by GetFirst/Next loop, for the relevant detail window
	FILTER_DOM_BKREFRESH,         // means refresh panes that look like lists
	FILTER_DOM_BKREFRESH_DETAIL,
	FILTER_IPM_EXPRESS_REFRESH,
} FilterCause;


// item kind
typedef enum
{
	ITEM_KIND_NORMAL = 1,
	ITEM_KIND_NOITEM,
	ITEM_KIND_ERROR
} ItemKind;

#define CHILDTYPE_UNDEF (-1)

#endif //_IPM_CONSTANTxSTRUCTURE_HEADER
