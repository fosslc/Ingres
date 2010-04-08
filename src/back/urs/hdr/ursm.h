/*
** Copyright (c) 2004 Ingres Corporation
**
*/

/**
**
**  Name: URSM.H - User Request Services Manager header
**
**  Description:
**      This file contains definitions and prototypes used internally by the
**      User Request Services Manager of the Frontier Application Server.
** 
** 	An instance of a Frontier Application Server is defined by
** 	parameters in the config.dat file. While executing, its information
** 	is kept in the FAS_SRVR control block. The application server
** 	starts one or more applications, each of which is a loadable unit, 
** 	e.g., a DLL in Win/NT. Within an application, there may be multiple
** 	interfaces (in OpenRoad, image files). An interface is composed of 
** 	multiple methods (functions), each with its own parameters (arguments).
** 
**    +-Frontier Server (FAS_SRVR)--------+
**    |                                   |
**    |   .fas_srvr_appl                  |
**    +---|-------------------------------+
**        v
**      +-Application (FAS_APPL)----------------+
**      |+-Application (FAS_APPL)----------------+
**      ||+-Application (FAS_APPL)----------------+
**      |||  fas_appl_name, e.g., "orntier.dll"   |
**      +||  ...                                  |
**       +| *fas_appl_intfc                       |
**        +-|-------------------------------------+
**          v
**        +-Interface (FAS_INTFC)-----------------+
**        |+-Interface (FAS_INTFC)-----------------+
**        ||+-Interface (FAS_INTFC)-----------------+
**        |||  fas_intfc_name, e.g., "query.img"    |
**        +||  ...                                  |
**         +| *fas_intfc_method                     |
**          +-|-------------------------------------+
**            v
**          +-Method/Function (FAS_METHOD)----------+
**          |+-Method/Function (FAS_METHOD)----------+
**          ||+-Method/Function (FAS_METHOD)----------+
**          |||  fas_method_name, e.g., "runquery"    |
**          +||  ...                                  |
**           +| *fas_method_arg                       |
**            +-|-------------------------------------+
**              v
**            +-Parameters/Arguments (FAS_ARG)--------+
**            |+-Parameters/Arguments (FAS_ARG)--------+
**            ||+-Parameters/Arguments (FAS_ARG)--------+
**            +||                                       |
**             +|                                       |
**              +---------------------------------------+
**
**  History:
**      11-Nov-1997 wonst02
**          Original User Request Services Manager.
** 	15-Dec-1997 wonst02
** 	    Add structures for the URS applications.
** 	17-Jan-1998 wonst02
** 	    Add URS_ExecuteMethod - Execute the application method.
** 	    Add urs_load_dll() for OpenRoad.
**	05-Mar-1998 wonst02
** 	    Add urs_get_type - Get a parameter from the parameter list.
** 	18-Mar-1998 wonst02
** 	    Add info for data dictionary services. Move to back/urs/hdr.
** 	    Add the missing "Interface" layer.
**	11-Aug-1998 (fanra01)
**	    Removed type redefinitions and changed prototype for urs_error.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-jul-2001 (toumi01)
**	    problem fixed for i64_aix port:
**	    protect MIN/MAX with ifdefs to avoid duplicate definition
**	7-oct-2004 (thaju02)
**	    Define memleft as SIZE_TYPE.
**/
#ifndef URSM_H
#define URSM_H
#include <cscl.h>
#include <urs.h>
#include <erurs.h>


/*
** Global and static variables
*/

/*
** 	Macro definitions
*/
#ifndef MAX
#define MAX(A, B) (((A) > (B)) ? (A) : (B))
#endif /* MAX */
#ifndef MIN
#define MIN(A, B) (((A) < (B)) ? (A) : (B))
#endif /* MIN */
#define SET_URSB_ERROR(ursb, err, status)				\
	{								\
	    if (status > ursb->ursb_error.err_data)			\
	    {								\
	    	ursb->ursb_error.err_code = err;			\
	    	ursb->ursb_error.err_data = status;			\
	    }								\
	}

/*
** 	Common DEFINES and TYPEDEFS
*/
#define	FAS_SRVR_NAME	DB_NAME
#define	FAS_APPL_NAME	DB_NAME
#define	FAS_INTFC_NAME	DB_NAME
#define	FAS_METHOD_NAME	DB_NAME
#define	FAS_ARG_NAME	DB_NAME

#define URS_APPMEM_DEFAULT      32768
#define	APPL_PARM_LENGTH	512
#define	INTFC_PARM_LENGTH	512
#define INGRES_BIN_DIR	"%s\\ingres\\bin"
#define INGRES_W4GL_DIR "%s\\ingres\\w4glapps"

/*
** 	Values for fas_srvr_flags, fas_appl_flags, fas_intfc_flags, ...
*/
#define	FAS_STARTED	0x0001

#define FAS_DISABLED	0x0010

#define	FAS_SEARCH	0x0100

/* 
** Config.dat parameters. Note that these require that the host name
** is substituted (using STprintf) for %s. 
*/
#define FAS_CONF_REPOSITORY	ERx("ii.%s.frontier.dictionary.name")
#define FAS_CONF_DLL_NAME	ERx("ii.%s.frontier.dictionary.driver")
#define FAS_CONF_NODE		ERx("ii.%s.frontier.dictionary.node")
#define FAS_CONF_CLASS		ERx("ii.%s.frontier.dictionary.class")

/*
** Objects used in the repository (data dictionary)
*/
#define	FAS_OBJ_SRVR		1
#define	FAS_OBJ_APPL		2
#define FAS_OBJ_INTFC		3
#define	FAS_OBJ_METHOD		4
#define	FAS_OBJ_ARG		5

/*
** typedefs for structures
*/
typedef struct _FAS_SRVR		FAS_SRVR;
typedef struct _FAS_APPL		FAS_APPL;
typedef struct _FAS_INTFC 		FAS_INTFC;
typedef struct _FAS_METHOD 		FAS_METHOD;
typedef struct _FAS_ARG 		FAS_ARG;

/******************************************************************************
**
** Name: URS_MGR_CB - The User Request Services Manager control block
**
** Description:
** 	This is the main control block used internally by the URSM to maintain
** 	information about the state of User Request Services.
** 
******************************************************************************/

typedef struct _URS_MGR_CB
{
	ULM_RCB		ursm_ulm_rcb;
	SIZE_TYPE	ursm_memleft;
	i4		ursm_flags;
	i4		ursm_srvr_id;
	PTR		ursm_ddg_descr;
	CS_SEMAPHORE	ursm_ddg_sem;  /* One-at-a-time dictionary use */	
	PTR		ursm_ddg_session; /* Data dictionary session */
	i4		ursm_ddg_connect;  /* Zero means not connected */ 
	FAS_SRVR	*ursm_srvr;	/* Ptr to Frontier Appl Srvr block */
} URS_MGR_CB;

/******************************************************************************
**
** Name: The Frontier Application Server (FAS) control blocks
**
** Description:
** 	These are the control blocks used internally by FAS to maintain
** 	information about the application server, each application, and each
** 	method in the application with its arguments.
**
******************************************************************************/

/*
** Application Server block - one for the Application Server
*/
struct _FAS_SRVR
{
	ULM_RCB		fas_srvr_ulm_rcb;  	/* ULM request block */
	SIZE_TYPE	fas_srvr_memleft;
	i4		fas_srvr_id;
	i4		fas_srvr_flags;
	CS_SEMAPHORE	fas_srvr_applsem;  	/* Protects the appl list */	
	FAS_APPL	*fas_srvr_appl;  	/* Ptr to first application */
	FAS_SRVR_NAME	fas_srvr_name;
};

/*
** Application Block - one per defined application
*/
struct _FAS_APPL
{
	ULM_RCB		fas_appl_ulm_rcb;  	/* ULM request block */
	SIZE_TYPE	fas_appl_memleft;
	i4		fas_appl_id;
	i4		fas_appl_flags;
	FAS_APPL       *fas_appl_next;		/* Ptr to next application */
	FAS_SRVR       *fas_appl_srvr; 		/* Ptr to parent appl server */
	CS_SEMAPHORE	fas_appl_intfcsem;  	/* Protects the intfc list */	
	FAS_INTFC      *fas_appl_intfc;		/* Ptr to first interface */
	FAS_APPL_NAME	fas_appl_name;		/* DLL name */
	LOCATION	fas_appl_loc;   	/* Path to DLL */
	PTR		fas_appl_handle;	/* Handle of DLL */
	PTR		fas_appl_addr[4];	/* Entry point addresses */
#define			AS_INITIATE		0
#define 		AS_EXECUTEMETHOD	1
#define 		AS_TERMINATE		2
#define			AS_MEFREE		3
#define 		AS_NULL			4
	i4		fas_appl_type;
#define			FAS_APPL_NONE		0
#define			FAS_APPL_OPENROAD	1	/* OpenRoad */
#define			FAS_APPL_CPP		2	/* C or C++ */
	char		*fas_appl_parm;
};

/*
** Interface block - one per interface (image) in the application
*/
struct _FAS_INTFC
{
	i4		fas_intfc_id;
	i4		fas_intfc_flags;
	FAS_INTFC      *fas_intfc_next;		/* Ptr to next interface */
	FAS_APPL       *fas_intfc_appl;		/* Ptr to parent application */
	i4		fas_intfc_appl_id;  	/* ID of parent appl */	
	FAS_METHOD     *fas_intfc_method;	/* Ptr to 1st method */
	FAS_INTFC_NAME	fas_intfc_name;		/* Interface name */
	LOCATION	fas_intfc_loc;  	/* Path to interface */
	char	       *fas_intfc_parm;
};

/*
** Method block - one per method (function) in the interface
*/
struct _FAS_METHOD
{
	i4		fas_method_id;
	i4		fas_method_flags;
	FAS_METHOD     *fas_method_next;	/* Ptr to next method */
	FAS_INTFC      *fas_method_intfc;	/* Ptr to parent interface */
	i4		fas_method_intfc_id;    /* ID of parent interface */	
	i4		fas_method_appl_id;  	/* ID of grandparent appl */
	FAS_ARG	       *fas_method_arg;		/* Ptr to first argument */
	FAS_METHOD_NAME	fas_method_name;
	PTR		fas_method_addr;  	/* Addr of method */	
	i4		fas_method_argcount;
	i4		fas_method_inargcount;
	i4		fas_method_outargcount;
};

/*
** Argument (Parameter) block - one per argument in the function (method)
*/
struct _FAS_ARG
{
	i4		fas_arg_id;
	i4		fas_arg_flags;
	FAS_ARG	       *fas_arg_next;	/* Ptr to next argument */
	FAS_METHOD     *fas_arg_method;	/* Ptr to parent method */
	i4		fas_arg_method_id;  /* ID of parent method */ 
	i4		fas_arg_intfc_id;  /* ID of grandparent interface */ 
	i4		fas_arg_appl_id;  /* ID of ancestor application */ 
	FAS_ARG_NAME	fas_arg_name;
	i4		fas_arg_type;
#define			FAS_ARG_NONE	0
#define			FAS_ARG_IN	1
#define			FAS_ARG_OUT	2
#define			FAS_ARG_INOUT	3
	DB_DATA_VALUE	fas_arg_data;
};

/******************************************************************************
**
**  User Request Services error codes
**
**
**  Error handling scheme is as follows:
**
**	There are a number of error categories:
**
**	informational		--  nothing logged; info passed to caller
**	internal		--  internally detected fatal errors
**	user errors		--  detailed message sent to user
**
**	All internal errors are logged by URS.
**
******************************************************************************/
#define	URS_USERERR	1
#define	URS_INTERR	2
#define	URS_CALLERR	3


/******************************************************************************
**
**	Function Prototypes
**
******************************************************************************/

/*
** 	In ursinit.c
*/
	
DB_STATUS
urs_startup(URSB       	*ursb);

DB_STATUS
urs_shutdown(URSB	*ursb);

/*
** 	In ursm.c
*/
/* VARARGS */
VOID
urs_error();

PTR
urs_palloc(ULM_RCB	*ulm,
	   URSB		*ursb,
	   i4		size);

/*
** 	Function prototypes in ursappl.c
*/

FAS_APPL *
urs_get_appl(URS_MGR_CB	*ursm,
	     i4		appl_id);
DB_STATUS
urs_start_appl(URS_MGR_CB	*ursm,
	       FAS_APPL		*appl);
DB_STATUS
urs_shut_appl(URS_MGR_CB	*ursm,
	      FAS_APPL		*appl,
	      URSB		*ursb);
DB_STATUS
urs_exec_method(URS_MGR_CB	*ursm,
	    	URSB		*ursb);
DB_STATUS
urs_begin_parm(URS_MGR_CB	*ursm,
	       URSB		*ursb);
DB_STATUS
urs_add_parm(URSB		*ursb);

DB_STATUS
urs_release_parm(URS_MGR_CB	*ursm,
	         URSB		*ursb);
DB_STATUS
urs_get_type(URSB		*ursb);

/*
** 	Function prototypes in ursintfc.c
*/

FAS_INTFC *
urs_find_intfc(URS_MGR_CB	*ursm,
	       i4		appl_id,
	       i4		intfc_id);
FAS_METHOD *
urs_find_method(URS_MGR_CB	*ursm,
	        i4		appl_id,
	        i4		intfc_id,	       
	        i4		meth_id);
DB_STATUS
urs_load_dll(URS_MGR_CB		*ursm,
	     FAS_APPL		*appl,
	     URSB		*ursb);
DB_STATUS
urs_free_dll(URS_MGR_CB		*ursm,
	     FAS_APPL		*appl,
	     URSB		*ursb);
DB_STATUS
urs_start_intfc(URS_MGR_CB	*ursm,
		FAS_INTFC	*intfc);
DB_STATUS
urs_shut_intfc(URS_MGR_CB	*ursm,
	       FAS_INTFC	*intfc,
	       URSB		*ursb);
DB_STATUS
urs_refresh_intfc(URS_MGR_CB	*ursm,
		  URSB		*ursb);
DB_STATUS
urs_get_intfc(URS_MGR_CB	*ursm,
	      URSB		*ursb);
DB_STATUS
urs_get_method(URS_MGR_CB	*ursm,
	       URSB		*ursb);
DB_STATUS
urs_get_arg(URS_MGR_CB	*ursm,
	    URSB	*ursb);

/*
** 	Function prototype in ursmethod.c
*/
int
urs_call_method(URS_MGR_CB	*ursm,
		URSB		*ursb);

/*
** 	Function prototype in ursrepos.c
*/
DB_STATUS
urs_readrepository(URS_MGR_CB	*ursm,
		   URSB		*ursb);

/*
** 	Function prototypes in urdserv.c
*/
DB_STATUS
urd_ddg_init(URS_MGR_CB *ursm,
	     URSB	*ursb);
DB_STATUS
urd_ddg_term(URS_MGR_CB *ursm,
	     URSB	*ursb);
DB_STATUS
urd_get_appl(URS_MGR_CB	*ursm,
	     FAS_APPL	*appl,
	     URSB	*ursb);
DB_STATUS
urd_get_intfc(URS_MGR_CB	*ursm,
	      FAS_INTFC		*intfc,
	      URSB		*ursb);
DB_STATUS
urd_get_method(URS_MGR_CB	*ursm,
	       FAS_METHOD	*method,
	       URSB		*ursb);
DB_STATUS
urd_get_arg(URS_MGR_CB	*ursm,
	    FAS_ARG	*arg,
	    URSB	*ursb);
DB_STATUS
urd_connect(URS_MGR_CB	*ursm,
	    URSB	*ursb);
DB_STATUS
urd_disconnect(URS_MGR_CB	*ursm);

#endif
