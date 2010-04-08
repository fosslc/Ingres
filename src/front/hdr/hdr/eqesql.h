/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <eqsym.h>

/*
+* Filename:	EQESQL.H
** Purpose:	Defines objects and constants for embedded SQL support.
**
** Defines:	
**	1. Type SQLCA_ELM, and its usages (for SQLCA management)
**	2. struct esq_struct	- ESQL specific support structure.
-*
** History:
**	09-aug-1985	- Written for ESQL (ncg)
**	14-dec-1989	- Updated for Phoenix/Alerters (bjb)
**	18-nov-1992 (larrym)
**	    added SQLCODE & SQLSTATE support.  Added new functions gen_sqlcode
**	    and gen_diaginit (Diagnostic Area Initialize).
**	18-nov-1992 (larrym)
**	    added definition of esq_struct to allow esq_init to check for
**	    inclusion of the SQLCA.
**      12-Dec-95 (fanra01)
**          Changed extern to GLOBALREF and added definitions for referencing
**          data in DLLs on windows NT from code built for static libraries.
**      06-Feb-96 (fanra01)
**          Changed flag from __USE_LIB__ to IMPORT_DLL_DATA.
**	06-mar-1996 (thoda04)
**	    added function prototypes and argument prototypes.
**    20-mar-1996 (kamal03)
**          changed # include "eqsym.h" to # include <eqsym.h>
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/************************* esqlca definitions ***************************/

/*
** SQLCA_ELM - Structure to manage the information used by the preprocessor to
**	       set up run-time usage of the SQLCA structure.
*/
typedef struct {
	i4	sq_action;		/* Stop, Continue, Goto, Call */
	char	*sq_label;		/* Label, Function or Null */
} SQLCA_ELM;

/* Entries for the routine esqlca */
# define sqcaNOTFOUND		0	/* First 5 are also used an indices */
# define sqcaERROR		1	/*   in code gens */
# define sqcaWARNING		2
# define sqcaMESSAGE		3	/* stored proc returning a msg */
# define sqcaEVENT		4	/* Event information returned */
# define sqcaDEFAULT		5	/* Probably internal error */
# define sqcaINIT		6	/* Reset on EOF before next file */

/* Actions to take on particular entries */
# define sqcaCONTINUE		0	/* Values to go into action field */
# define sqcaSTOP		1
# define sqcaGOTO		2
# define sqcaCALL		3

/* External definition of the SQLCA state */
# if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
GLOBALDLLREF	SQLCA_ELM	esqlca_store[];
# else              /* NT_GENERIC && IMPORT_DLL_DATA */
GLOBALREF	SQLCA_ELM	esqlca_store[];
# endif             /* NT_GENERIC && IMPORT_DLL_DATA */

void	esqlca(i4 what, i4  action, char *label);
	                                /* Save data for WHENEVER flags for SQLCA*/
i4  	esqlcaDump(/*void*/);       /* Dump esqlca_store[] */

/* Actions to take on gen_sqlca and for esq->inc */
# define sqcaSQL		0x01		/* Include file */
# define sqcaFRS		0x02		/* Some systems require this */
# define sqcaGEN		0x04		/* Dump sqlca calls */
# define sqcaSQLDA		0x08		/* gen sqlda */
# define sqcaSQPROC		0x10		/* exec-proc WHENEVER loop */
# define sqcaGETEVT		0x20		/* GET EVENT generation */

void	gen_sqlca(i4 flag);        /* Code generation for SQLCA and SQLDA */
void	gen_diaginit(void);         /* Generate Diagnostic Area init call */ 
void	gen_sqlcode(void);          /* Add address of SQLCODE to arg list */
i4  	gen_sqarg(void);            /* Add address of sqlca to arg list */

/*  
** Function prototypes for 
**    eqrecord.c - Handles record assignments and null indicator arrays.
*/
void	erec_setup(i4 first_time); /* Initialize */
void	erec_mem_add(char *mem_id, SYM *mem_sym, i4  mem_typ);
	                                /* Add a record member to the queue */
void	erec_ind_add(i4 ind_start,char *ind_id,char *ind_subs,SYM *ind_sym );
	                                /* Add indicator array to rec members */
void	erec_arg_add(char *arg);    /* Add a datahandler argument to rec mem */
void	erec_use(i4 func_code, i4  rep_flag, char *separator);
	                                /* Generate (or use) the list of rec mems*/
i4  	erec_vars(void);            /* Are there any record members stored? */
void	erec_dump(void);            /* Dump local data structures */
void	erec_desc(char *desc_id);   /* Save descrpt name or 
	                                   emit call to IIcsDaGet */


/*
** ESQL-specific support for the SQL statements that are embedded (not FRS).
**
** This information is only valid on a per-statement level, and should be
** zero-ed out after each statement. See GR_STMTFREE (in the slave grammer).
*/

# define	ESQ_STRMAX	255

struct esq_struct {
    i4          flag;           /* See flag bits below */
    i4          inc;            /* INCLUDE SQLCA or FRSCA */
    char        sbuf[ ESQ_STRMAX +1 ];   /* Extra string working buffer */
};
