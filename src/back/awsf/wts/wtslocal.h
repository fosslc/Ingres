/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: wtslocal.h
**
** Description:
**      Module contains the definitions and types for WTS.
**
** History:
**      03-Feb-1999 (fanra01)
**          Add additional fragment flag and mask.
**          Changed all nat and longnat to i4.
**      17-Jan-2000 (fanra01)
**          Bug 100035
**          Add client type field to structure.
**      29-Mar-2001 (fanra01)
**          Bug 104379
**          Add varcmplt flag to know when variables have no value.
*/
# ifndef WTSLOCAL_INCLUDED
# define WTSLOCAL_INCLUDED

# define FRAGMENT_HEAD  1       /* upload file start */
# define FRAGMENT_TAIL  2       /* upload file end */
# define FRAGMENT_INCOMPLETE  8 /* upload boundary incomplete */

# define FRAGMENT_MASK  (FRAGMENT_HEAD | FRAGMENT_TAIL)

typedef struct __WTS_UPLOAD  WTS_UPLOAD;
typedef struct __WTS_UPLOAD  *PWTS_UPLOAD;

/*
** Name: UPLOADED_FILE
**
** Description:
**      file    ice file structure for the temporary file.
**      next
*/
typedef struct __UPLOADED_FILE {
	WCS_PFILE		file;
	struct __UPLOADED_FILE *next;
} UPLOADED_FILE;

/*
** Name: WTS_SESSION
**
** Description:
**      Session context structure.
**
**      act_session current session structure
**      gateway     type of library extension
**      list        List of uploaded files
**      variable    list of variables
**      vlen        length of variable string
**      status      session status
**      load        upload context
**      client      client type from request
**                  used for error returns on connect
*/
typedef struct __WTS_SESSION
{
    ACT_PSESSION    act_session;
    char            *gateway;
    UPLOADED_FILE   *list;
    char            *variable;
    i4              vlen;
    u_i4            status;
    PWTS_UPLOAD     load;
    u_i4            client;
} WTS_SESSION;

/*
** Name: WTS_UPLOAD
**
** Description:
**      Structure used to handle each file in the upload.
**
**      next        list of upload contexts for each level of file
**                  To be used if multiple file uploads are supported.
**      state       current state of file upload
**      endfile     end of upload file flag
**      fragment    type of fragment read HEAD/TAIL
**      fraglen     length of fragment
**      buf         working buffer for incomplete messages
**      start       starting boundary string
**      end         ending boundary string
**      varname     variable name
**      icefname    ice file name
**      filename    upload filename
**      type        mime type string
**      encode      binary encoding string
**      varcmplt    flag indicates if a name/value pair received. Indicates
**                  empty values.
*/
struct __WTS_UPLOAD
{
    PWTS_UPLOAD next;
    i4          state;
    bool        endfile;
    i4          fragment;
    i4          fraglen;
    char*       buf;
    char*       start;
    char*       end;
    char*       varname;
    char*       icefname;
    char*       filename;
    char*       type;
    char*       encode;
    i4          varcmplt;
};

FUNC_EXTERN GSTATUS WTSUploadInit (void);

FUNC_EXTERN GSTATUS WTSOpenUpLoad( char* contentType, WTS_SESSION *session);

FUNC_EXTERN GSTATUS WTSUpLoad( char* data, i4* datalength,
    WTS_SESSION *session);

FUNC_EXTERN GSTATUS WTSCloseUpLoad( WTS_SESSION* session);

# endif
