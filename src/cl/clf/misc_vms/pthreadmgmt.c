/*
** Copyright (c) 2008 Ingres Corporation
**
** PTHREADMGMT.C --
**
** 10-Dec-2008 (stegr01)
**    Created for Itanium port
**
*/
/*
**    Copyright (c) 2008 Ingres Corporation
**
*/

/*
** Name: pthreadmgmt.c - pthread management functions.
**
**
** History:
**  18-Jun-2007 (stegr01)
*/



/* Additional VMS header files. */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


#include <ssdef.h>
#include <stsdef.h>
#include <builtins.h>
#include <descrip.h>
#include <dscdef.h>
#include <prvdef.h>
#include <psldef.h>

#include <lib$routines.h>         /* RTL prototypes            */
#include <starlet.h>              /* system service prototypes */
#include <ints.h>                 /* 64 bit ints               */
#include <pthread.h>



#include <pthreadmgmt.h>

typedef struct _PTHREAD_OBJECT_HDR
{
    unsigned int state_or_lock;
    unsigned int valid;
    char         the_rest[];
} PTHREAD_OBJECT_HDR;


typedef struct dsc$descriptor_s DESCRIP_S, *LPDESCRIP_S;

static char* pthread_sts_to_ercode (PTHREAD_STS status);

static pthread_key_t  objectNameKey;




/*
** Name:  report_pthread_error
**
** Description:
**    Report a pthread function call error
**
** Inputs:
**    none
**
** Outputs:
**    None
**
**  Returns:
**    None.
**
**  Exceptions:
**    None.
**
** Side Effects:
**    None
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/


void 
report_pthread_error (const char* calling_func,
                      i4          line,
                      char*       pthread_func,
                      PTHREAD_STS pthread_sts,
                      char*       pthread_obj)
{
    i4  n;
    i4  sts;
    char* errmsg =  strerror(pthread_sts);
    char* errcode = pthread_sts_to_ercode(pthread_sts);

    /*
    **
    ** format the message on the stack. 1024 bytes should be adequate room
    ** the actual buffer length returned does not include the trailing 0.
    **
    */

    char  msg[1024]  = {0};
    char  timbuf[24] = {0};
    char* thread;

    DESCRIP_S timdsc = {sizeof(timbuf)-1, DSC$K_DTYPE_T, DSC$K_CLASS_S, timbuf};
    
    sts = sys$asctim (&timdsc.dsc$w_length, &timdsc, NULL, 0);
    if (sts & STS$M_SUCCESS) timbuf[timdsc.dsc$w_length] = 0;

    thread = (lib$ast_in_prog()) ? "AST" : getObjectName((uint64 *)pthread_self());

    n = sprintf (msg,
       "%s Unexpected PTHREAD error in thread %s reported in function \"%s\" at line %d, %s returned sts %%x%08X %s [%s])\n",
       timbuf, thread, calling_func, line, pthread_func, pthread_sts, errmsg, errcode, pthread_obj);

    TRdisplay (msg);
    TRflush ();
}


/*
** Name:  report_sts_to_errcode
**
** Description:
**    Covert a pthread error status to its ascii name
**
** Inputs:
**    none
**
** Outputs:
**    None
**
**  Returns:
**    None.
**
**  Exceptions:
**    None.
**
** Side Effects:
**    None
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/

static char* 
pthread_sts_to_ercode (PTHREAD_STS status)
{
    switch (status)
    {
        case EINVAL:    return ("EINVAL");
        case EDEADLK:   return ("EDEADLK");
        case ENOMEM:    return ("ENOMEM");
        case ENOSYS:    return ("ENOSYS");
        case ENOTSUP:   return ("ENOTSUP");
        case ESRCH:     return ("ESRCH");
        case EBUSY:     return ("EBUSY");
        case EAGAIN:    return ("EAGAIN");
        case ETIMEDOUT: return ("ETIMEDOUT");
        case EPERM:     return ("EPERM");
        case EINTR:     return ("EINTR");

        default:        return ("no translation");
    }
}


/*
** Name:  getObjectName
**
** Description:
**    Return the name of a pthread object
**    (e.g.  CV, MTX, Thread etc.)
**
** Inputs:
**    none
**
** Outputs:
**    None
**
**  Returns:
**    None.
**
**  Exceptions:
**    None.
**
** Side Effects:
**    None
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/

char*
getObjectName(uint64* obj)
{
    u_i4        valid;
    i4          buflen;
    PTHREAD_STS ptSts;

    PTHREAD_OBJECT_HDR* hdr = (PTHREAD_OBJECT_HDR *)obj;


    /* get the thread specific buffer where we can safely store the object name */

    char* bufadr = pthread_getspecific(objectNameKey);
    if (bufadr == NULL)
        return (NULL);

    buflen = PTHREAD_MAX_OBJNAMLEN+1;

    /*
    ** If this is a thread object then the type is opaque
    ** so check if we can read a quadword at the given address
    ** if we can't then it's definitely a thread object
    ** if we can then we can go ahead and get it's signature
    */

    if (__PAL_PROBER(hdr, 8, PSL$C_USER))
    {
        valid = hdr->valid & 0x07FFFFFF; /* lose some status bits from the front */
    }
    else
    {
        valid = 0xFFFFFFFF;
    }

    if (valid == _PTHREAD_CVALID)
    {
        ptSts = pthread_cond_getname_np ((pthread_cond_t *)obj, bufadr, buflen);
    }
    else if (valid == _PTHREAD_MVALID)
    {
        ptSts = pthread_mutex_getname_np ((pthread_mutex_t *)obj, bufadr, buflen);
    }
    else
    {
        ptSts = pthread_getname_np ((pthread_t)obj, bufadr, buflen);
    }

    return ((ptSts) ? NULL : bufadr);
}
