/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: icegen.c
**
** Description:
**      Module provides the functions to store fragements of strings for
**      constructing an ICE macro in 2.5 form.
**
**      igen_setgenln       Inserts a generated line into the stack.
**      igen_flush          Generates output of completed macros and tags.
**      igen_getparent      Returns the parent of the current line.
**      igen_setflags       Sets the flags of the current line.
**      igen_testflags      Tests the setting of a flags in the current line.
**      igen_getstart       Gets the name of the tag in an ICE EXTEND tag.
**
** History:
**      08-Feb-2001 (fanra01)
**          Created.
**      21-Feb-2001 (peeje01)
**          Added igen_testflags
**      13-Mar-2001 (peeje01)
**          Added igen_getstart.
**	30-jul-2001 (somsa01)
**	    In igen_setgenln(), changed dlen to be of type SIZE_TYPE.
**	04-sep-2003 (somsa01)
**	    For .NET 2003, iostream has changed.
*/
#if defined(NT_GENERIC) || defined(LINUX)
# include <iostream>
using namespace std;
#else
# include <iostream.h>
#endif
# include <icestack.h>
# include "icegen.h"

/*
** Name: igen_setgenln
**
** Description:
**      Inserts a generated line into the stack.
**
** Inputs:
**      pstack      Pointer to the stack to insert into.  Returned from
**                  call to stk_alloc.
**      flags       Flag to determine if the entry is active or if there
**                  are child macros.
**      lvl         Level of the macro.
**      handler     The ice keyword or option.
**      data        Pointer to the generated ice string.
**      dlen        Length of the ice string.
**
** Outputs:
**      igenln      Pointer to the returned generated line structure.
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      08-Feb-2001 (fanra01)
**          Created.
**	30-jul-2001 (somsa01)
**	    Changed dlen to be of type SIZE_TYPE.
*/
STATUS
igen_setgenln( PSTACK pstack, u_i4 flags, u_i4 lvl,
    enum ICEKEY handler, char* data, SIZE_TYPE dlen, PICEGENLN* igenln )
{
    STATUS      status;
    PSTACKENTRY entry;
    PSTACKENTRY prev;
    u_i4        lnlen = sizeof( ICEGENLN ) + (u_i4)dlen;
    PICEGENLN   iceline;
    PICEGENLN   parent;
    char*       stkdata;

    if (pstack != NULL)
    {
        if (igenln != NULL)
        {
            if ((status = stk_pushentry( pstack, lnlen, &entry )) == OK)
            {
                if ((status = stk_getdata( entry, (VOID **)&iceline )) == OK)
                {
                    stkdata = ICE_LN_DATA( iceline );
                    iceline->flags |= flags;
                    iceline->nestlevel = lvl;
                    iceline->linelen = (u_i4)dlen;
                    iceline->handler = handler;
                    MEMCOPY( data, (u_i4)dlen, stkdata);
                    if ((status = stk_getprevious( pstack, entry, &prev )) == OK)
                    {
                        if ((status = stk_getdata( prev, (VOID **)&parent )) == OK)
                        {
                            /*
                            ** if previous entry does not have the same nesting
                            ** level then it must be a parent. Otherwise it
                            ** must be a sibling
                            */
                            if (parent->nestlevel < lvl)
                            {
                                if (parent->handler == ICEOPEN)
                                {
                                    iceline->parent = parent;
                                }
                                else
                                {
                                    iceline->parent = parent->parent;
                                }
				if (iceline->parent != NULL)
				{
				/*
				** If the parent exists, set the repeat flag
				*/
					iceline->parent->flags |= ICE_REPEAT;
				}
                            }
                            else if (parent->nestlevel == lvl)
                            {
                                if (parent->handler == ICEOPEN)
                                {
                                    iceline->parent = parent;
                                }
                                else
                                {
                                    iceline->parent = parent->parent;
                                }
                            }
                        }
                    }
                    *igenln = iceline;
                }
            }
        }
        else
        {
            status = E_IGEN_INVALID_PARAM;
        }
    }
    else
    {
        status = E_STK_INVALID_STACK;
    }
    return( status );
}

/*
** Name: igen_flush
**
** Description:
**      Generates output of completed macros and tags.
**
** Inputs:
**      pstack  Pointer to the stack to insert into.  Returned from
**              call to stk_alloc.
**
** Outputs:
**      None.   Stack is flushed and entries are poped and marked as available
**              for use.
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**
** History:
**      08-Feb-2001 (fanra01)
**          Created.
*/
STATUS
igen_flush( PSTACK pstack, OUTHANDLER fnout )
{
    STATUS status = OK;
    PSTACKENTRY loop;
    PICEGENLN   iceline;

    if (pstack != NULL)
    {
        if ((status = stk_gethead( pstack, &loop )) == OK)
        {
            for(; status == OK && loop != NULL;
                status = stk_getnext( pstack, loop, &loop ))
            {
                if ((status = stk_getdata( loop, (VOID **)&iceline )) == OK)
                {
                    if (iceline->flags & ICE_FLUSH_ENTRY)
                    {
                        u_i4 i;
//                        for(i=0; i < iceline->nestlevel; i+=1)
//                        {
//                            fnout( "    " );
//                        }
                        fnout( ICE_LN_DATA(iceline) );
                        if (iceline->flags & ICE_REPEAT)
                        {
//                            for(i=0; i < iceline->nestlevel; i+=1)
//                            {
//                                fnout( "    " );
//                            }
                            fnout( "REPEAT " );
                        }
                    }
//                    FFLUSH( fptr );
                }
            }
            status = (status == E_STK_NO_ENTRY) ? OK : status;
//            for(; status == OK && loop != NULL;
//                status = stk_getprevious( pstack, loop, &loop ))
            for(; status == OK && loop != NULL; )
            {
                status = stk_popentry( pstack, &loop );
            }
        }
    }
    else
    {
        status = E_STK_INVALID_STACK;
    }

    return(status);
}


/*
** Name: igen_getstart
**
** Description:
**      Gets the name of the tag in an ICE EXTEND tag.
**
** Inputs:
**      pstack  Pointer to the stack to insert into.  Returned from
**              call to stk_alloc.
**      flags   Flag value (Should be EXTEND!)
**      level   Indicates the level in the heirarchy of the entry to be found
**
** Outputs:
**      tagName Name of the tag found.
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**
** History:
**      13-Mar-2001 (peeje01)
**          Created.
*/
STATUS
igen_getstart( PSTACK pstack, u_i4 flags, u_i4 level, char* tagName )
{
    STATUS status = OK;
    PSTACKENTRY loop;
    PICEGENLN   iceline;
    u_i4 dlen;
    char *stkdata;

    if (pstack != NULL)
    {
        if ((status = stk_gettail( pstack, &loop )) == OK)
        {
            for(status = stk_getprevious( pstack, loop, &loop );
                status == OK && loop != NULL;
                status = stk_getprevious( pstack, loop, &loop ))
            {
                if ((status = stk_getdata( loop, (VOID **)&iceline )) == OK)
                {
                    if (iceline->flags & flags && iceline->nestlevel == level)
                    {
                        stkdata = ICE_LN_DATA( iceline );
                        dlen = iceline->linelen;
                        MEMCOPY(stkdata, dlen, tagName);
                    }
                }
            }
        }
    }
    else
    {
        status = E_STK_INVALID_STACK;
    }

    return(status);
}

/*
** Name: igen_getparent
**
** Description:
**      Returns the parent of the current line.
**
** Inputs:
**      igenln                  Get parent of this line.
**
** Outputs:
**      parent                  Pointer to parent.
**
** Return:
**      OK                      Command completed successfully.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      08-Feb-2001 (fanra01)
**          Created.
*/
STATUS
igen_getparent( PICEGENLN igenln, PICEGENLN* parent )
{
    STATUS status = E_IGEN_INVALID_PARAM;

    if (igenln != NULL)
    {
        *parent = igenln->parent;
        status = (parent == NULL) ? FAIL : OK;
    }
    return(status);
}

/*
** Name: igen_setflags
**
** Description:
**      Sets the flags of the current line.
**
** Inputs:
**      igenln                  Set flags of this line.
**      flags                   Flags values to set.
**
** Outputs:
**      None.
**
** Return:
**      OK                      Command completed successfully.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      08-Feb-2001 (fanra01)
**          Created.
*/
STATUS
igen_setflags( PICEGENLN igenln, u_i4 flags )
{
    STATUS status = E_IGEN_INVALID_PARAM;

    if (igenln != NULL)
    {
        igenln->flags |= flags;
        status = OK;
    }
    else
    {
    	status = FAIL;
    }
    return(status);
}

/*
** Name: igen_clrflags
**
** Description:
**      Clear the specified flags of the current line.
**
** Inputs:
**      igenln                  Clear flags of this line.
**      flags                   Flags values to reset.
**
** Outputs:
**      None.
**
** Return:
**      OK                      Command completed successfully.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      08-Feb-2001 (fanra01)
**          Created.
*/
STATUS
igen_clrflags( PICEGENLN igenln, u_i4 flags )
{
    STATUS status = E_IGEN_INVALID_PARAM;

    if (igenln != NULL)
    {
        igenln->flags &= ~flags;
        status = OK;
    }
    else
    {
    	status = FAIL;
    }
    return(status);
}
/*
** Name: igen_testflags
**
** Description:
**      Tests the setting of a named flag in the current line. Multiple
**      flags can be tested at once by 'or'ing them together and passing
**      the result in. But no indication of which flag actually matched will
**      be returned.
**
** Inputs:
**      igenln                  Test the flag setting in this line.
**      flags                   Flag value to test.
**
** Outputs:
**      None.
**
** Return:
**      OK                      Flag set, command completed successfully.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      21-Feb-2001 (peeje01)
**          Created.
*/
STATUS
igen_testflags( PICEGENLN igenln, u_i4 flags )
{
    STATUS status = FAIL;

    if (igenln != NULL)
    {
	if (igenln->flags & flags) status = OK;
    }
    return(status);
}
