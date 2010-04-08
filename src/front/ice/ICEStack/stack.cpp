/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: stack.c
**
** Description:
**      Module for handling a stack of memory for the ability to maintain
**      context through a number of layers.
**
**      The stack allocates a stack header and stamps the first entry onto
**      the stack ready for the first allocation.  The stack functions
**      allocates buffers in 16 byte blocks and uses paragraph (mod 16)
**      arithmetic.
**
**      stk_alloc       Create a stack of specified size
**      stk_free        Free the memory allocated for the stack
**      stk_pushentry   Request a block of memory from the stack
**      stk_popentry    Return the block of memory to the stack
**      stk_gethead     Get a pointer to the start of the stack
**      stk_getnext     Get the next stack entry from the current
**      stk_getprevious Get the previous stack entry from the current
**      stk_getdata     Get a pointer to the start of the data block
**      stk_getlength   Get the length of the data
**
** History:
**      08-Feb-2001 (fanra01)
**          Created.
**      01-Jun-2001 (peeje01)
**          Moving to Solaris:
**          Wrap up NT specific stuff in NT_GENERIC ifdefs
**	05-jul-2001 (somas01)
**	    Turnoff stack stuff for NT_IA64 until it is ported.
**      30-Oct-2001 (peeje01)
**          BUG 105899
**          Stack was running over its end. Leave safety buffer so
**          room is left for end of stack marker
**	02-oct-2003 (somsa01)
**	    Do the same for NT_AMD64 as is for NT_IA64.
*/
# include <icestack.h>

#define FILLER  0xFF

static u_i4
EXgetReturnAddress( void );

static u_i4
EXgetCallingAddress( void );

/*
** Name: stk_alloc
**
** Description:
**      Create a stack of specified size.
**
** Inputs:
**      stksize     Size of stack memory to allocate
**      ftable      Option function table pointer. (reserved)
**
** Outputs:
**      stkptr      Pointer to the start of the stack structure.
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     Invalid stack pointer supplied for return.
**
** History:
**      08-Feb-2001 (fanra01)
**          Created.
*/
STATUS
stk_alloc( u_i4 stksize, STKHANDLER* ftable, PSTACK* stkptr )
{
    STATUS      status;
    PSTACK      sstart = NULL;
    PSTACKENTRY estart = NULL;

    if (stkptr != NULL)
    {
        *stkptr = NULL;

        MEMALLOC( sizeof(STACK), PSTACK ,TRUE, status, sstart );
        if (sstart != NULL)
        {
            u_i4 entrysize = (sizeof(STACKENTRY) << 1) + stksize;
            entrysize = MEMPARA( entrysize );
            entrysize <<= 4;
            MEMALLOC( entrysize, PSTACKENTRY, TRUE, status, estart );
            if (estart == NULL)
            {
                MEMFREE( sstart );
            }
            else
            {
                sstart->stksize = stksize;
                sstart->entrylist = estart;
                sstart->freeentry = 0;
                sstart->end = MEMPARA( entrysize );
                sstart->remain = MEMPARA( entrysize );
                sstart->nest = 0;
                sstart->handlers = ftable;

                estart->startmark = 0xAAAAAAAA;
                estart->next = sstart->end;
                estart->previous = 0xFFFFFFFF;
                estart->length = stksize;
                estart->nest = 0;
                estart->ip = 0;
                estart->endmark = 0xCCCCCCCC;

                *stkptr = sstart;
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
** Name: stk_free
**
** Description:
**      Free the memory allocated for the stack.
**
** Inputs:
**      stkptr      Pointer to the stack, returned from a previous call to
**                  stk_alloc.
**
** Outputs:
**      None.
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     Invalid stack pointer supplied.
**
** History:
**      08-Feb-2001 (fanra01)
**          Created.
*/
STATUS
stk_free( PSTACK stkptr )
{
    STATUS status = OK;

    if (stkptr != NULL)
    {
        MEMFREE( stkptr->entrylist );
        MEMFREE( stkptr );
    }
    else
    {
        status = E_STK_INVALID_STACK;
    }
    return(status);
}

/*
** Name: stk_pushentry
**
** Description:
**      Request a block of memory from the stack.
**
** Inputs:
**      stkptr      Pointer to the stack, returned from a previous call to
**                  stk_alloc.
**      entrysize   The size of the entry to be returned.
**
** Outputs:
**      stkentry    Entry on the stack to pop.
**                  The stack MUST be unwound in the correct order.
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     Invalid stack pointer supplied.
**      E_STK_INVALID_PARAM     Invalid stack entry pointer.
**      E_STK_SIZE_UNAVAILABLE  Insuffient memory in the stack to fulfil
**                              request.
**
** History:
**      08-Feb-2001 (fanra01)
**          Created.
*/
STATUS
stk_pushentry( PSTACK stkptr, u_i4 entrysize, PSTACKENTRY* stkentry )
{
    STATUS      status = OK;
    u_i4        esize = STKENTRYSIZE( entrysize );
    PSTACKENTRY entry;
    u_i4        dlen;
    char*       data;

    if (stkptr != NULL)
    {
        if (stkentry != NULL)
        {
            if ((MEMPARA( esize ) + 2 ) <= stkptr->remain)
            {
                stkptr->nest += 1;

                entry = (PSTACKENTRY)
                    ((char *)stkptr->entrylist + (stkptr->freeentry << 4));
                entry->next = MEMPARA( esize );
                stkptr->remain -= entry->next;
                stkptr->freeentry += entry->next;
                stkptr->lastentry = entry;

                entry->length = entrysize;
                entry->nest = stkptr->nest;
                data = (char *)entry + sizeof(STACKENTRY);
                dlen = MEMPARA( entrysize );
                dlen <<= 4;
                MEMFILL( dlen, 0, data );

                entry->ip = EXgetCallingAddress();
                *stkentry = entry;

                entry = (PSTACKENTRY)
                    ((char *)stkptr->entrylist + (stkptr->freeentry << 4));
                entry->startmark = 0xAAAAAAAA;
                entry->next = 0xBBBBBBBB;
                entry->previous = MEMPARA( esize );
                entry->length = (stkptr->end - stkptr->freeentry) << 4;
                entry->endmark = 0xCCCCCCCC;
            }
            else
            {
                status = E_STK_SIZE_UNAVAILABLE;
            }
        }
        else
        {
            status = E_STK_INVALID_PARAM;
        }
    }
    else
    {
        status = E_STK_INVALID_STACK;
    }
    return(status);
}

/*
** Name: stk_popentry
**
** Description:
**      Return the block of memory to the stack.
**
** Inputs:
**      stkptr      Pointer to the stack, returned from a previous call to
**                  stk_alloc.
**      stkentry    Entry on the stack to pop.
**                  The stack MUST be unwound in the correct order.
**
** Outputs:
**      stkentry    The previous entry.
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     Invalid return pointer supplied.
**      E_STK_INVALID_ENTRY     Invalid stack entry pointer.
**
** History:
**      08-Feb-2001 (fanra01)
**          Created.
*/
STATUS
stk_popentry( PSTACK stkptr, PSTACKENTRY* stkentry )
{
    STATUS status = OK;
    PSTACKENTRY prev;
    PSTACKENTRY next;
    u_i4        dlen;
    char*       data;

    if (stkptr != NULL)
    {
        if ((stkentry != NULL) && (*stkentry != NULL))
        {
            if (stkptr->nest > 0)
            {
                stkptr->nest -= 1;

                data = (char *)*stkentry + sizeof(STACKENTRY);
                dlen = (MEMPARA((*stkentry)->length)) << 4;
                MEMFILL( dlen, 0, data );

                next = (PSTACKENTRY)((char *)(*stkentry) + ((*stkentry)->next << 4));

                if((*stkentry)->previous != 0xFFFFFFFF)
                {
                    prev = (PSTACKENTRY)
                        ((char *)((*stkentry)) - ((*stkentry)->previous << 4));
                    prev->next += (*stkentry)->next;

                    next->previous += (*stkentry)->previous;

                    MEMFILL( sizeof(STACKENTRY), FILLER, (*stkentry) );

                    *stkentry = prev;
                    stkptr->lastentry = prev;
                }
                else
                {
                    MEMFILL( sizeof(STACKENTRY), FILLER, next );

                    stkptr->freeentry = 0;
                    stkptr->remain = stkptr->end;
                    stkptr->nest = 0;
                    stkptr->lastentry = *stkentry;

                    (*stkentry)->next = stkptr->end;
                    (*stkentry)->length = stkptr->stksize;
                    (*stkentry)->nest = 0;
                }
                (*stkentry)->ip = EXgetCallingAddress();
            }
            else
            {
                status = E_STK_NO_ENTRY;
            }
        }
        else
        {
            status = E_STK_INVALID_ENTRY;
        }
    }
    else
    {
        status = E_STK_INVALID_STACK;
    }
    return(status);
}

/*
** Name: stk_gethead
**
** Description:
**      Get a pointer to the start of the stack.
**
** Inputs:
**      stkptr      Pointer to the stack, returned from a previous call to
**                  stk_alloc.
**
** Outputs:
**      stkentry    Entry at the top of the stack.
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     Invalid return pointer supplied.
**
** History:
**      08-Feb-2001 (fanra01)
**          Created.
*/
STATUS
stk_gethead( PSTACK stkptr, PSTACKENTRY* stkentry )
{
    STATUS status = OK;
    if (stkptr != NULL)
    {
        *stkentry = stkptr->entrylist;
    }
    else
    {
        status = E_STK_INVALID_STACK;
    }
    return(status);
}

/*
** Name: stk_gettail
**
** Description:
**      Get a pointer to the last entry on the stack.
**
** Inputs:
**      stkptr      Pointer to the stack, returned from a previous call to
**                  stk_alloc.
**
** Outputs:
**      stkentry    Entry at the bottom of the stack.
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     Invalid return pointer supplied.
**
** History:
**      08-Feb-2001 (fanra01)
**          Created.
*/
STATUS
stk_gettail( PSTACK stkptr, PSTACKENTRY* stkentry )
{
    STATUS status = OK;
    PSTACKENTRY entry;

    if (stkptr != NULL)
    {
        if (stkentry != NULL)
        {
            *stkentry = stkptr->lastentry;
        }
        else
        {
            status = E_STK_INVALID_ENTRY;
        }
    }
    else
    {
        status = E_STK_INVALID_STACK;
    }
    return(status);
}

/*
** Name: stk_getnext
**
** Description:
**      Get the next stack entry from the current.
**
** Inputs:
**      stkentry    Current entry to use as reference.
**
** Outputs:
**      stknext     Returned stack entry.
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_ENTRY     Invalid entry pointer supplied.
**      E_STK_INVALID_PARAM     The pointer for return is not valid.
**      E_STK_NO_ENTRY          No following entry to return.
**
** History:
**      08-Feb-2001 (fanra01)
**          Created.
*/
STATUS
stk_getnext( PSTACK stkptr, PSTACKENTRY stkentry, PSTACKENTRY* stknext )
{
    STATUS status = E_STK_INVALID_PARAM;
    PSTACKENTRY next;

    if (stkentry != NULL)
    {
        if (stknext != NULL)
        {
            if (stkentry->next < stkptr->end)
            {
                next = (PSTACKENTRY)
                    ((char *)stkentry + (stkentry->next << 4));
                if (next->next != 0xBBBBBBBB)
                {
                    *stknext = next;
                    status = OK;
                }
                else
                {
                    status = E_STK_NO_ENTRY;
                }
            }
            else
            {
                status = E_STK_NO_ENTRY;
            }
        }
    }
    else
    {
        status = E_STK_INVALID_ENTRY;
    }

    return(status);
}

/*
** Name: stk_getprevious
**
** Description:
**      Get the previous stack entry from the current.
**
** Inputs:
**      stkentry    Current entry to use as reference.
**
** Outputs:
**      stkprev     Returned stack entry.
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_ENTRY     Invalid entry pointer supplied.
**      E_STK_INVALID_PARAM     The pointer for return is not valid.
**      E_STK_NO_ENTRY          No previous entry to return.
**
** History:
**      08-Feb-2001 (fanra01)
**          Created.
*/
STATUS
stk_getprevious(  PSTACK stkptr, PSTACKENTRY stkentry, PSTACKENTRY* stkprev )
{
    STATUS status = E_STK_INVALID_PARAM;
    PSTACKENTRY prev;

    if (stkentry != NULL)
    {
        if (stkprev != NULL)
        {
            if (stkptr->nest > 0)
            {
                if (stkentry->previous != 0xFFFFFFFF)
                {
                    prev = (PSTACKENTRY)
                        ((char *)stkentry - (stkentry->previous << 4));
                    *stkprev = prev;
                    status = OK;
                }
                else
                {
                    status = E_STK_NO_ENTRY;
                }
            }
            else
            {
                status = E_STK_NO_ENTRY;
            }
        }
    }
    else
    {
        status = E_STK_INVALID_ENTRY;
    }

    return(status);
}

/*
** Name: stk_getdata
**
** Description:
**      Get a pointer to the start of the data block.
**
** Inputs:
**      stkentry    Current entry to use as reference.
**
** Outputs:
**      stkdata     Pointer to the data area contained in the block.
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_PARAM     The pointer for return is not valid.
**      E_STK_INVALID_ENTRY     Invalid entry pointer supplied.
**
** History:
**      08-Feb-2001 (fanra01)
**          Created.
*/
STATUS
stk_getdata( PSTACKENTRY stkentry, VOID** stkdata )
{
    STATUS status = E_STK_INVALID_PARAM;
    char*  next;

    if (stkentry != NULL)
    {
        if (stkdata != NULL)
        {
            *stkdata = NULL;
            next = (char *)stkentry + sizeof(STACKENTRY);
            *stkdata = next;
            status = OK;
        }
    }
    else
    {
        status = E_STK_INVALID_ENTRY;
    }
    return(status);
}

/*
** Name: stk_getlength
**
** Description:
**      Get the length of the data.
**
** Inputs:
**      stkentry    Current entry to use as reference.
**
** Outputs:
**      entrylen    Length passed in for the stk_pushentry.
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_PARAM     The pointer for return is not valid.
**      E_STK_INVALID_ENTRY     Invalid entry pointer supplied.
**
** History:
**      08-Feb-2001 (fanra01)
**          Created.
*/
STATUS
stk_getlength( PSTACKENTRY stkentry, u_i4* entrylen )
{
    STATUS status = E_STK_INVALID_PARAM;

    if (stkentry != NULL)
    {
        if (entrylen != NULL)
        {
            *entrylen = stkentry->length;
            status = OK;
        }
    }
    else
    {
        status = E_STK_INVALID_ENTRY;
    }

    return(status);
}

/*
** Name: EXgetReturnAddress
**
** Description:
**      Get the address of the instruction that the caller will return to.
**      i.e.
**          func1()
**          {
**              call func2();
**              some instruction;  <--- return from func2
**          }
**
**          func2()
**          {
**              printf( "Return to addr = %p\n", EXgetReturnAddress() );
**          }
**      Will print the address of some instruction in func1.
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      Address to where the invoking function will return.
**
** Assumptions:
**      Standard C calling convention.
**      32-bit instruction set.
**      The ebx register need not be preserved within the function.
**      The eax register is used for return values.
*/
static u_i4
EXgetReturnAddress()
{
# if defined(NT_GENERIC) && !defined(NT_IA64) && !defined(NT_AMD64)
    __asm
    {
        mov     ebx,    [ebp];
        mov     eax,    [ebx + 4];
    }
# endif
    return(0);
}

/*
** Name: EXgetCallingAddress
**
** Description:
**      Get the address of the instruction that called the function
**      i.e.
**          func1()
**          {
**              call func2();      <--- address from where function invoked
**              some instruction;
**          }
**
**          func2()
**          {
**              printf( "Return to addr = %p\n", EXgetCallingAddress() );
**          }
**      Will print the address of call func2 instruction in func1.
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      Address of the call instruction.
**
** Assumptions:
**      Standard C calling convention.
**      32-bit instruction set.
**      The ebx register need not be preserved within the function.
**      The eax register is used for return values.
*/
static u_i4
EXgetCallingAddress()
{
# if defined(NT_GENERIC) && !defined(NT_IA64) && !defined(NT_AMD64)
    __asm
    {
        mov     ebx,    [ebp];
        mov     eax,    [ebx + 4];
        sub     eax,    5;
    }
# endif
    return(0);
}
