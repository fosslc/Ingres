
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
** Name:	xfpermit.sc - write statements to create permit.
**
** Description:
**	This file defines:
**
**	xfpermits	write statement to create all permits for given object.
**	xfalltoall	write statement to create all-to-all and ret-to-all
**				permits for given object.
**
** History:
**	13-jul-87 (rdesmond) written.
**	14-sep-87 (rdesmond) 
**		changed att name "number" to "permit_number" in iipermits.
**	10-nov-87 (rdesmond) 
**		added 'language', 'alltoall' and 'rettoall' parms.
**	10-mar-88 (rdesmond)
**		now writes 'ret to all' and 'all to all' in SQL only, since
**		permit may be on view, and this is not allowed in QUEL;
**		removed trim() from target list for 'text'; writes language for
**		each permit before text of permit (based on first word of text
**		being 'create' or not); writes "\p\g" after each permit.
**	11-aug-88 (marian)
**		Added a check for 'grant' so the dml will be set correctly to
**		\sql when the 'grant' or 'create' commands are given for a
**		permit.
**	18-aug-88 (marian)
**		Added support for permits on stored procedures.  The retrieve
**		on iipermits is different than the retrieve for permits on
**		tables.  Even though the retrieves are different the retrieve
**		loop that is processed for each row is the same.
**	18-aug-88 (marian)
**		Changed retrieve statement to reflect column name changes in
**	11-oct-88 (marian)
**		Removed procedure specific retrieve and added where clause
**		restriction for object_type so the same retrieve can be used
**		for permits on tables and procedures.  Added type as a new
**		input parameter.
**	20-jul-89 (marian)	bug 6715
**		Define permits correctly for quel tables.  Don't always
**		use sql statements.
**	28-jul-89 (marian)
**		Fix 6715 correctly....messed up the if statement the first
**		time.
**	05-mar-1990 (mgw)
**		Changed #include <erxf.h> to #include "erxf.h" since this is
**		a local file and some platforms need to make the destinction.
**      27-jul-1992 (billc)
**              Rename from .qsc to .sc suffix.
**      21-Mar-2000 (hanal04) Bug 100911 INGDBA 63.
**              If we are not copying the whole database generate permit
**              information using a series of restricted queries to avoid
**              unnecessarily large retrievals.
**      28-jan-2009 (stial01)
**          Use DB_MAXNAME for database objects.
**/

/* # define's */
/* GLOBALDEF's */

/* extern's */
GLOBALREF bool	With_distrib;
GLOBALREF PTR   Obj_list;

/* static's */
static void writepermit( TXT_HANDLE	**tfdp, 
	char	*object_name,
	i2	permit_number,
	i4	text_sequence,
	char	*permit_grantor,
	char	*text_segment);

/*{
** Name:	xfpermits - write statement to create permit.
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
**	13-jul-87 (rdesmond) 
**		written.
**	14-sep-87 (rdesmond) 
**		changed att name "number" to "permit_number" in iipermits.
**	10-nov-87 (rdesmond) 
**		added 'language', 'alltoall' and 'rettoall' parms.
**	10-mar-88 (rdesmond)
**		now writes 'ret to all' and 'all to all' in SQL only, since
**		permit may be on view, and this is not allowed in QUEL;
**		removed trim() from target list for 'text'; writes language for
**		each permit before text of permit (based on first word of text
**		being 'create' or not); writes "\p\g" after each permit.
**	11-oct-88 (marian)
**		Changed create permit to grant permit and all to public for 
**		rettoall and alltoall.
**      21-Mar-2000 (hanal04) Bug 100911 INGDBA 63.
**              If we are not copying the whole database use the Obj_list
**              to generated queries that retrieve permit information
**              restricted by the object (table) name.
**	22-Oct-2002 (guphs01)
**		In ingres 6.5 the iipermits catalog does not contain the 
**		value 'V' for views and they are also marked as 'T'. Thus
**		fixed the sql in post 26 case, to also check iitables so 
**		views are not written out when writing permissions.
**      13-July-2004 (zhahu02)
**          Updted for writing permits on sequences (INGSRV2893/b112604).
**      31-Mar-08 (coomi01)  b120176
**          Do not transfer permits on iietab_xx_xx tables.
**          These are used for blobs, and the exact name of the target 
**          table can not be guaranteed.... It certainly will not be the 
**          same!
*/

void
xfpermits()
{
EXEC SQL BEGIN DECLARE SECTION;
    char	text_segment[XF_PERMITLINE + 1];
    char	permit_grantor[DB_MAXNAME + 1];
    char	object_name[DB_MAXNAME + 1];
    char        entry_name[DB_MAXNAME + 1];
    char	object_owner[DB_MAXNAME + 1];
    i2		permit_number;
    i2		permit_depth;
    i4		text_sequence;
EXEC SQL END DECLARE SECTION;
    TXT_HANDLE		*tfd = NULL;
    DB_STATUS           status;
    char                *entry_key;
    auto PTR            dat;


    if (With_65_catalogs)
    {
	/* 
	** permit_grantor column added in 6.5 to handle WITH GRANT OPTION.
	** permit_depth to handle WGO.  Can't use timestamps.  Consider this
	** scenario:
	** 
        **	A: grant select WGO to B, C
        **	B: grant select to X
        **	C: grant select WGO to B
        **	A: revoke select from B
	**
	** B's grant to X is still valid, but if we reload in timestamp order,
	** we get:
	** 
        **	A: grant select WGO to C
        **	B: grant select to X
        **	C: grant select WGO to B
	**
	** And B's grant will fail.  The fix is to order by "depth", an 
	** indicator of how "far" you are from the object owner.
	*/

        if(Owner[0] == EOS || Obj_list == NULL)
        {
	    /* Don't pick up system catalogs, but do do extended cats. */
            EXEC SQL SELECT p.object_name, p.object_owner,
                            p.permit_grantor, p.permit_depth,
                            p.permit_number, p.text_sequence, p.text_segment
                    INTO :object_name, :object_owner,
                            :permit_grantor, :permit_depth,
                            :permit_number, :text_sequence, :text_segment
                    FROM iipermits p
                    WHERE (p.object_owner = :Owner OR '' = :Owner)
                        AND p.object_type in('T','S')
			AND ( LEFT(LOWERCASE(p.object_name),7) != 'iietab_') 
			AND (LOWERCASE(p.object_owner) != '$ingres'
			    OR LOWERCASE(p.object_name) LIKE 'ii$_%' ESCAPE '$')
                    ORDER BY p.permit_depth, p.permit_grantor,
                                    p.object_owner, p.object_name,
                                    p.permit_number, p.text_sequence;
            EXEC SQL BEGIN;
            {
                writepermit(&tfd, object_name,
                            permit_number, text_sequence,
                            permit_grantor, text_segment);
            }
            EXEC SQL END;
        }
        else
        {
            status = IIUGhsHtabScan(Obj_list, FALSE, &entry_key, &dat);
 
            while(status)
            {
                STcopy((char *)dat, entry_name);
                EXEC SQL SELECT p.object_name, p.object_owner,
                                p.permit_grantor, p.permit_depth,
                                p.permit_number, p.text_sequence, p.text_segment
                        INTO :object_name, :object_owner,
                                :permit_grantor, :permit_depth,
                                :permit_number, :text_sequence, :text_segment
                        FROM iipermits p
                        WHERE (p.object_owner = :Owner OR '' = :Owner)
                                AND p.object_type = 'T'
                                AND p.object_name = :entry_name
                        ORDER BY p.permit_depth, p.permit_grantor,
                                        p.object_owner, p.object_name,
                                        p.permit_number, p.text_sequence;
                EXEC SQL BEGIN;
                {
                    writepermit(&tfd, object_name,
                                permit_number, text_sequence,
                                permit_grantor, text_segment);
                }
                EXEC SQL END;
 
                status = IIUGhsHtabScan(Obj_list, TRUE, &entry_key, &dat);
            }
        }
    }
    else
    {
	/* before 6.5, the object owner is the only possible permit grantor. */

        if(Owner[0] == EOS || Obj_list == NULL)
        {
            EXEC SQL SELECT p.object_name, p.object_owner,
                            p.permit_number, p.text_sequence, p.text_segment
                    INTO :object_name, :object_owner,
                            :permit_number, :text_sequence, :text_segment
                    FROM iipermits p, iitables t
                    WHERE (p.object_owner = :Owner OR '' = :Owner)
                            AND p.object_type = 'T'
			    AND t.table_name = p.object_name 	
			    AND t.table_owner = p.object_owner 
			    AND t.table_type = 'T'
                    ORDER BY p.object_owner, p.object_name,
                                    p.permit_number, p.text_sequence;
            EXEC SQL BEGIN;
            {
                writepermit(&tfd, object_name,
                            permit_number, text_sequence,
                            object_owner, text_segment);
            }
            EXEC SQL END;
        }
        else
        {
            status = IIUGhsHtabScan(Obj_list, FALSE, &entry_key, &dat);
 
            while(status)
            {
                STcopy((char *)dat, entry_name);
 
                EXEC SQL SELECT p.object_name, p.object_owner,
                                p.permit_number, p.text_sequence, p.text_segment
                        INTO :object_name, :object_owner,
                                :permit_number, :text_sequence, :text_segment
                        FROM iipermits p, iitables t
                        WHERE (p.object_owner = :Owner OR '' = :Owner)
                                AND p.object_type = 'T'
                                AND p.object_name = :entry_name
				AND t.table_name = p.table_name    
                                AND t.table_owner = p.object_owner 
                                AND t.table_type = 'T'
                        ORDER BY p.object_owner, p.object_name,
                                        p.permit_number, p.text_sequence;
                EXEC SQL BEGIN;
                {
                    writepermit(&tfd, object_name,
                                permit_number, text_sequence,
                                object_owner, text_segment);
                }
                EXEC SQL END;
 
                status = IIUGhsHtabScan(Obj_list, TRUE, &entry_key, &dat);
            }
        }
    }

    if (tfd != NULL)
	xfclose(tfd);
}

/*{
** Name:	writepermit - write out permission information. 
**
** Description:
**
** Inputs:
**    tfdp		pointer to TXT_HANDLE pointer, we open this on the
**				first call.
**    object_name	name of the object.
**    permit_number	the permit number
**    text_sequence	sequence number of the text segment.
**    permit_grantor	name of person granting the permission.
**    text_segment	the text of the permission.
**
** Outputs:
**    tfdp		pointer to TXT_HANDLE pointer, we open this on the
**				first call.
*/

static void
writepermit(
    TXT_HANDLE	**tfdp, 
    char	*object_name,
    i2		permit_number,
    i4		text_sequence,
    char	*permit_grantor,
    char	*text_segment
    )
{
    /* if new permit statement */
    xfread_id(object_name);
    if (!xfselected(object_name))
	return;

    if (text_sequence == 1)
    {
	if (*tfdp == NULL)
	{
	    xfwritehdr(GRANTS_COMMENT);
	    *tfdp = xfreopen(Xf_in, TH_IS_BUFFERED);
	}
	else
	{
	    xfflush(*tfdp);
	}

	/* Change authorization id if permit issuer is someone new. */
	xfread_id(permit_grantor);
	xfsetauth(*tfdp, permit_grantor);

	/* set language permit is written in */
	if (STbcompare(text_segment, 6, ERx("create"), 6, FALSE) == 0
	      || STbcompare(text_segment, 5, ERx("grant"), 5, FALSE) == 0)
	{
	    xfsetlang(*tfdp, DB_SQL);
	}
	else
	{
	    xfsetlang(*tfdp, DB_QUEL);
	}
    }
    xfwrite(*tfdp, text_segment);
}
/*{
** Name:	xfalltoall - write any all-to-all permits.
**
** Description:
**	These permits are special-cased because the permission information
**	is stored with the table/view information in the catalogs, rather
**	than in the iipermits catalog.
**
** Inputs:
**	op		pointer to XF_TABINFO struct describing object.
**
** Outputs:
**
**	Returns:
**
** History:
**	20-feb-92 (billc) 
**		cloned from xfpermit.
*/

void
xfalltoall(op)
XF_TABINFO	*op;
{
    char	pbuf[1024];

    if (With_65_catalogs)
    {
	/* Can't use alltoall and rettoall fields after 6.4. */
	return;
    }

    if (With_distrib)
    {
	/* No GRANT allowed in STAR. */
	return;
    }

    /*
    ** bug 6715
    ** If the language is QUEL and this is not a view, use quel statements,
    ** not sql.
    */
    if (op->alltoall[0] == 'Y')
    {
	STprintf(pbuf, ERx("grant all on %s to public%s"), op->name, GO_STMT);
	xfwrite(Xf_in, pbuf);
    }
    if (op->rettoall[0] == 'Y')
    {
	STprintf(pbuf, ERx("grant select on %s to public%s"), 
						op->name, GO_STMT);
	xfwrite(Xf_in, pbuf);
    }
}

