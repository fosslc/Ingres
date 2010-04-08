/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
** equtils.h
**	- debug, eqid, q, label, and str
**
**  History:
**	04-Mar-96 (thoda04)
**	    Added function prototypes and function prototype arguments.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Kill some sort of bizarre redeclaration of q_dump().
*/

/********************** begin debug utility **********************************/
/*
**	- A general purpose debug utility.
**	     #define xdbDEBUG 1
**	     #include "equtils.h"
**	   - dbSETBUG(x) will set the level of debug to x.
**	   - dbDEBUG( y,(--general SIprintf stuff--) );
**	     is used for debug statements.
**	   - The first arg (y) is the debug level needed to activate this
**	     statement.  The second arg is the printf string and args;
**	     note that the parens are required.  If all of the bits in the
**	     first arg are on in the debug level then the second arg will
**	     be printed.
**	   - Debug output goes to the dump file (eq->eq_dumpfile).
*/

/* Because not all systems allow the -D flag on the CC line define here  */
# define xdbDEBUG		/* To undefine open remove the delimiter ^^ */

#ifdef	xdbDEBUG
 GLOBALREF i4  	_db_debug;
# define	dbSETBUG(x)	_db_debug = x
# define	dbDEBUG(x,y)	if (((x) & _db_debug) == (x))	syPrintf y; else
# define	dbBUGRAW()
# define	dbBUGBUF()

# define  xdbLEV1	(i4)(1<<0)
# define  xdbLEV2	(i4)(1<<1)
# define  xdbLEV3	(i4)(1<<2)
# define  xdbLEV4	(i4)(1<<3)
# define  xdbLEV5	(i4)(1<<4)
# define  xdbLEV6	(i4)(1<<5)
# define  xdbLEV7	(i4)(1<<6)
# define  xdbLEV8	(i4)(1<<7)
# define  xdbLEV9	(i4)(1<<8)
# define  xdbLEV10	(i4)(1<<9)
# define  xdbLEV11	(i4)(1<<10)
# define  xdbLEV12	(i4)(1<<11)
# define  xdbLEV13	(i4)(1<<12)
# define  xdbLEV14	(i4)(1<<13)
# define  xdbLEV15	(i4)(1<<14)
# define  xdbLEV16	(i4)(1<<15)
# define  xdbLEV17	(i4)(1<<16)
# define  xdbLEV18	(i4)(1<<17)
# define  xdbLEV19	(i4)(1<<18)
# define  xdbLEV20	(i4)(1<<19)
# define  xdbLEV21	(i4)(1<<20)
# define  xdbLEV22	(i4)(1<<21)
# define  xdbLEV23	(i4)(1<<22)
# define  xdbLEV24	(i4)(1<<23)
# define  xdbLEV25	(i4)(1<<24)
# define  xdbLEV26	(i4)(1<<25)
# define  xdbLEV27	(i4)(1<<26)
# define  xdbLEV28	(i4)(1<<27)
# define  xdbLEV29	(i4)(1<<28)
# define  xdbLEV30	(i4)(1<<29)
# define  xdbLEV31	(i4)(1<<30)
# define  xdbLEV32	(i4)(1<<31)

#else	/* xdbDEBUG */
# define	dbSETBUG(x)
# define	dbDEBUG(x,y)
# define	dbBUGRAW()
# define	dbBUGBUF()
#endif	/* xdbDEBUG */
/********************** end debug utility ************************************/

/********************** begin eqid utility **********************************/
/* Maximum representation for Equel identifier usages */
# define 	ID_MAXLEN  255

/*
** Equel ID routines.
*/
char	*id_getname(void);
void	id_add(char *data);
void	id_key(char *data);
void	id_free(void);
    void	id_dump(char *calname);
/********************** end eqid utility *************************************/

/********************** begin label utility **********************************/
/*
**	- Defined names and routine declarations for label.c,
**	  the EQUEL label manager.
**
** History:
**	Jan 21, 1985	mrw	Written
**	Apr 18, 1991    teresal Add support for activate event block.
**	28-feb-00 (inkdo01)
**	    Add LBLmEXECLOOP for execute procedure loop exit.
*/

/*
** stack selectors
*/

# define	LBL_RET		0
# define	LBL_FORM	1
# define	LBL_TBL		2
# define	LBL_INTR	3

/*
** mode values
*/

# define	LBLmNONE	000	/* No active loop statement */
# define	LBLmDISPLAY	001
# define	LBLmRETLOOP	002
# define	LBLmEXECLOOP	003
# define	LBLmFORMDATA	004
# define	LBLmTBDATA	010
# define	LBLmTBUNLD	020
# define	LBLmTBLOOP	030	/* Either TBDATA or TBUNLD */
# define	LBLmSUBMENU	040

/*
** adjective values
*/

# define	LBLaNONE	0	/* A plain display */
# define	LBLaDISPLAY	1	/* In DISPLAY outer block */
# define	LBLaNESTMU	2	/* In a nested menu */
# define	LBLaSCROLL	3	/* In ACTIVATE SCROLL UP/DOWN block */
# define	LBLaMITEM	4	/* In ACTIVATE MENUITEM block */
# define	LBLaFRSKEY	5	/* In ACTIVATE FRSKEY block */
# define	LBLaFIELD	6	/* In ACTIVATE FIELD block */
# define	LBLaCOLUMN	7	/* In ACTIVATE COLUMN block */
# define	LBLaCOMMAND	8	/* In ACTIVATE COMMAND block */
# define	LBLaTIME	9	/* In ACTIVATE TIMEOUT block */
# define	LBLaEVENT	10	/* In ACTIVATE EVENT block */

/*
** invalid level definition
*/

# define	LBL_NOLEV	(-1)

/*
** external declarations
*/

void	lbl_init(void);
void	lbl_enter_block(void);
void	lbl_exit_block(void);
void	lbl_exit_block(), lbl_used();
void    lbl_a_reset(void);
i4  	lbl_next(i4 which);
i4  	lbl_current(i4 which);
void	lbl_set_mode(i4 which);
void	lbl_used(i4 which);
bool    lbl_is_used(i4 which);
i4      lbl_gen(i4 mask, bool do_gen);
i4  	lbl_value(i4 mode, i4  level);
i4  	lbl_which(i4 mode);
i4  	lbl_get_mode(i4 level);
i4  	lbl_adget(i4 level);
VOID	lbl_adset(i4 level, i4  adj);
/********************** end label utility ************************************/

/********************** begin str utility ************************************/
/*
** Structures and routines to use with the string table manager.
*/

/* Single buffer in (possible) list of buffers of parent string table */
typedef struct stbuf
{
	char		*bf_data;		/* Buffer area */
	char		*bf_end;		/* End of buffer */
	struct stbuf	*bf_next;		/* Next buffer on list */
} STR_BUF;

# define    SBNULL	(STR_BUF *)0		/* Null buffer pointer */

/* Real string table known to users */
typedef struct 
{
    i4			s_size;			/* Size of data area */
    char		*s_ptr;			/* Pointer into data buffer */
    STR_BUF		s_buffer;		/* First of buffer(s) */
    STR_BUF		*s_curbuf;		/* Current buffer on list */
} STR_TABLE;

# define    STRNULL	(STR_TABLE *)0		/* Null string table */


/*
** When using local buffers together with the string table manager use
** this macro to set it up for string table calls.
**
** Parameters:		buf	Local buffer
**			st	Name of the string table you get
**
** The macro assumes that the first element in STR_BUF is the real data area.
*/
# define    str_define( buf, st )   		  		       	\
	    STR_TABLE  st = {				    	    	\
			     sizeof(buf),  /* Caller's size */         	\
			     buf, 	   /* Current data pointer */  	\
			       { buf, 	   /* STR_BUF: Buffer head */   \
			         buf+sizeof(buf),   /* Buffer end */    \
			         SBNULL },          /* Next buffer */   \
			     &st.s_buffer /* Current buf pointer */ 	\
			    }

/* 
** String managing routines
*/
STR_TABLE	*str_new(i4 size);
char		*str_add(STR_TABLE *st, char *string);
char		*str_space(STR_TABLE *st, i4  size);
char		*str_mark(STR_TABLE *st);
void		str_chadd(STR_TABLE *st, char ch);
void		str_free(STR_TABLE *st, char *bufptr);
void		str_chget(/* STR_TABLE *st, char *bufptr, i4  (*chfunc)() */);
void		str_reset(void);
    void	str_dump(STR_TABLE *st, char *stname);
/********************** end str utility **************************************/

/********************** begin q utility **************************************/
/* 
** Queue manager structures.
**
** Requires that all Queue users have Q_ELM * as the first field in the
** queued structure.
*/

typedef struct q_obj {
	struct q_obj 	*q_next;		/* Dummy object used a link */
} Q_ELM;

# define 	QNULL	(Q_ELM *)0

typedef struct {				/* Queue known to user */
	Q_ELM	*q_head;			/* Head of queue */
	Q_ELM	*q_tail;			/* Tail of queue */
	Q_ELM	*q_frlist;			/* Free list of queue */
	i4	q_size;				/* Size of each q element */
} Q_DESC;

# define	QDNULL	(Q_DESC *)0

/* 
** Macros for First and Follow within a queue. 
** To avoid casting complaints pass the result type for text inclusion. 
** An example usage may be:
**
** typedef struct {
**	Q_ELM	t_q;
**	char	*t_name;
** } T_OBJ;
**
** T_OBJ	*t;
** QDESC	*q, *freelist;
**
** t = q_follow( (T_OBJ *), t );
**
** Note the parens around the passed type.
*/
# define	q_first( cast, qdesc )	\
				cast(((Q_DESC *)(qdesc))->q_head)
# define	q_follow( cast, qelm )	\
				cast(((Q_ELM *)(qelm))->q_next)

/*
** Macro to define allocation of new elements. 
** Example usage based on T_OBJ described above:
**
** q_new( (T_OBJ *), tq );
**
** Note the parens around the passed type.
*/
# define	q_new( cast, elm, qdesc )	\
				elm = cast(q_newelm(qdesc))

/*
** Queue manager routines
*/

void	q_init(Q_DESC *qdesc, i4  qelmsize);
void	q_enqueue(/* Q_DESC *qdesc, Q_ELM *qelm */);
void	q_rmqueue(/* Q_DESC *qdesc, Q_ELM **qelm */);	/* Rarely used and not casted */
void	q_walk(Q_DESC *qdesc, i4  (*qfunc)());
void	q_free(Q_DESC *qdesc, i4  (*qfunc)());
Q_ELM	*q_newelm(Q_DESC *qdesc);
void	q_dump(Q_DESC *qdesc, char *qname);
/********************** end q utility ****************************************/
