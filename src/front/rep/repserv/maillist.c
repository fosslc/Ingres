/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <lo.h>
# include <pc.h>
# include <si.h>
# include <er.h>
# include <iiapi.h>
# include <sapiw.h>
# include "conn.h"

/**
** Name:	maillist.c - send error mail
**
** Description:
**	Defines
**		maillist	- send error mail
**		mailer		- send mail
**
** History:
**	16-dec-96 (joea)
**		Created based on maillist.sc in replicator library.
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL.
**	16-Oct-98 (merja01)
**		Manually x-integ oping20 changes 437768 and 438233 to fix
**		replicator mail feature for hpux and axp_osf.
**	03-dec-98 (abbjo03)
**		Eliminate RSerr_ret global. Use messageit in mailer() since
**		maillist() protects itself against recursion. Disable error
**		mail if we can't write to the temporary mail file. Change
**		scheme for constructing the mail command to eliminate multiple
**		message numbers and having the temporary filename in more than
**		one place.
**	09-feb-99 (abbjo03)
**		Call IIsw_getRowCount before calling IIsw_close, otherwise row
**		count is invalid.
**	24-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-mar-2004 (gupsh01)
**	    Added getQinfoParm as input parameter to II_sw... 
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**/

GLOBALDEF bool	RSerr_mail = TRUE;


static bool	in_mail = FALSE;


static void mailer(char *str, char *user);


/*{
** Name:	maillist - send error mail
**
** Description:
**	Mails error messages to people listed in dd_mail_alert
**
** Inputs:
**	str - message string
**
** Outputs:
**	none
**
** Returns:
**	none
*/
void
maillist(
char	*str)
{
	struct
	{
		short	len;
		char	text[81];
	} user;
	IIAPI_DATAVALUE		cdv;
	II_LONG			cnt;
	IIAPI_STATUS		status;
	II_PTR			stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM	getQinfoParm;

# if defined(OS2) || defined(NT_GENERIC)
	return;
# endif

	/* is the -M flag set? */
	if (!RSerr_mail || in_mail)
		return;

	in_mail = TRUE;
	stmtHandle = NULL;
	SW_COLDATA_INIT(cdv, user);
	while (TRUE)
	{
		status = IIsw_selectLoop(RSlocal_conn.connHandle,
			&RSlocal_conn.tranHandle,
			ERx("SELECT mail_username FROM dd_mail_alert"),
			0, NULL, NULL, 1, &cdv, &stmtHandle, 
			&getQinfoParm, &errParm);
		if (status == IIAPI_ST_NO_DATA || status != IIAPI_ST_SUCCESS)
			break;
		SW_VCH_TERM(&user);
		mailer(str, user.text);
	}
	cnt = IIsw_getRowCount(&getQinfoParm);
	IIsw_close(stmtHandle, &errParm);
	if (status == IIAPI_ST_NO_DATA && !cnt && status != IIAPI_ST_SUCCESS)
		RSerr_mail = FALSE;

	in_mail = FALSE;
}


/*{
** Name:	mailer - send mail
**
** Description:
**	send mail to a given user
**
** Inputs:
**	str	- the message to be sent
**	user	- the user's mail address
**
** Outputs:
**	none
**
** Returns:
**	none
*/
static void
mailer(
char	*str,
char	*user)
{
	FILE		*mail_fp;
	char		mail_msg[1000];
	char		cmd[256];
	char		mail_cmd[16];
	char		subj[80];
	LOCATION	loc;
	char		filename[MAX_LOC+1];
	CL_ERR_DESC	cl_err;

	STcopy(ERx("mail_msg.tmp"), filename);
	LOfroms(FILENAME, filename, &loc);
	if (SIfopen(&loc, ERx("w"), SI_TXT, SI_MAX_TXT_REC, &mail_fp) != OK)
	{
		messageit(1, 109);
		RSerr_mail = FALSE;
		return;
	}
	messageit_text(mail_msg, 1581, (i4)RSserver_no, str);
	SIfprintf(mail_fp, mail_msg);
	SIfprintf(mail_fp, ERx("\n"));	/* HP wants the extra return */
	SIclose(mail_fp);

	messageit_text(subj, 1394);
# ifdef VMS
	STprintf(cmd, ERx("MAIL /SUBJECT=\"%s\" %s %s"), subj, filename, user);
# else
# if defined(sparc_sol) || defined(HPUX) || defined(axp_osf)
	STcopy(ERx("mailx"), mail_cmd);
# else
	STcopy(ERx("mail"), mail_cmd);
# endif /* sol || HPUX || axp_osf */
	STprintf(cmd, ERx("cat %s | %s -s \"%s\" %s"), filename, mail_cmd, subj,
		user);
# endif

	PCcmdline(NULL, cmd, PC_WAIT, NULL, &cl_err);

	PCsleep(3000);	/* let things catch up a bit! */
}
