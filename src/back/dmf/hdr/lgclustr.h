/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: LGCLUSTR.H - Definitions of Cluster LG types and constants.
**
** Description:
**
** History: 
**	15-may-1993 (rmuth)
**	    Created
**	21-jun-1993 (bryanp)
**	    Added RCP_NODE_RUNNING_LOCK.
**	    Note that the CSP_NODE_RUNNING_LOCK has the installation code
**		appended to it.
**	26-jul-1993 (bryanp)
**	    Added LGC_MAX_NODE_NAME_LEN.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-apr-2001 (devjo01)
**	    S103715: Replace LGcluster, LGCn_name, and LG_cluster_node with 
**	    macro wrappers around CX eqivalents.
**	10-Dec-2008 (jonj)
**	    SIR 120874: Remove last vestiges of CL_SYS_ERR
**/

# ifndef CX_H_INCLUDED
# include <cx.h>
# endif /* CX_H_INCLUDED */

#define CSP_NODE_RUNNING_LOCK 	"II_CSP_RUNNING_"
#define RCP_NODE_RUNNING_LOCK 	"II_RCP_RUNNING_"

/*
** A randomly-picked value for the maximum length of a node name in a
** VAXClustr, used for dimensioning character arrays to hold names:
*/
#define LGC_MAX_NODE_NAME_LEN	50

/*
** Prototypes
*/

/*
** s103715 note: Following were replaced by CX equivalents:
*/
# define LGcluster()	CXcluster_enabled()
# define LG_cluster_node(node_name) CXnode_number(node_name)
# define LGCn_name(node_name) (VOID)CXnode_name(node_name)

FUNC_EXTERN i4		LGcnode_info(
			    char                *name,
			    i4                  l_name,
			    CL_ERR_DESC         *sys_err);

FUNC_EXTERN STATUS	LGCnode_names(
			    i4         node_mask,
			    char            *node_names,
			    i4         length,
			    CL_ERR_DESC     *sys_err);

FUNC_EXTERN STATUS	LG_get_log_page_size(
			    char *path,
			    char *file,
			    i4 *page_size,
			    CL_ERR_DESC *sys_err);


FUNC_EXTERN STATUS	LGc_csp_online( char    *nodename);

FUNC_EXTERN STATUS	LGc_any_csp_online(VOID);


