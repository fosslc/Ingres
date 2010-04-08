/*
** Copyright (c) 2004 Ingres Corporation
*/
EXEC SQL INCLUDE sqlca;

# include <compat.h>
# include <st.h>
# include <cm.h>
# include <nm.h>
# include <lo.h>
# include <si.h>
# include <pc.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <generr.h>
# include "repgen.h"
# include "errm.h"

/**
** Name:	sqlgen.sc - Generate SQL replication objects
**
** Description:
**	Defines
**		sql_gen - Generate SQL replication objects
**
** History:
**	16-dec-96 (joea)
**		Created based on sqlgen.sc in replicator library.
**	25-sep-98 (abbjo03)
**		Replace ddba_messageit with IIUGerr.
**/

static char	directory[MAX_LOC+1] ZERO_FILL;	/* read directory or path */


/*{
** Name:	sql_gen - Generate SQL replication objects
**
** Description:
**	Expands tokens in input file and creates (or appends to) a file
**	&/or executes the result immediately.
**
** Inputs:
**	tablename	- table name
**	table_no	- table number
**	table_owner	- table owner
**	template_filename - template file name
**	output_filename	- output file name
**
** Outputs:
**	none
**
** Returns:
**	0		success
**	-1		bad parameter(s)
**	-N		messageit message number
**	GE_NO_RESOURCE	SQL code executed via TM
**	GE_NO_PRIVILEGE	SQL code executed via TM
**	> 0		Ingres error
**
** Side effects:
**	The output filename is created and executed.
*/
STATUS
sql_gen(
char	*tablename,
char	*table_owner,
i4	table_no,
char	*template_filename,
char	*output_filename)
{
	EXEC SQL BEGIN DECLARE SECTION;
	char	execute_string[MAX_EXEC_SIZE];
	char	error_text[1000];
	i4	err_ret = 0;
	char	localdb[DB_MAXNAME+1];
	char	tmp[MAX_SIZE];
	char	vnode_name[DB_MAXNAME+1];
	EXEC SQL END DECLARE SECTION;
	STATUS	return_code = OK;
	FILE	*in_file;
	FILE	*out_file;
	LOCATION	loc;
	LOCATION	temp_loc;
	char	loc_name[MAX_LOC+1];
	char	temp_loc_name[MAX_LOC+1];
	LOCATION dir_loc;
	char	*p;
	CL_ERR_DESC	err_code;

	if (tablename == NULL || *tablename == EOS)
	{
		IIUGerr(E_RM00EE_Null_empty_arg, UG_ERR_ERROR, 1, 
			ERx("sql_gen"));
		return (-1);
	}

	if (*directory == EOS)
	{
		if (NMloc(FILES, PATH, NULL, &dir_loc) != OK)
			return (-1);
		if (LOfaddpath(&dir_loc, ERx("rep"), &dir_loc) != OK)
			return (-1);
		LOtos(&dir_loc, &p);
		STcopy(p, directory);
	}

	if ((in_file = open_infile(directory, template_filename)) == NULL)
		return (-1);

	if (NMloc(TEMP, PATH, NULL, &temp_loc) != OK)
		return (-1);
	LOtos(&temp_loc, &p);
	STcopy(p, temp_loc_name);
	if ((out_file = open_outfile(output_filename, temp_loc_name, ERx("c"),
			loc_name, TRUE)) == NULL)
		return (-1);

	if (exp_token(tablename, table_owner, table_no, directory, in_file,
			out_file, TRUE, execute_string))
		return (-1);

	SIclose(in_file);

	SIfprintf(out_file, ERx("\\p\\g\nCOMMIT\\g\n\\q\n"));
	SIclose(out_file);

	/* Try to exec immediate first, then save to file if GE_NO_RESOURCE */
	EXEC SQL EXECUTE IMMEDIATE :execute_string;
	EXEC SQL INQUIRE_SQL (:err_ret = ERRORNO);
	return_code = err_ret;
	switch (err_ret)
	{
	case OK:
		EXEC SQL COMMIT;
		break;

	case GE_NO_RESOURCE:
		EXEC SQL ROLLBACK; /* to avoid deadlock */
		EXEC SQL SELECT vnode_name, database_name
			INTO	:vnode_name, :localdb
			FROM	dd_databases
			WHERE	local_db = 1;
		EXEC SQL INQUIRE_SQL (:err_ret = ERRORNO,
			:error_text = ERRORTEXT);
		EXEC SQL COMMIT;
		if (err_ret)
		{
			IIUGerr(E_RM00EF_Err_rtrv_node, UG_ERR_ERROR, 1,
				error_text);
			return_code = err_ret;
			break;
		}
		STtrmwhite(vnode_name);
		STtrmwhite(localdb);
		STprintf(tmp, ERx("rpgensql %s %s %s %s"), table_owner,
			vnode_name, localdb, loc_name);
		err_ret = PCcmdline(NULL, tmp, PC_WAIT, NULL, &err_code);
		if (err_ret)
			return_code = err_ret;
		break;

	default:
		EXEC SQL ROLLBACK;
		break;
	}

	if (return_code == OK)
	{
		LOfroms(PATH & FILENAME, loc_name, &loc);
		LOdelete(&loc);
	}

	return (return_code);
}
