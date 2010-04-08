/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project : CA/OpenIngres Visual DBA
**
**    Source : domdisp.h
**    manages the dom trees display update according to fresh information
**    supplied by calling DomGetFirstObject/DomGetNextObject
**
**  26-Mar-2002 (noifr01)
**   (bug 107442) removed the "force refresh" dialog box in DOM windows: force
**    refresh now refreshes all data
********************************************************************/

#ifndef __DOMDISP_INCLUDED__
#define __DOMDISP_INCLUDED__

// constants for iAction
// TO BE FINISHED: ADD, ALTER and DROP can be merged
#define ACT_EXPANDITEM                1   // after item expansion by user
#define ACT_ADD_OBJECT                2   // after add object
#define ACT_ALTER_OBJECT              3   // after alter object
#define ACT_DROP_OBJECT               4   // after drop object
#define ACT_CHANGE_BASE_FILTER        5   // after base filter change
#define ACT_CHANGE_OTHER_FILTER       6   // after other filter change
#define ACT_CHANGE_SYSTEMOBJECTS      7   // after system checkbox change
#define ACT_BKTASK                    8   // NOT TO BE USED YET !
#define ACT_UPDATE_DEPENDANT_BRANCHES 9   // Internal value, should not be used from outside domdisp.c
#define ACT_NO_USE                    10  // Internal value - TEMPORARY

// Values for forceRefreshMode in DOMUpdateDisplayData2
#define FORCE_REFRESH_NONE            0   // No force refresh needed
#define FORCE_REFRESH_ALLBRANCHES     3   // all branches with all their subbranches

// public functions
int  DOMUpdateDisplayData_MT (int      hnodestruct,    // handle on node struct
                           int      iobjecttype,    // object type
                           int      level,          // parenthood level
                           LPUCHAR *parentstrings,  // parenthood names
                           BOOL     bWithChildren,  // should we expand children?
                           int      iAction,        // why is this function called ?
                           DWORD    idItem,         // if expansion: item being expanded
                           HWND     hwndDOMDoc);    // handle on DOM MDI document
int  DOMUpdateDisplayData (int      hnodestruct,    // handle on node struct
                           int      iobjecttype,    // object type
                           int      level,          // parenthood level
                           LPUCHAR *parentstrings,  // parenthood names
                           BOOL     bWithChildren,  // should we expand children?
                           int      iAction,        // why is this function called ?
                           DWORD    idItem,         // if expansion: item being expanded
                           HWND     hwndDOMDoc);    // handle on DOM MDI document

// This function has to be called when the user asked for a "Force refresh"
// using the related menuitem and dialog box.
int  DOMUpdateDisplayData2(int      hnodestruct,    // handle on node struct
                           int      iobjecttype,    // object type
                           int      level,          // parenthood level
                           LPUCHAR *parentstrings,  // parenthood names
                           BOOL     bWithChildren,  // should we expand children?
                           int      iAction,        // why is this function called ?
                           DWORD    idItem,         // if expansion: item being expanded
                           HWND     hwndDOMDoc,     // handle on DOM MDI document
                           int      forceRefreshMode);  // which action for forceRefresh?

// Display Disable/Enable functions
int  DOMDisableDisplay(int hnodestruct, HWND hwndDOMDoc);
int  DOMEnableDisplay(int hnodestruct, HWND hwndDOMDoc);
int  DOMDisableDisplay_MT(int hnodestruct, HWND hwndDOMDoc);
int  DOMEnableDisplay_MT(int hnodestruct, HWND hwndDOMDoc);

// gives the type of the child that would be created under the current
// tree line of the given type, or OT_STATIC_DUMMY if sub-statics only
int GetChildType(int iobjecttype, BOOL bNoSolve);

// gives the static type for an object type
// example : OT_USER -> OT_STATIC_USER
int GetStaticType(int iobjecttype);

// says whether the object type is a OTR_ type
BOOL IsRelatedType(int iobjecttype);

// returns the basic type of an object. example: OT_GROUPUSER ---> OT_USER
// MUST be called before calling IsSystemObject, HasSystemObjects,
// HasExtraDisplayString or GetExtraDisplayStringCaption
int GetBasicType(int iobjecttype);

// says whether a type can have system objects
BOOL CanBeSystemObj(int iobjecttype);

// This function says whether, for the given object with the given
// parenthood, at least one dom opened on the given node has already
// expanded the branch
// This function will be used to optimize the save/load environment
BOOL IsDomDataNeeded (int      hnodestruct,    // handle on node struct
                      int      iobjecttype,    // object type
                      int      level,          // parenthood level
                      LPUCHAR *parentstrings); // parenthood names

#endif //__DOMDISP_INCLUDED__

