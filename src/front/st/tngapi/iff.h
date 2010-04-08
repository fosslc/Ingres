/*
** Copyright (c) 2004, 2005 Ingres Corporation
**
**
*/

/**
** Name: iff
**
** Description:
**  Structures, types and definitions.
**(E
** ING_CTX      - maintain context between calls to IFF API
** ING_PORT     - protocol and port information
** ING_ADDR     - server address and class
** ING_CONNECT  - connection attributes
** ING_VERSION  - encoded Ingres version
** ING_CONFIG   - configuration name/value pairs
** ING_INSTANCE - instance information from request
** ING_MDBNAME  - MDB name
** ING_MDBINFO  - MDB information
**)E
**
## History:
##      18-Feb-2004 (fanra01)
##          Created.
##      25-Mar-2004 (fanra01)
##          Add instance id to the instance structure.          
##      08-Apr-2004 (fanra01)
##          Use published types for data structures as internal
##          types will not be available.
##          Updated prototype for IFFgetConfigValues.
##          Add server address structure and made component of
##          ING_CONNECT.
##      08-Jul-2004 (noifr01)
##          Added prototype for iff_set_config(), and defined error codes
##          for too long variable names and values
##      21-Jul-2004 (noifr01)
##          Added prototype for IFFGetMdbName()
##      23-Aug-2004 (fanra01)
##          Sir 112892
##          Updated comments for documentation purposes.          
##      26-Aug-2004 (hweho01)
##          Added structure ING_MDBNAME.
##          Define MDB_DBNAME constant string and MAX_IFF_NUM_INST.
##          Changed the prototype of IFFGetMdbName().
##      24-Sep-2004 (fanra01)
##          Sir 113152
##          Amended copyright notice.
##          Add function pointer types to exposed function.
##          Add error codes.
##      07-Feb-2005 (fanra01)
##          Sir 113881
##          Merge Windows and UNIX source.
##      21-Feb-2005 (fanra01)
##          Sir 113975
##          Add MDBINFO structure and associated IFFGetMdbInfo function
##          prototype.
##      01-Jun-2005 (fanra01)
##          SIR 114614
##          Add system path string to instance structure.
**/
# ifndef __IFF_INCLUDED
# define __IFF_INCLUDED
# include <iiapi.h>


/*
** Defines
*/
# define    IFF_CALL

# define    IFF_TRC_ON          1
# define    IFF_TRC_OFF         2

# define    IFF_TRC_LOG         0
# define    IFF_TRC_ERR         1
# define    IFF_TRC_API         2
# define    IFF_TRC_CALL        3

# define    IFFTRC(c)           ((ING_INS_CTX*)((ING_CTX*)c)->instance_context)->trclvl

# define    MAX_PROTOCOLS       4
# define    MAX_IFF_PORT        32
# define    MAX_IFF_PROT        32
# define    MAX_IFF_HOST        128
# define    MAX_IFF_USER        64
# define    MAX_IFF_PASS        64
# define    MAX_IFF_SVRCLASS    32
# define    MAX_IFF_ID          16
# define    MAX_IFF_ADDR        64
# define    MAX_IFF_VERS        64
# define    MAX_IFF_LANG        48
# define    MAX_IFF_CHAR        16
# define    MAX_IFF_SYSTEM      256
# define    MAX_IFF_NUM_INST    10
# define    MAX_IFF_CNF_STR     10

# define    MDB_MDBNAME        ERx("MDB_DBNAME")
# define    MDB_MDBINFO        ERx("MDB")
# define    MDB_MDBVERS        ERx("VERSION")

#ifdef __cplusplus
extern "C" {
#endif

/*(
** Name: ING_CTX      - maintain context between calls to IFF API
**
** Description:
**  Context structure.  Maintains state between calls to the IFF
**  API.
**(E
** instance_context     Memory area for maintaining context between calls.
** exhandler            Function pointer to a non-default exception handler.
**)E
**
## History:
##      16-Feb-2004 (fanra01)
##          Created.
)*/
typedef struct _ing_ctx ING_CTX;

struct _ing_ctx
{
    II_PTR      instance_context;
    II_PTR      exhandler;
};

/*(
** Name: ING_PORT     - protocol and port information
**
** Description:
**  Port structure.  Identifies the network protocol and the port
**  identifier to be used in a addressing an Ingres instance.
**(E
**      protcol     Identifer for the network protocol
**      type        Data type of the data held in the following port field
**                  This should be the value of the iiapi integer or character
**                  types.
**      port        Port address
**)E
**
## History:
##      17-Feb-2004 (fanra01)
##          Created.
)*/
typedef struct _ing_port ING_PORT;

struct _ing_port
{
    II_INT4     protocol;
# define NOPROT 0
# define TCPIP  1
# define WINTCP 2
# define LANMAN 3
    II_INT4     type;
    II_CHAR     port[MAX_IFF_PORT];
    II_CHAR     id[MAX_IFF_ID];
};


/*(
** Name: ING_ADDR     - server address and class
**
** Description:
**  Server address.
**
## History:
##      19-Apr-2004 (fanra01)
##          Created.
)*/
typedef struct _ing_addr ING_ADDR;

struct _ing_addr
{
    II_CHAR     sclass[MAX_IFF_SVRCLASS];
    II_CHAR     addr[MAX_IFF_ADDR];
};

/*(
** Name: ING_CONNECT  - connection attributes
**
** Description:
**  Connect structure.  used for passing connection attributes into the
**  IFF API.
**
## History:
##      17-Feb-2004 (fanra01)
##          Created.
##      26-Apr-2004 (fanra01)
##          Removed protocol string field.
)*/
typedef struct _ing_connect ING_CONNECT;

struct _ing_connect
{
    II_CHAR     hostname[MAX_IFF_HOST];
    II_CHAR     user[MAX_IFF_USER];
    II_CHAR     password[MAX_IFF_PASS];
    ING_PORT    port;
    ING_ADDR    address;
};

/*(
** Name: ING_VERSION  - encoded Ingres version
**
** Description:
**  Structure to hold encoded Ingres version.
**(E
**      major       Major version number
**      minor       Minor version or point release number
**      genlevel    Generated level
**      bldlevel    Build level
**      byte_type   Compile time character encoding byte type
**      hardware    Hardware platform
**      os          Operating system
**      bldinc      Build increment
**E)
**
## History:
##      17-Feb-2004 (fanra01)
##          Created.
)*/
typedef struct _ing_ver ING_VERSION;

struct _ing_ver
{
    II_INT4        major;
    II_INT4        minor;
    II_INT4        genlevel;
    II_INT4        bldlevel;
    II_INT4        byte_type;
    II_INT4        hardware;
    II_INT4        os;
    II_INT4        bldinc;
};

/*(
** Name: ING_CONFIG   - configuration name/value pairs
**
** Description:
**  Requests and returns parameter values from the query mechanism.
**(E
**      param_name          Name of configuration parameter.
**      param_val_len       Length of value field.
**      param_val           Configuration value.
**)E
**
## History:
##      17-Feb-2004 (fanra01)
##          Created.
)*/
typedef struct _ing_config  ING_CONFIG;

struct _ing_config
{
    II_CHAR*    param_name;
    II_INT4     param_val_len;
    II_CHAR*    param_val;
};

/*(
** Name: ING_INSTANCE - instance information from request
**
** Description:
**  Describes the information returned from a directed request to a name
**  server.
**(E
**      length          Length of instance entry data in octets -
**                      including this length field
**      id              Instance identifier
**      addr            Instance address 
**      port            Port address
**      ver_string      Ingres version string
**      version         Ingres version
**      id              Instance 
**      conformance     SQL-92
**      language        Language setting
**      char_set        Character set
**      system          System and installation path
**      num_srvclass    Number of dbms server classes
**      srvclass        List of server class identifiers
**      num_cnf_params  Number of configuration parameters
**      cnf             List of configuration parameters
**      num_ports       Number of network ports
**      port            List of network ports
**)E
**
## History:
##      17-Feb-2004 (fanra01)
##          Created.
##      25-Mar-2004 (fanra01)
##          Add instance id.
##      01-Jun-2005 (fanra01)
##          Add system path field.
)*/
typedef struct _ing_instance ING_INSTANCE;

struct _ing_instance
{
    II_INT4     length;
    II_CHAR     addr[MAX_IFF_ADDR];
    II_CHAR     ver_string[MAX_IFF_VERS];
    ING_VERSION version;
    II_CHAR     id[MAX_IFF_ID];
    II_INT4     conformance;
    II_CHAR     language[MAX_IFF_LANG];
    II_CHAR     char_set[MAX_IFF_CHAR];
    II_CHAR     system[MAX_IFF_SYSTEM];
    II_INT4     num_srvclass;
    ING_ADDR*   server;
    II_INT4     num_cnf_params;
    ING_CONFIG* cnf;
    II_INT4     num_ports;
    ING_PORT    port;
};


/*(
** Name: ING_MDBNAME
**
** Description:
**      Describes the information on a MDB name 
**      
**(E
**    mdb_name        MDB name found in an installation 
**    id              Instance ID ( II_INSTALLATION )  
**)E
**
## History:
##      26-Aug-2004 ( hweho01 )
##          Created.
)*/
typedef struct _ing_mdbname  ING_MDBNAME;

struct _ing_mdbname
{
    II_CHAR*     mdb_name;
    II_CHAR*     id;
};

/*(
** Name: ING_MDBINFO
**
** Description:
**  Information describing MDB.
**
**(E
**  flags       State maintenance flag
**  name        MDB name found in an instance
**  id          Instance ID ( II_INSTALLATION )
**  version     Components of version number
**  numcnf      Number of items in the cnf array
**  cnf         Array of all name value pairs with ii.host.mdb introducers
**)E
**
## History:
##      21-Feb-2005 (fanra01)
##          Created.
)*/
typedef struct _ing_mdbinfo  ING_MDBINFO;

struct _ing_mdbinfo
{
    II_INT4     flags;
    II_CHAR*    name;
    II_CHAR*    id;
    II_INT4     version[4];
# define IFF_MDB_MAJOR  0
# define IFF_MDB_MINOR  1
# define IFF_MDB_BUILD  2
    II_INT4     numcnf;
    ING_CONFIG* cnf;
};

II_EXTERN II_INT4
IFFinitialize( ING_CTX** ctx );

typedef
II_INT4
(IFF_CALL *EP_IFFINITIALIZE)(
    ING_CTX** ctx
);

II_EXTERN II_INT4
IFFgetInstances(
    ING_CTX* context,
    ING_CONNECT* details,
    II_INT4 buflen,
    ING_INSTANCE* instances,
    II_INT4* num_instances
);

typedef
II_INT4
(IFF_CALL *EP_IFFGETINSTANCES)(
    ING_CTX* context,
    ING_CONNECT* details,
    II_INT4 buflen,
    ING_INSTANCE* instances,
    II_INT4* num_instances
);

II_EXTERN II_INT4
IFFgetConfigValues(
    ING_CTX* ctx,
    ING_CONNECT* details,
    II_INT4 num_parms,
    ING_CONFIG* parameters,
    II_INT4* num_ret_parms,
    ING_CONFIG** ret_parms
);

typedef
II_INT4
(IFF_CALL *EP_IFFGETCONFIGVALUES)(
    ING_CTX* ctx,
    ING_CONNECT* details,
    II_INT4 num_parms,
    ING_CONFIG* parameters,
    II_INT4* num_ret_parms,
    ING_CONFIG** ret_parms
);

II_EXTERN II_INT4
IFFsetConfigValues(
    ING_CTX* ctx,
    ING_CONNECT* details,
    II_INT4 num_parms,
    ING_CONFIG* parameters,
    II_INT4* num_ret_parms,
    ING_CONFIG** ret_parms
);

typedef
II_INT4
(IFF_CALL *EP_IFFSETCONFIGVALUES)(
    ING_CTX* ctx,
    ING_CONNECT* details,
    II_INT4 num_parms,
    ING_CONFIG* parameters,
    II_INT4* num_ret_parms,
    ING_CONFIG** ret_parms
);

II_EXTERN II_INT4
IFFGetMdbName(
    ING_CTX* ctx,
    ING_CONNECT* details,
    ING_MDBNAME** ing_mdbptr,
    II_INT4* mdb_cnt
);

typedef
II_INT4
(IFF_CALL *EP_IFFGETMDBNAME)(
    ING_CTX* ctx,
    ING_CONNECT* details,
    ING_MDBNAME** ing_mdbptr,
    II_INT4* mdb_cnt
);

II_EXTERN II_INT4
IFFGetMdbInfo(
    ING_CTX*        ctx,
    ING_CONNECT*    details,
    ING_MDBINFO**   mdbinfo,
    II_INT4*        infocnt 
);

typedef
II_INT4
(IFF_CALL *EP_IFFGETMDBINFO)(
    ING_CTX*        ctx,
    ING_CONNECT*    details,
    ING_MDBINFO**   mdbinfo,
    II_INT4*        infocnt 
);

II_EXTERN II_INT4
IFFterminate( ING_CTX** ctx );

typedef
II_INT4
(IFF_CALL *EP_IFFTERMINATE)(
    ING_CTX** ctx
);

II_EXTERN II_INT4
IFFtrace( ING_CTX* ctx, II_INT4 enable, II_INT4 level, II_CHAR* path );

typedef
II_INT4
(IFF_CALL *EP_IFFTRACE)(
    ING_CTX* ctx,
    II_INT4 enable,
    II_INT4 level,
    II_CHAR* path
);

/*
** Error codes
*/
# define IFF_SUCCESS            ((II_INT4) 0)   /* command completed */
# define IFF_ERR_FAIL           ((II_INT4)-1)   /* error with no extra info */
# define IFF_ERR_RESOURCE       ((II_INT4)-2)   /* error allocating resource*/
# define IFF_ERR_PARAM          ((II_INT4)-3)   /* invalid paramter */
# define IFF_ERR_STATE          ((II_INT4)-4)   /* invalid state */
# define IFF_ERR_LOG_FAIL       ((II_INT4)-5)   /* unable to open trace log */
# define IFF_ERR_COMMS          ((II_INT4)-6)   /* error making connection */
# define IFF_ERR_SQLCODE        ((II_INT4)-7)   /* error retrieving data */
# define IFF_ERR_PARAMNAME_LEN  ((II_INT4)-8)   /* name parameter too long */
# define IFF_ERR_PARAMVALUE_LEN ((II_INT4)-9)   /* value parameter too long */
# define IFF_ERR_MDBBUFSIZE     ((II_INT4)-10)  /* Buffer size exceeded */

#ifdef __cplusplus
}
#endif

# endif /* __IFF_INCLUDED */
