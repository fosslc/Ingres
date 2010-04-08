/*
** Copyright (c) 2004, 2005 Ingres Corporation
*/

/**
** Name: iffutil
**
** Description:
**  Module provides the functions for platform specific
**  exception handling.
**(E
**      IFFExceptFilter     - Process the exception to determine action
**)E
**
** History:
##      08-Mar-2004 (fanra01)
##          SIR 111718
##          Created.
##      23-Aug-2004 (fanra01)
##          Sir 112892
##          Update comments for documentation purposes.
##      28-Sep-2004 (fanra01)
##          Sir 113152
##          Add iff_return_status function.
##      07-Feb-2005 (fanra01)
##          Sir 113881
##          Merge Windows and UNIX sources.
##      25-Feb-2005 (fanra01)
##          Sir 113975
##          Add iff_get_strings function that returns an array of pointers to
##          each component strings.
**/
# include <compat.h>
# include <cm.h>
# include <ex.h>
# include <me.h>
# include <qu.h>
# include <st.h>

# include <gl.h>
# include <er.h>
# include <sp.h>
# include <mo.h>
# include <sl.h>
# include <iicommon.h>
# include <gca.h>

# include "iff.h"
# include "iffint.h"

# ifdef NT_GENERIC
/*
** Name: IFFExceptFilter    - Process the exception to determine action
**
** Description:
**  Exception filter for NT.  Determines the action for the specified
**  exception.
**      
** Inputs:
**      ep      exception pointers.
**      
** Outputs:
**      None.
**      
** Returns:
**      EXCEPTION_CONTINUE_EXECUTION    exception handled
**      EXCEPTION_CONTINUE_SEARCH       pass the exception up the stack
**
** History:
**      23-Aug-2004 (fanra01)
**          Commented.
*/
int
IFFExceptFilter( EXCEPTION_POINTERS* ep )
{
    EXCEPTION_RECORD *exrp = ep->ExceptionRecord;
    CONTEXT *ctxtp = ep->ContextRecord;
    
    switch ( exrp->ExceptionCode )
    {
        case EXCEPTION_ACCESS_VIOLATION :
    	    EXsignal( EXSEGVIO, 3, (i4)exrp->ExceptionCode, exrp, ctxtp );
	    return ( EXCEPTION_CONTINUE_EXECUTION );

	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
    	    EXsignal( EXBNDVIO, 3, (i4)exrp->ExceptionCode, exrp, ctxtp );
	    return ( EXCEPTION_CONTINUE_EXECUTION );

	case EXCEPTION_DATATYPE_MISALIGNMENT:
    	    EXsignal( EXBUSERR, 3, (i4)exrp->ExceptionCode, exrp, ctxtp );
	    return ( EXCEPTION_CONTINUE_EXECUTION );

	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    	    EXsignal( EXFLTDIV, 3, (i4)exrp->ExceptionCode, exrp, ctxtp );
	    return ( EXCEPTION_CONTINUE_EXECUTION );

	case EXCEPTION_FLT_OVERFLOW:
    	    EXsignal( EXFLTOVF, 3, (i4)exrp->ExceptionCode, exrp, ctxtp );
	    return ( EXCEPTION_CONTINUE_EXECUTION );

	case EXCEPTION_FLT_UNDERFLOW:
    	    EXsignal( EXFLTUND, 3, (i4)exrp->ExceptionCode, exrp, ctxtp );
	    return ( EXCEPTION_CONTINUE_EXECUTION );

	case EXCEPTION_FLT_DENORMAL_OPERAND:
	case EXCEPTION_FLT_INEXACT_RESULT:
	case EXCEPTION_FLT_INVALID_OPERATION:
	case EXCEPTION_FLT_STACK_CHECK:
    	    EXsignal( EXFLOAT, 3, (i4)exrp->ExceptionCode, exrp, ctxtp );
	    return ( EXCEPTION_CONTINUE_EXECUTION );

	case EXCEPTION_ILLEGAL_INSTRUCTION:
    	    EXsignal( EXOPCOD, 3, (i4)exrp->ExceptionCode, exrp, ctxtp );
	    return ( EXCEPTION_CONTINUE_EXECUTION );

	case EXCEPTION_IN_PAGE_ERROR:
    	    EXsignal( EXUNKNOWN, 3, (i4)exrp->ExceptionCode, exrp, ctxtp );
	    return ( EXCEPTION_CONTINUE_EXECUTION );

	case EXCEPTION_INT_DIVIDE_BY_ZERO:
    	    EXsignal( EXINTDIV, 3, (i4)exrp->ExceptionCode, exrp, ctxtp );
	    return ( EXCEPTION_CONTINUE_EXECUTION );

	case EXCEPTION_INT_OVERFLOW:
    	    EXsignal( EXINTOVF, 3, (i4)exrp->ExceptionCode, exrp, ctxtp );
	    return ( EXCEPTION_CONTINUE_EXECUTION );
	case 0xc0000008: /* bad handle */
	    return ( EXCEPTION_CONTINUE_EXECUTION );
    
       default:
    	    EXsignal( EXUNKNOWN, 3, (i4)exrp->ExceptionCode, exrp, ctxtp );
	    return ( EXCEPTION_CONTINUE_SEARCH );
    }
}
# endif

/*
** Name: iff_return_status      - Return an IFF status code for the error
**
** Description:
**      Function takes an internal status and maps it to a more generic
**      interface error code.
**
** Inputs:
**      status      Status code to be mapped.
**
** Outputs:
**      None.
**      
** Returns:
**      retcode     IFF equivalent error code.
**                  There is no error return.
**
** History:
**      28-Sep-2004 (fanra01)
**          Created.
*/
II_INT4
iff_return_status( STATUS status )
{
    II_INT4 retcode = status;
    
    switch(status)
    {
        case OK:
            retcode = IFF_SUCCESS;
            break;
        /*
        ** SQL OK with exception
        */
        case 100:
        case 700:
        case 710:
            retcode = IFF_ERR_SQLCODE;
            break;
        
        /*
        ** Ingres facility codes
        */
        case ME_NO_ALLOC:
        case ME_GONE:
            retcode = IFF_ERR_RESOURCE;
            break;
            
        case E_GC0002_INV_PARM:
        case E_GC0003_INV_SVC_CODE:
        case E_GC0004_INV_PLIST_PTR:
        case E_GC0006_DUP_INIT:
        case E_GC0007_NO_PREV_INIT:
            retcode = IFF_ERR_COMMS;
            break;
        
        /*
        ** No mapping for these errors
        */    
        case IFF_ERR_PARAM:
        case IFF_ERR_STATE:
        case IFF_ERR_LOG_FAIL:
        case IFF_ERR_PARAMNAME_LEN:
        case IFF_ERR_PARAMVALUE_LEN:
        case IFF_ERR_MDBBUFSIZE:
            break;
            
        case FAIL:
            retcode = IFF_ERR_FAIL;
            break;
            
        default:
            if ( status < 0)
            {
                retcode = IFF_ERR_SQLCODE;
            }
            else
            {
                retcode = IFF_ERR_FAIL;
            }
            break;
    }
    if (retcode != IFF_SUCCESS)
    {
        iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
            ERx("iff_return_status: 0x%x - 0x%x\n"), status, retcode );
    }

    return( retcode );
}

/*
** Name: iff_get_strings
**
** Description:
**  Parse string into delimited sections and return a pointer to each section.
**
** Inputs:
**      str         Input string to be split
**      delim       Character that delimits sections within the string
**      wc          Number of array elements
**      parray      Pointer to array elements
**
** Outputs:
**      wc          Number of delimited sections found
**      parray      Array elements initialized to each found section
**
** Returns:
**      status      IFF_SUCCESS     Function completed successfully
**                  IFF_ERR_PARAM   Invalid input parameter
*/
STATUS
iff_get_strings(
    char*   str,
    char*   delim,
    i4*     wc,
    char**  parray)
{
    STATUS  status = IFF_SUCCESS;
    i4      asize;
    i4      i = 0;
    char*   p;
    char**  res;
    i4      len;
    
    do
    {
        if ((str == NULL) || (delim == NULL) || (wc == NULL) ||
            (parray == NULL))
        {
            status = IFF_ERR_PARAM;
            break;
        }
        len = STlength( str );
        p = str;
        asize = *wc;
        *wc = 0;
        res = parray;
        /*
        ** Skip whitespace
        */
        while(CMwhite(p))
        {
            CMnext(str);
        }
        for (res[i] = p;(len > 0) && (i < asize); CMnext(p), len-=1)
        {
            if (CMcmpcase(p, delim) == 0)
            {
                i+=1;
                res[i] = p + CMbytecnt(p);
                *p = '\0';
            }
        }
    }
    while(FALSE);
    if (status != IFF_SUCCESS)
    {
        iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
            ERx("%08x = iff_get_strings( %s, \'%c\', %d, %p);\n" ),
            status, str, delim, *wc, parray );
    }
    else
    {
        /*
        ** Adjust index to count
        */
        *wc = (i > 0) ? i+1 : 0;
    }
    return(status);
}
