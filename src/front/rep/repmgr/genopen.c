/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <cm.h>
# include <lo.h>
# include <si.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include "errm.h"

/**
** Name:	genopen.c - open code generation files
**
** Description:
**	Defines
**		open_infile	- open a template file
**		open_outfile	- creates or appends to an output file
**
** History:
**	16-dec-96 (joea)
**		Created based on genopen.c in replicator library.
**	25-sep-98 (abbjo03)
**		Replace ddba_messageit with IIUGerr.
**/

/*{
** Name:	open_infile	- open a template file
**
** Description:
**	Opens a template file from a directory.
**
** Inputs:
**	directory		- template directory
**	template_filename	- input template file name
**
** Outputs:
**	none
**
** Returns:
**	NULL	- open failed
**	FILE *	- file pointer
*/
FILE *
open_infile(
char	*directory,
char	*template_filename)
{
	FILE	*in_file;
	char	*p;
	char	filename[MAX_LOC+1];
	LOCATION	loc;

	if (template_filename == NULL || *template_filename == EOS)
	{
		IIUGerr(E_RM00EE_Null_empty_arg, UG_ERR_ERROR, 1,
			ERx("open_infile"));
		return (NULL);
	}

	STcopy(directory, filename);
	LOfroms(PATH, filename, &loc);
	LOfstfile(template_filename, &loc);
	if (SIfopen(&loc, ERx("r"), SI_TXT, SI_MAX_TXT_REC, &in_file) != OK)
	{
		LOtos(&loc, &p);
		IIUGerr(E_RM00F1_Err_open_template, UG_ERR_ERROR, 1, p);
		return (NULL);
	}

	return (in_file);
}


/*{
** Name:	open_outfile	- creates or appends to an output file
**
** Description:
**	Creates or opens (for append) a file in a directory and sets a
**	LOCATION structure.
**
** Inputs:
**	out_filename	- output file name
**	out_dir		- output directory
**	create_or_append - create a new file or append to existing file
**	uniq		- make a unique filename
**
** Outputs:
**	loc_name	- output file path
**
** Returns:
**	NULL	- open failed
**	FILE *	- file pointer
*/
FILE *
open_outfile(
char	*out_filename,
char	*out_dir,
char	*create_or_append,
char	*loc_name,
bool	uniq)
{
	FILE	*out_file;
	char	*p;
	char	filename[MAX_LOC+1];
	LOCATION	loc;

	if (loc_name == NULL)
		return (NULL);

	if (CMcmpcase(create_or_append, ERx("c")) == 0)
	{
		STcopy(out_dir, filename);
		LOfroms(PATH, filename, &loc);

		/* if uniq is 1, create a unique filename */
		if (uniq == 1)
			LOuniq(out_filename, ERx(""), &loc);
		else
			LOfstfile(out_filename, &loc);

		if (SIfopen(&loc, ERx("w"), SI_TXT, SI_MAX_TXT_REC, &out_file)
			!= OK)
		{
			IIUGerr(E_RM00F2_Err_open_outfile, UG_ERR_ERROR, 1,
				out_filename);
			return (NULL);
		}
		LOtos(&loc, &p);
		STcopy(p, loc_name);
	}
	else
	{
		LOfroms(PATH & FILENAME, loc_name, &loc);
		if (SIfopen(&loc, ERx("a"), SI_TXT, SI_MAX_TXT_REC, &out_file)
			!= OK)
		{
			IIUGerr(E_RM00F2_Err_open_outfile, UG_ERR_ERROR, 1,
				out_filename);
			return (NULL);
		}
	}
	return (out_file);
}
