/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: GCN.H - The interface to GCF Name Service
**
** Description:
**
**      This file contains the definitions necessary for Name Server users 
**      to access the GCN service interface.
**
**
** History: $Log-for RCS$
**      02-Sep-87 (lin)
**          Created.
**      08-Sep-87 (berrow)
**          Moved message type definitions fron GCA.H
**	    Added some section comments in style consistent with GCA.H
**      25-Sept-87 (jbowers)
**          Added names to "typedef struct" statements, to allow mapping by
**          the debugger.
**	09-Nov-88  (jorge)
**	    Added a formal definition of the GCN_SHARED_TABLE keyword that
**	    defines a table as shared in a file system cluster environment.
**	01-Mar-89 (seiwald)
**	    Name Server (IIGCN) revamped.  Message formats between
**	    IIGCN and its clients remain the same, but the messages are
**	    constructed and extracted via properly aligning code.
**	    Variable length structures eliminated.  Strings are now
**	    null-terminated (they had to before, but no one admitted so).
**	02-Apr-89 (jorge)
**	    Put back 'C' structure definitions since they served as pseudo
**	    code definition of the GCN byte packed (un-alligned) messages)
[@history_template@]...
**/


/*
**  GCN message types
**
**  The essential service provided by GCN is the transfer of messages
**  between cooperating application programs (GCN users). A message is
**  characterized by a message type designation and (usually) a data
**  object. The collection of GCN message types and data objects is known
**  by and is semantically meaningful to GCN users. The GCN service
**  interface provides the mechanism for transferring message type
**  indications and message data across the GCN/user interface.
**
**  There are reserved ranges of message type numbers
**
**	0x00 ... 0x01	Internal
**	0x02 ... 0x3F	GCA messages
**	0x40 ... 0x4F	GCN messages
**	0x50 ... 0x5F	GCC messages
**	0x60 ... 0xFF	Unassigned
**	
**  Only GCN messages are of concern to the GCN user.
**
**  The following is a complete list of the possible values of the
**  "gcn_message_type" structure element.
**
*/

/* Ask Name Server to resolve an external name */
# define		    GCN_NS_RESOLVE	0x40

/* The resolved information returned by the Name Server */
# define		    GCN_RESOLVED	0x41

/* Ask the Name Server to operate on the Name Database */
# define		    GCN_NS_OPER		0x42

/* Result returned by the Name Server for GCA_NS_OPER */
# define		    GCN_RESULT		0x43

/* Define the Max. length of each field in the Name Database Record */
# define    GCN_OBJ_MAX_LEN	256
# define    GCN_TYP_MAX_LEN	32
# define    GCN_VAL_MAX_LEN	64
# define    GCN_EXTNAME_MAX_LEN 256
# define    GCN_LIS_MAX_LEN	200 /* Not Used, GCN_VAL_MAX_LEN is used */
# define    GCN_UID_MAX_LEN	32
# define    GCN_QUE_MAX		30
# define    GCN_SVR_MAX		20

# define    MAXLINE 80
# define    NETWORK_ENTRY   (i4)0x0001
# define    SERVER_ENTRY    (i4)0x0002
# define    USER_ENTRY	    (i4)0x0003

/*
** Define comparison mask
*/
# define    GCN_UID_MASK    (u_i4) 0x0001
# define    GCN_OBJ_MASK    (u_i4) 0x0002
# define    GCN_VALUE_MASK  (u_i4) 0x0004


/*
** Define the Server Class
*/

/* INGRES Name Service  table types */

# define    GCN_INGRES		(char *)"INGRES"    /* INGRES DBMS server */
# define    GCN_COMSVR		(char *)"COMSVR"    /* Communications server */
# define    GCN_NMSVR		(char *)"IINMSVR"   /* Name server */
# define    GCN_DBASE4		(char *)"DBASE4"    /* DBASE4 server */
# define    GCN_DB2		(char *)"DB2"	    /* DB2 server */
# define    GCN_DG		(char *)"DG"	    /* DG server */
# define    GCN_IDMR		(char *)"IDMSR"	    /* IDMSR server */
# define    GCN_IDMX		(char *)"IDMSX"	    /* IDMSX server */
# define    GCN_IMS		(char *)"IMS"	    /* IMS server */
# define    GCN_INFORMIX	(char *)"INFORMIX"  /* INFORMIX server */
# define    GCN_IUSVR		(char *)"IUSVR"	    /* INGRES utility server */
# define    GCN_ORACLE		(char *)"ORACLE"    /* ORACLE server */
# define    GCN_RDB		(char *)"RDB"	    /* RDB server */
# define    GCN_RMS		(char *)"RMS"	    /* RMS server */
# define    GCN_SQLDS		(char *)"SQLDS"	    /* SQLDS server */
# define    GCN_STAR		(char *)"STAR"	    /* STAR server */
#define     GCN_STSYN		(char *)"D"	    /* special STAR synonym */
# define    GCN_SYBASE		(char *)"SYBASE"    /* SYBASE server */
# define    GCN_TERADATA	(char *)"TERADATA"  /* TERADATA server */
# define    GCN_VSAM		(char *)"VSAM"	    /* VSAM server */

# define    GCN_PROTOCOL	(char *)"PROTOCOL"
# define    GCN_NODE		(char *)"NODE"
# define    GCN_USER		(char *)"USER"
# define    GCN_LOGIN		(char *)"LOGIN"
/*
** Define corresponding code for the above type
*/
# define	GCN_IIINGRES	1
# define	GCN_IICOMSVR	2
# define	GCN_IINMSVR	3
# define    	GCN_IIDBASE4	8
# define    	GCN_IIDB2	9
# define    	GCN_IIDG	10
# define    	GCN_IIIDMR	11
# define    	GCN_IIIDMX	12
# define    	GCN_IIIMS	13
# define    	GCN_IIINFORMIX	14
# define    	GCN_IIIUSVR	15
# define    	GCN_IIORACLE	16
# define    	GCN_IIRDB	17
# define    	GCN_IIRMS	18
# define    	GCN_IISQLDS	19
# define    	GCN_IISTAR	20
# define    	GCN_IISYBASE	21
# define    	GCN_IITERADATA	22
# define    	GCN_IIVSAM	23

# define	GCN_IIPROTOCOL	4 
# define	GCN_IINODE	5
# define	GCN_IIUSER	6
# define	GCN_IILOGIN	7
/*
** Define the keyword for Shared tables (ie. NFS or VAX Cluster)
*/
# define	GCN_SHARED_TABLE  (char *)"GLOBAL"


/*
**  GCN data objects
**
**  GCN is aware of and deals with a specific set of complex data objects
**  which pass across the GCN service interface as data associated with
**  various message types.  This is the set of objects which are meaningful
**  to the application users of GCN.  Following is the complete set.
*/

/*
** GCN_TUP - GCN_VAL pointers to the data in a name server tuple.
**	Use gcn_get_tup() and gcn_put_tup() for manipulation.
*/

typedef struct _GCN_TUP
{
	char	*uid;
	char 	*obj;
	char 	*val;
} GCN_TUP;

/*
** GCN_VAL - ????????
*/    
typedef	struct _GCN_VAL
{
    i4		gcn_l_item;
    char	gcn_value[1];
}  GCN_VAL;

/*
** GCN_TUPLE - This is the structure of each tuple in memory.
*/    
typedef struct _GCN_TUPLE
{
    GCN_VAL	gcn_uid;
    GCN_VAL	gcn_obj;
    GCN_VAL	gcn_val;
}   GCN_TUPLE;

/*
** GCN_TUPLE - This is the structure of each tuple  from client.
*/    
typedef struct _GCN_REQ_TUPLE
{
    GCN_VAL	gcn_type;
    GCN_VAL	gcn_uid;
    GCN_VAL	gcn_obj;
    GCN_VAL	gcn_val;
}   GCN_REQ_TUPLE;


/*
** GCN_TUP_QUE - This is the structure of each tuple in the memory queue.
*/    
typedef struct _GCN_TUP_QUE
{
    QUEUE	gcn_q;
    i4		gcn_rec_no;
    char	gcn_q_tup[1];	/* var length byte packed data goes here.
				** This is a var length version of GCN_TUPLE.
				*/
} GCN_TUP_QUE;

/*
** GCN_QUE_NAME - This is the structure containing the service classes
**		   and their QUEUE pointers.
*/
typedef struct _GCN_QUE_NAME
{
    char	gcn_type[GCN_TYP_MAX_LEN];
    char	gcn_str_file[GCN_TYP_MAX_LEN+GCN_OBJ_MAX_LEN+2];
    FILE	*gcn_file;
    i4		gcn_ty_code;
    QUEUE	gcn_que_head;
    bool	gcn_cache;
} GCN_QUE_NAME;
/*
** GCN_DB_RECORD - This is the record stored in the Name Database
*/
typedef struct _GCN_DB_RECORD
{
    i4		gcn_rec_no;
    i4		gcn_l_uid;
    char	gcn_uid[GCN_UID_MAX_LEN];
    i4		gcn_l_obj;
    char	gcn_obj[GCN_OBJ_MAX_LEN];
    i4		gcn_l_val;
    char	gcn_val[GCN_VAL_MAX_LEN];
    bool	gcn_temp;
    bool	gcn_deleted;
}   GCN_DB_RECORD;

/*
** GCN_NM_DATA - This may be a variable length data block containing the
**		 tuple counts and tuple data. This structure also describes
**		 the data format of the message type GCA_NS_RESULT
*/
typedef struct _GCN_NM_DATA
{
    i4		gcn_tup_cnt;
    GCN_REQ_TUPLE 	gcn_tuple[1];
} GCN_NM_DATA;


/*
** This data structure represents the data format for message type
** GCN_NS_OPER
*/
typedef struct _GCN_D_OPER
{
    i4		gcn_flag;	/* Default is PRIVATE|SHARE */

# define	GCN_DEF_FLAG	(i4) 0x0000
# define	GCN_PUB_FLAG	(i4) 0x0001
# define	GCN_XCL_FLAG	(i4) 0x0002
# define	GCN_SOL_FLAG	(i4) 0x0004	/* Sole server indication  */

    i4		gcn_opcode;
    GCN_VAL	gcn_install;
    GCN_NM_DATA	gcn_data;
} GCN_D_OPER;

/*
** This data structure represents the data format for messag type
** GCN_RESULT
*/
typedef struct _GCN_OPER_DATA
{
    i4		gcn_op;
    i4		gcn_tup_cnt;
    GCN_REQ_TUPLE 	gcn_tuple[1];
} GCN_OPER_DATA;



/*
** This data structure represents the data format for message type
** GCN_NS_RESOLVE
*/
typedef struct _GCN_D_NS_RESOLVE
{
    GCN_VAL	gcn_install;
    GCN_VAL	gcn_uid;       /* this is local user id */
    GCN_VAL	gcn_ext_name;
    GCN_VAL	gcn_user;
    GCN_VAL	gcn_passwd;    
} GCN_D_NS_RESOLVE;

/*
** GCN_SVR_ID - The data area of server id
*/
typedef struct _GCN_SVR_ID
{
    i4		gcn_count;
    GCN_VAL	gcn_svr_id[1];
} GCN_SVR_ID;

/*
** This data structure represents the data format for message type
** GCN_RESOLVED
*/
typedef struct _GCN_D_RESOLVED
{
    GCN_VAL	    gcn_node;
    GCN_VAL	    gcn_protocol;
    GCN_VAL	    gcn_port;
    GCN_VAL	    gcn_username;
    GCN_VAL	    gcn_password;
    GCN_VAL	    gcn_rmt_dbname;
    GCN_VAL	    gcn_server_class;
   GCN_SVR_ID	    gcn_server_id;
} GCN_D_RESOLVED;

/* 
** GCN Service Codes and Parameter List structures.
**
** Each GCN service has a service code and an associated parameter list.
** The service code values and parameter list structures are as follows.
*/          

/* Add an entry or entries to the Name Database */
# define	    GCN_ADD 		(i4) 0x0001

/* Delete an entry or entries from the Name Database */
# define	    GCN_DEL		(i4) 0x0002

/* Retrieve an entry or entries from the Name Database */
# define	    GCN_RET		(i4) 0x0003

/* modify an entry or entries in the Name Database */
# define	    GCN_MOD		(i4) 0x0004


/* initiate the  Name Database */
# define	    GCN_INITIATE 	(i4) 0x0005

/* shutdown the name server */
# define	    GCN_SHUTDOWN	(i4) 0x0006

/* define name server information */
# define	    GCN_SECURITY	(i4) 0x0007

/*
** The data structure of parameter for calling IIGCn_call
*/
typedef struct _GCN_CALL_PARMS
{
    STATUS	gcn_status;
    char	(*async_id)();
    i4		gcn_flag;
    PTR		gcn_type;
    PTR		gcn_uid;
    PTR		gcn_obj;
    PTR		gcn_value;
    PTR		gcn_install;
    GCN_NM_DATA *gcn_buffer;		
} GCN_CALL_PARMS;

/*
** The data structure containing INGRES super user queue.
*/
typedef struct _GCN_SUPER_USER
{
    QUEUE	gcn_q;
    char	gcn_uid[GCN_UID_MAX_LEN];
} GCN_SUPER_USER;



/*
**  Parameter list STATUS values
**
**  There are reserved ranges of STATUS values
**
**	0x0000 ... 0x0FFF	GCA STATUS values
**	0x1000 ... 0x13FF	GCN STATUS values
**	0x2000 ... 0x2FFF	GCC STATUS values
**	
**  The possible returned STATUS values for each GCN service are
**  listed in the GCN specification document.
*/

/*
**	E_GC_MASK	is defined in GCA.H
*/

/* Duplicate initiate request */
# define		E_GCN006_DUP_INIT	(STATUS) (E_GCF_MASK + 0x1006)



/* Define Name Server Exception code */
# define		GCN_JUMP_EX	    (i4) 1

/* The time interval in seconds to do name database cleanup */
# define		GCN_TIME_CLEANUP	(i4) 300
