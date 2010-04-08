/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <lo.h>
#include    <si.h>
#include    <pc.h>
#include    <ck.h>
#include    <nm.h>
#include    <cv.h>
#include    <me.h>

/**
**
**  Name: CKSUBST.C - Build CK commands from template
**
**  Description:
**	This file contains portable functions to build host OS commands
**	for CK operations.
**
**          CK_subst() - build CK command
**
**  History:
**      27-oct-88 (anton)
**	    Created.
**	26-apr-1993 (bryanp)
**	    Prototyping CK for 6.5.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	23-may-1994 (bryanp)
**	    Return CK_TEMPLATE_MISSING if the cktmpl.def file is missing.
**	02-dec-1994 (andyw)
**	    Temporary change to check granularity of 'D', 
**	    code is made redundant when partial backup implemented
**	17-may-1996 (hanch04)
**	    Moved <si.h> before <ck.h>, added <pc.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

static VOID CK_lfill(
		char	**cp,
		i4	len,
		char	*str);

/*{
** Name: CK_lfill - fill in a substitute string
**
** Description:
**	This function simply copies a string 
**      usually spans more than one line. 
**
** Inputs:
**	cp				pointer to destination space
**					(also an output)
**	len				length of string to substitute
**	str				string to substitute
**
** Outputs:
**      cp				string pointer changed to point
**					to end of filled area.
**
**	Returns:
**	    none
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      01-nov-88 (anton)
**          Created.
*/

static VOID
CK_lfill(
char	**cp,
i4	len,
char	*str)
{
    while (len--)
    {
	*(*cp)++ = *str++;
    }
}

/*{
** Name: CK_subst - build a CK command from template and requirements
**
** Description:
**	This function will search a CK command template file for
**	a line meeting the requirements of the inputs and will substitute
**	various esacpe sequences with other strings such as file names.
**
**	Based on a II_subst of the 5.0 UNIX CL.
**
** Inputs:
**	comlen			length of cline space
**	oper			operation - one of 'B'egin, 'E'nd, 'W'ork
**	dev			device type - one of 'T'ape, 'D'isk
**	dir			direction - one of 'S'ave, 'R'estore
**	type			type (device type) 0 for disk, 1 for tape
**	locnum			location number (sequence or quantity)
**	di_l_path		length of di path string
**	di_path			di path string
**	ckp_l_path		length of checkpoint path string
**	ckp_path		checkpoint path string
**	ckp_l_file		length of checkpoint file string
**	ckp_file		checkpoint file string
**
** Outputs:
**	cline			space for generated command
**
**	Returns:
**	    E_DB_OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-oct-88 (anton)
**	    Created.
**	23-may-1994 (bryanp)
**	    Return CK_TEMPLATE_MISSING if the cktmpl.def file is missing.
**	    This message contains text that specifically describes that the
**		problem relates to the cktmpl.def file, rather than returning
**		the more generic "error opening a file" message which SIopen
**		or NMf_open may return.
**	02-dec-1994 (andyw)
**	    Check on granularity for partial backup
*/
STATUS
CK_subst(
char	*cline,
i4	comlen,
char	oper,
char	dev,
char	dir,
u_i4	type,
u_i4	locnum,
u_i4	di_l_path,
char	*di_path,
u_i4	ckp_l_path,
char	*ckp_path,
u_i4	ckp_l_file,
char	*ckp_file)
{
    STATUS		ret_val = OK;
    auto LOCATION	loc;
    auto char		*s;
    auto FILE		*fp;
    auto char		*cp;
    char		*command;

    command = MEreqmem(0, comlen, TRUE, &ret_val);

    if (!ret_val)
    {
	NMgtAt("II_CKTMPL_FILE", &s);
	if (s == NULL || *s == EOS)
	{
	    ret_val = NMf_open("r", "cktmpl.def", &fp);
	}
	else
	{
	    LOfroms( PATH & FILENAME, s, &loc );
	    ret_val = SIopen( &loc, "r", &fp);
	}
	if (ret_val)
	    ret_val = CK_TEMPLATE_MISSING;
    }

    /* find command line - skip comments and blank lines */
    if (!ret_val)
    {
	while ((ret_val = SIgetrec(command, comlen, fp)) == OK)
	{
	    if (*command == '\n')
	    {
		continue;
	    }
	    if (command[0] == oper
		&& (command[1] == dir || command[1] == 'E')
		&& (command[2] == dev || command[2] == 'E')
		&& (command[3] == 'D' || command[3] == 'E')
		&& command[4] == ':')
	    {
		break;
	    }
	}
    }

    _VOID_ SIclose(fp);

    if (!ret_val)
    {
	/* found the line - do the substitution */
	s = command + 5;
	cp = cline;
	while (*s != '\n')
	{
	    if (*s == '%')
	    {
		switch (*++s)
		{
		case '%':
		    *cp++ = '%';
		    break;
		case 'D':
		    CK_lfill(&cp, di_l_path, di_path);
		    break;
		case 'C':
		    CK_lfill(&cp, ckp_l_path, ckp_path);
		    break;
		case 'F':
		    CK_lfill(&cp, ckp_l_file, ckp_file);
		    break;
		case 'A':
		    CK_lfill(&cp, ckp_l_path, ckp_path);
# ifdef	UNIX
		    *cp++ = '/';
# endif
		    CK_lfill(&cp, ckp_l_file, ckp_file);
		    break;
		case 'T':
		    *cp++ = type ? '1' : '0';
		    break;
		case 'N':
		    CVna(locnum, cp);
		    while (*cp)
			cp++;
		    break;
		default:
		    *cp++ = '%';
		    *cp++ = *s;
		}
		++s;
	    }
	    else
	    {
		*cp++ = *s++;
	    }
	}
	*cp = EOS;
    }

# ifdef	xDEBUG
    TRdisplay("CK_subst %c %c %c: %s\n", oper, dir, dev, cline);
# endif
    return(ret_val);
}
