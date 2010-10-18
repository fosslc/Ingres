/*
**Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <dbdbms.h>
# include <cs.h>
# include <cv.h>
# include <me.h>
# include <sp.h>
# include <st.h>
# include <tm.h>
# include <tr.h>
# include <erglf.h>
# include <mo.h>
# include <scf.h>
# include <adf.h>
# include <dmf.h>
# include <dmrcb.h>
# include <dmtcb.h>
# include <dmucb.h>
# include <ulf.h>
# include <ulm.h>
# include <gca.h>

# include <gwf.h>
# include <gwfint.h>

# include "gwmint.h"
# include "gwmplace.h"

/*
** Name: gwmkey.c  -- GM_GKEY related functions
**
** Description:
**
**	Functions for working on keys in the GWM_RSB structures.  Contains the
**	following public functions:
**
** 	GM_clearkey	clear a key to known state.
** 	GM_pos_sub	common code to GOposition and GXposition
**
**	And the following internal functions
**
** 	GM_st_partial_key	set up part of a user search key.
** 	GM_st_key		given a user search key, set up a GKEY.
**
** History:
**	1-Jul-92 (daveb)
**	    Don't set key elements in st_key if inkey_len is 0.  They
**	    may be null pointers.
**	17-Sep-92 (daveb)
**	    pulled from gwmxtab.c, gwmmob.c, and gwmop.c
**	28-sep-92 (daveb)
**	    Big change around use the GM_ATT_ENTRYs, so we now
**	    properly handle keys that aren't in the same order
**	    or location as the attributes in the relation.
**	30-oct-92 (daveb)
**	    Don't use FAIL as a return status in GM_knext_place,
**	    use MO_NO_NEXT.  Remove obsolete GM_find_dmt_att().
**	    Remove unused GM_key_classid().
**	9-oct-92 (daveb)
**	    moved kfirst_place and knext_place to gwmplace.c; documented
**	    some loose ends, removed dead code/vars.
**	19-Nov-1992 (daveb)
**	    include <gca.h> now that we're using GCA_FS_MAX_DATA for the
**	    OPBLK buffer size.
**	2-Dec-1992 (daveb)
**	    Move GM_pos_sub here from gwmutil.c.
**	11-jan-1992 (mikem)
**        Fixed typo in the DB_INT_TYPE i2 and i1 handling, the result of
**        which was that values were being assigned from unitialized memory.
**	2-Feb-1993 (daveb)
**	    improve comments, make set key functions static.
**	12-Mar-1993 (daveb)
**	    Forward declare GM_st_partial_key and GM_st_key to
**	    clear up warnings about static redeclarations.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      17-Nov-1993 (daveb) 58934
**          Cast dest address to MEcopy.
**      25-Jan-1994 (daveb) 59124, 59125
**          Add tracing.  Fix terrible bugs in GM_st_partial key.
**          There was an off-by-one error in picking the key
**          attribute, and then we were looking at the gma_type
**          instead of the gma_key_type when deciding which key field
**  	    to fill in.  The simple test cases where all attributes
**  	    were varchar worked; more complicated ones failed to
**  	    position correctly, resulting either in scans or failure
**  	    to find rows that should exist.
**      18-Mar-1994 (daveb) 60738
**          If we do set a place key, make sure it is valid.  We may
**          be given a partial key, from a statement where place like
**          'noden%' instead of 'nodename::/@server'
**      02-Oct_1994 (Bin Li) 58934
**          Fix the syntax error at line 296, change (src) to src.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
*/

/* forwards */

static DB_STATUS GM_st_partial_key( GM_RSB *gwm_rsb,
				  bool xtab,
				  DM_DATA *keydata,
				  i4  keyatt,
				  DM_PTR *keyatt_list,
				  GM_GKEY *key );

static DB_STATUS GM_st_key( GM_RSB *gwm_rsb,
			  bool xtab,
			  DM_DATA *keydata,
			  DM_PTR *key_att_list,
			  GM_GKEY *key );


/*{
** Name:	GM_clearkey	clear a key to known state.
**
** Description:
**	Zero out the key in preparation of being filled.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	key		key to clear.
**
** Outputs:
**	key		cleared key.
**
** Returns:
**	none.
**
** History:
**	9-oct-92 (daveb)
**	    documented.
*/

/* completely clear a key to a known invalid state */

void
GM_clearkey( GM_GKEY *key )
{
    key->valid = FALSE;
    key->place.len = 0;
    key->classid.len = 0;
    key->instance.len = 0;
    key->place.str[0] = EOS;
    key->classid.str[0] = EOS;
    key->instance.str[0] = EOS;
}

/*
** Name: GM_st_partial_key	set up part of a user search key.
**
**	Fill in the place, classid, or instance part of a GKEY from
**	the tuple given a position call.
**
**	if xtab, then look at keyatt names to decide which key
**	to fill in.  If PLACE, VNODE or SRVR, fill in place; otherwise
**	fill in instance.
**
**	There should never be more than three attributes
**	in an xtab key.  If there is only one, it should be place,
**	or you'll end up scanning.
**	On a non-xtab, you should have one or two or threes keys, with
**	the first being place, and their order is assumed to be place,
**	classid, instance.  We may want to revisit this for keyed
**	indexes.
**
** Inputs:
**	gwm_rsb.
**		gwm_att_list	desc of atts in the tuple.
**		gwm_gwm_atts	decoded desc. of atts and keys.
**
**	xtab			is this a cross-tab key?
**
**	keydata.		input key tuple data
**		data_address	the key buffer.
**	
**	keyatt			attribute to do this trip, 0..nkeys-1;
**				(these correspond to att_key_seq_number - 1.)
**
**	keyatt_list		array indexing key pos to tuple location
**
**	key			key to fill in.
**
** Outputs:
**
**	key			filled in as needed.
**
** History:
**	whenever (Daveb)
**	    Created.
**	28-sep-92 (daveb)
**	    Big change around use the GM_ATT_ENTRYs, so we now
**	    properly handle keys that aren't in the same order
**	    or location as the attributes in the relation.
**	23-Nov-1992 (daveb)
**	    Handle input integer keys.
**	11-jan-1992 (mikem)
**        Fixed typo in the DB_INT_TYPE i2 and i1 handling, the result of
**        which was that values were being assigned from unitialized memory.
**	2-Feb-1993 (daveb)
**	    made static.
**      17-Nov-1993 (daveb)
**          Cast dest address to MEcopy.
**      25-Jan-1994 (daveb)
**          Add tracing.  Fix terrible bugs in GM_st_partial key.
**          There was an off-by-one error in picking the key
**          attribute, and then we were looking at the gma_type
**          instead of the gma_key_type when deciding which key field
**  	    to fill in.  The simple test cases where all attributes
**  	    were varchar worked; more complicated ones failed to
**  	    position correctly, resulting either in scans or failure
**  	    to find rows that should exist.
*/
static DB_STATUS
GM_st_partial_key( GM_RSB *gwm_rsb,
		  bool xtab,
		  DM_DATA *keydata,
		  i4  keyatt,
		  DM_PTR *keyatt_list,
		  GM_GKEY *key )
{
    DB_STATUS	db_stat = E_DB_OK;
    
    GM_ANY_KEY	*anykey;	/* key element to fill in */
    
    char *dst;			/* str ptr in anykey */
    i4  dstlen;			/* len of dst */
    
    char *src;			/* source string in keydata */
    i4  srclen;			/* len of src */
    
    GM_ATT_ENTRY	*gwm_key_att;
    GM_ATT_ENTRY	*gwm_tup_att;
    DMT_ATT_ENTRY	*dmt_att;	/* description of attribute in tuple */
    i2	varlen;			/* length of variable len data in key */
    i2	*key_att_index;	/* map of key att to tuple att number */
    i4	tupatt;		/* att in tuple of key */
    i4	i4val;
    i2	i2val;
    i1	i1val;
    char	buf[ 20 ];	/* number from i4 */
    
    do				/* one-time only */
    {
	/*
	 ** First, locate the key in the buffer provided,
	 ** setting src to it's start as a char buffer, and
	 ** srclen as it's length.
	 */
	
	/* find the offset into the buffer */

	gwm_key_att = &gwm_rsb->gwm_gwm_atts[ keyatt ];
	src = &keydata->data_address[ gwm_key_att->gma_key_offset ];
	key_att_index = (i2 *)keyatt_list->ptr_address;
	
	/* find it's description to get the type and declared length */

	tupatt = key_att_index[ keyatt ] - 1;
	gwm_tup_att = &gwm_rsb->gwm_gwm_atts[ tupatt ];
	dmt_att = gwm_tup_att->gma_dmt_att;
	if( dmt_att == NULL )
	{
	    /* "GWM Internal error:  NULL dmt_attribute setting key att %0d
	       (tup att %1d)" */
	    GM_2error( (GWX_RCB *)0, E_GW8082_GCN_UNPACK,
		      GM_ERR_INTERNAL|GM_ERR_LOG,
		      sizeof( keyatt ), (PTR)&keyatt,
		      sizeof( tupatt ), (PTR)&tupatt );
	    db_stat = E_DB_ERROR;
	    break;
	}
	srclen = dmt_att->att_width;

	/* then get real length and location based on type */
	
	switch( dmt_att->att_type )
	{
	case DB_CHA_TYPE:	/* 20 */
	    
	    /* src and srclen are correct */
	    break;
	    
	case DB_VCH_TYPE:	/* 21 */
	    
	    /* actual length in i2 at start of src */
	    MEcopy( src, sizeof( varlen ), (PTR)&varlen );
	    srclen = varlen;
	    
	    /* adjust src past the length */
	    src += sizeof(varlen);
	    break;
	    
	case DB_INT_TYPE:	/* 30 'i' */

	    switch( dmt_att->att_width )
	    {
	    case sizeof( i4val ):
		MEcopy( src, sizeof( i4val ), (PTR)&i4val );
		break;
	    case sizeof( i2val ):
		MEcopy( src, sizeof( i2val ), (PTR)&i2val );
		i4val = i2val;
		break;
	    case sizeof( i1val ):
		i1val = *src;
		i4val = I1_CHECK_MACRO(i1val);
	    default:
		/* "GWM Internal error:  Bad integer size %0d
		   for column %1c" */
		GM_2error( (GWX_RCB *)0, E_GW8142_BAD_INT_SIZE,
			  GM_ERR_INTERNAL|GM_ERR_LOG,
			  sizeof( dmt_att->att_width ),
			  (PTR)&dmt_att->att_width,
			  dmt_att->att_nmlen, dmt_att->att_nmstr );
		db_stat = E_DB_ERROR;
	    }
	    if( db_stat == E_DB_OK )
	    {
		CVla( i4val, buf );
		src = buf;
		srclen = STlength( buf );
	    }
	    break;
	    
	default:

	    /* "GWM Internal error:  Bad type %0d for column %0c" */
	    GM_2error( (GWX_RCB *)0, E_GW8143_BAD_TYPE,
		      GM_ERR_INTERNAL|GM_ERR_LOG,
		      sizeof( dmt_att->att_type ),
		      (PTR)&dmt_att->att_type,
		      dmt_att->att_nmlen, dmt_att->att_nmstr );
	    db_stat = E_DB_ERROR;
	    break;

	}
	if( db_stat != E_DB_OK )
	    break;

	
	/*
	 * Second, locate and describe the destination
	 * key in "anykey" and dstlen.
	 */
	if( xtab )
	{
	    if( gwm_key_att->gma_key_type == GMA_USER )
		keyatt = 2;
	    else
		keyatt = 0;
	}

	switch( keyatt )
	{
	case 0:
	    
	    anykey = (GM_ANY_KEY *)&key->place;
	    dstlen = sizeof( key->place.str );
	    break;
		
	case 1:
		
	    anykey = (GM_ANY_KEY *)&key->classid;
	    dstlen = sizeof( key->classid.str );
	    break;
		
	case 2:
	    
	    anykey = (GM_ANY_KEY *)&key->instance;
	    dstlen = sizeof( key->instance.str );
	    break;
		
	default:

	    /* "GWM Internal error:  Bad keyatt %0d for column %0c" */
	    
	    GM_2error( (GWX_RCB *)0, E_GW8144_BAD_KEYATTR,
		      GM_ERR_INTERNAL|GM_ERR_LOG,
		      sizeof( keyatt ), (PTR)&keyatt,
		      gwm_key_att->gma_dmt_att->att_nmlen,
		      gwm_key_att->gma_dmt_att->att_nmstr );
	    db_stat = E_DB_ERROR;
	    break;
	}
	
	/*
	 ** Third and finally, copy it and clean it up.
	 */
	dst = anykey->str;
	while( srclen-- && dstlen-- && *src )
	    *dst++ = *src++;
	*dst = EOS;
	
	STtrmwhite( anykey->str );
	anykey->len = STlength( anykey->str );
	
    } while ( FALSE );
    
    return( db_stat );
}


/*
** Name: GM_st_key	given a user search key, set up a GKEY.
**
**	Break down the input string into a place, classid, instance in
**	the provided GKEY structure.
**
** Inputs:
**	gwm_rsb.
**		gwm_att_list	desc of atts in the tuple.
**		gwm_gwm_atts	decoded desc. of atts and keys.
**
**	keydata.		input key tuple data
**		data_in_size	number of keys in the key buffer
**	
**	keyatt_list		array indexing key pos to tuple location
**
**	key			key to fill in.
**
** Outputs:
**
**	key			the key to fill in.
**
** Returns:
**	E_DB_ERROR	if input key length looks wrong.
**	E_DB_OK		if key was loaded properly.
**
** History:
**	1-Jul-92 (daveb)
**	    Don't set key elements in st_key if inkey_len is 0.  They
**	    may be null pointers.
**	2-Feb-1993 (daveb)
**	    made static.
*/
static DB_STATUS
GM_st_key( GM_RSB *gwm_rsb,
	  bool xtab,
	  DM_DATA *keydata,
	  DM_PTR *key_att_list,
	  GM_GKEY *key )
{
    DB_STATUS db_stat = E_DB_OK;
    i4  keyatt;
    
    if( keydata->data_in_size > key_att_list->ptr_in_count )
    {
	/* E_GW8145_BAD_ATTR_COUNT:SS50000_MISC_ING_ERRORS
	   "GWM Internal error:  Bad key attr count %0d, expecting %1d" */

	GM_2error( (GWX_RCB *)0, E_GW8145_BAD_ATTR_COUNT,
		  GM_ERR_INTERNAL|GM_ERR_LOG,
		  sizeof( keydata->data_in_size ),
		  (PTR)&keydata->data_in_size,
		  sizeof( key_att_list->ptr_in_count),
		  (PTR)&key_att_list->ptr_in_count );
	db_stat = E_DB_ERROR;
    }
    else
    {
	for( keyatt = 0;
	    db_stat == E_DB_OK && keyatt < keydata->data_in_size;
	    keyatt++ )
	{
	    db_stat = GM_st_partial_key( gwm_rsb,
					xtab,
					keydata,
					keyatt,
					key_att_list,
					key );
	}
    }
    return( db_stat );
}



/*
** Name: GM_pos_sub - common code to GOposition and GXposition
**
** Description:
**	Set the GM_RSB keys for the position specified.
**
**	For a flat table, the input key comes in three parts: a place,
**	an classid, and an instance.  The start key is validated.
**	Only if it is valid do we mark the key as valid, so we will do
**	an exact get on the first gateway "get" row.  If the row is
**	not present, then we zero classid and instance out to force a
**	scan.  
**
**	For an xtab, the key only has two pieces:  place and instance.
**	Thus we can't validate the start key, because we don't know
**	the name of the index classid to use.  We leave this dangling.
**
** Inputs:
**	gwx_rcb->xrcb_rsb.
**	    .xrcb_var_data1.data_in_size	number of start key attrs
**						supplied
**	    .xrcb_var_data1.data_address	tuple holding start key attrs
**
**	    .xrcb_var_data2.data_in_size	number of stop key attrs
**						supplied
**	    .xrcb_var_data2.data_address	tuple holding stop key attrs
**
**	    .xrcb_var_data_lst.ptr_address	key description array.
**	    .xrcb_var_data_lst.ptr_in_count	number of elements in key
**						description array.
** Outputs:
**	gwx_rcb->xrcb_rsb->gwrsb_internal_rsb.cur_key
**	gwx_rcb->xrcb_rsb->gwrsb_internal_rsb.last_key
**
** Returns:
**	DB_STATUS
**
** History:
**	30-sep-92 (daveb)
**	    Remove active key validation from GM_pos_sub, punting
**	    it to the G?get routines, which should know how to do
**	    it properly, and which won't waste an extra exchange
**	    if the positioned key is a match.
**	14-Nov-1992 (daveb)
**	    Set cur_key to first_place if user didn't set it.
**	2-Feb-1993 (daveb)
**	    improve comments.
*/
DB_STATUS
GM_pos_sub( GWX_RCB *gwx_rcb, bool xtab )
{
    GW_RSB	*rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GM_RSB	*gwm_rsb = (GM_RSB *)rsb->gwrsb_internal_rsb;
    DB_STATUS	db_stat = E_DB_OK;
    GM_GKEY	*key;
    char    	*p;
    
    if( GM_globals.gwm_trace_positions )
	TRdisplay("GWM positioning rsb %p, gwm_rsb %p\n", rsb, gwm_rsb );

    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    do
    {
	if (gwm_rsb == NULL)
	{
	    gwx_rcb->xrcb_error.err_code = E_GW0401_NULL_RSB;
	    db_stat = E_DB_ERROR;
	    break;
	}
	
	gwm_rsb->didfirst = FALSE;
	
	/* Set initial key, if provided */
	
	key = &gwm_rsb->cur_key;
	GM_clearkey( key );
	if (gwx_rcb->xrcb_var_data1.data_address != NULL)
	{
	    if( GM_globals.gwm_trace_positions )
		TRdisplay("  cur key");

	    db_stat = GM_st_key( gwm_rsb, xtab,
				&gwx_rcb->xrcb_var_data1,
				&gwx_rcb->xrcb_var_data_list, key );
	    if( db_stat == E_DB_OK )
		key->valid = TRUE;
	    else
		break;
	}
	/* if we have a place, validate it as best we can (60738) */
	if( key->place.len != 0 )
	    GM_key_place( gwm_rsb );

	if( key->place.len == 0 )
	{
	    GM_rsb_place( gwm_rsb );
	    if( GM_globals.gwm_trace_positions )
		TRdisplay(" (start place defaulted)\n");
	}

	/* Set the terminating key if one is provided */

	key = &gwm_rsb->last_key;
	GM_clearkey( key );
	if ( gwx_rcb->xrcb_var_data2.data_address != NULL )
	{
	    if( GM_globals.gwm_trace_positions )
		TRdisplay("  last key");

	    db_stat = GM_st_key( gwm_rsb, xtab,
				&gwx_rcb->xrcb_var_data2,
				&gwx_rcb->xrcb_var_data_list, key );
	    if( db_stat == E_DB_OK )
		key->valid = TRUE;
	    else
		break;
	}
	if( key->place.len == 0 )
	{
	    GM_klast_place( gwm_rsb );
	    if( GM_globals.gwm_trace_positions )
		TRdisplay(" (last place defaulted)\n");
	}
	       
	/*
	** make sure end is not before the start key.  If the
	** request is for vnode = 'literal', then the start key is
	** likely to be 'literal::/@real_server', and a comparison
	** would say it was done before starting.
	*/
	if( gwm_rsb->dom_type == GMA_VNODE &&
	   STcompare( key->place.str, gwm_rsb->cur_key.place.str ) < 0 )
	{
	    if( GM_globals.gwm_trace_positions )
		TRdisplay(" (last place forced after start)\n");
	    key->place = gwm_rsb->cur_key.place;
	}

    } while ( FALSE );

    if( GM_globals.gwm_trace_positions )
    {
	GM_dump_key( "\tcur_key:  ", &gwm_rsb->cur_key );
	GM_dump_key( "\tlast_key: ", &gwm_rsb->last_key );
    }

    gwx_rcb->xrcb_error.err_code =
	db_stat == E_DB_OK ? db_stat : E_GW8146_GM_POS_ERROR;

    return( db_stat );
}


/*{
** Name:	GM_dump_key -- print key with TRdisplay for tracing.
**
** Description:
**	Prefixes the message, then dumps the key to the log nicely.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	msg 	string to prefix the contents with.
**  	key 	key to show; may be invalid.
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** Exceptions: (if any)
**	none.
**
** History:
**	21-Jan-1994 (daveb)
**	    created.
*/

void
GM_dump_key( char *msg, GM_GKEY *key )
{
    TRdisplay("%s%s place '%s'  classid '%s'  instance '%s'\n",
	      msg,
	      key->valid ? "(valid) " : "INVALID",
	      key->place.str,
	      key->classid.str,
	      key->instance.str );
}

