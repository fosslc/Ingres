/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: icelocal.h
**
** Description:
**      File contains declarations for the ice configuration API to abstract
**      the compatibility facilities.
**      Contains internal structures and definitions for the configuration
**      API.
**
** History:
**      21-Feb-2001 (fanra01)
**          Sir 103813
**          Created.
**      04-May-2001 (fanra01)
**          Sir 103813
**          Add storage of session cookie and addition of testaction function.
**          Save the connection type based on whether a cookie has been
**          specified.
*/
# ifndef icelocal_h_INCLUDED
# define icelocal_h_INCLUDED
# include <ice_c_api.h>

# include <compat.h>
# include <cv.h>
# include <er.h>
# include <lo.h>
# include <me.h>
# include <si.h>
# include <st.h>

# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>

# include <erwu.h>

# define MEMALLOC( size, type , clear, stat, mptr ) \
    { \
        mptr = (type)MEreqmem( 0, size, clear, &stat ); \
    }

# define MEMFREE( mptr ) MEfree( (PTR)mptr )

# define MEMCOPY( src, size, dst )  MEcopy( src, size, dst )

# define MEMFILL( size, filler, mptr )  MEfill( size, filler, mptr )

# define PRINTF             SIprintf

# define FPRINTF            SIfprintf

# define FFLUSH             SIflush

# define STLENGTH( mptr )   STlength( mptr )

# define STRCAT                 STcat

# define STRCMP( str, cstr ) STcompare( str, cstr )

# define STRCOPY( src, dest )   STcopy( src, dest )

# define STRPRINTF          STprintf

# define STRDUP( str )    STalloc( str )

# define ASCTOI( str, iptr ) \
    { \
        CVan( str, iptr ); \
    }

typedef struct _iceapi  ICEAPI;
typedef struct _iceapi  * PICEAPI;

typedef struct _ic_context  ICECTX;
typedef struct _ic_context* PICECTX;

typedef struct _ctxmap      ICECTXMAP;
typedef struct _ctxmap*     PICECTXMAP;

typedef struct _srvfunc SRVFUNC;
typedef struct _srvfunc * PSRVFUNC;
typedef enum   _icefunc ICEFUNC;

# define MAX_MAP_WIDTH      32
# define MAX_MAP_LENGTH     32
# define MAX_MSG_SIZE       256

# define E_IA_INVALID_OUTPARM   E_WU000B_INVALID_OUTPARM
# define E_IA_INVALID_INPPARM   E_WU000C_INVALID_INPPARM
# define E_IA_VALUE_NULL        E_WU000D_VALUE_NULL
# define E_IA_UNKNOWN_FIELD     E_WU000E_UNKNOWN_FIELD

# define    IA_STR_ACTION   "action"
# define    IA_STR_SELECT   "select"
# define    IA_STR_RETRIEVE "retrieve"
# define    IA_STR_INSERT   "insert"
# define    IA_STR_UPDATE   "update"
# define    IA_STR_DELETE   "delete"
# define    IA_STR_IN       "in"
# define    IA_STR_OUT      "out"
# define    IA_STR_CHECKED  "checked"
# define    IA_STR_ICE_AUTH "ICE"
# define    IA_STR_OS_AUTH  "OS"
# define    IA_STR_PAGE     "page"
# define    IA_STR_FACET    "facet"

/*
** Name: SRVFUNC
**
** Description:
**      Structure declares the names, parameter names an possible actions
**      for ice server functions.
**
** Fields:
**      fname           Function name
**      srvparms        Server function parameter name list
**      actions         Mask of permissible actions.
**
*/
struct _srvfunc
{
    II_CHAR*    fname;
    II_CHAR**   srvparms;
    II_INT4     actions;
# define    IA_SELECT      0x0001
# define    IA_RETRIEVE    0x0002
# define    IA_INSERT      0x0004
# define    IA_UPDATE      0x0008
# define    IA_DELETE      0x0010
# define    IA_IN          0x0020
# define    IA_OUT         0x0040
};

/*
** Name: ICEAPI
**
** Description:
**      Structure for the manipulation of function parameters.
**
** Fields:
**      fnumber         Function number taken from the emunerated list.
**      fparms          A named list of the parameter structures.
**      fcount          Number of parameters in the list.
**
*/
struct _iceapi
{
    ICEFUNC         fnumber;
    ICE_C_PARAMS*   fparms;
    II_INT4         fcount;
    ICE_STATUS      (*getfuncname)( PICEAPI iceapi, II_INT4 fnumber, II_CHAR** func );
    ICE_STATUS      (*getaction)( PICEAPI iceapi, II_CHAR** action );
    ICE_STATUS      (*setaction)( PICEAPI iceapi, II_CHAR* action );
    ICE_STATUS      (*testaction)( II_INT4 fnumber, II_INT4 action, II_BOOL* compared );
    ICE_STATUS      (*gettype)  ( PICEAPI iceapi, II_INT4 pos, II_INT4* type );
    ICE_STATUS      (*settype)  ( PICEAPI iceapi, II_INT4 pos, II_INT4 type );
    ICE_STATUS      (*getname)  ( PICEAPI iceapi, II_INT4 pos, II_CHAR** name );
    ICE_STATUS      (*setname)  ( PICEAPI iceapi, II_INT4 pos, II_CHAR* name );
    ICE_STATUS      (*getvalue) ( PICEAPI iceapi, II_INT4 pos, II_CHAR** value );
    ICE_STATUS      (*setvalue) ( PICEAPI iceapi, II_INT4 pos, II_CHAR* value );
    ICE_STATUS      (*getpos)   ( PICEAPI iceapi, II_CHAR* name, II_INT4* pos );
    ICE_STATUS      (*getparmname)( PICEAPI iceapi, II_CHAR* name, ICE_C_PARAMS** parm );
    ICE_STATUS      (*getparmpos)( PICEAPI iceapi, II_INT4 pos, ICE_C_PARAMS** parm );
    ICE_STATUS      (*getcount) ( PICEAPI iceapi, II_INT4* fcount );
    ICE_STATUS      (*getnumber)( PICEAPI iceapi, II_INT4* fnumber );
    ICE_STATUS      (*createparms)( II_CHAR* funcname, ICE_C_PARAMS** params);
    ICE_STATUS      (*destroyparms)( ICE_C_PARAMS* params );
};

/*
** Name: ICECTX
**
** Description:
**      Structure for connection and result manipulation.
**
** Fields:
**      initialized     Flag indicates the structure is in use
**      node            Optional node of the remote ice server.  NULL is local.
**      user            User name for the ice server connection.
**      passwd          Password for the user.
**      cookie          The connection context to an ice server session.
**      sessid          Communication context once connection is established.
**      curr            Current request context.  Used for nesting requests.
**      status          Last ICE C API error status.
**      errinfo         Text translation of status.
**
*/
struct  _ic_context
{
    II_INT4         initialized;
    II_INT4         connecttype;
# define    IA_SESSION_EXCLUSIVE    1
# define    IA_SESSION_SHARED       2
    II_CHAR*        node;
    II_CHAR*        user;
    II_CHAR*        passwd;
    II_CHAR*        cookie;
    ICE_C_CLIENT    sessid;
    PICESTK         curr;
    ICE_STATUS      status;
    II_CHAR*        errinfo;
    ICE_STATUS      (*select)( HICECTX icectx, II_CHAR* func );
    ICE_STATUS      (*idselect)( HICECTX icectx, II_CHAR* func, II_INT4 id );
    ICE_STATUS      (*idretrieve)( HICECTX icectx, II_CHAR* func, II_INT4 id );
    ICE_STATUS      (*getnext)( HICECTX icectx );
    ICE_STATUS      (*getval)( HICECTX icectx, II_CHAR** value );
    ICE_STATUS      (*getnxtval)( HICECTX icectx, II_CHAR** value );
    ICE_STATUS      (*release)( HICECTX icectx, II_CHAR* func );
    ICE_STATUS      (*getitem)( HICECTX icectx, II_INT4 fnumber, II_CHAR* cmpname, II_CHAR* cmpval, II_CHAR* resname, II_CHAR** resval);
    ICE_STATUS      (*getitemwith)( HICECTX icectx, II_INT4 fnumber, ICE_C_PARAMS* params,
                        II_CHAR* resname, II_CHAR** resval);
    II_VOID         (*seterror)( HICECTX icectx, II_INT4 status );
    II_VOID         (*saveapi)( HICECTX icectx, PICESTK* icestk );
    II_VOID         (*restapi)( HICECTX icectx, PICESTK icestk );
};

/*
** Name: ICECTXMAP
**
** Description:
**      Structure defines a 16x16 context mapping to remove the need for
**      applications from declaring and accessing the icectx structure.
**
** Fields:
**      inuse           16-bit field indicates which row entry is in use/free.
**          used        16-bit field indicates which col entry is in use/free.
**          col[]       List of ICECTX pointers.
**      row[]           Array of ICECTX lists.
*/
struct _ctxmap
{
    i4 inuse;
    struct _tag_list
    {
        i4 used;
        PICECTX col[MAX_MAP_WIDTH];
    }row[MAX_MAP_LENGTH];
};

ICE_STATUS
ia_retfuncname( II_INT4 fnumber, II_CHAR** func );

II_EXTERN II_EXPORT ICE_STATUS
ia_create( II_CHAR* funcname, PICEAPI* iceapi );

II_EXTERN II_EXPORT ICE_STATUS
ia_destroy( PICEAPI iceapi );

ICE_STATUS
ia_getfuncname( PICEAPI iceapi, II_INT4 fnumber, II_CHAR** func );

II_EXTERN II_EXPORT ICE_STATUS
ia_getaction( PICEAPI iceapi, II_CHAR** action );

II_EXTERN II_EXPORT ICE_STATUS
ia_setaction( PICEAPI iceapi, II_CHAR* action );

II_EXTERN II_EXPORT ICE_STATUS
ia_testaction( II_INT4 fnumber, II_INT4 actions, II_BOOL* compared );

II_EXTERN II_EXPORT ICE_STATUS
ia_gettype( PICEAPI iceapi, II_INT4 pos, II_INT4* type );

II_EXTERN II_EXPORT ICE_STATUS
ia_settype  ( PICEAPI iceapi, II_INT4 pos, II_INT4 type );

II_EXTERN II_EXPORT ICE_STATUS
ia_getname  ( PICEAPI iceapi, II_INT4 pos, II_CHAR** name );

II_EXTERN II_EXPORT ICE_STATUS
ia_setname  ( PICEAPI iceapi, II_INT4 pos, II_CHAR* name );

II_EXTERN II_EXPORT ICE_STATUS
ia_getvalue ( PICEAPI iceapi, II_INT4 pos, II_CHAR** value );

II_EXTERN II_EXPORT ICE_STATUS
ia_setvalue ( PICEAPI iceapi, II_INT4 pos, II_CHAR* value );

ICE_STATUS
ia_getpos( PICEAPI iceapi, II_CHAR* name, II_INT4* pos );

ICE_STATUS
ia_getparmname( PICEAPI iceapi, II_CHAR* name, ICE_C_PARAMS** parm );

ICE_STATUS
ia_getparmpos( PICEAPI iceapi, II_INT4 pos, ICE_C_PARAMS** parm );

II_EXTERN II_EXPORT ICE_STATUS
ia_getcount ( PICEAPI iceapi, II_INT4* fcount );

II_EXTERN II_EXPORT ICE_STATUS
ia_getnumber ( PICEAPI iceapi, II_INT4* fnumber );

II_EXTERN II_EXPORT ICE_STATUS
ia_createparms( II_CHAR* funcname, ICE_C_PARAMS** params);

II_EXTERN II_EXPORT ICE_STATUS
ia_destroyparms( ICE_C_PARAMS* params );

II_EXTERN II_EXPORT ICE_STATUS
ic_getfreeentry( PICECTX picectx, HICECTX* hicectx );

II_EXTERN II_EXPORT ICE_STATUS
ic_getentry( HICECTX icectx, PICECTX* picectx );

II_EXTERN II_EXPORT ICE_STATUS
ic_freeentry( HICECTX icectx );

# endif     /* icelocal_h_INCLUDED */
