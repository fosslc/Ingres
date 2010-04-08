/*
**
**  Name: perfwiz.h
**
**  Description:
**	This is the main header file for the PerfWiz application.
**
**  History:
**	06-sep-1999 (somsa01)
**	    Created.
**	18-oct-1999 (somsa01)
**	    Added NumCountersSelected, a global variable which keeps track of
**	    counters selected for charting.
**	29-oct-1999 (somsa01)
**	    Added the global variable PersonalCounterID for keeping track
**	    of the counter id form which the personal stuff starts.
**	06-nov-2001 (somsa01)
**	    Added definition of SPLASH_DURATION.
**	07-nov-2001 (somsa01)
**	    Changed SPLASH_DURATION to 3000.
**	30-May-2008 (whiro01) Bug 119642, 115719; SD issue 122646
**	    Put common declaration of our main registry key value (PERFCTRS_KEY) in perfwiz.cpp.
**	03-Nov-2008 (drivi01)
**	    Replace Windows help with HTML help.
*/

#if !defined(AFX_PERFWIZ_H__C6B114D9_5BEB_11D3_B867_AA000400CF10__INCLUDED_)
#define AFX_PERFWIZ_H__C6B114D9_5BEB_11D3_B867_AA000400CF10__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/* offsets for first and other columns, used for list box custom drawing */
#define OFFSET_FIRST	2
#define OFFSET_OTHER	6

/* The diferent counter classes */
#define CacheClass	0
#define LockClass	1
#define ThreadClass	2
#define SamplerClass	3
#define PersonalClass	4

/* The number of groups per counter class */
#define NUM_CACHE_GROUPS    6
#define NUM_LOCK_GROUPS	    1
#define NUM_THREAD_GROUPS   27
#define NUM_SAMPLER_GROUPS  1
#define Num_Groups	    (NUM_CACHE_GROUPS + NUM_LOCK_GROUPS + \
			    NUM_THREAD_GROUPS + NUM_SAMPLER_GROUPS)

/* The number of counters per group */
#define Num_Cache_Counters	21
#define Num_Lock_Counters	2
#define Num_Thread_Counters	28
#define Num_Sampler_Counters	7

/* The number of personal counters */
#define NUM_PERSONAL_COUNTERS		((NUM_CACHE_GROUPS*Num_Cache_Counters)+\
					 (NUM_LOCK_GROUPS*Num_Lock_Counters)+\
					 (NUM_THREAD_GROUPS*Num_Thread_Counters)+\
					 (NUM_SAMPLER_GROUPS*Num_Sampler_Counters))

/* The first personal #define id */
#define FIRST_PERSONAL_DEF_ID		((NUM_PERSONAL_COUNTERS+NUM_CACHE_GROUPS+\
					  NUM_LOCK_GROUPS+NUM_THREAD_GROUPS+\
					  NUM_SAMPLER_GROUPS)*2)

#define SPLASH_DURATION (3000)

typedef struct
{
    char    *Name;
    char    *Help;
    int	    CounterID;
    bool    UserSelected;
} Group;

typedef struct
{
    char    *Name;
    char    *Help;
    int	    classid;
    int	    CounterID;
} GroupClass;

typedef struct
{
    char	*CounterName;
    char	*PCounterName;
    char	*CounterHelp;
    char	*UserName;
    char	*UserHelp;
    char	*DefineName;
    int		classid, groupid, ctrid, groupidx;
    int		CounterID;
    bool	selected, UserSelected;
} PERFCTR;

struct PersCtrList
{
    PERFCTR		PersCtr;
    char		*dbname;
    char		*qry;
    int			scale;
    struct PersCtrList	*next;
};

struct grouplist
{
    char		*GroupName;
    char		*GroupDescription;
    PERFCTR		CounterList[NUM_PERSONAL_COUNTERS];
    struct PersCtrList	*PersCtrs;
    int			NumCtrsSelected;
    int			CounterID;
    struct grouplist	*next;
};

extern HPALETTE		hSystemPalette;
extern HWND		MainWnd;
extern CString		ChosenVnode, ChosenServer, ChartName;
extern CString		ObjectName, ObjectDescription;
extern BOOL		PersonalGroup, InitFileRead, UseExisting;
extern int		CounterID, PersonalCounterID;
extern DWORD		NumCountersSelected;
extern struct grouplist	*GroupList;

extern GroupClass	GroupNames[Num_Groups];
extern Group		CacheGroup[NUM_CACHE_GROUPS*Num_Cache_Counters];
extern Group		LockGroup[NUM_LOCK_GROUPS*Num_Lock_Counters];
extern Group		ThreadGroup[NUM_THREAD_GROUPS*Num_Thread_Counters];
extern Group		SamplerGroup[NUM_SAMPLER_GROUPS*Num_Sampler_Counters];
extern PERFCTR		PersonalCtr_Init[NUM_PERSONAL_COUNTERS];

extern void		FreePersonalCounters(struct grouplist *glptr);

extern const char * const PERFCTRS_KEY;

/*
** CPerfwizApp:
** See perfwiz.cpp for the implementation of this class
*/
class CPerfwizApp : public CWinApp
{
public:
    CPerfwizApp();

/* Overrides */
    /* ClassWizard generated virtual function overrides */
    //{{AFX_VIRTUAL(CPerfwizApp)
    public:
    virtual BOOL InitInstance();
	virtual VOID OnHelp();
    //}}AFX_VIRTUAL

/* Implementation */
    //{{AFX_MSG(CPerfwizApp)
	    // NOTE - the ClassWizard will add and remove member functions here.
	    //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PERFWIZ_H__C6B114D9_5BEB_11D3_B867_AA000400CF10__INCLUDED_)
