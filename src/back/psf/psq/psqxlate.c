/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <cm.h>
#include    <qu.h>
#include    <st.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <cui.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <ddb.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <uls.h>

/**
**
**  Name: PSQXLATE.C - Functions for storing translated single-site query text.
**
**  Description:
**	For single-site queries we want to store translated text to be sent to
**	that site.  Functions contained in this module would allow one to add
**	a string to the translated query text buffer, append contents of one
**	list of buffers to the other,
**
**          psq_x_add	       - Add null-terminated string to a buffer.
**	    psq_x_new	       - Allocate new element of the list of packets
**			         containing translated query.
**	    psq_x_backup       - Back over some most recently added chars.
**	    psq_same_site      - Determine if two sites are the same
**          psq_prt_tabname    - print out LDB table name (possibly along
**				 with "owner.").
**  History:
**      30-sep-88 (andre)
**          written
**	15-aug-91 (kirke)
**	    Removed MEcmp() declaration, added include of me.h.
**	15-jun-92 (barbara)
**	    Sybil merge.  PSF no long stores single-site query text as
**	    a means of by-passing OPF so much of the functionality of
**	    this module has been removed.  Some query buffering takes
**	    place on single-site statements that don't go through OPF such as
**	    CREATE TABLE, COPY and SET.
**	01-sep-1992 (rog)
**	    Included uls.h to pull in ULF prototypes.
**	03-dec-1992 (rog)
**	    Included ulf.h before qsf.h.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp(),
**	    psq_x_new(), psq_x_add().
**	17-Jan-2001 (jenjo02)
**	    Removed redeclarations of psq_x_new(), psq_x_add(),
**	    properly prototyped in pshparse.h.
**	    Added PSS_SESSCB* parameter to psq_prt_tabname().
**      01-apr-2010 (stial01)
**          Changes for Long IDs
[@history_template@]...
**/

/*{
** Name: psq_x_add	- Add null-terminated string to a translated query
**			  buffer.
**
** Description:
**      This function will add a string to the translated query buffer.
**	If there is not enough room to store all of the string, new
**	element of the list may be allocated and the remainder of the string
**	will be stored in the new buffer.
**
** Inputs:
**      str_2_store			string to be stored;
**	mem_stream			ptr to descriptor of the stream to be
**					used for allocating of new list
**					elemnents;
**	buf_size			size of buffer to allocate;
**	pkt_list			header of the list of packets.
**	str_len				length of string (in bytes);
**					used only for SCONSTs;
**					will be disregarded if -1;
**	open_delim			ptr to string const opening delimiter
**	close_delim			ptr to string const closing delimiter
**	separator 			a null-terminated string acting as a
**					separator (between words);  very
**					frequently this will be just one char
**	err_blk				ptr to error block
**
** Outputs:
**      pkt_list			string will be added to the end of the
**					list
**	err_blk				Filled in if an error happens
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    May allocate memory
**
** History:
**      30-sep-88 (andre)
**          written
**	29-oct-88 (andre)
**	    changed to allow the separator be a null-terminated string.
**	04-oct-88 (andre)
**	    Modified to handle non-[NULL|space] 0-length strings.
**	15-jun-92 (barbara)
**	    Sybil merge.  This function stays basically the same.
**	    If there's time, I would consider simplifying the interface
**	    to this function.  There's no need to pass in mem_stream
**	    and pkt_list because they are always the same.
**	28-Oct-2010 (kschendel) SIR 124544
**	    Allow for open-delim and close-delim of NULL when str_len is
**	    specified.  There's no need to make the caller jump thru hoops.
**	    YES, this routine needs an interface cleanup.
*/
DB_STATUS
psq_x_add(
	PSS_SESBLK	    *sess_cb,
	char		    *str_2_store,
	PSF_MSTREAM	    *mem_stream,
	i4		     buf_size,
	PSS_Q_PACKET_LIST   *pkt_list,
	i4		     str_len,
	char		    *open_delim,
	char		    *close_delim,
	char		    *separator,
	DB_ERROR	    *err_blk)
{

    DB_STATUS	    status;

					/* ptr to length of CURRENT buffer */
    i4	    *buflen;
    
				     /* ptr to ptr to next position in buf */
    char	    **nextch;

	/* the next 2 vars will be used only when storing string constants */
    char	    *text_const   = str_2_store;
    i4		    txt_const_len = str_len;
    i4		    close_delim_len;

    /*
    ** if there are no buffers in the list, add one
    */
    if (pkt_list->pss_tail == (DD_PACKET *) NULL)
    {
	status = psq_x_new(sess_cb, mem_stream, buf_size, pkt_list, err_blk);
	if (DB_FAILURE_MACRO(status))
	{
	    return(status);
	}
    }
    
    buflen = &pkt_list->pss_tail->dd_p1_len;
    nextch = &pkt_list->pss_next_ch;

    /*									    
    ** If dealing with a string constant, we must account for opening and
    ** closing delimiters.
    */
    if (str_len >= 0)
    {
	if (open_delim != NULL)
	{
	    str_len += CMbytecnt(open_delim);
	    str_2_store = open_delim;	/* Start with open delim if any */
	}
	close_delim_len = 0;
	if (close_delim != NULL)
	{
	    close_delim_len = CMbytecnt(close_delim);
	    str_len += close_delim_len;
	}
    }
        
    for (;;)
    {
	if (str_len < 0)	    /* NOT a string constant */
	{			    /* may be terminated with blank or EOS  */

	    if(CMspace(str_2_store) || (*str_2_store == EOS))
	    {
		/* must have been the last char */
		break;
	    }
	}
	else			    /* must be dealing with text string */
	{
	    if (str_len == 0)
	    {
		break;				/* done copying the string */
	    }
	    else if (str_len == close_delim_len)
	    {
		str_2_store = close_delim;	/* need closing delimiter */
	    }
	    else if (str_len == txt_const_len + close_delim_len)
	    {
		str_2_store = text_const;	/* start copying text */
	    }

	    str_len -= CMbytecnt(str_2_store);
	}

	/*								    
	** if there is no room in the current buffer,
	** allocate new list element
*********** FIXME and what if the string is > buf_size?  gah.
	*/
    	if ((*buflen + CMbytecnt(str_2_store)) > (i4) buf_size)
	{
	    status = psq_x_new(sess_cb, mem_stream, buf_size, pkt_list, err_blk);
	    if (DB_FAILURE_MACRO(status))
	    {
		return(status);
	    }

	    buflen = &pkt_list->pss_tail->dd_p1_len;
	    nextch = &pkt_list->pss_next_ch;
	}

	*buflen += CMbytecnt(str_2_store);	/* increment byte count */
	CMcpyinc(str_2_store, *nextch);		/* copy next char */
    
    }

    /* we may need to add string separator at the end of the current string */
    if (separator != (char *) NULL)
    {
	do {
	    /*								    
	    ** if there is no room in the current buffer,
	    ** allocate new list element
	    */
	    if ((*buflen + CMbytecnt(separator)) > (i4) buf_size)
	    {
		status = psq_x_new(sess_cb, mem_stream, buf_size, pkt_list, err_blk);
		if (DB_FAILURE_MACRO(status))
		{
		    return(status);
		}

		buflen = &pkt_list->pss_tail->dd_p1_len;
		nextch = &pkt_list->pss_next_ch;
	    }

	    *buflen += CMbytecnt(separator);	/* increment byte count */
	    CMcpyinc(separator, *nextch);	/* copy the separator */
	} while (*separator != EOS);

    }
    
    return (E_DB_OK);
}

/*{
** Name: psq_x_new 	- Allocate new element of the list of packets
**			  containing translated query.
**
** Description:
**      This function will allocate a new element of the list of packets and a
**	packet buffer, insert new element into the list and initialize all
**	values as appropriate.
**
** Inputs:
**	mem_stream			ptr to descriptor of the stream to be
**					used for allocating of new list
**					elemnents;
**	buf_size			size of buffer to allocate;
**	pkt_list			header of the list of packets.
**	err_blk				ptr to error block
**
** Outputs:
**      pkt_list			new element will be added to the list
**	err_blk				Filled in if an error happens
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    May allocate memory
**
** History:
**      30-sep-88 (andre)
**          written
**	09-dec-88 (andre)
**	    Added ability to reserve first n bytes in a new packet.  Was
**	    introduced to accomodate CREATE LINK/REGISTER in STAR.
*/
DB_STATUS
psq_x_new(
	PSS_SESBLK	    *sess_cb,
	PSF_MSTREAM	    *mem_stream,
	i4		     buf_size,
	PSS_Q_PACKET_LIST   *pkt_list,
	DB_ERROR	    *err_blk)
{

    DD_PACKET		    *new_elem;
    DB_STATUS		    status;

    /*								    
    ** if as a part of backing up we had empty packet(s) left, reuse one;
    ** otherwise, allocate a new one
    */

    if ((pkt_list->pss_tail == (DD_PACKET *) NULL)		||
	((new_elem = pkt_list->pss_tail->dd_p3_nxt_p) == (DD_PACKET *) NULL))
    {
	status = psf_malloc(sess_cb, mem_stream, sizeof(DD_PACKET), (PTR *) &new_elem,
			    err_blk);
	if (DB_FAILURE_MACRO(status))
	{
	    return(status);
	}

	status = psf_malloc(sess_cb, mem_stream, buf_size,
			    (PTR *) &new_elem->dd_p2_pkt_p, err_blk);
	if (DB_FAILURE_MACRO(status))
	{
	    return(status);
	}

	if (pkt_list->pss_tail != (DD_PACKET *) NULL)
	{
	    pkt_list->pss_tail->dd_p3_nxt_p = new_elem;
	}
	else
	{
	    pkt_list->pss_head = new_elem;
	}

	new_elem->dd_p3_nxt_p = NULL;
    }

    pkt_list->pss_tail = new_elem;
    new_elem->dd_p1_len = (i4) pkt_list->pss_skip;
    pkt_list->pss_next_ch = new_elem->dd_p2_pkt_p + pkt_list->pss_skip;

    return (E_DB_OK);
}

/*{
** Name: psq_x_backup	- Back up over some previously added chars
**
** Description:
**      This function will backup over some previously added characters.  If,
**	in the process, we exhaust contents of the present packet, we'll go to
**	the previous packet.
**
** Inputs:
**	packet_list		ptr to a header of apcket list over contents of
**				which we need to back up.
**	str			string of chars over which to backup
**
** Outputs:
**      packet_list		apprpriate fields will be modified to reflect
**				the fact that some chars have been backed over.
**
**  Returns:
**	    E_DB_OK		Everything's OK
**	    E_DB_ERROR		Tried to backup opver more chars than are
**				available.
**	Exceptions:
**	    none
**
** Side Effects:
**	    NOTE that if we have to back up over more chars than are available
**	    in the present buffer, the present buffer will be retained to be
**	    reused later (in psq_x_new).
**
** History:
**      24-oct-88 (andre)
**          written
*/
DB_STATUS
psq_x_backup(
	PSS_Q_PACKET_LIST   *packet_list,
	char		    *str)
{
    i4			len = STlength(str);
    DD_PACKET		*cur = packet_list->pss_tail;
    
    for (;;)
    {
	if (len <= cur->dd_p1_len)
	{
	    cur->dd_p1_len		-= len;
	    packet_list->pss_next_ch	-= len;
	    break;
	}
	else
	{
	    len -= cur->dd_p1_len;
	    cur->dd_p1_len = 0;

	    /* if no more packets to back over, we have a problem */
	    if (packet_list->pss_head == cur)
	    {
		return(E_DB_ERROR);
	    }
	    else
	    {
		for (cur = packet_list->pss_head;
		     cur->dd_p3_nxt_p != packet_list->pss_tail;
		     cur = cur->dd_p3_nxt_p)
		;

		packet_list->pss_tail = cur;
		packet_list->pss_next_ch = cur->dd_p2_pkt_p + cur->dd_p1_len;
	    }
	}
    }

    return(E_DB_OK);
}

/*
**  psq_same 	- determine if two sites are the same
**
**  Description:
**  psq_same_site compares two LDB descriptors passed in.  If node,
**  database and dbms names match, the function returns TRUE; otherwise
**  FALSE.
**
**  Inputes:
**	ldb_desc1		ptr to first LDB descriptor
**	ldb_desc2 		ptr to second LDB descriptor
**  
**  Outputs:
**
**  Returns:
**	TRUE 	    Descriptors are the same
**	FALSE	    Descriptors are not the same
**
**  Side effects:
**
**  28-oct-88 (andre)
**	Adopted from psl_xlate_link.
**  07-apr-89 (andre)
**	Check dbms_type when deciding if the site is the same.  Also, use
**	MEcmp() instead of STscompare().  Note that comparisons of node and
**	database are case-sensitive, while comparison of dbms_type is
**	case-insensitive.
**  10-jun-92 (barbara)
**	Adapted from psq_single_site.  The former version of the function
**	was part of the mechanism for deciding whether or not to buffer
**	text (for single-site queries).  The function is now used to
**	determine if two sites are the same.
*/
bool
psq_same_site(
	DD_LDB_DESC	    *ldb_desc1,
	DD_LDB_DESC	    *ldb_desc2)
{
    if (MEcmp((PTR) ldb_desc1->dd_l2_node_name,
	      (PTR) ldb_desc2->dd_l2_node_name, sizeof(DD_NODE_NAME))
	||

	MEcmp((PTR) ldb_desc1->dd_l3_ldb_name, 
	      (PTR) ldb_desc2->dd_l3_ldb_name, sizeof(DD_256C))
	||

	STbcompare(
	    ldb_desc1->dd_l4_dbms_name, sizeof(ldb_desc1->dd_l4_dbms_name),
	    ldb_desc2->dd_l4_dbms_name, sizeof(ldb_desc2->dd_l4_dbms_name),
		   (i4) TRUE))
    {
	return FALSE;
    }
    return TRUE;
}

/*
**  psq_prt_tabname 	- print out LDB table name
**
**  Description:
**  psq_prt_tabname will print out LDB table name (possibly along with
**  "owner.".
**
**  Inputs:
**	xlated_qry	 	    info for printing to buffer
**	mem_stream		    ptr to memory stream
**      rng_var           	    ptr to range entry containing description
**	                            of LDB table (among other things) 
**	qmode			    query mode
**	
**  Outputs:
**	err_blk 		    will be filled in if error occurs
**
**  Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**
**  Side effects:
**	May allocate memory for the translated query text.
**
**  21-dec-88 (andre)
**	written
**  09-mar-89 (andre)
**	added qmode, lang, and print_alias + will use uls_ownname() to decide
**	if owner.table construct may be used.
**  10-jun-92 (barbara)
**	As part of the Sybil project, simplified to print only one table
**	(from a range variable).  Lists of tables are no longer required
**	to be printed because single-site query buffering of text is no
**	longer done.
**  20-nov-92 (barbara)
**	Removed double-dereference of 'mem_stream' argument on psq_x_add call.
**  01-mar-93 (barbara)
**	Support for delimited identifiers.
**  10-may-93 (barbara)
**	Minor fixes in buffering delimited owner.tabname.
**  02-sep-93 (barbara)
**	Instead of checking LDB's OPEN/SQL_LEVEL capability to decide whether
**	or not to use delimited identifiers, check existence of
**	DB_DELIMITED_CASE capability.
*/
DB_STATUS
psq_prt_tabname(
	PSS_SESBLK	    *sess_cb,
	PSS_Q_XLATE	    *xlated_qry,
	PSF_MSTREAM	    *mem_stream,
	PSS_RNGTAB	    *rng_var,
	i4		    qmode,
	DB_ERROR	    *err_blk)
{
    DD_2LDB_TAB_INFO	*ldb_tab_info =
	       &rng_var->pss_rdrinfo->rdr_obj_desc->dd_o9_tab_info;
    char		*c1, *c2, *c3;
    DB_STATUS		status = E_DB_OK;
    char		*delimiter;
    u_char		unorm_buf[DB_MAXNAME *2 +4];
    u_i4		unorm_len;
    char		*bufp;
    bool		use_owner = TRUE;
    u_i4		name_len;
                
    name_len = psf_trmwhite(sizeof(DD_OWN_NAME), ldb_tab_info->dd_t2_tab_owner);

    if (ldb_tab_info->dd_t9_ldb_p->dd_i2_ldb_plus.dd_p3_ldb_caps.
				dd_c1_ldb_caps & DD_8CAP_DELIMITED_IDS)
    {
	/* if LDB > 605, okay to delimit names */
	unorm_len = DB_OWN_MAXNAME *2 +2;
	status = cui_idunorm((u_char *)ldb_tab_info->dd_t2_tab_owner, &name_len,
			    unorm_buf, &unorm_len, CUI_ID_DLM, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	c1 = " ";
	c2 = ".";
	c3 = (char *) NULL;

	bufp = (char *)unorm_buf;
	name_len = unorm_len;
    }
    else
    if (use_owner =
	uls_ownname(&ldb_tab_info->dd_t9_ldb_p->dd_i2_ldb_plus.dd_p3_ldb_caps,
			qmode, DB_SQL, &delimiter))
    {
        /* check if OK to use owner.table and the type of delimiter to use */

	if (delimiter != (char *) NULL)
	{
	    c1 = c2 = delimiter;
	    c3 = ".";
	}
	else
	{
	    c1 = " ";
	    c2 = ".";
	    c3 = (char *) NULL;
	}
	
	bufp = ldb_tab_info->dd_t2_tab_owner;
    }	

    if (use_owner)
    {
    	status = psq_x_add(sess_cb, bufp, mem_stream, xlated_qry->pss_buf_size,
		    &xlated_qry->pss_q_list, (i4) name_len, c1, c2, c3,
		    err_blk);

	if (DB_FAILURE_MACRO(status))
	    return(status);
    }

    name_len = psf_trmwhite(sizeof(DD_TAB_NAME), ldb_tab_info->dd_t1_tab_name);

    if (ldb_tab_info->dd_t9_ldb_p->dd_i2_ldb_plus.dd_p3_ldb_caps.
				dd_c1_ldb_caps & DD_8CAP_DELIMITED_IDS)
    {
	unorm_len = DB_TAB_MAXNAME *2 +2;
	status = cui_idunorm((u_char *)ldb_tab_info->dd_t1_tab_name, &name_len,
				unorm_buf, &unorm_len, CUI_ID_DLM, err_blk);

	if (DB_FAILURE_MACRO(status))
	    return (status);

	unorm_buf[unorm_len] = ' ';
	unorm_buf[unorm_len+1] = '\0';  /* Accommodate psq_x_add interface */

        status = psq_x_add(sess_cb, "", mem_stream, xlated_qry->pss_buf_size,
		    &xlated_qry->pss_q_list, -1, (char *) NULL,  (char *) NULL,
		    (char*)unorm_buf, err_blk);
	return (status);
    }

    c1 = ldb_tab_info->dd_t1_tab_name;
	c2 = c1 + CMbytecnt(c1);
	name_len -= CMbytecnt(c1);

    status = psq_x_add(sess_cb, c2, mem_stream, xlated_qry->pss_buf_size,
		    &xlated_qry->pss_q_list, name_len, c1, " ", (char *) NULL,
		    err_blk);

    return(status);
}
