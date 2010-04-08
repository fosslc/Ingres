/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <eqsym.h>

/**
+*  Name: eqstmt.h - Definitions for embedded utilities.
**
**  Description:
**	Defines constants and externals for the following utilities:
**		activate
**		cursor
**		query (db)
**		fips
**		frs
**		repeat
**		retrieve
**		select
-*
**  History:
**	01-jan-1984	- Created
**      09-aug-1990 (barbara)
**          Added a mode flag FRSload for use in eqfrs/eqgenfrs routines to
**          distinguish between code generation for SET/INQUIRE and that
**          code gen for WITH clause on INSERTROW and LOADTABLE.
**	22-apr-1991 (teresal)
**	    Add ACT_EVENT for activate event block.
**	04-aug-1992 (teresal)
**	    Added fips checking utility function prototypes (eqckfips.c).
**	16-nov-1992 (lan)
**	    Added DB_HDLR.
**	26-apr-1993 (lan)
**	    Added prototype for eqck_tgt.
**	04-Mar-96 (thoda04)
**	    Added function prototypes and function prototype arguments.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	6-feb-2007 (dougi)
**	    Add ECS_SCROLL, ECS_KEYSET flags for scrollable cursors.
**	26-Aug-2009 (kschendel) b121804
**	    Remove peculiar dbg-comment strangeness.
*/

/********************** begin act utility **********************************/
/* Definitions needed for Activate block manager */

# define ACT_BADTYPE (-1)
# define ACT_NOTYPE  0
# define ACT_COMMAND 1
# define ACT_FIELD   2
# define ACT_MENU    3
# define ACT_COLUMN  4
# define ACT_SCROLL  5
# define ACT_FRSKEY  6
# define ACT_TIMEOUT 7
# define ACT_EVENT   8

/*
** Activate object routines.
*/
void      act_open(void);
i4  	* act_var(SYM *var, char *id);
i4  	* act_string(char *string);
i4  	  act_command(char *ctl);
void      act_args(i4 add, char *action, char *name, i4  type, SYM	*var);
void      act_num(char *action, char *name, i4  type, SYM *var);
void      act_add(i4 type, i4  intrpval, i4  mode, i4  *actobj);
void      act_close(i4 form_no);


    void	act_dump(void);
    void	act_stats();

/********************** end act utility ***********************************/

/********************** begin db utility **********************************/
/* Database types that correspond to variables */
# define  DB_REG	0		/* Regular variable type */
# define  DB_ID		1		/* Database identifier - IIwrite */
# define  DB_NOTRIM	2		/* Do not trim blanks (non-C) */
# define  DB_STRING	3		/* Variable as a string */
# define  DB_HDLR	4		/* Datahandler */

/* Database routines */
void	db_key(char *kword);
void	db_op(char *op);
void	db_var(i4 dbtyp, SYM *var, char *varname, 
                         SYM *indsym, char *indname, char *arg);
void	db_close(i4 func);
void	db_send(void);
void    db_linesend(void);
void	db_attname(char *str, i4  num);
char	*db_sattname(char *str, i4  num);
void	db_sconst(char *str);
void    db_delimid(char *str);
void	db_hexconst(char *str);
void	db_uconst(char *str);
void	db_dtconst(char *str);
char	*form_sconst(char *str);	

/*}
** Name: EDB_QUERY_ID - A query id
**
** Description:
**      A query id is a timestamp assigned to a query by a pre-processor.
**	It also contains the query name, for error reporting purposes.
**	Like the backend DB_CURSOR_ID, but the name is nul-padded.
**
** History:
**     15-aug-86	- Written. (mrw)
**     22-jun-88	- Modified for longer names (for gateways). (ncg)
*/

# define	EDB_QRY_MAX	32

typedef struct
{
    i4              edb_qry_id[2];		 /* A timestamp is 2 i4s */
    char	    edb_qry_name[EDB_QRY_MAX+1]; /* Name of the cursor */
} EDB_QUERY_ID;

void	edb_unique(EDB_QUERY_ID *qry_id, char *qry_name);

# define EDB_QRYNAME(cid)	((cid)->edb_qry_name)
# define EDB_QRY1NUM(cid)	((cid)->edb_qry_id[0])
# define EDB_QRY2NUM(cid)	((cid)->edb_qry_id[1])

/********************** end db utility ************************************/

/********************** begin eqcursor utility ****************************/

/* CURSOR.c_flags bits, and csr_setmode argument */
# define	CSR_DEF		0	/* default */
# define	CSR_RDO		ECS_RDONLY
# define	CSR_UPD		ECS_UPDATE
# define	CSR_ERR		ECS_ERR

/* cursor flag bits */
# define ECS_ERR	0x00000001	/* Cursor had an error */
# define ECS_REPEAT	0x00000002	/* Cursor had a repeat clause */
# define ECS_UPDATE	0x00000004	/* Cursor is updatable */
# define ECS_DIRECTU	0x00000008	/* Cursor wants direct updates */
# define ECS_DEFERRU	0x00000010	/* Cursor wants deferred updates */
# define ECS_RDONLY	0x00000020	/* Cursor is open read-only */
# define ECS_UNIQUE	0x00000040	/* Cursor specified UNIQUE */
# define ECS_SORTED	0x00000080	/* Cursor specified SORT BY */
# define ECS_WHERE	0x00000100	/* Cursor had a WHERE clause */
# define ECS_UPDVAR	0x00000200	/* (Temp) Update list had a variable */
# define ECS_UPDLIST	0x00000400	/* Cursor had an update list */
# define ECS_DYNAMIC	0x00000800	/* Cursor is dynamic */
# define ECS_UNION	0x00001000	/* Cursor had a UNION of subselects */
# define ECS_GROUP	0x00002000	/* Cursor had a GROUP BY clause */
# define ECS_HAVING	0x00004000	/* CURSOR had a HAVING clause */
# define ECS_FUNCTION	0x00008000	/* CURSOR had a function in tgt-list */
# define ECS_SCROLL	0x00010000	/* CURSOR is scrollable */
# define ECS_KEYSET	0x00020000	/* CURSOR is keyset scrollable */
# define ECS_NONEXIST	0x80000000	/* CURSOR doesn't exist: ecs_gcsrbits */

/* for ecs_colupd: column flag bits */
# define	ECS_EXISTS	0x0001L	/* Column was defined (always) */
# define	ECS_EXPR	0x0002L	/* Column is result of an expression */
# define	ECS_UPD		0x0004L	/* Column is upatable */
# define	ECS_MANY	0x0008L	/* More than one column has this name */
# define	ECS_CONSTANT	0x0010L	/* Column is a constant */
# define	ECS_ISWILD	0x0020L	/* Column specified by a var or '*' */

/* ecs_colupd commands: these go at the end */
# define	ECS_ADD		0x8000L	/* Add this column name */
# define	ECS_CHK		0x4000L	/* Return flag bits for this column */

/* ecs_query options */
# define	ECS_START_QUERY		0
# define	ECS_STOP_QUERY		1
# define	ECS_END_QUERY		2

/*
** forward declarations
*/

void	ecs_declcsr(char *cs_name, SYM *cs_sym);
void	ecs_open(char *cs_name, char *ro_str, SYM *sy, i4  is_dyn);
void	ecs_close(char *cs_name, i4  is_dyn);
void	ecs_retrieve(char *cs_name, i4  is_dyn, i4 fetch_o, i4 fetch_c,
				SYM * fetch_s, char * fetch_v);
void	ecs_delete(char *cs_name, char *tbl_name, i4  is_dyn);
void	ecs_replace(i4 begin_end, char *cs_name, i4  is_dyn);
void	ecs_opquery(char *cs_name);
void	ecs_get(char *var_name, SYM *sy, i4  sy_btype,
                char *ind_name, SYM *indsy, char *arg);
void	ecs_eretrieve(char *cs_name, i4  uses_sqlda);
void	ecs_addtab(char *cs_name, char *tbl_name);
i4  	ecs_colupd(char *cs_name, char *col_name, i4 what);
i4  	ecs_query(char *cs_name, i4  init);
void	ecs_csrset(char *cs_name, i4  val);
i4  	ecs_gcsrbits(char *cs_name);
char	*ecs_namecsr(void);
void	ecs_chkupd(char *cs_name, char *col_name, SYM *sy);
void	ecs_recover(void);
void	ecs_fixupd(char *cs_name);
void	ecs_clear(void);
void	ecs_coladd(char	*name);
void	ecs_colrslv(i4 doit);
i4  	ecs_dump(void);
EDB_QUERY_ID
        *ecs_id_curcsr(void);

/********************** end eqcursor utility ******************************/

/********************** begin frs utility **********************************/

/* Global modes known in eqfrs.c */
# define	FRSinq		0
# define	FRSset		1
# define	FRSload		2

void	frs_inqset(i4 flag);
void	frs_object(char *objtype);
void	frs_parentname(char *nm, i4  typ, PTR var);
i4      frsck_constant(char *constname, char *objname, 
                       i4  type, bool var, i4  *value);
void    frs_iqsave(char *name, i4  type);
i4      frsck_valmap(char *name); 
char *  frsck_mode(char *stmt, char *mode);
i4      frsck_key(char *spec);
void	frs_head(void);
void	frs_constant(char *constname, char *objname, i4  type, SYM *var);
void	frs_iqvar(char *name, i4  type, SYM *var, char *indid, SYM *indsym);
void	frs_setval(char *name, i4  type, SYM *var, char *indid, SYM *indsym);
void	frs_close(void);
i4  	frs_dump(void);
void	frs_mode(char *stmt, char *mode);
/********************** end frs utility ************************************/

/********************** begin rep utility **********************************/
/*
** Repeat Query routines
*/

void	rep_begin(char *dbstmt, EDB_QUERY_ID *qry);
void	rep_param(void);
void	rep_add(SYM *varsym, char *varid, SYM *indsym, char *indid, char *arg);
void	rep_close(i4 dbfunc, EDB_QUERY_ID *qry, i4  single_sel);
void	rep_recover(void);
    void	rep_dump(char *calname);

/* Interface to other DML users of Repeat queries */
# define REP_EXEC	0		/* Execute part */
# define REP_DEFINE	1		/* Define part */

/********************** end rep utility ************************************/

/********************** begin ret utility **********************************/
/* 
** Retrieve statement routines 
*/
void   ret_add(SYM *varsym, char *varid, 
               SYM *indsym, char *indid, char *arg, i4  type);
void   ret_close(void);
    void	ret_dump(char *calname);
    void	ret_stats(void);
/********************** end ret utility ************************************/

/********************** begin select utility *****************************/
/* SELECT buffering routines */

# define ESL_START_QUERY	1
# define ESL_SNGL_STOP_QUERY	2
# define ESL_BULK_STOP_QUERY	3
/********************** end select utility *******************************/

/********************** begin fips utility *****************************/
/*
** FIPS checking routines
*/
VOID	eqck_regid(char *idname);
VOID	eqck_delimid(char *idname);
bool	eqck_tgt(short action, short value);
/********************** end fips utility *****************************/
