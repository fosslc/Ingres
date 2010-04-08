/*
** Copyright (c) 2004, 2005 Ingres Corporation
*/

/**
** Name: iff
**
** Description:
**  Structures, types and definitions.
**
** History:
**      18-Feb-2004 (fanra01)
**          Created.
**      24-Mar-2004 (fanra01)
**          Organize function prototypes.
**          Enclose strings in required ERx macro.
**      02-Apr-2004 (hweho01)
**          Defined BOOL to char for Unix platforms.
**      20-Apr-2004 (fanra01)
**          Add session replated API context.
**      29-Apr-2004 (hweho01)
**          Defined BOOL to int for Unix platforms.
**      08-Jul-2004 (noifr01)
**          Add prototype for iff_set_config()
**      20-Jul-2004 (noifr01)
**          - Add prototype for iff_getmdbname()
**          - Updated MAX_IFF_PMVALUE to 256
**      19-Aug-2004 (hweho01)
**          -Star #13608350
**           Make MDB name available as part of the instance/public data,  
**           the retrieval will be performed in IFFGetMdbName().  
**           Remove iff_getmdbname() function prototype.
**      23-Aug-2004 (fanra01)
**          Sir 112892
**          Update comments for documentation purposes.
**      24-Sep-2004 (fanra01)
**          Sir 113152
**          Add prototype for iff_return_status.
**      07-Feb-2005 (fanra01)
**          Sir 113881
**          Merge Windows and UNIX sources.
**      21-Feb-2005 (fanra01)
**          Sir 113975
**          Add MDB info flags.
**          Add iff_get_strings prototype.
**      01-Aug-2005 (fanra01)
**          Bug 114967
**          Add IFF_EX_SEM_ERROR and remove duplicated error value.
**      22-Sep-2005 (fanra01)
**          SIR 115252
**          Add MAX_IFF_MDBINFO.
**/
# ifndef __IFFINT_INCLUDED
# define __IFFINT_INCLUDED
/*
** Defines
*/
# define    IFF_EX_BAD_PARAM    1
# define    IFF_EX_RESOURCE     2
# define    IFF_EX_BAD_STATE    3
# define    IFF_EX_LOG_FAIL     4
# define    IFF_EX_COMMS_ERR    5
# define    IFF_EX_SEM_ERROR    6

# define    IFF_RESERVED_USR    ERx("*")
# define    IFF_RESERVED_PWD    ERx("*")

# define    IFF_TARGET_REGISTRY 1
# define    IFF_TARGET_NMSVR    2
# define    IFF_TARGET_SERVER   3

# define    MAX_IFF_PMNAME      128
# define    MAX_IFF_PMVALUE     256
# define    MAX_IFF_COND        512
# define    MAX_IFF_MDBINFO     50

# define    IFF_MDB_NAME        0x0001
# define    IFF_MDB_INST        0x0002
# define    IFF_MDB_VMAJ        0x0004
# define    IFF_MDB_VMIN        0x0008
# define    IFF_MDB_VREV        0x0010
# define    IFF_MDB_MASK        (IFF_MDB_NAME | IFF_MDB_INST | IFF_MDB_VMAJ | \
                                 IFF_MDB_VMIN | IFF_MDB_VREV)

# define    MDB_ELEMENT        ERx(".MDB.")

#if defined(UNIX)
#define BOOL int
#endif

/*
** Macros
*/
# ifdef NT_GENERIC
# define    EXCEPT_BEGIN    __try
                            
# define    EXCEPT_END      __except ( IFFExceptFilter( GetExceptionInformation() ) ) \
                            { \
                                ; /* should not get here */ \
                            }
# else
# define    EXCEPT_BEGIN
# define    EXCEPT_END
# endif

/*(
** Name: NET_PORT
**
** Description:
**  Protocol definitions
**
**      protocol    protocol string
**      prot_id     protocol identifier
**      type        protocol type
**
** History:
**      22-Apr-2004 (fanra01)
**          Moved from iffget.
)*/
typedef struct _net_port NET_PORT;
struct _net_port
{
    char*   protocol;
    i4      prot_id;
    i4      type;
};

/*(
** Name: ING_INS_CTX
**
** Description:
**  Context for IFF API.
**
**      state       State of the interface
**      trclvl      Flag determines if tracing has been enabled.
**      msg_buff    message buffer for the session. 
** History:
**      16-Feb-2004 (fanra01)
**          Created.
**      20-Apr-2004 (fanra01)
**          Add API session environment for context.
)*/
typedef struct _ing_ins_ctx ING_INS_CTX;

struct _ing_ins_ctx
{
    i4          state;
# define    IFF_UNINITIALIZED   0
# define    IFF_INITIALIZED     1
# define    IFF_TERMINATED      0
    i4          trclvl;
    char*       msg_buff;
    u_i2        metag;
    PTR         envhndle;
    PTR         conhndle;
    PTR         trnhndle;
    IIAPI_GETEINFOPARM	errParm;
};

/*(
** Name: IFF_INS_DATA
**
** Description:
**  IFF instance data.
**
**      list    List of items returned
**      fields  Count of fields per item
**
** History:
**      01-Mar-2004 (fanra01)
**          Created.
)*/
typedef struct _ing_ins_data ING_INS_DATA;
struct _ing_ins_data
{
    QUEUE   list;
    i4      fields;
};

/*(
** Name: IFF_CNF_DATA
**
** Description:
**  IFF configuration data.
**
**      list    List of items returned
**      data    Count of fields per item
**
** History:
**      01-Mar-2004 (fanra01)
**          Created.
)*/
typedef struct _ing_cnf_data ING_CNF_DATA;
struct _ing_cnf_data
{
    QUEUE   list;
    char*   data[2];
};

# ifdef NT_GENERIC
extern int
IFFExceptFilter( EXCEPTION_POINTERS* ep );
# endif

FUNC_EXTERN STATUS
iff_trace_open( char* path, i4 level, i4 global );

FUNC_EXTERN VOID
iff_trace_close( i4 global );

FUNC_EXTERN VOID
iff_trace( i4 trc, i4 trclvl, char* fmt, ... );

FUNC_EXTERN STATUS
iff_query_config( ING_CTX* ctx, ING_CONNECT* details, II_INT4 num_parms,
	ING_CONFIG* parameters, II_INT4* num_ret_parms, ING_CONFIG** ret_parms);

FUNC_EXTERN STATUS
iff_set_config( ING_CTX* ctx, ING_CONNECT* details, II_INT4 num_parms,
	ING_CONFIG* parameters, II_INT4* num_ret_parms, ING_CONFIG** ret_parms);

FUNC_EXTERN II_INT4
iff_return_status( STATUS status );

FUNC_EXTERN STATUS
iff_get_strings( char* str, char* delim, i4* wc, char**  parray);

# endif /* __IFFINT_INCLUDED */
