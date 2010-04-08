/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>

#include    <cm.h>
#include    <cv.h>				 
#include    <cvgcc.h>
#include    <gcccl.h>
#include    <lo.h>
#include    <me.h>
#include    <nm.h>
#include    <qu.h>
#include    <si.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>

#include    <sl.h>
#include    <iicommon.h>
#include    <gca.h>
#include    <gcu.h>
#include    <gcc.h>
#include    <gccpci.h>
#include    <gccer.h>
#include    <gccpl.h>
#include    <gccgref.h>
#include    <gcxdebug.h>

/*
**
**  Name: GCCCSET.C - Character set negotiation support
**
**  Description:
**
**	This module provides character set negotiation, which supports
**	interoperability between INGRES installations using different
**	character sets.  On startup, it reads in character set
**	transliteration tables, which provide mappings between the
**	various character sets.  On connection establishment, it
**	negotiates which of the character sets will be used in the
**	network connection, by offering both the local installation's
**	character set and several character sets into which there is a
**	mapping from the local character set.
**
** 	This implementation currently does not support mapping 16 bit 
**	character sets.
**
**	See "INGRES/NET Character Sets," last updated May 1991, by
**	Christopher Seiwald.
**    
**      Reading the two support files and managing the internal tables is 
**      done by:
**    
**	    gcc_read_cnam - read gcccset.nam
**	    gcc_add_cnam - callback routine for gcc_read_cnam/gcu_read_cset
**	    gcc_find_cnam - find character id by its name
**	    gcc_read_ctbl - read gcccset.xlt
**	    gcc_new_ctbl - create character set transliteration table
**	    gcc_find_ctbl - find a transliteration table, given char set ids
**    
**      The main entry point for initialization, called by the gccpl, is:
**    
**	    gcc_init_cset - initialize character set tables
**    
**      Support routines for negotiation are:
**    
**	    gcc_nput_av - translate GCC_ARCH_VEC into network transfer syntax
**	    gcc_nget_av - translate GCC_ARCH_VEC from network transfer syntax
**	    gcc_init_av - initialize arch vec for this arch/char set
**	    gcc_cmp_av - compare arch vectors
**    
**      The routines which actually perform the negotiation on connection
**      request, indication, response and confirm are:
**    
**	    gcc_rqst_av - initialize arch vec for connection request
**	    gcc_ind_av - examine architecture vector on connection indication
**	    gcc_rsp_av - format arch vec for connection response
**	    gcc_cnf_av - examine architecture vector on connection confirmation
**
**  History:
**	20-May-91 (seiwald)
**	    Written.
**	15-Jul-91 (cmorris)
**	    Fixed tracing in gcc_ind_av.
**	23-Jul-91 (seiwald)
**	    Rename II_GCC_CHARSET to II_CHARSET and moved the files 
**	    gcccset.xlt and gcccset.nam down underneath files/charsets.
**	31-Jul-91 (seiwald)
**	    Scan strings by hand, since STscanf is dead.
**	2-Aug-91 (seiwald)
**	    Use CVahxl since CVuahxl died in infancy.
**	    Renamed directory charset to charsets, to agree with our
**	    agreed upon name.
**	2-Aug-91 (seiwald)
**	    Added required length argument to STindex.
**      13-Aug-91 (cmorris)
**  	    Added include of tm.h.
**	14-Aug-91 (seiwald)
**	    Renamed CONTEXT structure to PL_CONTEXT to avoid clash on OS/2.
**	18-Sep-91 (seiwald)
**	    Fix coding of << operator in gcc_init_cset.
**	15-nov-91 (leighb)
**	    DeskTop Porting Change: added include of <cv.h>
**	6-Apr-92 (seiwald)
**	    Support for doublebyte characters.
**	7-Apr-92 (seiwald)
**	    Use u_i2 instead of non-portable u_short.
**	16-Feb-93 (seiwald)
**	    Including gcc.h now requires including gca.h.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	21-Sep-93 (brucek)
**	    Cast args to MEcopy into PTRs.
**      24-mar-95 (peeje01)
**	    Cross Integration doublebyte changes from 6500db_su4_us42:
**          21-Sep-93 (hming)
**             Fixed bug in inserting new transliteration table, ctbl, onto
**             IIGCc_global->gcc_cset_xlt_q in class order.
**             Added a new variable prev_ptr to keep the address of the previous
**             node in the list, and pass this address to QUinsert.
**      10-apr-95 (angusm)
**          cross-int from ingres63p: force default char set to be
**          ISO 8895-1 as documented if II_CHARSETxx is not set
**          (else problems passing chars in range 0x80->0xff over net)
**	    bug 56258.
**	20-Nov-95 (gordy)
**	    Added prototypes.  Bumped trace level for character set
**	    mappings to reduce large output at level 3.
**	22-Dec-95 (gordy)
**	    Only maintain character set mapping tables for our local
**	    character set.  When initiating a connection we only send
**	    character sets which can be mapped with our local set.
**	    When accepting a connection or connection response we look
**	    for sets which can be mapped to our local set.  Mappings
**	    which are not related to our local set are never used so
**	    we shouldn't bother using resources for them.
**	24-par-96 (chech02, lawst01)
**	    cast %d arguments to i4  in TRdisplay for windows 3.1 port.
**	19-jan-1999 (muhpa01)
**	    Removed NO_OPTIM for hp8_us5.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	25-may-1999 (hanch04)
**	    Length passed to STncasecmp should not include the EOS.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      25-Jun-2001 (wansh01) 
**          replace gcxlevel with IIGCc_global->gcc_trace_level
**	20-Aug-02 (gordy)
**	    If no mapping to NSCS exists, use local <-> local as default.
**	    Must then generate error during het/net negotiation if no
**	    compatible network character set exists.  This permits the
**	    definition of character sets which are only compatible with
**	    themselves.
**	26-Dec-02 (gordy)
**	    Character set definition file processing moved to gcu.
**      18-Nov-2004 (Gordon.Thorpe@ca.com and Ralph.Loen@ca.com)
**          Use CMGetDefCS() to retrive the CMdefcs global, otherwise
**          the reference to CMdefcs cannot be resolved in Windows.
**	22-Jan-10 (gordy)
**	    Added identity mappings and optimized mapping storage.
**	    Charset translation moved to gco.  Force-het made static to 
**	    reduce overhead.  Use CM for common charset initialization.
*/

#define	GCC_HET_UNK	-1
#define	GCC_HET_OFF	0
#define	GCC_HET_ON	1

/*
**  Forward and/or External function references.
*/

GLOBALREF u_char	IIGCc_inxl_tbl[];
static	  i4		cset_count = 0;
static    i4		force_het = GCC_HET_UNK;

static STATUS		gcc_read_cnam( VOID );
static bool		gcc_add_cnam( char *, u_i4 );
static GCC_CSET_NAME	*gcc_find_cnam( char *name );
static STATUS		gcc_read_ctbl( u_i2 lcl_id );
static VOID		gcc_fill_ctbl( u_i1 *n2l, u_i1 *l2n );
static STATUS		gcc_new_ctbl( GCC_CSET_XLT *usrctbl );
static GCC_CSET_XLT	*gcc_find_ctbl( u_i2 lcl_id, u_i2 net_id );
static VOID		gcc_nget_av( char *ptr, GCC_ARCH_VEC *av );
static VOID		gcc_init_av( u_i2 lcl_id, GCC_ARCH_VEC *av );
static int		gcc_cmp_av(GCC_ARCH_VEC *lcl_av, GCC_ARCH_VEC *rmt_av);


/*
** Name: gcc_read_cnam - read gcccset.nam
**
** Description:
**	Reads the II_INGRES/ingres/files/gcccset.nam file, which maps
**	between character set names and their u_i2 ids.  Each name
**	to id mapping is QUEUE'd onto IIGCc_global->gcc_cset_nam_q.
**
**	The format of this file is:
**
**		# comments
**		<name> <id>
**
**	where <name> is the (case insensitive) name of the character set,
**	and id is a u_i2 in hex.
**
**	This routine succeeds if the gcccset.nam file doesn't exist, to 
**	provide for backward compatibility.
**
** Returns:
**	OK
**	E_GC2004_ALLOCN_FAIL	alloc failed
**	E_GC2451_CSET_FAIL	error in file format
**
** Side effects:
**	Logs error as well as returning status.
**
** History:
**	8-Mar-91 (seiwald)
**	    Written.
**	23-Jul-91 (seiwald)
**	    Moved gcccset.nam into charsets subdirectory.
**	31-Jul-91 (seiwald)
**	    Scan strings by hand, since STscanf is dead.
**	01-Feb-93 (mrelac)
**	    For MPE/iX (hp9_mpe) only, don't call LOfaddpath().
**	24-mar-93 (smc)
**	    Added forward declaration of gcc_new_ctbl().
**	28-apr-93 (vijay)
**	    Correct forward declaration of gcc_new_ctbl.
**	26-Dec-02 (gordy)
**	    File processing extracted to gcu_read_cset().
*/

static STATUS
gcc_read_cnam( VOID )
{
    i4		lineno;
    GCC_ERLIST	erlist;
    STATUS	status = OK;

    cset_count = 0;

    if ( (lineno = gcu_read_cset( gcc_add_cnam )) < 0 )
	status = E_GC2004_ALLOCN_FAIL;	/* Only reason gcc_add_cnam() stops */
    else  if ( lineno > 0 )
    {
	status = E_GC2452_BAD_CNAM;
	erlist.gcc_parm_cnt = 1;
	erlist.gcc_erparm[0].size = sizeof( lineno );
	erlist.gcc_erparm[0].value = (PTR)&lineno;
	gcc_er_log( &status, (CL_ERR_DESC *)NULL, (GCC_MDE *)NULL, &erlist );
	status = E_GC2451_CSET_FAIL;
    }
   
# ifdef GCXDEBUG
    if ( IIGCc_global->gcc_trace_level >= 1)
	TRdisplay( "gcc_read_map: read %d mappings\n", cset_count );
# endif /* GCXDEBUG */

    return( status );
}

/*
** Name: gcc_add_cnam
**
** Description:
**	Callback routine for gcu_read_cset() as called by gcc_read_cnam().
**	Saves the character set name and numeric ID in global queue.
**	Terminates file processing if memory allocation occurs.
**
** Input:
**	name	Character set name.
**	id	Character set numeric ID.
**
** Output:
**	None.
**
** Returns:
**	bool	FALSE to terminate file processing, TRUE to continue.
**
** History:
**	26-Dec-02 (gordy)
**	    Extracted from gcc_read_cnam().
*/

static bool
gcc_add_cnam( char *name, u_i4 id )
{
    GCC_CSET_NAME	*cname;

    if ( ! (cname = (GCC_CSET_NAME *)gcc_alloc( sizeof(*cname) )) )
	return( FALSE );	/* End processing */

    cset_count++;
    STncpy( cname->name, name, sizeof( cname->name ) - 1 );
    cname->name[ sizeof( cname->name ) - 1 ] = EOS;
    cname->id = id;
    QUinsert( (QUEUE *)cname, &IIGCc_global->gcc_cset_nam_q );

    return( TRUE );	/* Continue processing */
}

/*
** Name: gcc_find_cnam - find character id by its name
**
** Description:
**	Scans the IIGCc_global->gcc_cset_nam_q for a character set with
**	the given name.
**
** Returns:
**	0	no such character set
**	pointer to GCC_CSET_NAME of character set
**
** History:
**	8-Mar-91 (seiwald)
**	    Written.
*/

static GCC_CSET_NAME *
gcc_find_cnam( char *name )
{
    QUEUE		*cnq;
    GCC_CSET_NAME 	*cname;

    /* Scan the queue.  Return the first (and hopefully only) name match. */

    for( cnq = IIGCc_global->gcc_cset_nam_q.q_next; 
	 cnq != &IIGCc_global->gcc_cset_nam_q;
	 cnq = cnq->q_next )
    {
	cname = (GCC_CSET_NAME *)cnq;

	if( !STbcompare( cname->name, 0, name, 0, TRUE ) )
	    return cname;
    }

    /* Bogus name. */

    return 0;
}


/*
** Name: gcc_read_ctbl - read gcccset.xlt
**
** Desription:
**	Reads the file II_INGRES/ingres/files/gcccset.xlt, which maps
**	characters between various character set pairs.  Each mapping
**	is QUEUE'd on IIGCc_global->gcc_cset_xlt_q.
**
** File format:
**
**	    # comments
**	    local <name>
**	    remote <name>
**	    class <class>
**	    doublebyte
**	    begin
**	    <lclval> = <rmtval> [, ... ]
**	    ...
**	    end
**
**	<name> is the (case insensitive) name of the character
**	set as found in gcccset.nam.
**
**	<class> is one of "first," "second," or "third," indicating the
**	preference of the mapping, which determines the order in which
**	the character sets are offered:
**
**	    first 	Denotes a preferred mapping from one minor 
**			character set to another.
**
**	    second 	Denotes a mapping from a character set to the 
**			representative character set from a group of 
**			similar character sets.
**
**	    third 	For mappings into the NSCS, which is considered 
**			the last resort.  
**
**	Earlier mappings are preferred over later mappings in the same class.
**
**	The doublebyte line, if present, indicates that the mappings will
**	map between 16 bit character sets.
**
**	For 8 bit character sets, <lclval> and <rmtval> specifies the
**	local and remote character; they are hex numbers in the range
**	0x00 to 0xFF.
**
**	For 16 bit character sets, <lclval> and <rmtval> are hex
**	numbers of the range 0x0000 to 0xFFFF, and a non-zero high-byte
**	indicates that the corresponding value is a doublebyte
**	character.
**
**	This routine succeeds if the gcccset.xlt file doesn't exist, to 
**	provide for backward compatibility.
**
** Input:
**	lcl_id		Local character set.
**
** Output:
**	None.
**
** Returns:
**	OK
**	E_GC2004_ALLOCN_FAIL	alloc failed
**	E_GC2451_CSET_FAIL	error in file format
**
** Side effects:
**	Logs error as well as returning status.
**
** History:
**	8-Mar-91 (seiwald)
**	    Written.
**	23-Jul-91 (seiwald)
**	    Moved gcccset.xlt into charsets subdirectory.
**	31-Jul-91 (seiwald)
**	    Scan strings by hand, since STscanf is dead.
**	6-Apr-92 (seiwald)
**	    Support for doublebyte characters: if the word "doublebyte"
**	    appears alone on a line after the "class" line in a character
**	    set mapping table, the mapping may be doublebyte.
**	01-Feb-93 (mrelac)
**	    For MPE/iX (hp9_mpe) only, don't call LOfaddpath().
**	22-Dec-95 (gordy)
**	    Only load mapping tables which match our local character set.
**	22-Jan-10 (gordy)
**	    Added identity mappings and optimized mapping storage.
**	    Charset translation moved to gco.
*/

static struct {
	char	*word;
	i4	len;
} keys[] = {
	"local", sizeof( "local" ),
	"remote", sizeof( "remote" ),
	"class", sizeof( "class" ),
	"doublebyte", sizeof( "doublebyte" ),
	"begin", sizeof( "begin" ),
	"end", sizeof( "end" ),
	0, 0 
} ;

# define WORD_LOCAL	0
# define WORD_REMOTE	1
# define WORD_CLASS	2
# define WORD_DOUBLE	3
# define WORD_BEGIN	4
# define WORD_END	5
# define WORD_MAP	6
# define WORD_DUP	7

# define GOT_LCL	(1<<WORD_LOCAL)
# define GOT_RMT	(1<<WORD_REMOTE)
# define GOT_CLASS	(1<<WORD_CLASS)
# define GOT_DOUBLE	(1<<WORD_DOUBLE)
# define GOT_BEGIN	(1<<WORD_BEGIN)
# define GOT_END	(1<<WORD_END)
# define GOT_MAP	(1<<WORD_MAP)
# define GOT_ALL	(GOT_LCL|GOT_RMT|GOT_CLASS|GOT_BEGIN|GOT_END|GOT_MAP)

static STATUS
gcc_read_ctbl( u_i2 lcl_id )
{
    i4			count = 0;	/* count of tables read */
    i4			lineno = 0;	/* line number for logging */
    u_i4		words = 0;	/* or'ed bits of keywords seen */
    bool		loading = FALSE;
    LOCATION		file_loc;
    FILE		*file_ptr;
    char		linebuf[ 256 ];
    GCC_CSET_XLT	ctbl;		/* temp copy of table being created */
    static u_i1		n2l[ GCO_L_SB_CSET ];
    static u_i1		l2n[ GCO_L_SB_CSET ];
    static u_i2		n2l16[ GCO_L_DB_CSET ];
    static u_i2		l2n16[ GCO_L_DB_CSET ];
    STATUS		status = OK;
    GCC_ERLIST		erlist;

    /* Dances with LO, NM, & SI */

    (VOID)NMloc( FILES, 0, (char *)NULL, &file_loc );
#ifndef hp9_mpe
    (VOID)LOfaddpath( &file_loc, "charsets", &file_loc );
#endif
    (VOID)LOfstfile( "gcccset.xlt", &file_loc );

    if( SIopen( &file_loc, "r", &file_ptr ) != OK )
	return OK;

    /* Scan the file with very little smarts */

    while( SIgetrec( linebuf, sizeof( linebuf ), file_ptr ) != ENDFILE )
    {
	GCC_CSET_NAME	*cname;
	char		*p_word;
	char		*p;
	i4		w;

	/* 
	** Count lines and skip comment lines. 
	** Strip leading whitespace and skip blank lines. 
	*/

	lineno++;

	if( p = STchr( linebuf, '#' ) )
	    *p = '\0';

	p = linebuf;

	while( CMwhite( p ) )
		p++;

	if( !*p )
	    continue;

	/* 
	** Lookup first keyword.
	** If not recognised, w == WORD_MAP. 
	*/

	for( w = 0; keys[ w ].word; w++ )
	    if( !STncasecmp( p, keys[ w ].word, keys[ w ].len - 1) )
		break;

	/* 
	** Skip keyword and whitespace.  
	** If w == WORD_MAP then keys[ w ].len == 0. 
	*/

	p += keys[ w ].len;

	while( CMwhite( p ) )
		p++;

	/* 
	** Set bit in flag indicating we've seen this keyword.
	** Don't allow duplicate keywords except for WORD_MAP. 
	*/

	if( !( words & ( 1 << w ) ) )
	    words |= ( 1 << w );
	else if( w != WORD_MAP )
	    w = WORD_DUP;
	
	/* Process line based on keyword. */

	switch( w )
	{
	case WORD_LOCAL:
	case WORD_REMOTE:
	    /* 
	    ** Get character set name and look it up in cname table. 
	    ** Flag error if it's unknown, else stash id. 
	    */

	    for( p_word = p; *p && !CMwhite( p ); p++ )
		;

	    if( p == p_word )
		goto bad;

	    *p++ = '\0';

	    if( !( cname = gcc_find_cnam( p_word ) ) )
		goto bad;

	    if( w == WORD_LOCAL )
		ctbl.lcl_id = cname->id;
	    else
		ctbl.net_id = cname->id;

	    break;

	case WORD_CLASS:
	    /* Get class name and check it.  Flag error if unknown. */

	    ctbl.class = 0;

	    for( p_word = p; *p && !CMwhite( p ); p++ )
		;

	    if( p != p_word )
	    {
		*p++ = '\0';

		if( !STcasecmp( p_word, "first" ) )
		    ctbl.class = FIRST_CLASS;
		else if( !STcasecmp( p_word, "second" ) )
		    ctbl.class = SECOND_CLASS;
		else if( !STcasecmp( p_word, "third" ) )
		    ctbl.class = THIRD_CLASS;
	    }

	    if( !ctbl.class )
		goto bad;

	    break;

	case WORD_DOUBLE:
	    /*
	    ** Nothing to do unless we load the table (see WORD_BEGIN).
	    */
	    break;

	case WORD_BEGIN:
	    /*
	    ** Check to see if we should load the current 
	    ** mapping table.  Only tables for our local
	    ** character set are used.
	    */
	    if ( (words & GOT_LCL)  &&  lcl_id == ctbl.lcl_id )
	    {
		loading = TRUE;

		/*
		** Initialize the mapping tables.
		*/
		if ( words & GOT_DOUBLE )
		{
		    ctbl.xlt.type = GCO_XLT_DOUBLE;
		    ctbl.xlt.n2l.xlt = gco_xlt_db;
		    ctbl.xlt.l2n.xlt = gco_xlt_db;
		    ctbl.xlt.n2l.map = (PTR)n2l16;
		    ctbl.xlt.l2n.map = (PTR)l2n16;
		    MEfill( sizeof( n2l16 ), 0, (PTR)n2l16 );
		    MEfill( sizeof( l2n16 ), 0, (PTR)l2n16 );
		}
	        else
		{
		    ctbl.xlt.type = GCO_XLT_SINGLE;
		    ctbl.xlt.n2l.xlt = gco_xlt_sb;
		    ctbl.xlt.l2n.xlt = gco_xlt_sb;
		    ctbl.xlt.n2l.map = (PTR)n2l;
		    ctbl.xlt.l2n.map = (PTR)l2n;
		    MEfill( sizeof( n2l ), 0, (PTR)n2l );
		}
	    }
	    break;
	    
	case WORD_MAP:
	    /*
	    ** A mapping line.  Make sure we got a "begin" line. 
	    */
	    if ( ! (words & GOT_BEGIN) )  goto bad;

	    /*
	    ** Check to see if we are loading this table.
	    */
	    if ( ! loading )  break;
	     
	    /* 
	    ** Parse the mappings. 
	    */
	    for(;;)
	    {
		char *b, buf[ 32 ];
		u_i4 lcl_char, rmt_char;

		/* Skip white and quit if we hit EOS. */

		while( CMwhite( p ) )
			p++;

		if( !*p )
		    break;

		/* 
		** Pick up <lclval> = <rmtval> from before the ',' and 
		** then make p point after the ','. 
		*/

		while( CMwhite( p ) )
		    p++;

		for( b = buf; CMhex( p ); )
		    *b++ = *p++;

		while( CMwhite( p ) )
		    p++;

		if( *p++ != '=' )
		    goto bad;

		*b++ = '\0';

		if( CVahxl( buf, &lcl_char ) != OK )
		    goto bad;

		while( CMwhite( p ) )
		    p++;

		for( b = buf; CMhex( p ); )
		    *b++ = *p++;

		while( CMwhite( p ) )
		    p++;

		if( *p == ',' )
		    p++;
		else if( *p )
		    goto bad;

		*b++ = '\0';

		if( CVahxl( buf, &rmt_char ) != OK )
		    goto bad;
		
		/* Check for obvious errors. */

		if( words & GOT_DOUBLE )
		{
		    /* Doublebyte mappings. */

		    if( lcl_char >= GCO_L_DB_CSET 
		     || rmt_char >= GCO_L_DB_CSET ) 
			goto bad;

		    /* Build both mappings */

		    ((u_i2 *)ctbl.xlt.n2l.map)[ rmt_char ] = lcl_char;
		    ((u_i2 *)ctbl.xlt.l2n.map)[ lcl_char ] = rmt_char;
		}
		else
		{
		    /* Singlebyte mappings. */

		    if( lcl_char >= GCO_L_SB_CSET 
		     || rmt_char >= GCO_L_SB_CSET ) 
			goto bad;

		    /* 
		    ** Build one-way mapping. 
		    ** Other half is made by gcc_fill_ctbl. 
		    */
		    ((u_i1 *)ctbl.xlt.n2l.map)[ rmt_char ] = lcl_char;
		}
	    }
	    break;

	case WORD_END:
	    /* 
	    ** End of mapping.  Check to make sure all is well. 
	    */
	    if ( (words & GOT_ALL) != GOT_ALL )
		goto bad;

	    if ( loading )
	    {
		/*
		** Save the mapping table.
		*/
		if ( ! (words & GOT_DOUBLE) )
		{
		    /* 
		    ** Build singlebyte l2n table. 
		    */
		    gcc_fill_ctbl( (u_i1 *)ctbl.xlt.n2l.map, 
				   (u_i1 *)ctbl.xlt.l2n.map );
		}

		if ( (status = gcc_new_ctbl( &ctbl )) != OK )
		    goto done;

		loading = FALSE;
	    }

	    /* 
	    ** Reset words for next table. 
	    */
	    count++;
	    words = 0;
	    break;

	bad:
	case WORD_DUP:

# ifdef GCXDEBUG
	    if( IIGCc_global->gcc_trace_level >= 1)
		TRdisplay( "gcc_read_ctbl: bad read (%x)\n", words );
# endif /* GCXDEBUG */

	    status = E_GC2453_BAD_CXLT;
	    erlist.gcc_parm_cnt = 1;
	    erlist.gcc_erparm[0].size = sizeof( lineno );
	    erlist.gcc_erparm[0].value = (PTR)&lineno;
	    gcc_er_log(&status, (CL_ERR_DESC *)NULL, (GCC_MDE *)NULL, &erlist);
	    status = E_GC2451_CSET_FAIL;
	    break;
	}
    }

done:
    /* Clean up & return. */

    SIclose( file_ptr );

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1)
	TRdisplay( "gcc_read_ctbl: read %d tables\n", count );
# endif /* GCXDEBUG */

    return status;
}


/*
** Name: gcc_fill_ctbl - fill transliteration table holes
**
** Description:
**	Translates a network->local mapping into network->local and 
**	local->network mappings, filling holes to maintain an (arbitrary) 
**	one-to-one mapping.
**
** History:
**	6-Apr-92 (seiwald)
**	    Moved here from gcc_new_ctbl.
**	22-Jan-10 (gordy)
**	    Charset translation moved to gco.
*/

static VOID
gcc_fill_ctbl( u_i1 *n2l, u_i1 *l2n )
{
    i4			cn;	/* network char */
    i4			cl;	/* local char */

    /*
    ** Copy network->local mapping 
    ** zero local->network mapping 
    */

    MEfill( GCO_L_SB_CSET, 0, l2n );

    /*
    ** Build local->network from network->local 
    ** Forcibly unmap duplicates in n2l table 
    ** Zero always maps to zero 
    */

    for( cn = 1; cn < GCO_L_SB_CSET; cn++ )
    {
	cl = n2l[ cn ];

	/*
	** Unmap network char if local already mapped 
	** Otherwise, map local to network (skipping empties) 
	*/

	if( l2n[ cl ] != 0 )
	{
	    n2l[ cn ] = 0;
	}
	else if( cl )
	{
	    l2n[ cl ] = cn;
	}
    }

    /* Count holes and fill them one-to-one */

    for( cl = cn = 1; cl < GCO_L_SB_CSET; )
    {
	/*
	** Scan for next unmapped local 
	** Then scan for next unmapped network 
	*/

	if( l2n[ cl++ ] != 0 )
	    continue;

	while( n2l[ cn++ ] != 0 )
	    ;

	/* Map both ways */

	l2n[ cl - 1 ] = cn - 1;
	n2l[ cn - 1 ] = cl - 1;
    }
}


/*
** Name: gcc_new_ctbl - create character set transliteration table
**
** Description:
**	Allocates a GCC_CSET_XLT, copying its contents from the
**	usrctbl handed in, and puts it on the list of transliteration
**	tables.
**
** Returns:
**	OK
**	E_GC2004_ALLOCN_FAIL	alloc failed
**	
** History:
**	8-Mar-91 (seiwald)
**	    Written.
**	22-Jan-10 (gordy)
**	    Added identity mappings and optimized mapping storage.
**	    Charset translation moved to gco.
*/

static STATUS
gcc_new_ctbl( GCC_CSET_XLT *usrctbl )
{
    GCC_CSET_XLT	*ctbl;		/* the table we define */
    QUEUE		*ctq;		/* for scanning known tables */
    QUEUE               *prev_ptr;      /* for scanning known tables */
    i4			arrsize = 0;

    /* Alloc a char tbl */
    switch( usrctbl->xlt.type )
    {
    case GCO_XLT_SINGLE :  
	arrsize = GCO_L_SB_CSET * sizeof( u_i1 );
	ctbl = (GCC_CSET_XLT *)gcc_alloc( sizeof( *ctbl ) + (arrsize * 2) );
	if ( ! ctbl )  return( E_GC2004_ALLOCN_FAIL );

	ctbl->xlt.n2l.map = (PTR)((u_i1 *)ctbl + sizeof( *ctbl ));
	ctbl->xlt.l2n.map = (PTR)((u_i1 *)ctbl->xlt.n2l.map + arrsize);
	break;

    case GCO_XLT_DOUBLE :  
	arrsize = GCO_L_DB_CSET * sizeof( u_i2 );
	ctbl = (GCC_CSET_XLT *)gcc_alloc( sizeof( *ctbl ) );
	if ( ! ctbl )  return( E_GC2004_ALLOCN_FAIL );

	ctbl->xlt.n2l.map = gcc_alloc( arrsize );
	ctbl->xlt.l2n.map = gcc_alloc( arrsize );

	if ( ! (ctbl->xlt.n2l.map  &&  ctbl->xlt.l2n.map) )
	{
	    if ( ctbl->xlt.n2l.map )  MEfree( ctbl->xlt.n2l.map );
	    if ( ctbl->xlt.l2n.map )  MEfree( ctbl->xlt.l2n.map );
	    MEfree( (PTR)ctbl );
	    return( E_GC2004_ALLOCN_FAIL );
	}
	break;

    default :
	ctbl = (GCC_CSET_XLT *)gcc_alloc( sizeof( *ctbl ) );
	if ( ! ctbl )  return( E_GC2004_ALLOCN_FAIL );
	break;
    }

    /*
    ** Copy user ctbl 
    */
    QUinit( &ctbl->q );
    ctbl->lcl_id = usrctbl->lcl_id;
    ctbl->net_id = usrctbl->net_id;
    ctbl->class = usrctbl->class;
    ctbl->xlt.type = usrctbl->xlt.type;
    ctbl->xlt.n2l.xlt = usrctbl->xlt.n2l.xlt;
    ctbl->xlt.l2n.xlt = usrctbl->xlt.l2n.xlt;

    switch( ctbl->xlt.type )
    {
    case GCO_XLT_SINGLE :
    case GCO_XLT_DOUBLE :
	MEcopy( usrctbl->xlt.n2l.map, arrsize, ctbl->xlt.n2l.map );
	MEcopy( usrctbl->xlt.l2n.map, arrsize, ctbl->xlt.l2n.map );
        break;

    default :
	ctbl->xlt.n2l.map = usrctbl->xlt.n2l.map;
	ctbl->xlt.l2n.map = usrctbl->xlt.l2n.map;
	break;
    }

    /*
    ** Now insert onto IIGCc_global->gcc_cset_xlt_q 
    ** Insertion sort by class; low-to-high 
    */
    prev_ptr = &IIGCc_global->gcc_cset_xlt_q;

    for( ctq = IIGCc_global->gcc_cset_xlt_q.q_next; 
	 ctq != &IIGCc_global->gcc_cset_xlt_q;
	 ctq = ctq->q_next )
    {
	if( ctbl->class < ((GCC_CSET_XLT *)ctq)->class )
	    break;

	prev_ptr = ctq;
    }

    QUinsert( (QUEUE *)ctbl, prev_ptr );

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 2)
	TRdisplay( "gcc_new_ctbl: lcl %x net %x class %d\n", 
		ctbl->lcl_id, ctbl->net_id, ctbl->class );

    if( IIGCc_global->gcc_trace_level >= 4 )
    {
	i4  i;

	switch( ctbl->xlt.type )
	{
	case GCO_XLT_SINGLE :
	    for( i = 0; i < GCO_L_SB_CSET; i++ )
		if ( ((u_i1 *)ctbl->xlt.n2l.map)[ i ]  ||  
		     ((u_i1 *)ctbl->xlt.l2n.map)[ i ] )
		    TRdisplay( "mapping %x: %x %x\n", i, 
			       ((u_i1 *)ctbl->xlt.n2l.map)[i], 
			       ((u_i1 *)ctbl->xlt.l2n.map)[i] );
	    break;

	case GCO_XLT_DOUBLE :
	    for( i = 0; i < GCO_L_DB_CSET; i++ )
		if ( ((u_i2 *)ctbl->xlt.n2l.map)[ i ]  ||  
		     ((u_i2 *)ctbl->xlt.l2n.map)[ i ] )
		    TRdisplay( "mapping %x: %x %x\n", i, 
			       ((u_i2 *)ctbl->xlt.n2l.map)[i], 
			       ((u_i2 *)ctbl->xlt.l2n.map)[i] );
	    break;
	}
    }
# endif /* GCXDEBUG */

    return( OK );
}

/*
** Name: gcc_find_ctbl - find a transliteration table, given char set ids
**
** Description:
**	Scans the list of character set transliteration tables for a table 
**	with matching local and network character set ids.
**
** Returns:
**	0	no such character set
**	pointer to GCC_CSET_XLT
**
** History:
**	8-Mar-91 (seiwald)
**	    Written.
*/

static GCC_CSET_XLT *
gcc_find_ctbl( u_i2 lcl_id, u_i2 net_id )
{
    QUEUE		*ctq;
    GCC_CSET_XLT	*ctbl;

    /* Scan the queue.  Return the first match. */

    for( ctq = IIGCc_global->gcc_cset_xlt_q.q_next; 
	 ctq != &IIGCc_global->gcc_cset_xlt_q;
	 ctq = ctq->q_next )
    {
	ctbl = (GCC_CSET_XLT *)ctq;

	if( ctbl->lcl_id == lcl_id && ctbl->net_id == net_id )
		return ctbl;
    }

    /* Bogus ids. */

    return (GCC_CSET_XLT *)NULL;
}

/*
** Name: gcc_init_cset - initialize character set tables
**
** Description:
**	Initializes character set tables and associated data.
**
**	Computes the default local character set, given by CV_NCSJ_TYPE
**	and CV_NCSN_TYPE.  The built-in IIGCc_inxl_tbl maps between the
**	default local character set and the default network standard 
**	character set, NSCS.
**
**	Reads in II_CONFIG/iigcc.chars to build the queue of character
**	table mappings.  Orders the queue from most-preferred mapping
**	to least-preferred mapping.
**
**	Determines local character set by matching II_GCCxx_CHARS
**	against name entries in iigcc.chars; if none is found, uses
**	the default local character set.
**
**	Builds a local<->local mapping and places it at the head
**	of the mapping queue, making it the most preferred.
**
**	Builds a default local <-> NSCS mapping from the built-in
**	IIGCc_inxl_tbl, and places it at the end of the queue, making
**	it the least preferred.
**
**	Finds the local <-> NSCS mapping and places a pointer in
**	IIGCc_global->gcc_cset_xlt.  NSCS is used during connection
**	establishment (before the character set has been negotiated)
**	and at all times with het peers whose PL_PROTOCOL_VRSN < HET.
**	The local char set id is thus IIGCc_status->gcc_cset_xlt->lcl_id.
**
** Returns:
**	OK
**	E_GC2004_ALLOCN_FAIL	alloc failed
**	E_GC2451_CSET_FAIL	error in file formats
**	E_GC2454_BAD_LCL_CHARSET  bogus II_CHARSET
**	E_GC2455_NO_LCL_MAPPING	  no mapping for II_CHARSET
**
** History:
**	8-Mar-91 (seiwald)
**	    Written.
**	23-Jul-91 (seiwald)
**	    Renamed II_GCCxx_CHARSET to II_CHARSETxx.
**	18-Sep-91 (seiwald)
**	    Parenthesize << operator because the + binds more tightly!
**	10-may-95 (emmag)
**	    Semi-colon missing from desktop code which sets IBMPC850.
**	22-Dec-95 (gordy)
**	    Only build a default to NSCS mapping table if no local to
**	    NSCS table exists and our local character set was not
**	    changed from the default.  
**	20-Aug-02 (gordy)
**	    If no NSCS mapping exists, use local <-> local to permit
**	    connections to installations with same character set.
**	22-Jan-10 (gordy)
**	    Added identity mappings and optimized mapping storage.
**	    Use CM for common charset initialization.
*/	

STATUS
gcc_init_cset( VOID )
{
    u_i2		def_id;
    u_i2		lcl_id;
    char		cname[ CM_MAXATTRNAME + 1 ];
    GCC_CSET_XLT	ctbl;
    STATUS		status;

    /* 
    ** Compute default local character set id.
    ** NCSJ and NCSN were the pair of "network character set" ids
    ** from pre-VRSN_CSET code.  They are now rolled into a single 16 
    ** bit quantity to match up with the ids in gcc_cset_nam_q.
    */
    lcl_id = def_id = ( CV_NCSJ_TYPE << 8 ) + CV_NCSN_TYPE;

    /* Read character name map */
    if ( (status = gcc_read_cnam()) != OK )  return status;

    /*
    ** Get local character set from enviroment.
    */
    CMget_charset_name( cname );

    /*
    ** Bug 56258
    ** If II_CHARSET not set, we should assume ISO88591.
    ** or whatever was the 'compiled-in' charset in cm
    ** else, problems with binary chars in range 0x80->...
    ** transferred over NET.
    */
    if ( cname[0] == EOS )
    {
        switch (CMgetDefCS())
        {
            case CM_DECMULTI :          /* DECMULTI   */
                STcopy( "DECMULTI", cname );
                break;
 
#if ( !defined(UNIX) && !defined(VMS) )
            case CM_IBM :               /* CSIBM */
                STcopy( "IBMPC850", cname );
                break;
#endif

            case CM_88591 :             /* CSASCII   */
            default:
                STcopy( "ISO88591", cname );
                break;
        }
    }

    if ( cname[0] != EOS )
    {
	GCC_CSET_NAME	*cset;

	if ( ! (cset = gcc_find_cnam( cname )) )
	    return( E_GC2454_BAD_LCL_CHARSET );

	lcl_id = cset->id;
    }

# ifdef GCXDEBUG
    if ( IIGCc_global->gcc_trace_level >= 1 )
	TRdisplay( "gcc_init_cset: local id %x\n", lcl_id );
# endif /* GCXDEBUG */

    /* Read character set transliteration tables */
    if( (status = gcc_read_ctbl( lcl_id )) != OK )  return( status );

    /* 
    ** Make a ZERO_CLASS local<->local mapping and enqueue it.
    */
    ctbl.lcl_id = lcl_id;
    ctbl.net_id = lcl_id;
    ctbl.class = ZERO_CLASS;
    ctbl.xlt.type = GCO_XLT_IDENTITY;
    ctbl.xlt.n2l.xlt = gco_xlt_ident;
    ctbl.xlt.l2n.xlt = gco_xlt_ident;
    ctbl.xlt.n2l.map = NULL;
    ctbl.xlt.l2n.map = NULL;

    if ( (status = gcc_new_ctbl( &ctbl )) != OK )  return( status );

    /* 
    ** Find local <-> NSCS and use it for connection encoding.
    */
    IIGCc_global->gcc_cset_xlt = (PTR)gcc_find_ctbl( lcl_id, GCC_ID_NSCS );

    /*
    ** If local <-> NSCS is not defined and local matches default,
    ** build character map for default <-> NSCS.
    */
    if ( ! IIGCc_global->gcc_cset_xlt  &&  lcl_id == def_id )
    {
	u_i1	n2l[ GCO_L_SB_CSET ];
	u_i1	l2n[ GCO_L_SB_CSET ];

	/* 
	** Make a THIRD_CLASS default <-> NSCS mapping and enqueue it.
	*/
	ctbl.lcl_id = def_id;
	ctbl.net_id = GCC_ID_NSCS;
	ctbl.class = THIRD_CLASS;
	ctbl.xlt.type = GCO_XLT_SINGLE;
	ctbl.xlt.n2l.xlt = gco_xlt_sb;
	ctbl.xlt.l2n.xlt = gco_xlt_sb;
	ctbl.xlt.n2l.map = (PTR)n2l;
	ctbl.xlt.l2n.map = (PTR)l2n;

	MEcopy( (PTR)IIGCc_inxl_tbl, GCO_L_SB_CSET, ctbl.xlt.n2l.map );
	gcc_fill_ctbl( (u_i1 *)ctbl.xlt.n2l.map, (u_i1 *)ctbl.xlt.l2n.map );

	if ( (status = gcc_new_ctbl( &ctbl )) != OK )  return( status );

	IIGCc_global->gcc_cset_xlt = (PTR)gcc_find_ctbl( lcl_id, GCC_ID_NSCS );
    }

    /*
    ** If no mapping to NSCS exists, use local <-> local.  Connections will
    ** only be possible with installations using the same character set.
    */
    if ( ! IIGCc_global->gcc_cset_xlt )
	IIGCc_global->gcc_cset_xlt = (PTR)gcc_find_ctbl( lcl_id, lcl_id );

    return( IIGCc_global->gcc_cset_xlt ? OK : E_GC2455_NO_LCL_MAPPING );
}


/*
** Name: gcc_nput_av - translate GCC_ARCH_VEC into network transfer syntax
**
** History:
**	8-Mar-91 (seiwald)
**	    Written.
**	22-Jan-10 (gordy)
**	    Don't trace duplicate charset entries.
*/

static VOID
gcc_nput_av( char *ptr, GCC_ARCH_VEC *av )
{
    i4  i;

    /* Copy arch vector into transfer syntax area of context struct */

    ptr += GCaddn1( av->int_type, ptr );
    ptr += GCaddn1( av->flt_type, ptr );
    ptr += GCaddn2( av->char_type, ptr );

    for( i = 0; i < GCC_L_CHAR_VEC; i++ )
	ptr += GCaddn2( av->char_vec[ i ], ptr );

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1)
    {
	TRdisplay( "gcc_nput_av: arch vec = %d/%d/%x [%x", 
		(i4)av->int_type, (i4)av->flt_type, 
		av->char_type, av->char_vec[0] );

	for( i = 1; i < GCC_L_CHAR_VEC; i++ )
	    if ( av->char_vec[i] != av->char_vec[i-1]  &&
	         av->char_vec[i] != (u_i2)CV_HET_TYPE )
		TRdisplay( "/%x", av->char_vec[i] );

	TRdisplay( "]\n" );
    }
# endif /* GCXDEBUG */
}


/*
** Name: gcc_nget_av - translate GCC_ARCH_VEC from network transfer syntax
**
** History:
**	8-Mar-91 (seiwald)
**	    Written.
**	22-Jan-10 (gordy)
**	    Don't trace duplicate charset entries.
*/

static VOID
gcc_nget_av( char *ptr, GCC_ARCH_VEC *av )
{
    i4  i;

    /* Copy transfer syntax area of context struct into arch vector */

    ptr += GCgeti1( ptr, &av->int_type );
    ptr += GCgeti1( ptr, &av->flt_type );
    ptr += GCgeti2( ptr, &av->char_type );

    for( i = 0; i < GCC_L_CHAR_VEC; i++ )
	ptr += GCgeti2( ptr, &av->char_vec[ i ] );

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1)
    {
	TRdisplay( "gcc_nget_av: arch vec = %d/%d/%x [%x", 
		(i4)av->int_type, (i4)av->flt_type, 
		av->char_type, av->char_vec[0] );

	for( i = 1; i < GCC_L_CHAR_VEC; i++ )
	    if ( av->char_vec[i] != av->char_vec[i-1] &&
	         av->char_vec[i] != (u_i2)CV_HET_TYPE )
		TRdisplay( "/%x", av->char_vec[i] );

	TRdisplay( "]\n" );
    }
# endif /* GCXDEBUG */
}


/* 
** Name: gcc_init_av - initialize arch vec for this arch/char set
**
** Description:
**	Fills the architecture vector with constant values describing
**	the local datatype formats and character set of the platform. 
**
** 	The arch vec maintains compatibility with < PL_PROTOCOL_VRSN_CSET 
** 	peers, which treat the first 4 bytes of the arch vector as:
**		[ INT_TYPE, FLT_TYPE, NCSJ_TYPE, NCSN_TYPE ]
**
** History:
**	8-Mar-91 (seiwald)
**	    Written.
**	22-Jan-10 (gordy)
**	    Force-het made static to reduce overhead.  Fill unused portion 
**	    of charset array with het mask (prior zero fill mapped to NSCS).
*/

static VOID
gcc_init_av( u_i2 lcl_id, GCC_ARCH_VEC *av )
{
    char *ptr;
    i4   entry;

    if ( force_het == GCC_HET_UNK )
    {
	NMgtAt( "II_FORCE_HET", &ptr );

	if ( ! ptr  ||  ! *ptr )  
	    force_het = GCC_HET_OFF;
	else
    	    force_het = GCC_HET_ON;
    }

    MEfill( sizeof( *av ), 0, (PTR)av );

    /* fill in base of arch vec */

    av->int_type = force_het ? CV_HET_TYPE : CV_INT_TYPE;
    av->flt_type = force_het ? CV_HET_TYPE : CV_FLT_TYPE;
    av->char_type = force_het ? CV_HET_TYPE : lcl_id;

    for( entry = 0; entry < GCC_L_CHAR_VEC; entry++ )
    	av->char_vec[ entry ] = (u_i2)CV_HET_TYPE;

}

/*
** Name: gcc_cmp_av - compare arch vectors
**
** Description:
**	Computes the het mask based on the differences between the local
**	and remote architecture vectors.
**
**	If any of the types compare differently, then mask will be non-
**	zero and heterogenous conversion will take place.
**
** Returns:
**	bit mask representing which types differ
**
** History:
**	8-Mar-91 (seiwald)
**	    Written.
*/

static int
gcc_cmp_av( GCC_ARCH_VEC *lcl_av, GCC_ARCH_VEC *rmt_av )
{
    int mask = 0;

    if( lcl_av->int_type == (u_char)CV_HET_TYPE 
     || lcl_av->int_type != rmt_av->int_type )
	    mask |= GCC_INTEGER_HET;

    if( lcl_av->flt_type == (u_char)CV_HET_TYPE 
     || lcl_av->flt_type != rmt_av->flt_type )
	    mask |= GCC_FLOAT_HET;

    if( lcl_av->char_type == (u_i2)CV_HET_TYPE 
     || lcl_av->char_type != rmt_av->char_type )
	    mask |= GCC_CHARACTER_HET;

    return mask;
}

/*
** Name: gcc_rqst_av - initialize arch vec for connection request
**
** Description:
**	This routine formats the architecture vector component of the 
**	P_CONNECT message sent to the responding peer when a connection
**	request is made.
**
**	It first calls gcc_init_av() to initialize the local datatype
**	identifiers and then produces the list of character sets
**	we are willing to speak.
**
**	The architecture vector is formatted such that the responding 
**	peer can either be VRSN_CSET or prior.
**
** History:
**	8-Mar-91 (seiwald)
**	    Written.
*/

VOID
gcc_rqst_av( PL_CONTEXT *lcl_cntxt )
{
    QUEUE		*ctq;
    GCC_ARCH_VEC	av;
    GCC_CSET_XLT	*defctbl;	/* local char set table */
    GCC_CSET_XLT	*ctbl;
    i4			entry = 0;

    /* Get local character set trans table, setup by gcc_init_cset */

    defctbl = (GCC_CSET_XLT *)IIGCc_global->gcc_cset_xlt;

    gcc_init_av( defctbl->lcl_id, &av );

    /*
    ** Scan table for character sets to which we can translate. 
    ** This table is sorted in order of preference.  First will 
    ** be our native character set.
    */
    for( 
	 ctq = IIGCc_global->gcc_cset_xlt_q.q_next; 
	 ctq != &IIGCc_global->gcc_cset_xlt_q;
	 ctq = ctq->q_next 
       )
    {
	ctbl = (GCC_CSET_XLT *)ctq;

	if ( ctbl->lcl_id == defctbl->lcl_id )
	{
	    av.char_vec[ entry++ ] = ctbl->net_id;
	    if ( entry >= GCC_L_CHAR_VEC )  break;
	}
    }

    /* Copy arch vector into transfer syntax area of context struct */
    gcc_nput_av( lcl_cntxt->trsnfr_syntax, &av );
}

/*
** Name: gcc_ind_av - examine architecture vector on connection indication
**
** Description:
**	This routine examines the architecture vector component of the
**	P_CONNECT message received from the initiating peer when a 
**	connection request is made.
**	
**	It compares the remote peer's architecture vector with the local
**	one to determine if heterogenous network conversion is necessary.
**	It then negotiates the network character set, by examining the
**	list of character sets offered by the peer.
**
**	If the initiating peer is pre-VRSN_CSET, the negotiated network
**	character set is always NSCS.
**	
** History:
**	8-Mar-91 (seiwald)
**	    Written.
**	20-Aug-02 (gordy)
**	    Return error status if no compatible network character set found.
**	22-Jan-10 (gordy)
**	    Skip unused (filler) entries in the charset array.
*/

STATUS
gcc_ind_av( GCC_CCB *ccb, PL_CONTEXT *rmt_cntxt )
{
    GCC_ARCH_VEC	lcl_av;		/* local arch char */
    GCC_ARCH_VEC	rmt_av;		/* peer's arch char */
    GCC_CSET_XLT	*defctbl;	/* local <-> NSCS table */
    GCC_CSET_XLT	*ctbl = NULL;	/* char set table we'll use */
    STATUS		status = OK;
    i4			i;

    /* Get local character set trans table, setup by gcc_init_cset. */

    defctbl = (GCC_CSET_XLT *)IIGCc_global->gcc_cset_xlt;

    /* Get local and remote arch vecs. */
    gcc_init_av( defctbl->lcl_id, &lcl_av );
    gcc_nget_av( rmt_cntxt->trsnfr_syntax, &rmt_av );

    /* Compute het mask by comparing arch vecs. */
    ccb->ccb_pce.het_mask = gcc_cmp_av( &lcl_av, &rmt_av );

    /* Pick network char set, according to protocol */
    if( ccb->ccb_pce.p_version < PL_PROTOCOL_VRSN_CSET )
    {
# ifdef GCXDEBUG
	if ( IIGCc_global->gcc_trace_level >= 4 )
	    TRdisplay( "gcc_ind_av: proto < CSET\n" );
# endif /* GCXDEBUG */

	/* Pre CSET comm servers use default NSCS always. */
	if ( defctbl->net_id == GCC_ID_NSCS )  ctbl = defctbl;
    }
    else
    {
	/* 
	** CSET comm servers pick the first set in the offering. 
	** See if they offer our local char set.
	*/
	for( i = 0; i < GCC_L_CHAR_VEC; i++ )
	{
	    /*
	    ** Check for filler marker (end of entries).
	    */
	    if ( rmt_av.char_vec[i] == (u_i2)CV_HET_TYPE )  break;

# ifdef GCXDEBUG
	    if ( IIGCc_global->gcc_trace_level >= 4 )
		TRdisplay( "gcc_ind_av: check lcl %x\n", rmt_av.char_vec[i] );
# endif /* GCXDEBUG */

	    if ( rmt_av.char_vec[ i ] == defctbl->lcl_id  &&
		 (ctbl = gcc_find_ctbl( defctbl->lcl_id, rmt_av.char_vec[i] )) )
		break;
	}

	/* See if they offer a set we translate to */
	for( i = 0; i < GCC_L_CHAR_VEC; i++ )
	{
	    int j;

	    /*
	    ** Check for filler marker (end of entries).
	    */
	    if ( rmt_av.char_vec[i] == (u_i2)CV_HET_TYPE )  break;

	    /*
	    ** Don't check duplicates.
	    */
	    for( j = 0; j < i; j++ )
		if ( rmt_av.char_vec[ j ] == rmt_av.char_vec[ i ] )  break;
	    if ( j < i )  continue;

# ifdef GCXDEBUG
	    if ( IIGCc_global->gcc_trace_level >= 4 )
		TRdisplay( "gcc_ind_av: check net %x\n", rmt_av.char_vec[i] );
# endif /* GCXDEBUG */

	    if ( (ctbl = gcc_find_ctbl( defctbl->lcl_id, rmt_av.char_vec[i] )) )
		break;
	}
    }

    if ( ctbl )
    {
	GCC_ERLIST	erlist;
	STATUS		status;

	/* Log the negotiated character set. */
	status = E_GC2461_CHARSET;
	erlist.gcc_parm_cnt = 2;
	erlist.gcc_erparm[0].size = sizeof( ctbl->lcl_id );
	erlist.gcc_erparm[0].value = (PTR)&ctbl->lcl_id;
	erlist.gcc_erparm[1].size = sizeof( ctbl->net_id );
	erlist.gcc_erparm[1].value = (PTR)&ctbl->net_id;
	gcc_er_log(&status, (CL_ERR_DESC *)NULL, (GCC_MDE *)NULL, &erlist);
    }
    else
    {
	/*
	** This is a rather odd circumstance: either our peer did not offer
	** NSCS or we don't speak it and no other compatible character set
	** is defined.  Drop back to local default and return error.
	*/
	ctbl = defctbl;
	status = E_GC2462_NO_CHARSET;
    }

    /* Stash the result for gcc_conv_vec to find. */
    ccb->ccb_pce.p_csetxlt = (PTR)ctbl;

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1)
	TRdisplay( "gcc_ind_av: het mask=%x; char lcl=%x net=%x, status=%x\n", 
		ccb->ccb_pce.het_mask,
		ctbl->lcl_id, ctbl->net_id, status );
# endif /* GCXDEBUG */

    return( status );
}

/* 
** Name: gcc_rsp_av - format arch vec for connection response
**
** Description:
**	This routine formats the architecture vector component of the
**	P_CONNECT message sent to the responding peer when responding to
**	a connection request.
**	
**	It first calls gcc_init_av() to initialize the local datatype
**	identifiers and then places negotiated character set id into
**	the first slot of the character set vector.
**	
** History:
**	8-Mar-91 (seiwald)
**	    Written.
*/

VOID
gcc_rsp_av( GCC_CCB *ccb, PL_CONTEXT *lcl_cntxt )
{
    GCC_ARCH_VEC	lav;
    GCC_CSET_XLT	*defctbl;	/* local <-> NSCS table */
    GCC_CSET_XLT	*ctbl;

    /* Get local character set trans table, setup by gcc_init_cset. */
    defctbl = (GCC_CSET_XLT *)IIGCc_global->gcc_cset_xlt;

    /* Initialize arch vec. */
    gcc_init_av( defctbl->lcl_id, &lav );

    /* Get actual character set trans table, as determined in gcc_ind_av. */
    ctbl = (GCC_CSET_XLT *)ccb->ccb_pce.p_csetxlt;

    /* Specify negotiated char set. */
    lav.char_vec[ 0 ] = ctbl->net_id;

    /* Fill in PCI. */
    gcc_nput_av( lcl_cntxt->trsnfr_syntax, &lav );
}

/*
** Name: gcc_cnf_av - examine architecture vector on connection confirmation
**
** Description:
**	This routine examines the architecture vector component of the
**	P_CONNECT message received from the responding peer when a 
**	connection request is confirmed.
**	
**	This is the same behavior as on connection indication, so we
**	just call gcc_ind_av().
**	
** History:
**	8-Mar-91 (seiwald)
**	    Written.
**	20-Aug-02 (gordy)
**	    Return status of het/net negotiation.
*/

STATUS
gcc_cnf_av( GCC_CCB *ccb, PL_CONTEXT *rmt_cntxt )
{
    return( gcc_ind_av( ccb, rmt_cntxt ) );
}
