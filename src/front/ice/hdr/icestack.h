/*
** Copyright (c) 2004 Ingres Corporation
** 
** History
**  29-Oct-2001 (peeje01)
**     BUG 105899
**     Fix up round brackets around MEMPARA macro, this was causing
**     the space calculation to go awry
*/
# ifndef stack_h_INCLUDED
# define stack_h_INCLUDED

# include <compat.h>
# include <me.h>
# include <si.h>
# include <st.h>

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

# define E_STK_INVALID_STACK    E_WU001E_INVALID_STACK
# define E_STK_INVALID_PARAM    E_WU001F_INVALID_PARAMETER
# define E_STK_SIZE_UNAVAILABLE E_WU0020_SIZE_UNAVAILABLE
# define E_STK_INVALID_ENTRY    E_WU0021_INVALID_ENTRY
# define E_STK_NO_ENTRY         E_WU0022_NO_ENTRY

#define STKENTRYSIZE( size )    (sizeof(STACKENTRY) + size)

# define MEMPARA( bytes ) ((bytes & 0x0000000F) ? \
    ((bytes >> 4) & 0x0FFFFFFF) + 1 : ((bytes >> 4) & 0x0FFFFFFF))

typedef struct _tag_stack       STACK, *PSTACK;
typedef struct _tag_stk_entry   STACKENTRY, *PSTACKENTRY;

typedef STATUS (*STKHANDLER)( PSTACK, PSTACKENTRY );

struct _tag_stk_entry
{
    u_i4            startmark;  /* identifies start of an entry header*/
    u_i4            next;       /* neighboring entry before this one */
    u_i4            previous;   /* neighboring entry after this one */
    u_i4            length;     /* size of entry including this header */
    u_i4            nest;       /* number of levels of nesting */
    u_i4            ip;         /* who owns this memory? */
    u_i4            endmark;    /* identifies end of an entry header*/
};

struct _tag_stack
{
    u_i4            stksize;
    u_i4            freeentry;  /* start paragraph of next free stack entry */
    u_i4            end;        /* end paragraph of stack */
    u_i4            remain;     /* remaining paragraph count */
    u_i4            nest;       /* current nesting */
    STKHANDLER*     handlers;
    PSTACKENTRY     entrylist;
    PSTACKENTRY     lastentry;
};

#ifdef __cplusplus
extern "C"
{
#endif
/*
** Stack functions to handle the stack
*/
FUNC_EXTERN STATUS
stk_alloc( u_i4 stksize, STKHANDLER* ftable, PSTACK* stkptr );

FUNC_EXTERN STATUS
stk_free( PSTACK stkptr );

FUNC_EXTERN STATUS
stk_pushentry( PSTACK stkptr, u_i4 entrysize, PSTACKENTRY* stkentry );

FUNC_EXTERN STATUS
stk_popentry( PSTACK stkptr, PSTACKENTRY* stkentry );

/*
** Stack functions to handle an entries
*/
FUNC_EXTERN STATUS
stk_gethead( PSTACK stkptr, PSTACKENTRY* stkentry );

FUNC_EXTERN STATUS
stk_gettail( PSTACK stkptr, PSTACKENTRY* stkentry );

FUNC_EXTERN STATUS
stk_getnext( PSTACK stkptr, PSTACKENTRY stkentry, PSTACKENTRY* stknext );

FUNC_EXTERN STATUS
stk_getprevious( PSTACK stkptr, PSTACKENTRY stkentry, PSTACKENTRY* stkprev );

FUNC_EXTERN STATUS
stk_getdata( PSTACKENTRY stkentry, VOID** stkdata );

STATUS
stk_getlength( PSTACKENTRY stkentry, u_i4* entrylen );

#ifdef __cplusplus
}
#endif

# endif /* stack_h_INCLUDED */
