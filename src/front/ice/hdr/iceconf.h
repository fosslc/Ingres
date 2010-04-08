/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: iceconf.h
**
** Description:
**      File contains declarations and prototypes for the ICE configuration
**      API.
**
## History:
##      21-Feb-2001 (fanra01)
##          Sir 103813
##          Created.
##      04-May-2001 (fanra01)
##          Sir 103813
##          Add cookie to ic_initialize prototype and ICE_VAR to enumerations.
##          Add byref functions for select and retrieves.
*/
# ifndef iceconf_h_INCLUDED
# define iceconf_h_INCLUDED
# include <ice_c_api.h>

# define    IA_ICE_AUTH     0
# define    IA_OS_AUTH      1

# define    IA_PUBLIC_LOC   (II_INT4)0x0001
# define    IA_HTTP_LOC     (II_INT4)0x0002
# define    IA_II_LOC       (II_INT4)0x0004

# define    IA_USER_ADM     (II_INT4)0x0001
# define    IA_USER_SEC     (II_INT4)0x0002
# define    IA_USER_UNIT    (II_INT4)0x0004
# define    IA_USER_MONIT   (II_INT4)0x0008

# define    IA_PROF_ADM     (II_INT4)0x0001
# define    IA_PROF_SEC     (II_INT4)0x0002
# define    IA_PROF_UNIT    (II_INT4)0x0004
# define    IA_PROF_MONIT   (II_INT4)0x0008

# define    IA_PUBLIC       (II_INT4)0x0001
# define    IA_PRE_CACHE    (II_INT4)0x0002
# define    IA_FIX_CACHE    (II_INT4)0x0004
# define    IA_SESS_CACHE   (II_INT4)0x0008
# define    IA_EXTERNAL     (II_INT4)0x0010
# define    IA_PAGE         (II_INT4)0x0020
# define    IA_FACET        (II_INT4)0x0040

# define    IA_READ_PERM    (II_INT4)0x0001
# define    IA_INSERT_PERM  (II_INT4)0x0002
# define    IA_EXE_PERM     (II_INT4)0x0010

# define    E_IC_MASK       (264 * 0x10000)
# define    E_IC_INVALID_INPPARAM      (II_ULONG)(E_IC_MASK + 0x000F)
# define    E_IC_INVALID_OUTPARAM      (II_ULONG)(E_IC_MASK + 0x0010)
# define    E_IC_INVALID_USERNAME      (II_ULONG)(E_IC_MASK + 0x0011)
# define    E_IC_INVALID_PASSWORD      (II_ULONG)(E_IC_MASK + 0x0012)
# define    E_IC_OUT_OF_DATA           (II_ULONG)(E_IC_MASK + 0x0013)
# define    E_IC_DATASET_EXHAUSTED     (II_ULONG)(E_IC_MASK + 0x0014)
# define    E_IC_INVALID_FNAME         (II_ULONG)(E_IC_MASK + 0x0015)
# define    E_IC_ROW_COMPLETE          (II_ULONG)(E_IC_MASK + 0x0016)
# define    E_IC_INVALID_UNIT_NAME     (II_ULONG)(E_IC_MASK + 0x0017)
# define    E_IC_INVALID_ROLE_NAME     (II_ULONG)(E_IC_MASK + 0x0018)
# define    E_IC_INVALID_PROF_NAME     (II_ULONG)(E_IC_MASK + 0x0019)
# define    E_IC_INVALID_LOC_NAME      (II_ULONG)(E_IC_MASK + 0x001C)
# define    E_ID_INVALID_FILENAME      (II_ULONG)(E_IC_MASK + 0x001D)

enum _icefunc {
    ICE_DBUSER, ICE_ROLE, ICE_USER, ICE_DB, ICE_USERROLE, ICE_USERDB,
    ICE_SESS, ICE_UNIT, ICE_DOC, ICE_DOCROLE, ICE_DOCUSER, ICE_UNITROLE,
    ICE_UNITUSER, ICE_UNITLOC, ICE_UNITCOPY, ICE_LOCATION, ICE_ACTSESSION,
    ICE_USRSESION, ICE_USERXACT, ICE_USERCURSOR, ICE_CACHE, ICE_CONNECTION,
    ICE_PROFILE, ICE_PROFROLE, ICE_PROFDB, ICE_TAG2STR, ICE_GETLOCFILES,
    ICE_GETVARS, ICE_GETSRVVARS, ICE_VAR };

typedef II_UINT4 HICECTX;

typedef struct _ic_stack    ICESTK;
typedef struct _ic_stack*   PICESTK;

struct  _ic_stack
{
    II_VOID*        iceapi;
    II_INT4         endset;
    II_INT4         colcount;
};

II_EXTERN II_EXPORT ICE_STATUS
ic_initialize( II_CHAR* node, II_CHAR* name, II_CHAR* password,
    II_CHAR* cookie, HICECTX* icectx );

II_EXTERN II_EXPORT II_VOID
ic_seterror( HICECTX icectx, II_INT4 status );

II_EXTERN II_EXPORT II_VOID
ic_saveapi( HICECTX icectx, PICESTK* icestk );

II_EXTERN II_EXPORT II_VOID
ic_restapi( HICECTX icectx, PICESTK icestk );

II_EXTERN II_EXPORT ICE_STATUS
ic_close( HICECTX icectx );

static ICE_STATUS
ic_getermessage( II_INT4  id, II_INT4 flags, II_CHAR** msg );

II_EXTERN II_EXPORT II_VOID
ic_errormsg( HICECTX hicectx, ICE_STATUS status, II_CHAR** errormsg );

II_EXTERN II_EXPORT ICE_STATUS
ic_getfileparts( II_CHAR* filename, II_CHAR** file, II_CHAR** ext );

II_EXTERN II_EXPORT II_VOID
ic_terminate( II_VOID );

II_EXTERN II_EXPORT ICE_STATUS
ic_select( HICECTX icectx, II_CHAR* func );

II_EXTERN II_EXPORT ICE_STATUS
ic_idselect( HICECTX icectx, II_CHAR* func, II_INT4 id );

II_EXTERN II_EXPORT ICE_STATUS
ic_getnext( HICECTX icectx );

II_EXTERN II_EXPORT ICE_STATUS
ic_idretrieve( HICECTX icectx, II_CHAR* func, II_INT4 id );

II_EXTERN II_EXPORT ICE_STATUS
ic_getval( HICECTX icectx, II_CHAR** value );

II_EXTERN II_EXPORT ICE_STATUS
ic_getnxtval( HICECTX icectx, II_CHAR** value );

II_EXTERN II_EXPORT ICE_STATUS
ic_getvalbyref( HICECTX hicectx, II_CHAR* value );

II_EXTERN II_EXPORT ICE_STATUS
ic_getnxtvalbyref( HICECTX hicectx, II_CHAR* value );

II_EXTERN II_EXPORT ICE_STATUS
ic_release( HICECTX icectx, II_CHAR* func );

II_EXTERN II_EXPORT ICE_STATUS
ic_getitem( HICECTX icectx, II_INT4 fnumber, II_CHAR* cmpname, II_CHAR* cmpval,
    II_CHAR* resname, II_CHAR** resval);

II_EXTERN II_EXPORT ICE_STATUS
ic_getitemwith( HICECTX icectx, II_INT4 fnumber, ICE_C_PARAMS* params,
    II_CHAR* resname, II_CHAR** resval);

II_EXTERN II_EXPORT ICE_STATUS
ic_createsessgrp( HICECTX icectx, II_CHAR* name, II_INT4* sessid );

II_EXTERN II_EXPORT ICE_STATUS
ic_deletesessgrp( HICECTX icectx, II_CHAR* name );

II_EXTERN II_EXPORT ICE_STATUS
ic_createrole( HICECTX icectx, II_CHAR* name, II_CHAR* comment, II_INT4* roleid );

II_EXTERN II_EXPORT ICE_STATUS
ic_deleterole( HICECTX icectx, II_CHAR* name );

II_EXTERN II_EXPORT ICE_STATUS
ic_createdbuser( HICECTX icectx, II_CHAR* alias, II_CHAR* name,
    II_CHAR* password, II_CHAR* comment, II_INT4* dbuserid );

II_EXTERN II_EXPORT ICE_STATUS
ic_deletedbuser( HICECTX icectx, II_CHAR* name );

II_EXTERN II_EXPORT ICE_STATUS
ic_createuser( HICECTX icectx, II_CHAR* name, II_CHAR* password,
    II_INT4 dbuserid, II_INT4 usrflag, II_INT4 timeout, II_CHAR* comment,
    II_INT4 authtype, II_INT4* userid );

II_EXTERN II_EXPORT ICE_STATUS
ic_deleteuser( HICECTX icectx, II_CHAR* name );

II_EXTERN II_EXPORT ICE_STATUS
ic_createloc( HICECTX icectx, II_CHAR* locname, II_INT4 type,
    II_CHAR* path, II_CHAR* ext, II_INT4* locid );

II_EXTERN II_EXPORT ICE_STATUS
ic_deleteloc( HICECTX icectx, II_CHAR* locname );

II_EXTERN II_EXPORT ICE_STATUS
ic_createbu( HICECTX icectx, II_CHAR* buname, II_CHAR* owner, II_INT4* buid );

II_EXTERN II_EXPORT ICE_STATUS
ic_deletebu( HICECTX icectx, II_CHAR* buname );

II_EXTERN II_EXPORT ICE_STATUS
ic_createdoc( HICECTX hicectx, II_INT4 buid, II_CHAR* docname,
    II_CHAR* docext, II_INT4 doctype, II_INT4 locid, II_INT4 docflags,
    II_CHAR* extfilespec, II_CHAR* owner, II_INT4* docid );

II_EXTERN II_EXPORT ICE_STATUS
ic_deletedoc( HICECTX icectx, II_CHAR* docname, II_CHAR* docext, II_INT4 locid );

II_EXTERN II_EXPORT ICE_STATUS
ic_createprofile( HICECTX icectx, II_CHAR* name, II_INT4 dbuser, II_INT4 perms,
     II_INT4 timeout, II_INT4* profid );

II_EXTERN II_EXPORT ICE_STATUS
ic_deleteprofile( HICECTX icectx, II_CHAR* name );

II_EXTERN II_EXPORT ICE_STATUS
ic_assocunitrole( HICECTX icectx, II_CHAR* uname, II_CHAR* rname, II_INT4 perms );

II_EXTERN II_EXPORT ICE_STATUS
ic_releaseunitrole( HICECTX icectx, II_CHAR* uname, II_CHAR* rname );

II_EXTERN II_EXPORT ICE_STATUS
ic_assocprofilerole( HICECTX icectx, II_CHAR* pname, II_CHAR* rname );

II_EXTERN II_EXPORT ICE_STATUS
ic_releaseprofilerole( HICECTX icectx, II_CHAR* pname, II_CHAR* rname );

II_EXTERN II_EXPORT ICE_STATUS
ic_retvariable( HICECTX hicectx, II_CHAR* scope, II_CHAR* name,
    II_CHAR** value );

II_EXTERN II_EXPORT ICE_STATUS
ic_setvariable( HICECTX hicectx, II_CHAR* scope, II_CHAR* name,
    II_CHAR* value );

# endif
