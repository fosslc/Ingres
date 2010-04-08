
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<st.h>
# include	<er.h>
# include       <si.h>
# include       <lo.h>
# include	<fe.h>
# include	<ug.h>
# include       <adf.h>
# include       <afe.h>
EXEC SQL INCLUDE <xf.sh>;
# include	"erxf.h"

/*
** Fool MING, which doesn't know about EXEC SQL INCLUDE
# include <xf.qsh>
*/

/**
** Name:	xfsecalm.sc - write statements to create security_alarm.
**
** Description:
**	This file defines:
**
**	xfsecalarms	write statement to create security alarms 
**
** History:
**	17-nov-93 (robf)
**	   Created from xfpermit.sc
**      28-jan-2009 (stial01)
**          Use DB_MAXNAME for database objects.
*/
/* # define's */
/* GLOBALDEF's */

/* extern's */
GLOBALREF bool	With_distrib;
GLOBALREF bool  With_security_alarms;
/* static's */
static void writealarm( TXT_HANDLE	**tfdp, 
	char	*object_name,
	i2	alarm_number,
	i4	text_sequence,
	char	*alarm_grantor,
	char	*text_segment);

/*{
** Name:	xfsecalarms - write statement to create security_alarm.
**
** Description:
**
** Inputs:
**
** Outputs:
**
**	Returns:
**
** History:
**	17-nov-93 (robf)
**         Created for Secure 2.0.
*/

void
xfsecalarms()
{
EXEC SQL BEGIN DECLARE SECTION;
    char	text_segment[XF_PERMITLINE + 1];
    char	alarm_grantor[DB_MAXNAME + 1];
    char	object_name[DB_MAXNAME + 1];
    char	object_owner[DB_MAXNAME + 1];
    i2		alarm_number;
    i2		alarm_depth;
    i4		text_sequence;
EXEC SQL END DECLARE SECTION;
    TXT_HANDLE		*tfd = NULL;

    if (!With_security_alarms)
	return;


    EXEC SQL SELECT a.object_name, a.object_owner,
			a.security_number, a.text_sequence, a.text_segment 
		INTO :object_name, :object_owner, 
			:alarm_number, :text_sequence, :text_segment 
		FROM iisecurity_alarms a
		WHERE (a.object_owner = :Owner OR '' = :Owner)
			AND a.object_type = 'T'
		ORDER BY a.object_owner, a.object_name,
				a.security_number, a.text_sequence;
    EXEC SQL BEGIN;
    {
	    writealarm(&tfd, object_name, 
			alarm_number, text_sequence, 
			object_owner, text_segment);
    }
    EXEC SQL END;

    if (tfd != NULL)
	xfclose(tfd);
}

/*{
** Name:	writealarm - write out security alarm information. 
**
** Description:
**
** Inputs:
**    tfdp		pointer to TXT_HANDLE pointer, we open this on the
**				first call.
**    object_name	name of the object.
**    alarm_number	the alarm number
**    text_sequence	sequence number of the text segment.
**    alarm_grantor	name of person granting the permission.
**    text_segment	the text of the permission.
**
** Outputs:
**    tfdp		pointer to TXT_HANDLE pointer, we open this on the
**				first call.
*/

static void
writealarm(
    TXT_HANDLE	**tfdp, 
    char	*object_name,
    i2		alarm_number,
    i4		text_sequence,
    char	*alarm_grantor,
    char	*text_segment
    )
{
    /* if new alarm statement */
    xfread_id(object_name);
    if (!xfselected(object_name))
	return;

    if (text_sequence == 1)
    {
	if (*tfdp == NULL)
	{
	    xfwritehdr(ALARMS_COMMENT);
	    *tfdp = xfreopen(Xf_in, TH_IS_BUFFERED);
	}
	else
	{
	    xfflush(*tfdp);
	}

	/* Change authorization id if permit issuer is someone new. */
	xfread_id(alarm_grantor);
	xfsetauth(*tfdp, alarm_grantor);

        xfsetlang(*tfdp, DB_SQL);
    }
    xfwrite(*tfdp, text_segment);
}
