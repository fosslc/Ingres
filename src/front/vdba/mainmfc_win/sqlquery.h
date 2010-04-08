#if !defined (SQLQUERY_HEADER)
#define SQLQUERY_HEADER
#include "mainmfc.h"
#include "mainfrm.h"

enum QepType {QEP_NORMAL    = 1, QEP_STAR};
enum QepNode {QEP_NODE_ROOT = 1, QEP_NODE_INTERNAL, QEP_NODE_LEAF};

#define QEP_BASE (WM_USER +5000)
#define WM_QEP_REDRAWLINKS              (QEP_BASE + 1)
#define WM_SQL_STATEMENT_CHANGE         (QEP_BASE + 2)
#define WM_SQL_STATEMENT_EXECUTE        (QEP_BASE + 3)
#define WM_SQL_QUERY_UPADATE_DATA       (QEP_BASE + 4)
#define WM_SQL_QUERY_PAGE_SHOWHEADER    (QEP_BASE + 5)
#define WM_SQL_QUERY_PAGE_HIGHLIGHT     (QEP_BASE + 6)
#define WM_SQL_STATEMENT_SHOWRESULT     (QEP_BASE + 7)
#define WM_SQL_GETPAGE_DATA             (QEP_BASE + 8)
#define WM_SQL_FETCH                    (QEP_BASE + 9)
#define WM_SQL_FETCH_FORWARD            (QEP_BASE +10)
#define WM_SQL_FETCH_BACKWARD           (QEP_BASE +11)
#define WM_SQL_CURSOR_KEYUP             (QEP_BASE +12)
#define WM_SQL_CLOSE_CURSOR             (QEP_BASE +13)
#define WM_SQL_TAB_BITMAP               (QEP_BASE +14)
//
// Result page has the GroupID number = 0 at the create time.
// When the page is load from the serialization, its GroupID will increase by one from
// from the one it was saved.
#define WM_SQL_GETPAGE_GROUPID          (QEP_BASE + 15)
#define WM_QEP_OPEN_POPUPINFO           (QEP_BASE + 16)
#define WM_QEP_CLOSE_POPUPINFO          (QEP_BASE + 17)
#define WM_SQL_GETMAXROWLIMIT           (QEP_BASE + 18)
#define WM_EXECUTE_TASK                 (QEP_BASE + 19)


enum {BM_NEWTAB, BM_SELECT, BM_NON_SELECT, BM_SELECT_CLOSE, BM_SELECT_BROKEN, BM_SELECT_OPEN, BM_QEP, BM_TRACE, BM_XML};

#define __OPING_CURSOR__
#endif



