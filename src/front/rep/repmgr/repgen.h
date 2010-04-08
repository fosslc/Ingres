/*
** Copyright (c) 1996, 2009 Ingres Corporation
*/
# ifndef REPGEN_H_INCLUDED
# define REPGEN_H_INCLUDED
# include <compat.h>
# include <si.h>

/**
** Name:	repgen.h - Replicator code generator include
**
** Description:
**	Defines prototypes of the code generator routines.
**
** History:
**	16-dec-96 (joea)
**		Created based on repgen.h in replicator library.
**	05-feb-97 (joea)
**		Remove unused functions.
**	23-jul-97 (joea) bug 81741
**		Add flag parameter to expand_key to deal with DECIMAL key
**		columns. Consolidate table object functions into expand_tblobj.
**	03-jun-98 (abbjo03)
**		Remove code_gen prototype.
**	23-jul-98 (abbjo03)
**		Remove functions made obsolete by generic RepServer.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      22-apr-2009 (joea)
**          Add prototype for expand_oiv.
**/

# define	MAX_SIZE	50000
# define	MAX_EXEC_SIZE	80000


/* function prototypes */

FUNC_EXTERN STATUS exp_token(char *tablename, char *table_owner, i4 table_no,
	char *directory, FILE *template_file, FILE *output_file,
	bool exec_immediate, char *execute_string);
FUNC_EXTERN void expand_cll(i4 table_no, char *p1, char *p2, i4  indent,
	char *expand_string);
FUNC_EXTERN void expand_iocol(i4 table_no, char *p1, i4  indent,
	char *expand_string);
FUNC_EXTERN void expand_ddk(i4 table_no, char *p1, i4  indent,
	char *expand_string);
FUNC_EXTERN void expand_ddl(i4 table_no, char *p1, i4  indent,
	char *expand_string);
FUNC_EXTERN void expand_jon(i4 table_no, bool keys_only, char *p1,
	char *p2, i4  indent, char *expand_string);
FUNC_EXTERN void expand_key(i4 table_no, char *p1, i4  indent,
	char *expand_string);
FUNC_EXTERN FILE *open_infile(char *directory, char *template_filename);
FUNC_EXTERN FILE *open_outfile(char *out_filename, char *out_dir,
	char *create_or_append, char *loc_name, bool uniq);
FUNC_EXTERN STATUS sql_gen(char *tablename, char *table_owner, i4 table_no,
	char *template_filename, char *output_filename);
FUNC_EXTERN void expand_tblobj(i4 table_no, i4  obj_type, char *expand_string);
FUNC_EXTERN void expand_odt(char *tablename, char *table_owner,
	char *expand_string);
FUNC_EXTERN void expand_oiv(i4 table_no, char *expand_string);
FUNC_EXTERN void expand_vps(i4 table_no, char *expand_string);

# endif
