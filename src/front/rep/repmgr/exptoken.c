/*
** Copyright (c) 1996, 2009 Ingres Corporation
*/
# include <compat.h>
# include <cm.h>
# include <st.h>
# include <si.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <tblobjs.h>
# include "repgen.h"
# include "errm.h"

/**
** Name:	exptoken.c - expand tokens
**
** Description:
**	Defines
**		exp_token - expand tokens
**
** History:
**	16-dec-96 (joea)
**		Created based on exptoken.c in replicator library.
**	04-feb-97 (joea)
**		Eliminate unused tokens.
**	23-jul-97 (joea) bug 81741
**		Add flag parameter to expand_key to deal with DECIMAL key
**		columns. Consolidate table object functions into expand_tblobj.
**	25-aug-97 (joea) bug 84852
**		Suppress comments at the beginning of any line, not just at the
**		beginning of the file.
**	18-sep-97 (joea)
**		Add $vps token to handle variable page sizes.  Add CM_NEXT4
**		macro to eliminate repetition.
**	23-jul-98 (abbjo03)
**		Remove processing of tokens made obsolete by generic RepServer.
**	25-sep-98 (abbjo03)
**		Replace ddba_messageit with IIUGerr.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      21-apr-2009 (joea)
**          Add oiv token to support replication of identity columns.
**/

# define TOKEN_MARKER		ERx("$")
# define SPACE			ERx(" ")
# define NLINE			ERx("\n")
# define CM_NEXT4(s, p)	\
			(p) += 4; CMnext(s); CMnext(s); CMnext(s); CMnext(s)


/*{
** Name:	exp_token - expand tokens
**
** Description:
**	code block that expands tokens
**
**	TOKENS SUPPORTED:
**
**	$arc			- archive table name
**	$cll(string)(string)	- column list assignment
**	$col[(string)]		- table columns
**	$ddk[(string)]		- data definition for key columns
**	$ddl			- data definition
**		supports date, money
**			integerX, floatX where X is the length
**			anything_else(X) where X is the length
**	$jon(string)[(string)]	- join expansion / keys
**	$key[(string)]		- key columns
**	$odt			- owner qualified table name
**      $oiv                    - overriding identity value
**	$rdp			- remote delete procedure name
**	$rip			- remote insert procedure name
**	$rup			- remote update procedure name
**	$sha			- shadow table name
**	$sx1			- shadow index 1 name
**	$vps			- variable page size
**
** Inputs:
**	tablename	- what table
**	table_owner	- who owns the table
**	table_no	- what table
**	directory	- read directory or path
**	template_file	-
**	output_file	-
**	exec_immediate	- FALSE means don't execute
**	execute_string	- pointer to exec immediate string
**
** Outputs:
**	none
**
** Returns:
**	0	success
**	-1	bad parameter(s)
**	-N	message message number
*/
STATUS
exp_token(
char	*tablename,
char	*table_owner,
i4	table_no,
char	*directory,
FILE	*template_file,
FILE	*output_file,
bool	exec_immediate,
char	*execute_string)
{
	char	file_string[MAX_SIZE];
	STATUS	err_ret = 0;
	char	tmp[MAX_SIZE];
	char	tmp2[MAX_SIZE];
	char	*token_string;
	char	in_string[MAX_SIZE];
	char	expand_string[MAX_SIZE];
	char	*exec_p, *in_p, *file_p;
	char	*ptmp, *ptmp2;
	i4	in_chars;
	i4	in_pos, file_pos, exec_pos;	/* character counters */
	i4	indent;	/* indentation of token for continuation lines */
	i4	i, j;
	i4	com_cnt;
	bool	in_comment;
	bool	end_of_file;
	char	*com_ptr;

	in_comment = end_of_file = FALSE;
	exec_pos = 0;
	exec_p = execute_string;
	while (SIgetrec(in_string, (i4)MAX_SIZE, template_file)
		!= ENDFILE && *in_string != EOS)
	{
		/* ignore blank lines */
		while (STlength(in_string) == 1 && CMcmpcase(in_string, NLINE)
				== 0 && !end_of_file)
			end_of_file = SIgetrec(in_string, (i4)MAX_SIZE,
				template_file) == ENDFILE;
		if (end_of_file)
			break;

		/* ignore leading white space */
		com_ptr = in_string;
		while (CMwhite(com_ptr))
			CMnext(com_ptr);

		in_chars = STlength(com_ptr);
		in_p = com_ptr;

		/* strip off leading comment */
		if (CMcmpcase(com_ptr, ERx("/")) == 0)
		{
			/* may have start of comment */
			CMnext(com_ptr);
			if (CMcmpcase(com_ptr, ERx("*")) == 0)
			{
				/* we have start of comment */
				in_comment = TRUE;
				CMnext(com_ptr);
				while (in_comment && !end_of_file)
				{
					for (com_cnt = STlength(com_ptr);
						com_cnt > 0 && in_comment;
						com_cnt--)
					{
						/* check for "*" */
						while (CMcmpcase(com_ptr,
							ERx("*")) == 0 &&
							in_comment)
						{
							CMnext(com_ptr);
							com_cnt--;
							/* check for "/" */
							if (CMcmpcase(com_ptr,
								ERx("/")) == 0)
							{
								/*
								** found end of
								** comment
								*/
								in_comment =
									FALSE;
								continue;
							}
							else if (CMcmpcase(
								com_ptr,
								ERx("*")) != 0)
							{
								CMnext(com_ptr);
								--com_cnt;
							}
						}
						if (!in_comment)
							continue;
						CMnext(com_ptr);
					}

					if (in_comment)
					{
						/*
						** get next line and continue
						** looking for comment
						*/
						if (SIgetrec(in_string,
							(i4)MAX_SIZE,
							template_file) ==
							ENDFILE)
							/* nothing to do */
							end_of_file = TRUE;
						else
							com_ptr = in_string;
					}
					else
					{
						/*
						** position to char after end
						** of comment
						*/
						CMnext(com_ptr);
						in_chars = STlength(com_ptr);
						in_p = com_ptr;
					}
				}
			}
		}
		if (end_of_file)
			break;

		file_pos = 0;
		in_pos = 0;
		file_p = file_string;

		while ((token_string = STindex(in_p, TOKEN_MARKER,
			in_chars - in_pos)) != NULL)
		{
			indent = 0;
			*expand_string = EOS;
			while (in_p != token_string)
			{
				if (CMcmpcase(in_p, NLINE) == 0)
					indent = 0;
				else if (CMcmpcase(in_p, ERx("\t")) == 0)
					indent = ((indent + 8) / 8) * 8;
				else
					indent++;
				STlcopy(in_p, file_p, 1);
				file_pos++;
				CMnext(file_p);
				CMnext(in_p);
				in_pos++;
			}
			*tmp = EOS;
			*tmp2 = EOS;
			if (STbcompare(token_string, 4, "$ddl", 4, 1) == 0)
			{
				CMnext(token_string);
				CMnext(token_string);
				CMnext(token_string);
				CMnext(token_string);
				if (CMcmpcase(token_string, "(") == 0)
				{
					CMnext(token_string);
					STcopy(token_string, tmp);
					ptmp = STindex(tmp, ")", 0);
					STlcopy(tmp, tmp, ptmp - tmp);
					/* add 4 for the token, 2 for the () */
					j = 4 + STlength(tmp) + 2;
					in_pos += j;
					for (i = 0; i < j; i++)
						CMnext(in_p);
				}
				else
				{
					*tmp = EOS;
					CM_NEXT4(in_p, in_pos);
				}
				expand_ddl(table_no, tmp, indent, expand_string);
			}
			else if (STbcompare(token_string, 4, "$ddk", 4, 1) == 0)
			{
				CMnext(token_string);
				CMnext(token_string);
				CMnext(token_string);
				CMnext(token_string);
				if (CMcmpcase(token_string, "(") == 0)
				{
					CMnext(token_string);
					STcopy(token_string, tmp);
					ptmp = STindex(tmp, ")", 0);
					STlcopy(tmp, tmp, ptmp - tmp);
					/* add 4 for the token, 2 for the () */
					j = 4 + STlength(tmp) + 2;
					in_pos += j;
					for (i = 0; i < j; i++)
						CMnext(in_p);
				}
				else
				{
					*tmp = EOS;
					CM_NEXT4(in_p, in_pos);
				}
				expand_ddk(table_no,tmp, indent, expand_string);
			}
			else if (STbcompare(token_string, 4, "$jon", 4, 1) == 0)
			{
				CM_NEXT4(in_p, in_pos);
				CMnext(token_string);
				CMnext(token_string);
				CMnext(token_string);
				CMnext(token_string);
				if (CMcmpcase(token_string, "(") == 0)
				{
					CMnext(token_string);
					STcopy(token_string, tmp);
					ptmp = STindex(tmp, ")", 0);
					CMnext(ptmp);
					if (CMcmpcase(ptmp, "(") == 0)
					{
						CMnext(ptmp);
						ptmp2 = STindex(ptmp, ")", 0);
						STlcopy(ptmp, tmp2, ptmp2 - ptmp);
						j = 2 + STlength(tmp2);
						in_pos += j;
						for (i = 0; i < j; i++)
							CMnext(in_p);
					}
					else
						*tmp2 = EOS;
					ptmp = STindex(tmp, ")", 0);
					STlcopy(tmp, tmp, ptmp - tmp);
					j = STlength(tmp) + 2;
					in_pos += j;
					for (i = 0; i < j; i++)
						CMnext(in_p);
				}
				else
				{
					*tmp = EOS;
				}
				expand_jon(table_no, 1, tmp, tmp2,
					indent, expand_string);
			}
			else if (STbcompare(token_string, 4, "$col", 4, 1) == 0)
			{
				CMnext(token_string);
				CMnext(token_string);
				CMnext(token_string);
				CMnext(token_string);
				if (CMcmpcase(token_string, "(") == 0)
				{
					CMnext(token_string);
					STcopy(token_string, tmp);
					ptmp = STindex(tmp, ")", 0);
					STlcopy(tmp, tmp, ptmp - tmp);
					/* add 4 for the token, 2 for the () */
					j = 4 + STlength(tmp) + 2;
					in_pos += j;
					for (i = 0; i < j; i++)
						CMnext(in_p);
				}
				else
				{
					*tmp = EOS;
					CM_NEXT4(in_p, in_pos);
				}
				expand_iocol(table_no, tmp, indent,
					expand_string);
			}
			else if (STbcompare(token_string, 4, "$key", 4, 1) == 0)
			{
				CMnext(token_string);
				CMnext(token_string);
				CMnext(token_string);
				CMnext(token_string);
				if (CMcmpcase(token_string, "(") == 0)
				{
					CMnext(token_string);
					STcopy(token_string, tmp);
					ptmp = STindex(tmp, ")", 0);
					STlcopy(tmp, tmp, ptmp - tmp);
					/* add 4 for the token, 2 for the () */
					j = 4 + STlength(tmp) + 2;
					in_pos += j;
					for (i = 0; i < j; i++)
						CMnext(in_p);
				}
				else
				{
					*tmp = EOS;
					CM_NEXT4(in_p, in_pos);
				}
				expand_key(table_no, tmp, indent,
					expand_string);
			}
			else if (STbcompare(token_string, 4, "$cll", 4, 1) == 0)
			{
				CM_NEXT4(in_p, in_pos);
				CMnext(token_string);
				CMnext(token_string);
				CMnext(token_string);
				CMnext(token_string);
				if (CMcmpcase(token_string, "(") == 0)
				{
					CMnext(token_string);
					STcopy(token_string, tmp);
					ptmp = STindex(tmp, ")", 0);
					CMnext(ptmp);
					if (CMcmpcase(ptmp, "(") == 0)
					{
						CMnext(ptmp);
						ptmp2 = STindex(ptmp, ")", 0);
						STlcopy(ptmp, tmp2, ptmp2 - ptmp);
						j = 2 + STlength(tmp2);
						in_pos += j;
						for (i = 0; i < j; i++)
							CMnext(in_p);
					}
					else
					{
						*tmp2 = EOS;
					}
					ptmp = STindex(tmp, ")", 0);
					STlcopy(tmp, tmp, ptmp - tmp);
					j = STlength(tmp) + 2;
					in_pos += j;
					for (i = 0; i < j; i++)
						CMnext(in_p);
				}
				else
				{
					*tmp = EOS;
				}
				expand_cll(table_no, tmp, tmp2, indent, expand_string);
			}
			else if (STbcompare(token_string, 4, "$arc", 4, 1) == 0)
			{
				CM_NEXT4(in_p, in_pos);
				expand_tblobj(table_no, TBLOBJ_ARC_TBL,
					expand_string);
			}
			else if (STbcompare(token_string, 4, "$sha", 4, 1) == 0)
			{
				CM_NEXT4(in_p, in_pos);
				expand_tblobj(table_no, TBLOBJ_SHA_TBL,
					expand_string);
			}
			else if (STbcompare(token_string, 4, "$rip", 4, 1) == 0)
			{
				CM_NEXT4(in_p, in_pos);
				expand_tblobj(table_no, TBLOBJ_REM_INS_PROC,
					expand_string);
			}
			else if (STbcompare(token_string, 4, "$rup", 4, 1) == 0)
			{
				CM_NEXT4(in_p, in_pos);
				expand_tblobj(table_no, TBLOBJ_REM_UPD_PROC,
					expand_string);
			}
			else if (STbcompare(token_string, 4, "$rdp", 4, 1) == 0)
			{
				CM_NEXT4(in_p, in_pos);
				expand_tblobj(table_no, TBLOBJ_REM_DEL_PROC,
					expand_string);
			}
			else if (STbcompare(token_string, 4, "$sx1", 4, 1) == 0)
			{
				CM_NEXT4(in_p, in_pos);
				expand_tblobj(table_no, TBLOBJ_SHA_INDEX1,
					expand_string);
			}
			else if (STbcompare(token_string, 4, "$odt", 4, 1)== 0)
			{
				CM_NEXT4(in_p, in_pos);
				expand_odt(tablename, table_owner,
					expand_string);
			}
			else if (STbcompare(token_string, 4, "$oiv", 4, 1)== 0)
			{
				CM_NEXT4(in_p, in_pos);
				expand_oiv(table_no, expand_string);
			}
			else if (STbcompare(token_string, 4, "$vps", 4, 1)== 0)
			{
				CM_NEXT4(in_p, in_pos);
				expand_vps(table_no, expand_string);
			}
			else
			{
				STcopy(TOKEN_MARKER, expand_string);
				in_pos++;
				CMnext(in_p);
			}
			STcopy(expand_string, file_p);
			file_pos += STlength(expand_string);
			file_p += STlength(expand_string);
		} /* while STindex(in_p) */

		STcopy(in_p, file_p);
		file_pos += in_chars - in_pos;
		SIfprintf(output_file, ERx("%s"), file_string);
		if (exec_immediate)
		{
			file_p = file_string;
			STlcopy(file_p, exec_p, 1);
			exec_pos++;
			for (i = 0; i < file_pos; ++i)
			{
				CMnext(file_p);
				if (CMcmpcase(file_p, SPACE) != 0 ||
					CMcmpcase(exec_p, SPACE) != 0)
				{
					CMnext(exec_p);
					if (CMcmpcase(file_p, NLINE) == 0
						|| CMcmpcase(file_p, ERx("\t"))
							== 0)
						STlcopy(SPACE, exec_p, 1);
					else
						STlcopy(file_p, exec_p, 1);
					exec_pos++;
				}
				if (exec_pos > MAX_EXEC_SIZE)
				{
					/* string to long for immediate execution */
					IIUGerr(E_RM00F0_Exec_str_too_long,
						UG_ERR_ERROR, 0);
					return (E_RM00F0_Exec_str_too_long);
				}
			}
		}
	}
	return (OK);
}
