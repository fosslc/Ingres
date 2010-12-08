/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <bt.h>
#include    <st.h>
#include    <sr.h>
#include    <hshcrc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <adf.h>
#include    <ade.h>
#include    <cs.h>
#include    <scf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <dmf.h>
#include    <dmhcb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <qefmain.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefscb.h>
#include    <qefdsh.h>
#include    <ex.h>
#include    <dmmcb.h>
#include    <dmucb.h>
#include    <tm.h>

#include    <dudbms.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <sxf.h>
#include    <qefprotos.h>

/**
**
**  Name: QENHASHUTL.C	- hash join utility functions.
**
**  Description:
**          qen_hash_partition - partition set of rows (build 
**		or probe)
**	    qen_hash_build  - partitioning done for one set
**		of rows (build or probe). Build a hash table
**          qen_hash_probe1 - probe source phase 1 processing
**	    qen_hash_probe2 - phase 2 processing
**
**
**  History:
**      4-mar-99 (inkdo01)
**          Written.
**	23-aug-01 (devjo01)
**	    Corrected offset calculations for non-32 bit platforms. b105598.
**	01-oct-2001 (devjo01)
**	    Rename QEN_HASH_DUP to QEF_HASH_DUP.
**	19-apr-02 (inkdo01)
**	    Added parm to qen_hash_flush call to resolve 107539.
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety.
**	6-May-2005 (schka24)
**	    Hash values are unsigned (they can be -infinity).
**	    Take now-incorrect u_i2 casts off MEcopy sizes.
**	17-may-05 (inkdo01)
**	    Close and destroy work files after they've been used.
**	7-Jun-2005 (schka24)
**	    A variety of fixes found by 100 Gb TPC-H, mostly in CO joining.
**	22-Jun-2005 (schka24)
**	    Bit filters are allocated per-partition, but they weren't being
**	    used that way.  Fix.
**	10-Nov-2005 (schka24)
**	    Clarify row-filter stuff when repartitioning, rename filter flag.
**	    Refine OJ indicator code so that we don't hand the pointers
**	    around, which turned out to just be a nuisance.  Fix OJ bug
**	    involving role reversal.
**	7-Dec-2005 (kschendel)
**	    Join key compare-list is now built into node, minor adjustment
**	    in several places here.
**	14-Dec-2005 (kschendel)
**	    Eliminate rcb parameter from routines, dsh suffices now
**	    that we have an ADF CB pointer in the DSH.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fix, row ptrs aren't passed as qef-hash-dup *
**	    yet, so more casting needed.
**	4-Jun-2009 (kschendel) b122118
**	    Remove link level row counts, not used.
**	5-Aug-2009 (kschendel) SIR 122512
**	    Merge datallegro changes:
**	    Add same-key flag to hash chains to reduce key compares
**	    needed when join key is heavily duplicated.
**	    Allow read buffer size to be different from the write page size,
**	    as long as both are a multiple of the basic HASH_PGSIZE.
**	    Re-type all row pointers to be qef-hash-dup *, which they are.
**	    Move the outer-join flag into the row header.
**	    Unify build and probe spill files to improve chances of good
**	    disk layout, and to halve the file-descriptor load.
**	    Implement optional compression of the non-key part of a row.
**	    Data-warehouse queries often carry long non-key columns thru
**	    the join, and simple RLL compression can improve space usage
**	    and reduce spillage.  Row compression implies variable length
**	    rows, with attendant changes throughout.
**	2-Dec-2010 (kschendel) SIR 124685
**	    Warning / prototype fixes.
**/



/*	local structures 	*/
typedef struct _FREE_CHUNK {
    struct _FREE_CHUNK	*free_next;  /* ptr to next free chunk */
    u_i4	    free_bytes;	/* size of free chunk (in bytes) */
}	FREE_CHUNK;

/*	static functions	*/

/* Only used for debug, ignore "defined but not used" warnings */
static VOID qen_hash_verify(QEN_HASH_BASE *hbase);

static DB_STATUS
qen_hash_insert(
QEE_DSH		*dsh,
QEN_HASH_BASE	*hbase,
QEF_HASH_DUP	*brptr);

static DB_STATUS
qen_hash_recurse(
QEE_DSH		*dsh,
QEN_HASH_BASE	*hbase);
 
static DB_STATUS
qen_hash_repartition(
QEE_DSH		*dsh,
QEN_HASH_BASE	*hbase,
QEN_HASH_PART	*hpptr);

static DB_STATUS
qen_hash_probe(
QEE_DSH		*dsh,
QEN_HASH_BASE	*hbase,
QEF_HASH_DUP	**buildptr,
QEF_HASH_DUP	*probeptr,
QEN_OJMASK	*ojmask,
bool		*gotone,
bool		try_compress);
 
static DB_STATUS qen_hash_readrow(
	QEE_DSH		*dsh,
	QEN_HASH_BASE	*hbase,
	QEN_HASH_PART	*hpptr,
	QEF_HASH_DUP	**probep);

static DB_STATUS
qen_hash_cartprod(
QEE_DSH		*dsh,
QEN_HASH_BASE	*hbase,
QEF_HASH_DUP	**buildptr,
QEF_HASH_DUP	**probeptr,
QEN_OJMASK	*ojmask,
bool		*gotone);

static DB_STATUS qen_hash_cpreadrow(
	QEE_DSH		*dsh,
	QEN_HASH_BASE	*hbase,
	QEN_HASH_CARTPROD *hcpptr,
	QEF_HASH_DUP	**rowpp,
	bool		readouter,
	bool		rewrite_inner);

static DB_STATUS qen_hash_reset_cpinner(
	QEE_DSH *dsh,
	QEN_HASH_BASE *hbase,
	QEN_OJMASK pojmask);
 
static VOID
qen_hash_bvinit(
QEN_HASH_BASE	*hbase,
QEN_HASH_PART	*hpptr);
 
static bool
qen_hash_checknull(
QEN_HASH_BASE	*hbase,
PTR		bufptr);

static DB_STATUS qen_hash_htalloc(
	QEE_DSH		*dsh,
	QEN_HASH_BASE	*hbase);

static DB_STATUS qen_hash_palloc(
	QEF_CB		*qefcb,
	QEN_HASH_BASE	*hbase,
	SIZE_TYPE	psize,
	void		*pptr,
	DB_ERROR	*dsh_error,
	char		*what);

static DB_STATUS
qen_hash_write(
QEE_DSH		*dsh,
QEN_HASH_BASE	*hbase,
QEN_HASH_FILE	*hfptr,
PTR		buffer,
u_i4		nbytes);

static DB_STATUS
qen_hash_read(
QEN_HASH_BASE	*hbase,
QEN_HASH_FILE	*hfptr,
PTR		buffer,
u_i4		nbytes);

static DB_STATUS
qen_hash_open(
QEE_DSH		*dsh,
QEN_HASH_BASE	*hbase,
QEN_HASH_PART	*hpptr);
 
static DB_STATUS
qen_hash_close(
QEN_HASH_BASE	*hbase,
QEN_HASH_FILE	**hfptrp,
i4		flag);
/* qen_hash_close flag settings */
#define CLOSE_RELEASE	1	
#define CLOSE_DESTROY	2

static DB_STATUS qen_hash_halfclose(
	QEN_HASH_BASE	*hbase,
	QEN_HASH_PART	*hpptr,
	i4 flag);
 
static DB_STATUS
qen_hash_flush(
QEE_DSH		*dsh,
QEN_HASH_BASE	*hbase,
bool		nonspilled_too);
 
static DB_STATUS
qen_hash_flushone(
QEE_DSH		*dsh,
QEN_HASH_BASE	*hbase,
QEN_HASH_PART	*hpptr);
 
static DB_STATUS
qen_hash_flushco(
QEE_DSH		*dsh,
QEN_HASH_BASE	*hbase);
 


/*
** Name: QEN_HASH_VALUE - generate a row's hash value
**
** Description:
**	This routine computes a 32-bit (unsigned) hash number based
**	on the prepared byte-string row key.  The caller provides
**	the row address (as a QEF_HASH_DUP *), and the length of
**	the key part which is always immediately after the QEF_HASH_DUP
**	header bit.
**
**	Originally, hash join used HSH_CRC, albeit indirectly.
**	Why do anything different?  Well, it turns out that when
**	hash-joining a partitioned table that is hash partitioned,
**	weird "beat" effects can occur if we join on the partitioning
**	columns.  For instance, a partition-compatible join by its
**	nature selects rows with [the high-order bits of] a certain
**	hash value.  If we then attempt to hash-join on the same
**	columns, using the same hash value, we may find that only
**	a few of our hash-join partitions receive rows -- because
**	we selected just those table partitions!  Worse yet, we
**	may discover that many of the rows to be joined land
**	in the same hash-join chain, making the chains very long
**	and slowing the join to a crawl.  Since the user has no
**	control on the number of hash-join partitions selected
**	by qee, the slowdown can appear and disappear seemingly
**	at random.
**
**	A simple solution is to just use a different hash algorithm
**	for hash joining, and that's what we do here.  The chosen
**	algorithm needs to be reasonably well behaved under a variety
**	of inputs.  I've chosen the "lookup3" hash by Bob Jenkins,
**	http://burtleburtle.net/bob/c/lookup3.c
**	which is (amazingly) as efficient as multiplicative hashes
**	like the FNV hash, gives outstanding results, and is free.
**
**	In the hash implementation, I have eliminated a few of the
**	conditionals because we ensure that the row key starts at an
**	aligned 32-bit boundary.  (See QEF_HASH_DUP definition.)
**	The hash operates on 3 i4's at a time, with some special
**	stuff for any partial block at the end.  There are two
**	versions of the final block handling, one for little-endian
**	machines and one for big-endian machines.
**
**	"By Bob Jenkins, 2006.  bob_jenkins@burtleburtle.net.  You may
**	use this code any way you wish, private, educational, or
**	commercial.  It's free."
**	Thanks, Bob!
**
** Inputs:
**	rowptr			QEF_HASH_DUP pointer to a row
**				with key data materialized via a
**				build- or probe-materialize CX
**	keysz			Length in bytes of the key byte string
**
** Outputs:
**	returns nothing.
**	Sets the hash_number in *rowptr.
**
** History:
**	22-Nov-2006 (kschendel) SIR 122512
**	    Written, using the Jenkins hash.
**	6-Jul-2009 (kschendel)
**	    BYTE_SWAP should have been replaced by BIT/LITTLE_ENDIAN_INT. Fix.
*/

/* PS "I" in the commentary below refers to Bob Jenkins.  (kschendel) */

/* Some macros first... */

#define rot(x,k) (((x)<<(k)) ^ ((x)>>(32-(k))))

/*
-------------------------------------------------------------------------------
mix -- mix 3 32-bit values reversibly.

This is reversible, so any information in (a,b,c) before mix() is
still in (a,b,c) after mix().

If four pairs of (a,b,c) inputs are run through mix(), or through
mix() in reverse, there are at least 32 bits of the output that
are sometimes the same for one pair and different for another pair.
This was tested for:
* pairs that differed by one bit, by two bits, in any combination
  of top bits of (a,b,c), or in any combination of bottom bits of
  (a,b,c).
* "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
  the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
  is commonly produced by subtraction) look like a single 1-bit
  difference.
* the base values were pseudorandom, all zero but one bit set, or 
  all zero plus a counter that starts at zero.

Some k values for my "a-=c; a^=rot(c,k); c+=b;" arrangement that
satisfy this are
    4  6  8 16 19  4
    9 15  3 18 27 15
   14  9  3  7 17  3
Well, "9 15 3 18 27 15" didn't quite get 32 bits diffing
for "differ" defined as + with a one-bit base and a two-bit delta.  I
used http://burtleburtle.net/bob/hash/avalanche.html to choose 
the operations, constants, and arrangements of the variables.

This does not achieve avalanche.  There are input bits of (a,b,c)
that fail to affect some output bits of (a,b,c), especially of a.  The
most thoroughly mixed value is c, but it doesn't really even achieve
avalanche in c.

This allows some parallelism.  Read-after-writes are good at doubling
the number of bits affected, so the goal of mixing pulls in the opposite
direction as the goal of parallelism.  I did what I could.  Rotates
seem to cost as much as shifts on every machine I could lay my hands
on, and rotates are much kinder to the top and bottom bits, so I used
rotates.
-------------------------------------------------------------------------------
*/

#define mix(a,b,c) \
{ \
  a -= c;  a ^= rot(c, 4);  c += b; \
  b -= a;  b ^= rot(a, 6);  a += c; \
  c -= b;  c ^= rot(b, 8);  b += a; \
  a -= c;  a ^= rot(c,16);  c += b; \
  b -= a;  b ^= rot(a,19);  a += c; \
  c -= b;  c ^= rot(b, 4);  b += a; \
}

/*
-------------------------------------------------------------------------------
final -- final mixing of 3 32-bit values (a,b,c) into c

Pairs of (a,b,c) values differing in only a few bits will usually
produce values of c that look totally different.  This was tested for
* pairs that differed by one bit, by two bits, in any combination
  of top bits of (a,b,c), or in any combination of bottom bits of
  (a,b,c).
* "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
  the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
  is commonly produced by subtraction) look like a single 1-bit
  difference.
* the base values were pseudorandom, all zero but one bit set, or 
  all zero plus a counter that starts at zero.

These constants passed:
 14 11 25 16 4 14 24
 12 14 25 16 4 14 24
and these came close:
  4  8 15 26 3 22 24
 10  8 15 26 3 22 24
 11  8 15 26 3 22 24
-------------------------------------------------------------------------------
*/
#define final(a,b,c) \
{ \
  c ^= b; c -= rot(b,14); \
  a ^= c; a -= rot(c,11); \
  b ^= a; b -= rot(a,25); \
  c ^= b; c -= rot(b,16); \
  a ^= c; a -= rot(c,4);  \
  b ^= a; b -= rot(a,14); \
  c ^= b; c -= rot(b,24); \
}

void
qen_hash_value( QEF_HASH_DUP *rowptr, i4 length)
{
    u_i4 a,b,c;				/* internal state */
    u_char *key = (u_char *) &rowptr->hash_key[0];
    const u_i4 *k = (const u_i4 *)key;	/* read 32-bit chunks */

    /* Set up the internal state */
    a = b = c = 0xdeadbeef + (u_i4)length;

    /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
    while (length > 12)
    {
	a += k[0];
	b += k[1];
	c += k[2];
	mix(a,b,c);
	length -= 12;
	k += 3;
    }

    /* handle the last (probably partial) block.  If there isn't any
    ** remainder block, we're done;  if there is, mix it.
    */

#if defined(LITTLE_ENDIAN_INT)
    /* little-endian version */

    /* 
     * "k[2]&0xffffff" actually reads beyond the end of the string, but
     * then masks off the part it's not allowed to read.  Because the
     * string is aligned, the masked-off tail is in the same word as the
     * rest of the string.  Every machine with memory protection I've seen
     * does it on word boundaries, so is OK with this.  But VALGRIND will
     * still catch it and complain.  The masking trick does make the hash
     * noticably faster for short strings (like English words).
     */
#ifndef VALGRIND

    switch(length)
    {
	case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
	case 11: c+=k[2]&0xffffff; b+=k[1]; a+=k[0]; break;
	case 10: c+=k[2]&0xffff; b+=k[1]; a+=k[0]; break;
	case 9 : c+=k[2]&0xff; b+=k[1]; a+=k[0]; break;
	case 8 : b+=k[1]; a+=k[0]; break;
	case 7 : b+=k[1]&0xffffff; a+=k[0]; break;
	case 6 : b+=k[1]&0xffff; a+=k[0]; break;
	case 5 : b+=k[1]&0xff; a+=k[0]; break;
	case 4 : a+=k[0]; break;
	case 3 : a+=k[0]&0xffffff; break;
	case 2 : a+=k[0]&0xffff; break;
	case 1 : a+=k[0]&0xff; break;
	case 0 : rowptr->hash_number = c; return;
    }

#else /* make valgrind happy */

    {
	u_char *k8 = (u_char *)k;
	switch(length)
	{
	    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
	    case 11: c+=((u_i4)k8[10])<<16;  /* fall through */
	    case 10: c+=((u_i4)k8[9])<<8;    /* fall through */
	    case 9 : c+=k8[8];               /* fall through */
	    case 8 : b+=k[1]; a+=k[0]; break;
	    case 7 : b+=((u_i4)k8[6])<<16;   /* fall through */
	    case 6 : b+=((u_i4)k8[5])<<8;    /* fall through */
	    case 5 : b+=k8[4];               /* fall through */
	    case 4 : a+=k[0]; break;
	    case 3 : a+=((u_i4)k8[2])<<16;   /* fall through */
	    case 2 : a+=((u_i4)k8[1])<<8;    /* fall through */
	    case 1 : a+=k8[0]; break;
	    case 0 : rowptr->hash_number = c; return;
	}
    }

#endif /* !valgrind */

#else /* endian-int */

    /* This is the big-endian version, different addressing/masking */
#ifndef VALGRIND

    switch(length)
    {
	case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
	case 11: c+=k[2]&0xffffff00; b+=k[1]; a+=k[0]; break;
	case 10: c+=k[2]&0xffff0000; b+=k[1]; a+=k[0]; break;
	case 9 : c+=k[2]&0xff000000; b+=k[1]; a+=k[0]; break;
	case 8 : b+=k[1]; a+=k[0]; break;
	case 7 : b+=k[1]&0xffffff00; a+=k[0]; break;
	case 6 : b+=k[1]&0xffff0000; a+=k[0]; break;
	case 5 : b+=k[1]&0xff000000; a+=k[0]; break;
	case 4 : a+=k[0]; break;
	case 3 : a+=k[0]&0xffffff00; break;
	case 2 : a+=k[0]&0xffff0000; break;
	case 1 : a+=k[0]&0xff000000; break;
	case 0 : rowptr->hash_number = c; return;
    }

#else  /* make valgrind happy */

    {
	u_char *k8 = (u_char *)k;
	switch(length)
	{
	    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
	    case 11: c+=((u_i4)k8[10])<<8;  /* fall through */
	    case 10: c+=((u_i4)k8[9])<<16;  /* fall through */
	    case 9 : c+=((u_i4)k8[8])<<24;  /* fall through */
	    case 8 : b+=k[1]; a+=k[0]; break;
	    case 7 : b+=((u_i4)k8[6])<<8;   /* fall through */
	    case 6 : b+=((u_i4)k8[5])<<16;  /* fall through */
	    case 5 : b+=((u_i4)k8[4])<<24;  /* fall through */
	    case 4 : a+=k[0]; break;
	    case 3 : a+=((u_i4)k8[2])<<8;   /* fall through */
	    case 2 : a+=((u_i4)k8[1])<<16;  /* fall through */
	    case 1 : a+=((u_i4)k8[0])<<24; break;
	    case 0 : rowptr->hash_number = c; return;
	}
    }

#endif /* !VALGRIND */
#endif /* byte-swap */

    final(a,b,c);
    rowptr->hash_number = c;

} /* qen_hash_value */

/*{
** Name: QEN_HASH_PARTITION - partition a set of rows
**
** Description:
**	Rows of either build or probe sources are delivered to this 
**	function to be partitioned. Function is either called directly
**	from qen_hjoin (with build source rows) or from inside qenhashutl.c
**	(with either probe or build rows, possibly while recursing on
**	a partition too large to fit into the hash table). If more rows 
**	arrive than will fit into the buffer for a partition, the blocks
**	for that partition are written to disk for later processing.
**
**	The caller can specify whether rows should be filtered based on
**	the bit-vector for the partition on the other side.  Filtering
**	is normally turned off for build rows (because there isn't any
**	"other" side yet) and on for probe rows.  Filtering might be
**	turned off if the caller is repartitioning, where it might not
**	be in a position to deal with outer-join nomatch processing.
**
**	The caller can also specify whether the row should be RLL-compressed,
**	assuming that it's not filtered out entirely.  Compression would
**	be attempted if the row is being delivered from one of the original
**	join inputs, and if query setup decides that it might be worthwhile.
**
** Inputs:
**	dsh		- Pointer to thread's DSH
**	hbase		- ptr to base hash structure
**	rowptr		- ptr to build/probe row to be stowed
**	filterflag	- ptr to boolean: TRUE to filter rows, FALSE to
**			  always just store the row
**	try_compress	- TRUE if RLL compression should be attempted on
**			  the row if it isn't filtered out.
**
** Outputs:
**
**	filterflag	- If TRUE upon entry, remains TRUE if row was
**			  filtered out.  FALSE if row was not filtered.
**
** Side Effects:
**
**
** History:
**	4-mar-99 (inkdo01)
**	    Written.
**	jan-00 (inkdo01)
**	    Changes for bit filters.
**	28-jan-00 (inkdo01)
**	    Tidy up hash memory management.
**	7-sep-01 (inkdo01)
**	    Fix up ojpartflag set after bit vector test. Fixes OJ logic.
**	13-aug-02 (inkdo01)
**	    Add nullcheck before partitioning row (to re-fix 105710).
**	22-Jun-2005 (schka24)
**	    Bit-filter position can be based on hashno within partition
**	    for better selectivity.
**	10-Nov-2005 (schka24)
**	    "ojpartflag" is really a filtering yes/no flag, rename.
**	17-Jun-2008 (kschendel) SIR 122512
**	    Row-length changes: track min sizes, current buffer page.
**	    Attempt RLL compression if caller says so.
*/
 
DB_STATUS
qen_hash_partition(
QEE_DSH		*dsh,
QEN_HASH_BASE	*hbase,
QEF_HASH_DUP	*rowptr,
bool		*filterflag,
bool		try_compress)
{

    QEN_HASH_PART	*hpptr;
    i4			pno, rowsz;
    u_i4		hashno, hashquo;
    i4			spillmask;
    u_i4		bvbits = hbase->hsh_bvsize * BITSPERBYTE - 1;
    i4			bvind;
    i4			sametest, sameflag;
    DB_STATUS		status;
    PTR			target;
    bool		reverse = hbase->hsh_reverse;
    bool		bvuse = FALSE, bvset = FALSE;


    /* Use hash number to determine target partition, increment row counts
    ** and copy row into corresponding block. */

    hashno = rowptr->hash_number;
    pno = hashno % hbase->hsh_pcount;
    hashquo = hashno / hbase->hsh_pcount;
    hpptr = &hbase->hsh_currlink->hsl_partition[pno]; /* target partition */

    /* "reverse" here means "loading probe rows" as opposed to loading
    ** build rows.
    */
    if (reverse)
    {
	rowsz = hbase->hsh_prowsz;
	spillmask = HSP_PSPILL;
	if (hpptr->hsp_brcount > 0) bvuse = TRUE;
	else if (hpptr->hsp_flags & spillmask) bvset = TRUE;
	sametest = hpptr->hsp_prcount;
	sameflag = HSP_PALLSAME;
    }
    else 
    {
	rowsz = hbase->hsh_browsz;
	spillmask = HSP_BSPILL;
	if (hpptr->hsp_prcount > 0) bvuse = TRUE;
	else if (hpptr->hsp_flags & spillmask) bvset = TRUE;
	sametest = hpptr->hsp_brcount;
	sameflag = HSP_BALLSAME;
    }

    /* Coarse checks - does row split to empty partition, or
    ** is corresponding bit vector entry empty, or is there
    ** a null key column. In any of the cases, throw the row 
    ** away (or make into OJ). */
    if (*filterflag)
    {
	if (!bvuse)
	    return (E_DB_OK);		/* Other side is empty */
	else
	{
	    bvind = hashquo % bvbits;
	    if (BTtest(bvind, hpptr->hsp_bvptr) == 0)
	    {
		hbase->hsh_bvdcnt++;	/* incr discard count */
		return(E_DB_OK);
	    }
	    /* Maybe row filters because null join col can't match anything */
	    if (hbase->hsh_flags & HASH_CHECKNULL &&
	      qen_hash_checknull(hbase, &rowptr->hash_key[0]))
		return(E_DB_OK);
	}
    }
    *filterflag = FALSE;		/* Not filtering out this row */

    /* Set bit filter (if necessary). */
    if (bvset) BTset(hashquo % bvbits, hpptr->hsp_bvptr);

    /* Check for continued "all same hashno in partition" condition. */
    if (hpptr->hsp_flags & sameflag)
    {
	if (sametest == 0)
	    hpptr->hsp_prevhash = hashno;
	else if (hashno != hpptr->hsp_prevhash)
	    hpptr->hsp_flags &= ~sameflag;
				/* if diff hashno, turn off flag */
    }

    /* First check if partition buffer been allocated (& if not, do it). */
    if (hpptr->hsp_bptr == NULL)
    {
	status = qen_hash_palloc(dsh->dsh_qefcb, hbase,
		    hbase->hsh_csize + hbase->hsh_bvsize,
		    &hpptr->hsp_bptr,
		    &dsh->dsh_error, "part_buf+bv");
	if (status != E_DB_OK)
	    return (status);

	hpptr->hsp_bvptr = &hpptr->hsp_bptr[hbase->hsh_csize];
			    /* bit filter follows partition buffer */
	hpptr->hsp_offset = 0;	/* Should be, make sure! */
    }

    /* Attempt RLL compression if caller says so */
    if (try_compress)
    {
	target = &hpptr->hsp_bptr[hpptr->hsp_offset];

	/* rowsz here is the maximum row length for the side being loaded.
	** If we can compress the row directly into the buffer, do so;
	** otherwise compress into workrow2 which is usable at this
	** point.
	*/
	if (rowsz + hpptr->hsp_offset > hbase->hsh_csize)
	    target = (PTR) hbase->hsh_workrow2;
	rowptr = qen_hash_rll(hbase->hsh_keysz, rowptr, target);
    }
 
    rowsz = QEFH_GET_ROW_LENGTH_MACRO(rowptr);

    /* Count rows and bytes added to partition. */
    if (reverse)
    {
	hpptr->hsp_prcount++;
	hpptr->hsp_pbytes += rowsz;
	if (rowsz < hbase->hsh_min_prowsz)
	    hbase->hsh_min_prowsz = rowsz;
    }
    else
    {
	hpptr->hsp_brcount++;
	hpptr->hsp_bbytes += rowsz;
	if (rowsz < hbase->hsh_min_browsz)
	    hbase->hsh_min_browsz = rowsz;
    }

    if (rowsz + hpptr->hsp_offset > hbase->hsh_csize)
    {
	i4	chunk;
	PTR	source;

	/* Not enough space left in partition buffer. Stuff as much
	** as will fit in this block, write it and store rest of
	** row in next block. */

	chunk = hbase->hsh_csize - hpptr->hsp_offset;
					/* amount that'll fit */
	if (chunk)
	{
	    target = (PTR)&hpptr->hsp_bptr[hpptr->hsp_offset];
	    MEcopy ((PTR) rowptr, chunk, target);
					/* copy what fits (if anything) */
	}
	if (!(hpptr->hsp_flags & spillmask))
	{
	    /* Must open, before we can write. */
	    status = qen_hash_open(dsh, hbase, hpptr);
	    if (status != E_DB_OK) 
			    return(status);
	    hpptr->hsp_flags |= spillmask;	/* now flag it */

	    /* At this point, must also initialize bit filter with 
	    ** rows in first buffer full (if we're building, not
	    ** probing). */
	    if (!bvuse)
	    {
		qen_hash_bvinit(hbase, hpptr);
		/* bvset wasn't on before, so set the bit for the current
		** row now.  (Perhaps redundantly if the hash number for
		** this row fit into the tail end of the buffer -- no
		** harm done though.)
		*/
		BTset(hashquo % bvbits, hpptr->hsp_bvptr);
	    }
	}

	status = qen_hash_write(dsh, hbase, hpptr->hsp_file,
		hpptr->hsp_bptr,
		hbase->hsh_csize);
	if (status != E_DB_OK) 
			return(status);

	/* Track page number now in buffer, for hash build optimization */
	hpptr->hsp_buf_page = hpptr->hsp_file->hsf_currbn;
	source = (PTR)&((char *)rowptr)[chunk];
	chunk = rowsz - chunk;
	hpptr->hsp_offset = chunk;	/* reset buffer offset */
	target = (PTR)hpptr->hsp_bptr;
	MEcopy (source, chunk, target);
					/* copy rest of row to front */
	return(E_DB_OK);

    }
    else
    {
	/* Compute target address of new row and copy it there. */
	target = (PTR)&hpptr->hsp_bptr[hpptr->hsp_offset];
	if ((PTR) rowptr != target)
	    MEcopy((PTR)rowptr, rowsz, target);
	hpptr->hsp_offset += rowsz;
    }
    return(E_DB_OK);

}	/* end of qen_hash_partition */


/*{
** Name: QEN_HASH_BUILD - time to build a hash table from current
**			partition set
**
** Description:
**	This function examines the rows input so far and tries to build
**	a hash table to probe against.  The presumption is that we've
**	loaded rows from at least one side (perhaps both) and we need
**	to try to create a hash table.  If no hash table can be created,
**	because all input partitions are too big to fit, a partition is
**	recursively repartitioned until a hash table can be created.
**
**	This function is sensitive to the specific needs of CO joins.
**	The nature of CO joins prevents the use of role reversal and
**	some effort is expended in this module to assure that only the
**	original build source is used to load the hash table for CO
**	joins.
**
**	Comment fatigue warning!
**	Sorry for the outrageous amount of blather, but it's simply
**	a complex task.  The code can probably be simplified beyond
**	what you see, but one is reluctant to break things...
**
**	Because of role reversal, terminology is not as simple as a
**	usual join where there is just left and right.  Here we have
**	the build or hashtable side, the side we propose to build a
**	hashtable from;  and the probing side.  Without any role
**	reversal, the hashtable side is the left (build) input and the
**	probing side is the right (probe) input, but given role
**	reversal the hashtable and probing sides might be switched.
**	If just one side has been loaded so far it's the build/hashtable
**	side.
**
**	Building a hash table is complex because there are so many
**	cases to deal with:
**	- both sides of the current partitioning level loaded, or just one;
**	- some partitions may have spilled to disk;
**	- some partitions on either side may not have spilled at all (*);
**	- in-memory partitions may have extra buffer space that can be
**	  used to help load additional spilled partitions into;
**	- some partitions may be empty, on either side, with or without a
**	  partition buffer allocated;
**	- If just one side is loaded, spilled partitions may not have
**	  flushed (so that the last block is still in the partition buffer).
**	
**	(*) note however that only one side, hashtable or probing, may have
**	nonempty in-memory partitions; not both sides.  If some partition
**	of the first side loaded fits into memory, a hash table can and
**	will be constructed from that partition, and it will be consumed
**	as the other, probing side loads.
**
**	To build a hash table, there are again many possibilities.
**	- if the hashtable side has in-memory (unspilled) partitions, use
**	them to build a hash table, plus whatever spilled partitions
**	we can fit in.
**	- If the hashtable side has no in-memory partitions, but the probing
**	side does, do a role reversal and build a hash table out of those
**	probing partitions, plus any other probing partitions that fit.
**	- If there aren't any in-memory partitions, try to use available
**	partition buffer space to reload one or more spilled partitions.
**	Important note here: if just one side has been loaded, we can't
**	steal buffers from other spilled partitions.  We need those
**	buffers to load and maybe spill the probing side.  If we've loaded
**	both sides, than any spilled partition buffer is fair game.
**	- If we can't fit any hashtable-side partitions, but we can reload
**	and fit some probing-side partitions, do a role reversal.
**	- If nothing fits, do a recursion (i.e. open a new level, choose
**	a partition, and subdivide it).  Then repeat the above process,
**	starting with whichever side recursion decided to load first.
**
**	Notes to above discussion:
**	- CO joins can't do role reversal, and the code has to deal with
**	that limitation.  For CO joins, the hashtable side is ALWAYS the
**	left input and the probing side is ALWAYS the right input.
**	- Upon initial entry at the very top, either both sides are loaded,
**	or just the left/build side is (at the top level).  The only time
**	that the probe side is loaded first is during a role-reversed
**	recursion, and we do that inside the body of the decision loops.
**	- If both sides are loaded, it doesn't matter which side we
**	start looking at, we can figure out role reversal if needed.
**	- Given the last two points, upon initial entry we'll always start
**	with the build/left side as the hashtable side, and go from there.
**	- After finishing up loading the second side (ie when setting
**	BOTH-LOADED), we always immediately flush spilled buffers.
**	So, in the both-loaded case, we can assume that any buffer for
**	a spilled partition has nothing important in it, and is fair game.
**	- If you read the code very closely, you'll see that if the hashtable
**	side is entirely empty, we announce "done" no matter what's in the
**	probing side.  That is safe because the situation never arises.
**	(You never recurse on an empty partition, and if the build
**	input is empty at the top level, the hash join driver handles
**	it as a special case - either no joins or by-hand right/full OJ.)
**
** Inputs:
**	dsh		- Pointer to thread's DSH
**	hbase		- ptr to base hash structure
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	5-mar-99 (inkdo01)
**	    Written.
**	jan-00 (inkdo01)
**	    Changes for cartprod logic, various bug fixes.
**	28-jan-00 (inkdo01)
**	    Tidy up hash memory management.
**	8-feb-00 (inkdo01)
**	    Added logic for CO joins.
**	23-aug-01 (devjo01)
**	    Remove redundant and incorrect init of htable[0].
**	25-oct-01 (inkdo01)
**	    Fix bug with CO joins in which probe partitions fit in memory.
**	10-jan-02 (inkdo01)
**	    Fix minor bug in free chain management (106785).
**	13-mar-02 (inkdo01)
**	    Fix a bug when the free chain starts life empty.
**	28-mar-03 (inkdo01)
**	    Don't take "last block still in buffer" shortcut if FLUSHED
**	    flag is on for partition.
**	27-june-03 (inkdo01)
**	    "Last block in buffer" logic entered with faulty test that
**	    didn't work if block was almost full (bug 110498).
**	4-nov-03 (inkdo01)
**	    Another boundary condition involving overflowed partition and
**	    partition size that's a multiple of row size (bug 111226).
**	12-jan-04 (inkdo01)
**	    Fine tuning of previous fix - left out the "exact multiple" bit.
**	21-june-04 (inkdo01)
**	    Allocate hsp_bptr/bvptr in tandem to avoid later SEGVs.
**	14-july-04 (inkdo01)
**	    Add QEF_CHECK_FOR_INTERRUPT to avoid long loops in hash processing.
**	22-jul-04 (toumi01)
**	    A partition can be used even if it is an exact fit; this change
**	    keeps us from falling into a black hole looking for partitions.
**	3-Jun-2005 (schka24)
**	    Release files when a recursion level is done.  This reduces
**	    disk usage, as we'll be able to re-use the files if we return
**	    to that level later.  Also, copy back partition buffer addresses,
**	    since the just-completed level may have allocated more partition
**	    buffers (if some outer level partitions had nothing hash to them.)
**	8-Jun-2005 (schka24)
**	    Pitch out partitions that have both sources loaded, and one
**	    side is empty, and we don't need the other side for OJ.
**	    Not only does this eliminate useless partitions more quickly,
**	    but it also prevents am SC0206 "didn't need to recurse" error.
**	13-Jun-2005 (schka24)
**	    Rework partition selection.  Various comments notwithstanding,
**	    we were still grabbing partition buffers for the hash heap when
**	    they were needed to buffer/write probe rows.
**	    Fix bug related to 111226 above, a row can start at the beginning
**	    of the last-block-in-memory partition buffer even if rowsize isn't
**	    a multiple of the partition buffer size.
**	10-Jul-2005 (schka24)
**	    Last round of changes forgot to clear bufrows upon role reversal,
**	    often leading to QE0002.  Fix.
**	23-Dec-2005 (kschendel)
**	    Make sure that "firstbr" is cleared.  If last probe of prior
**	    hash table matched, it can be left on, pointing at garbage and
**	    confusing the first probe into the new hash.
**      23-Oct-2006 (huazh01)
**          check if both 'this' and 'other' side of partition are empty 
**          before calling qen_hash_insert(). This prevents setting 
**          'this_inmem', 'mustflip' ...., etc. to wrong value when there is 
**          actually nothing being inserted into the hash table. 
**          This fixes b116580.  
**	24-Apr-2006 (kschendel)
**	    Fix the test for when we've reached the last (in-memory) block
**	    when building spilled partitions.  Old test broke if build rows
**	    were very large (testcase had 3768 byte rows in a 16K chunk).
**	28-May-2008 (jgramling)
**	    Preliminary hack to improve hash table allocation in PC joins.
**	    Check if current hash table is too small (at least 10% more rows
**	    in incoming build size than current hash table dimension)
**	    before deciding to reuse.  Skewed data in PC joins can have a 
**	    severe performance impact: a small partition processed
**	    early-on can result in a hash table that is much too small 
**	    for later partitions that have many more rows.
**	9-Jan-2009 (kibro01) b121352
**	    It is possible to end up with a single partition containing too
**	    much to stop it spilling but not all the same values, and with
**	    no rows on the other side, so in this case it needs to be recursed
**	    upon, but the existing logic leaves us trying to recurse and break
**	    down the side with the fewer rows, which is wrong (0 rows can't
**	    be split) so reverse rolls before recursing.
**	18-Jun-2008 (kschendel) SIR 122512
**	    Rework to operate on bytes rather than rows, now that rows can
**	    be compressed (and hence of varying lengths).  One implication is
**	    that space can no longer be calculated exactly, including row
**	    splits, so allow a modicum of extra hashtable space.
**	    Stop wasting the bitvector space, we can use it.
**	    Refine/move Jim's hash pointer table fixes from above.
**	31-Mar-2010 (smeke01 b123516
**	    Add BCANDIDATE/PCANDIDATE to trace displays.
**	24-Jun-2010 (smeke01) b123456
**	    Whenever we set the flag HASH_NOHASHT in hbase->hsh_flags, make
**	    sure that we also unset the HASH_BUILD_OJS flag.
*/
 
DB_STATUS
qen_hash_build(
QEE_DSH		*dsh,
QEN_HASH_BASE	*hbase)
{

    QEN_HASH_LINK	*hlink;
    QEN_HASH_PART	*hpptr;
    QEN_HASH_PART	*prev_hpptr;
    i4			emptyps;
    QEF_CB		*qcb = dsh->dsh_qefcb;
    QEF_HASH_DUP	*brptr;
    FREE_CHUNK		*free;		/* Head of free chunk list */
    i4			rowsz;
    i4			empty_size, min_useful_size;
    i4			i, j;
    i4			spillmask, probing_spillmask, inhashmask, flushmask;
    i4			candidate_mask;	/* BCANDIDATE or PCANDIDATE */
    i4			probing_cand_mask;
    i8			candidates, probing_candidates;  /* row counts */
    i8			ht_bytes, probing_bytes;	/* Total byte counts */
    u_i4		empty_total;
    u_i4		hashrows;
    u_i4		probing_rowcnt;
    u_i4		rowcnt;
    PTR			extras;
    DB_STATUS		status;
    bool		recurse;
    bool		reverse, done;
    bool		ht_oj, probing_oj;
    bool		ht_inmem, probing_inmem;
    bool		dontreverse = (hbase->hsh_nodep->node_qen.qen_hjoin.hjn_kuniq);
					/* probe rows ineligible for hash */

    hbase->hsh_firstbr = NULL;		/* Clean out saved state */
    hbase->hsh_nextbr = NULL;
    hbase->hsh_flags &= ~HASH_BUILD_OJS;
    hbase->hsh_flags |= HASH_NOHASHT;

    /* An empty, available-for-use partition buffer includes the immediately
    ** following bit-vector as well.
    */

    empty_size = hbase->hsh_csize + hbase->hsh_bvsize;

    /* Always start with the build input.  It's normally estimated to be
    ** the smaller one, unless we're in a CO join, in which case we MUST
    ** build a hash table from the build input.
    **
    ** If the context is such that there isn't a build side, or the
    ** probe side is preferable, we'll figure that out as we go.
    **
    ** At this outer point, either we're at the top level and the build
    ** input has been loaded;  or we're at any level and both sides
    ** have been loaded.
    */

    hbase->hsh_reverse = FALSE;

    /* Big loop oversees continued attempts to build the hash table. If
    ** any iteration fails because all available partitions at this level
    ** are too big to fit the hash table, a recursion is performed
    ** (by qen_hash_recurse) and we try again to build a hash table.
    ** Note that successive trips through the loop can be operating
    ** on different recursion levels.
    */

    do	/* hash table build loop - extends to end of function */
    {

	/* Set local variables, depending on current role reversal. */
	reverse = hbase->hsh_reverse;
	hlink = hbase->hsh_currlink;
	free = NULL;

	if (dontreverse && reverse)
	{
	    /* Can't build a hash table with probe rows if
	    ** hjn_kuniq (CO join). */
	    reverse = FALSE;
	    hbase->hsh_reverse = FALSE;
	}

	if (dsh->dsh_hash_debug)
	{
	    /* Print some stuff */
	    TRdisplay ("%@ hjoin %s htbuild, node: %d, t%d, level %d\n",
		reverse ? "probe" : "build",
		hbase->hsh_nodep->qen_num, dsh->dsh_threadno,
		hlink->hsl_level);
	    for (i = 0; i < hbase->hsh_pcount; i++)
	    {
		hpptr = &hlink->hsl_partition[i];
		TRdisplay ("t%d: P[%d]: b:%lu (%u)  p:%lu (%u), flags %v\n",
			dsh->dsh_threadno, i,
			hpptr->hsp_bbytes, hpptr->hsp_brcount,
			hpptr->hsp_pbytes, hpptr->hsp_prcount,
			"BSPILL,PSPILL,BINHASH,PINHASH,BFLUSHED,PFLUSHED,BALLSAME,PALLSAME,DONE,BCANDIDATE,PCANDIDATE",hpptr->hsp_flags);
	    }
	}

	probing_inmem = FALSE;
	ht_inmem = FALSE;		/* These are mutually exclusive */
	if (reverse)
	{
	    spillmask = HSP_PSPILL;
	    probing_spillmask = HSP_BSPILL;
	    inhashmask = HSP_PINHASH;
	    flushmask = HSP_PFLUSHED;
	    ht_oj = (hbase->hsh_flags & HASH_WANT_RIGHT) != 0;
	    probing_oj = (hbase->hsh_flags & HASH_WANT_LEFT) != 0;
	    candidate_mask = HSP_PCANDIDATE;
	    probing_cand_mask = HSP_BCANDIDATE;
	    min_useful_size = hbase->hsh_min_prowsz;
	}
	else
	{
	    spillmask = HSP_BSPILL;
	    probing_spillmask = HSP_PSPILL;
	    inhashmask = HSP_BINHASH;
	    flushmask = HSP_BFLUSHED;
	    ht_oj = (hbase->hsh_flags & HASH_WANT_LEFT) != 0;
	    probing_oj = (hbase->hsh_flags & HASH_WANT_RIGHT) != 0;
	    candidate_mask = HSP_BCANDIDATE;
	    probing_cand_mask = HSP_PCANDIDATE;
	    min_useful_size = hbase->hsh_min_browsz;
	}
	if (min_useful_size < sizeof(FREE_CHUNK))
	    min_useful_size = sizeof(FREE_CHUNK);
	hashrows = 0;
	candidates = 0;
	probing_candidates = 0;
	done = TRUE;

	if (dontreverse) 
	{
	    if (hlink->hsl_flags & HSL_BOTH_LOADED)
	    {
		/* Can't reverse, make sure that probe input is flushed,
		** even partitions that would normally stay in memory.
		*/
		status = qen_hash_flushco(dsh, hbase);
		if (status != E_DB_OK)
		    return(status);
	    }
	}
	recurse = TRUE;		/* Turned off if we can make a hash table */

	/* Usage note:
	** empty_total = total space available in unallocated or on-free-list
	**   buffers or buffer pieces, or "extra" memory chunks.  The
	** latter are not placed on the free list since we want to use them
	** as a last resort.
	*/

	/* Initialize extra space to include extra memory chunks, minus one.
	** The -1 is to leave one chunk to absorb overage due to rows
	** not being split across buffers;  otherwise successive hash
	** table builds will tend to add extra chunks due to excess
	** optimism in space guessing.
	*/
	empty_total = 0;
	if (hbase->hsh_extra_count > 1)
	    empty_total = (hbase->hsh_extra_count-1) * hbase->hsh_csize;
	extras = hbase->hsh_extra;

	/* Do a pre-pass over the partitions, looking for partitions
	** that are or can be marked DONE.  This makes it easier to
	** place them on the free list, next pass.
	** Partitions caught here are:
	** - unallocated, partition has never been used at all;
	** - completed from previous hash table (actually, shouldn't happen);
	** - uninteresting: both sides loaded and either side is empty
	**   (with suitable adjustments for outer joins!)
	*/
	emptyps = 0;
	for (i = 0; i < hbase->hsh_pcount; i++)
	{
	    hpptr = &hlink->hsl_partition[i];
	    hpptr->hsp_flags &= ~(HSP_BCANDIDATE | HSP_PCANDIDATE);
	    if (hpptr->hsp_bptr == NULL) 
	    {
		/* Completely empty partition, never used.  Mark it
		** DONE to make sure we don't waste time on it later,
		** even if we fill in its bptr for extra hash space.
		*/
		emptyps++;
		empty_total += empty_size;
		hpptr->hsp_flags = HSP_DONE;
		continue;
	    }
	    if (hpptr->hsp_flags & (HSP_DONE | inhashmask))
	    {
		/* Shouldn't see "inhashmask" here?  but if we do, it's
		** from last time, so it's done.
		*/
		if (dsh->dsh_hash_debug && (hpptr->hsp_flags & HSP_DONE) == 0)
		{
		    TRdisplay("n%d t%d pno %d was in-hash? is done\n",
			hbase->hsh_nodep->qen_num,
			dsh->dsh_threadno, i);
		}
		hpptr->hsp_flags = HSP_DONE;
		continue;
	    }
	    /* Check out partition to see if it's useless.  We can ditch
	    ** the partition if either side is empty, and either it's an
	    ** inner join or the outer side(s) is empty.
	    ** (The "both loaded" test is needed so that we don't
	    ** drop initial or freshly-recursed partitions that
	    ** we haven't had a chance to load probing side rows into.)
	    */
	    rowcnt = hpptr->hsp_brcount;
	    probing_rowcnt = hpptr->hsp_prcount;
	    if (reverse)
	    {
		rowcnt = probing_rowcnt;
		probing_rowcnt = hpptr->hsp_brcount;
	    }
	    if (hlink->hsl_flags & HSL_BOTH_LOADED
	      && (rowcnt == 0 || probing_rowcnt == 0)
	      && (!ht_oj || rowcnt == 0)
	      && (!probing_oj || probing_rowcnt == 0) )
	    {
		/* This is a now-useless partition, make it look
		** DONE, free up disk file if any.
		** P1P2 in-between phase should take care of this for
		** top level, but this code might still be useful for
		** recursion levels.
		*/
		if (dsh->dsh_hash_debug)
		{
		    TRdisplay("n%d t%d dropping empty p[%d] brcount %u prcount %u\n",
			hbase->hsh_nodep->qen_num,
			dsh->dsh_threadno, i, hpptr->hsp_brcount,
			hpptr->hsp_prcount);
		}
		status = qen_hash_close(hbase, &hpptr->hsp_file, CLOSE_DESTROY);
		if (status != E_DB_OK)
		    return (status);
		hpptr->hsp_flags = HSP_DONE;
		continue;
	    }
	} /* prepass for */

	/* Now look for partitions that are unspilled and can be loaded
	** directly into the hash table.  Also, any partitions marked
	** DONE can be added to the free list.
	*/
	for (i = 0; i < hbase->hsh_pcount; i++)
	{
	    hpptr = &hlink->hsl_partition[i];
	
	    /* Uninitialized partitions were completely handled in
	    ** the prepass loop, skip them here.
	    */
	    if (hpptr->hsp_bptr == NULL) 
		continue;

	    /* DONE partitions can be added to the free list */
	    if (hpptr->hsp_flags & HSP_DONE)
	    {
		((FREE_CHUNK *)hpptr->hsp_bptr)->free_next = free;
		free = (FREE_CHUNK *)hpptr->hsp_bptr;
		free->free_bytes = empty_size;
		empty_total += empty_size;
		continue;
	    }

	    probing_rowcnt = (reverse) ? hpptr->hsp_brcount : hpptr->hsp_prcount;

	    /* See if the probing side of this partition is sitting in the
	    ** buffer and didn't spill.  That's a dead giveaway that we
	    ** are on the wrong side.  Set a flag that asks for a role
	    ** reversal, and drop out of this partition loop.
	    ** As remarked a few other places, it's impossible to have
	    ** unspilled partitions from both sides, simultaneously.
	    ** (e.g. if initially some build partitions don't spill, they
	    ** can and do form a hash table, and are consumed as the
	    ** probes load.)
	    ** Note that "probing side empty" counts as in-memory if both
	    ** sides are loaded.  (It must be an OJ partition on this
	    ** side, or it would have been caught as done earlier.)
	    */
	    if ((hpptr->hsp_flags & probing_spillmask) == 0
	      && (probing_rowcnt > 0
		  || ((hlink->hsl_flags & HSL_BOTH_LOADED)
			&& probing_rowcnt == 0)) )
	    {
		probing_inmem = TRUE;
		break;
	    }

	    /* Skip spilled partitions, we'll deal with them next loop. */
	    if (hpptr->hsp_flags & spillmask)
		continue;

	    /* We get here for partitions which didn't spill to disk. */

	    rowcnt = (reverse) ? hpptr->hsp_prcount : hpptr->hsp_brcount;
	    if (rowcnt == 0 && (hlink->hsl_flags & HSL_BOTH_LOADED) == 0)
	    {
		/* Nothing here, just skip this partition if the other side
		** isn't loaded yet.
		*/
		continue;
	    }

	    /* Load this possibly empty (!) partition into the hash table.
	    ** (The partition can be empty only if the probing side is OJ
	    ** and nonempty;  otherwise it would be DONE instead.)
	    */

	    if (hashrows == 0)
	    {
		/* (re)allocate hash pointer table if new hash table */
		status = qen_hash_htalloc(dsh, hbase);
		if (status != E_DB_OK)
		    return (status);
	    }
	    hbase->hsh_flags &= ~HASH_NOHASHT;	/* allow loop exit */
	    done = FALSE;
	    recurse = FALSE;			/* Recursion not needed */
	    ht_inmem = TRUE;
	    ++ hbase->hsh_memparts;		/* for stats */
	    hashrows += rowcnt;
	    hpptr->hsp_flags |= inhashmask;	/* part'n in hash tble */
	    brptr = (QEF_HASH_DUP *)hpptr->hsp_bptr;  /* 1st row */

	    /* Now loop over rows in partition buffer, adding each to
	    ** hash table. */
	    for (j = 0; j < rowcnt; j++)
	    {
		rowsz = QEFH_GET_ROW_LENGTH_MACRO(brptr);
		status = qen_hash_insert(dsh, hbase, brptr);
		if (status != E_DB_OK) 
				return(status);

		brptr = (QEF_HASH_DUP *)((char *)brptr + rowsz);
					/* next row in partition buffer */
	    }	/* end of partition buffer loop */

/* qen_hash_verify(hbase); */

	    /* If there is still more space in this block, add to a free chain
	    ** to permit space to load disk resident partitions.
	    */
	    if (empty_size - ((PTR)brptr - hpptr->hsp_bptr) >= min_useful_size)
	    {
		((FREE_CHUNK *)brptr)->free_next = free;
		free = (FREE_CHUNK *)brptr;
		free->free_bytes = empty_size - ((PTR)brptr - hpptr->hsp_bptr);
		empty_total += free->free_bytes;
	    }

	} /* end load unspilled loop */

	/* If the first loop detected an unspilled partition on the probing
	** side, that's the side we want - do a role reversal.
	** Because CO join does "flushco" above, we can't have probing_inmem
	** set if dontreverse is set.  CO joins don't allow role reversal.
	*/
	if (probing_inmem)
	{
	    if (ht_inmem)
	    {
		/* Should not be possible, defensive code here */
		TRdisplay("%@ qen_hash_build: node %d: both in-mem at level %d looking at %s side\n",
			hbase->hsh_nodep->qen_num,
			hlink->hsl_level,reverse ? "probe" : "build");
		dsh->dsh_error.err_code = E_QE0002_INTERNAL_ERROR;
		return (E_DB_ERROR);
	    }
	    hbase->hsh_reverse = !hbase->hsh_reverse;  /* Flip it */
	    continue;				/* Try again this way */
	}

	/* We may or may not have started a hash table.  Either way,
	** attempt to select spilled partitions to include in the hash
	** table.
	** If no hash table was started yet, we still have the option
	** of trying a role reversal if the other side looks better than
	** this side.  If we do a role reversal, we can assume that
	** all of the probing-side partitions spilled (else we would have
	** set probing-inmem, above).
	*/

	/* If we haven't loaded both sides of this level, we can't use
	** spilled partition buffers for hash data.  The reason is that
	** unless we can manage to squeeze the partition into "extra"
	** space (empty partitions, done partitions, etc), we can't join
	** probe rows that map to spilled partitions, and we'll need the
	** partition buffers to write the probe rows!  It's probably a
	** bad idea to use a partition for hashtable heap and probe output
	** simultaneously...
	*/

	if ((hlink->hsl_flags & HSL_BOTH_LOADED) == 0)
	{
	    /* What we can do in the one-side-loaded case is to use
	    ** not-done partitions with buffers, but no rows.  This can
	    ** only happen if recursion created the buffer.
	    ** We can't drop or end the partition (yet), but we do know
	    ** that any probes for the to-be-loaded side will either
	    ** not join or will OJ; either way we don't need the buffer.
	    */
	    for (i = 0; i < hbase->hsh_pcount; i++)
	    {
		hpptr = &hlink->hsl_partition[i];
		rowcnt = (reverse) ? hpptr->hsp_prcount : hpptr->hsp_brcount;
		if ((hpptr->hsp_flags & (HSP_DONE | inhashmask)) == 0
		  && rowcnt == 0)
		{
		    ((FREE_CHUNK *)hpptr->hsp_bptr)->free_next = free;
		    free = (FREE_CHUNK *)hpptr->hsp_bptr;
		    free->free_bytes = empty_size;
		    empty_total += empty_size;
		}
	    }
	}
	else
	{
	    /* If both sides are loaded (and therefore flushed, see
	    ** introductory comments), we can assume that any spilled
	    ** buffer is flushed/empty and can be used for hash space.
	    ** (in-memory role reversal has already been detected.)
	    ** Simplify things a little by taking a quick trip through
	    ** the partitions, and for any one that has spilled (either
	    ** side), include it in the free list.
	    */
	    for (i = 0; i < hbase->hsh_pcount; i++)
	    {
		hpptr = &hlink->hsl_partition[i];
		if ((hpptr->hsp_flags & (HSP_DONE | inhashmask)) == 0
		  && hpptr->hsp_flags & (spillmask | probing_spillmask))
		{
		    ((FREE_CHUNK *)hpptr->hsp_bptr)->free_next = free;
		    free = (FREE_CHUNK *)hpptr->hsp_bptr;
		    free->free_bytes = empty_size;
		    empty_total += empty_size;
		}
	    }
	    /* Just an observation here:  if we haven't started a hash
	    ** table yet, at this point extra-space should equal the
	    ** total available space, ie empty_size times pcount plus any
	    ** extra chunks.  All partitions are either empty or spilled.
	    */
	}

	if ((hlink->hsl_flags & HSL_BOTH_LOADED) == 0)
	{
	    bool one_doesnt_fit = FALSE;

	    /* For the one-side-loaded case, while in-play partition buffers
	    ** are not normally usable (and aren't counted in empty_total),
	    ** it MAY be that the spilled partition is larger than the buffer
	    ** but smaller than the buffer+bitvector.  (At present, the load
	    ** process spills when the partition buffer is filled, on the
	    ** reasonable assumption that it's going to spill beyond what
	    ** the bitvector can hold.)  Run a loop to look for partitions
	    ** in this (unusual) state and mark them as candidates.
	    **
	    ** It suffices to just use the candidate flag, rather than
	    ** inventing a "must load this one" flag, since the loading
	    ** selection always chooses the smallest remaining partition.
	    ** Hence we'll load all of these just-barely-spilled partitions
	    ** before trying to jam any larger ones into any remaining space.
	    **
	    ** Only the hashtable side is loaded, ignore the other side.
	    */
	    for (i = 0; i < hbase->hsh_pcount; i++)
	    {
		hpptr = &hlink->hsl_partition[i];
		/* Only care about spilled partitions, skip bogus ones */
		if (hpptr->hsp_flags & (HSP_DONE | inhashmask)
		  || (hpptr->hsp_flags & spillmask) == 0)
		    continue;

		rowcnt = hpptr->hsp_brcount;
		ht_bytes = hpptr->hsp_bbytes;
		if (reverse)
		{
		    rowcnt = hpptr->hsp_prcount;
		    ht_bytes = hpptr->hsp_pbytes;
		}
		if (rowcnt == 0)
		    continue;
		/* Got a spilled partition nonempty on the loaded side */
		done = FALSE;
		if (ht_bytes > empty_size)
		{
		    one_doesnt_fit = TRUE;
		}
		else
		{
		    /* Hashtable side of partition fits in memory */
		    recurse = FALSE;
		    candidates += rowcnt;
		    hpptr->hsp_flags |= candidate_mask;
		    empty_total += empty_size;
		}
	    } /* for */

	    /* If there's a spilled partition that still didn't fit, we
	    ** can assume its buffer is available.  We can't assume more
	    ** than one though, must keep the others free for spilling
	    ** probes as we load them.
	    */
	    if (one_doesnt_fit)
		empty_total += empty_size;
	} /* if one side loaded */

	/* Now, do a loop over spilled partitions.  It's possible that
	** we might be able to fit some of them into the hash table.
	** A partition will fit if we can squeeze it into "extra" space,
	** which includes (if available) all of the spilled partition
	** buffers, including ours.
	**
	** As it happens, if both sides are loaded, we're always looking
	** at the build input now, and the "probing" side is the probe
	** input.  It's very little extra effort to code it properly
	** for the general case, though.
	*/
	for (i = 0; i < hbase->hsh_pcount; i++)
	{
	    hpptr = &hlink->hsl_partition[i];
	    /* Only care about spilled partitions;
	    ** skip bogus, done, already-in-hash partitions.
	    ** Also skip partitions already selected as a candidate above.
	    */
	    if (hpptr->hsp_flags & (HSP_DONE | inhashmask | candidate_mask)
	      || (hpptr->hsp_flags & (spillmask | probing_spillmask)) == 0)
		continue;

	    rowcnt = hpptr->hsp_brcount;
	    ht_bytes = hpptr->hsp_bbytes;
	    probing_rowcnt = hpptr->hsp_prcount;
	    probing_bytes = hpptr->hsp_pbytes;
	    if (reverse)
	    {
		rowcnt = probing_rowcnt;
		probing_rowcnt = hpptr->hsp_brcount;
		ht_bytes = probing_bytes;
		probing_bytes = hpptr->hsp_bbytes;
	    }
	    if (rowcnt == 0)
		continue;
	    /* Got a spilled partition that's nonempty on the hashtable
	    ** build side.  See if we can fit it into the
	    ** space we're allowed to use.
	    */
	    done = FALSE;
	    if (ht_bytes <= empty_total)
	    {
		/* Hashtable side of partition fits in memory */
		recurse = FALSE;
		candidates += rowcnt;
		hpptr->hsp_flags |= candidate_mask;
	    }
	    /* Perhaps we could do a role reversal and fit the partition.
	    ** Obviously that's only going to fly if no hash table has been
	    ** started yet, and if both sides have been loaded.
	    ** (As pointed out above, those conditions also imply that
	    ** "empty-total" is all partitions, not that it matters here.)
	    */
	    if (!dontreverse && !ht_inmem &&
		hlink->hsl_flags & HSL_BOTH_LOADED &&
		probing_rowcnt > 0 &&
		probing_bytes <= empty_total)
	    {
		/* Probing side of the partition can fit. */
		recurse = FALSE;
		probing_candidates += probing_rowcnt;
		hpptr->hsp_flags |= probing_cand_mask;
	    }
	} /* end of spilled partition loop */

	if (done)
	{
	    if (hlink->hsl_level == 0)
	    {
		/* We appear to be finished! */
		dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
		return(E_DB_WARN);
	    }
	    /* Done with this recursion level, clean it up and
	    ** return to the previous level.
	    */
	    /* Close all the files at the completed recursion level, but
	    ** retain them -- chances are that we'll be back, unless
	    ** hashing was very uneven.
	    ** Also, copy back the partition buffer addresses, just in case
	    ** we allocated more.  (We want to re-use those buffers if this
	    ** recursion level is re-entered later.)
	    */
	    if (dsh->dsh_hash_debug)
	    {
		TRdisplay("%@ hj node: %d, exiting recursion lvl %d for pno %d, t%d\n",
			hbase->hsh_nodep->qen_num,
			hlink->hsl_level, hlink->hsl_prev->hsl_pno, dsh->dsh_threadno);
	    }
	    prev_hpptr = &hlink->hsl_prev->hsl_partition[hbase->hsh_pcount-1];
	    for (hpptr = &hlink->hsl_partition[hbase->hsh_pcount-1];
		 hpptr >= &hlink->hsl_partition[0];
		 --hpptr, --prev_hpptr)
	    {
		/* Forcibly close the file, clean up hpptr */
		status = qen_hash_close(hbase, &hpptr->hsp_file, CLOSE_RELEASE);
		if (status != E_DB_OK) return(status);
		prev_hpptr->hsp_bptr = hpptr->hsp_bptr;
		prev_hpptr->hsp_bvptr = hpptr->hsp_bvptr;
		/* All lower-level partitions are done, just make sure */
		hpptr->hsp_flags = HSP_DONE;
	    }
	    /* reset to previous recursion level */
	    hbase->hsh_currlink = hlink = hlink->hsl_prev;
	    hpptr = &hlink->hsl_partition[hlink->hsl_pno];
	    /* zeroing byte counts is purely for clean debug output! */
	    hpptr->hsp_bbytes = hpptr->hsp_pbytes = 0;
	    hpptr->hsp_flags = HSP_DONE;
	    /* Don't need partition files, close them (if any) */
	    i = CLOSE_RELEASE;
	    if (hlink->hsl_level == 0)
		i = CLOSE_DESTROY;
	    status = qen_hash_close(hbase, &hpptr->hsp_file, i);
	    if (status != E_DB_OK) 
		return(status);
	    continue;			/* Restart at the now-current level */
	}
 
	if (dsh->dsh_hash_debug)
	{
	    TRdisplay("n%d t%d empty_total %u, emptyps %d (%u bytes) empty_size %d\n      inmem loaded %u rows, recurse %s, cand:%ld, pcand:%ld\n",
		hbase->hsh_nodep->qen_num, dsh->dsh_threadno,
		empty_total, emptyps, emptyps*empty_size,
		empty_size, hashrows,
		recurse ? "YES" : "No", candidates, probing_candidates);
	}

	if (recurse) 
	{
	    /* Couldn't build a hash table from current partition set.
	    **
	    ** If we haven't loaded both sides of this level, do so now.
	    ** If we're at the top, return.  (No probes have been loaded
	    ** at all, so load and partition the probes.)
	    ** If we're in a recursion, pull the rows out of the other
	    ** side of the partition being recursed, and repartition them.
	    **
	    ** If we HAVE loaded both sides of this level, all the partitions
	    ** on both sides are too large.  (Or perhaps the build side
	    ** is too large and it's a check-only join, that can't be
	    ** flipped.)  Do a recursion, which will pick a partition
	    ** and repartition it into (hopefully!) smaller chunks that
	    ** can be joined.
	    */

	    if ((hlink->hsl_flags & HSL_BOTH_LOADED) == 0)
	    {
		/* We've loaded one side and can't make a hash table.
		** We're going to load the other side now, so flush to
		** ensure that partition buffers are free.
		** Assert: partitions are either empty or spilled here.
		*/
		status = qen_hash_flush(dsh, hbase, FALSE);
		if (status != E_DB_OK) 
		    return(status);

		/* If top level, this is first call, just load the probe
		** side (for the first time).
		*/

		if (hlink->hsl_level == 0)
		    return (E_DB_OK);

		/* Just did recursion and first repartition. If we're 
		** back here, we still couldn't build hash table, so 
		** must repartition other source.
		** After repartitioning, try again to build a hash table
		** at this new recursion level, starting with the build
		** side.  (CO joins have to run that direction, and for
		** other joins, we'll do all the usual role reversal
		** machinery as appropriate.)
		** Turn on flag BEFORE call to repartition so it knows
		** that this is repartition of the other side.
		*/
		hlink->hsl_flags |= HSL_BOTH_LOADED;
		/* Flip role reversal so that we repartition the "other"
		** side of the previous level's recursed partition.
		*/
		hbase->hsh_reverse = !hbase->hsh_reverse;  /* Flip it */
		status = qen_hash_repartition(dsh, hbase, 
			&hlink->hsl_prev->hsl_partition[hlink->
				hsl_prev->hsl_pno]);
		/* Flush the side we just loaded.  We don't have the machinery
		** to keep careful enough track of which side's in the
		** buffers if they spilled.
		*/
		if (status == E_DB_OK) 
		    status = qen_hash_flush(dsh, hbase, FALSE);
		if (status != E_DB_OK) 
		    return(status);
		/* Reset back to the build side, try again. */
		hbase->hsh_reverse = FALSE;
	    }
	    else
	    {
		/* Both sides loaded, no place to go at this level.  Need
		** to recurse and repartition.  Recursion may select build
		** or probe, whichever is smallest.
		*/
		status = qen_hash_recurse(dsh, hbase);
		if (status != E_DB_OK) 
		    return(status);

		/* We are now at a new lower level with just one side
		** loaded.  (Or with cart-prod flagged, a special case!)
		** Leave role-reversal set whichever way recursion sets
		** it, since that's the side that's loaded.  Maybe the
		** recursed side can build a hash table at this new level.
		*/
	    }

	    hbase->hsh_flags &= ~HASH_BUILD_OJS;
	    hbase->hsh_flags |= HASH_NOHASHT;
	    if (hbase->hsh_flags & HASH_CARTPROD) return(E_DB_OK);
					/* next part is an "all same"
					** and requires CP joining */
	    continue;
	}

	/* Since "recurse" was off, the selection loops have decided
	** that we can build a hash table.   (Indeed, we may have already
	** started one.)
	**
	** If no hash table is in progress, it's possible that one
	** might be built in either direction.  Ideally we would pick
	** the direction that loads the fewest rows, using the unproven
	** but plausible theory that smaller hash tables are easier to
	** search and thus should produce an overall speedup.
	** Unfortunately, the only way to tell this for sure would be
	** to simulate partition candidate selection twice, once in
	** each direction, and see which wins.  This sounds like a lot
	** of effort, and as a substitute heuristic we'll simply choose
	** the side with the fewer (but nonzero) candidate rows total.
	**
	** *** FIXME might also be desirable to choose the non-OJ side
	** as the hash side, lacking other reasons to choose.
	**
	** It may be that we HAVE to reverse to get anywhere.
	**
	** Note that if we can't reverse roles, eg if a hash table was
	** already started, or a CO join, "probing" candidates remains zero.
	*/

	if (probing_candidates > 0
	  && (candidates == 0 || probing_candidates < candidates))
	{
	    /* We want to or have to role-reverse.  Do so, and rather than
	    ** fool with the big loop again, just continue with the
	    ** partition loading.  (Everything is spilled at this stage
	    ** and there is no point in doing the whole outer loop thing.)
	    */

	    hbase->hsh_reverse = reverse = ! reverse;
	    if (reverse)
	    {
		spillmask = HSP_PSPILL;
		inhashmask = HSP_PINHASH;
		flushmask = HSP_PFLUSHED;
		candidate_mask = HSP_PCANDIDATE;
		min_useful_size = hbase->hsh_min_prowsz;
	    }
	    else
	    {
		/* As noted above, we don't actually reverse from probe
		** to build when all spilled, but no harm in writing
		** the code in case things change later on.
		*/
		spillmask = HSP_BSPILL;
		inhashmask = HSP_BINHASH;
		flushmask = HSP_BFLUSHED;
		candidate_mask = HSP_BCANDIDATE;
		min_useful_size = hbase->hsh_min_browsz;
	    }
	    if (min_useful_size < sizeof(FREE_CHUNK))
		min_useful_size = sizeof(FREE_CHUNK);
	}

	/* There are now disk resident partitions small enough to fit
	** the remaining space in the current hash table heap. Nested
	** loops are executed to search successively for the smallest
	** such partition which will fit, add it to the table and keep
	** trying until no more will fit (or there are no more 
	** partitions).
	** As noted above, we may find that there aren't any spilled
	** partitions, in which case we take a harmless spin thru the
	** partition array and declare victory.
	*/

	for ( ; ; )	/* loop 'til no more fit */
	{
	    FREE_CHUNK *frptr;
	    FREE_CHUNK *frprev;			/* Single-linked-list chaser */
	    i4 bytes_read;			/* Bytes read into buffer */
	    i4 bytes_used;			/* Bytes used by this row */
	    i4 chunkleft;			/* Bytes left in this chunk */
	    i4 partial_row;			/* Size of start of split row */
	    i4 smalli;				/* Index of fit partition */
	    i4 smallest_rows;			/* For partition search */
	    i8 smallest_bytes;			/* For partition search */
	    PTR bptr;				/* Read buffer position */
	    PTR chunkptr;			/* Target chunk position */
	    u_i4 pages_per_read;		/* Pages per read buffer */
	    u_i4 stop_at_page;			/* First page to not read in */
	    bool in_part_buf;

	    if (QEF_CHECK_FOR_INTERRUPT(qcb, dsh) == E_DB_ERROR)
		return (E_DB_ERROR);

	    smallest_bytes = MAXI8;
	    smalli = -1;

	    /* First loop to find next qualifying disk partition.
	    ** We'll pick the smallest candidate that fits.
	    */
	    for (i = 0; i < hbase->hsh_pcount; i++)
	    {
		hpptr = &hlink->hsl_partition[i];
		/* Only look at partitions marked as candidates */
		if ((hpptr->hsp_flags & candidate_mask) == 0)
		    continue;
		/* We can't necessarily fit all the candidates.  Can we
		** still fit this one?
		*/
		if (reverse)
		{
		    rowcnt = hpptr->hsp_prcount;
		    ht_bytes = hpptr->hsp_pbytes;
		}
		else
		{
		    rowcnt = hpptr->hsp_brcount;
		    ht_bytes = hpptr->hsp_bbytes;
		}
		if (ht_bytes < smallest_bytes && ht_bytes <= empty_total)
		{
		    /* New smallest. */
		    smallest_bytes = ht_bytes;
		    smallest_rows = rowcnt;
		    smalli = i;
		}
	    }

	    if (smalli < 0) break;	/* no more found */

	    /* "smalli" is the next partition to be added to the hash
	    ** table. It must be opened, read and its rows added to the
	    ** current hash table (after we move them to available
	    ** row slots in the hash table heap). 
	    */

	    if (hashrows == 0)
	    {
		/* (re)allocate hash pointer table if new hash table */
		status = qen_hash_htalloc(dsh, hbase);
		if (status != E_DB_OK)
		    return (status);
	    }
	    hbase->hsh_flags &= ~HASH_NOHASHT;	/* allow loop exit */
	    hpptr = &hlink->hsl_partition[smalli];
	    rowcnt = smallest_rows;

	    /* Reset spill file to its start for this side */
	    hpptr->hsp_file->hsf_currbn = reverse ? hpptr->hsp_pstarts_at
						  : hpptr->hsp_bstarts_at;

	    /* Set up heap/buffer variables.  Note that this adjustment to
	    ** space available is inaccurate, as it doesn't allow for
	    ** unusable space due to splits, etc.  However making it more
	    ** "accurate" doesn't really help in the RLL-compressed world;
	    ** so we'll take the easy road, and if we're a little bit off,
	    ** an extra-space chunk will save the day.
	    */
	    empty_total -= smallest_bytes;
	    stop_at_page = 0xffffffff;	/* FIXME need a MAXUI4 */
	    /* If we didn't flush, the last row(s) are still in the partition
	    ** buffer, not on disk.  If both loaded, everything ought to be
	    ** flushed.  (both-loaded check is redundant?)
	    */
	    if ((hpptr->hsp_flags & flushmask) == 0
	      && (hlink->hsl_flags & HSL_BOTH_LOADED) == 0)
	    {
		stop_at_page = hpptr->hsp_buf_page;
		/* Since this partition is going to become part of the hash
		** table, grab any unused part of the partition buffer and
		** stick it onto the free list now.
		*/
		chunkleft = empty_size - hpptr->hsp_offset;
		if (chunkleft >= min_useful_size)
		{
		    frptr = (FREE_CHUNK *) (hpptr->hsp_bptr + hpptr->hsp_offset);
		    frptr->free_next = free;
		    frptr->free_bytes = chunkleft;
		    free = frptr;
		}
	    }
	    pages_per_read = hbase->hsh_rbsize / hbase->hsh_pagesize;
	    in_part_buf = FALSE;
	    partial_row = 0;
	    chunkleft = 0;
	    chunkptr = NULL;
	    /* Outer loop reads a new bufferful from the spill file,
	    ** inner loop inserts rows from the buffer into the hash table.
	    */
	    do
	    {
		/* Read the next bufferful, stopping short of reading the
		** stop-at page (because if set, those last pages are already
		** in the partition buffer)
		*/
		bytes_read = hbase->hsh_rbsize;
		if (hpptr->hsp_file->hsf_currbn + pages_per_read > stop_at_page)
		{
		    bytes_read = (stop_at_page - hpptr->hsp_file->hsf_currbn)
					* hbase->hsh_pagesize;
		}
		if (bytes_read > 0)
		{
		    status = qen_hash_read(hbase, hpptr->hsp_file,
					hbase->hsh_rbptr, bytes_read);
		    if (status != E_DB_OK)
			return (status);
		    bptr = hbase->hsh_rbptr;
		}
		else
		{
		    /* We must have reached the stop-at page.  All remaining
		    ** rows will come from the partition buffer itself.
		    ** The row count will stop the looping, just set the
		    ** bytes "read" high enough to not cause more reading.
		    */
		    bptr = hpptr->hsp_bptr;
		    bytes_read = hbase->hsh_csize;
		    in_part_buf = TRUE;
		}
		/* Now loop to read the rows out of the read buffer and
		** insert them.  This loop exits when the read buffer is
		** exhausted, or when we've inserted all the rows as
		** indicated by the row counter.
		*/
		do
		{
		    /* Worry about block splitting.
		    **
		    ** If this is a fresh row, make sure we have at least
		    ** the row header available in the read buffer.  If not,
		    ** we're splitting.  If yes, get the row size out of the
		    ** header and decide whether the whole row is there or not.
		    **
		    ** If we were reassembling a split row, finish the assembly
		    ** now.  The row is built in the work-row area so that we
		    ** can figure out its size before placing it.
		    */
		    brptr = (QEF_HASH_DUP *) bptr;
		    if (partial_row == 0)
		    {
			if (bytes_read < QEF_HASH_DUP_SIZE
			  || bytes_read < QEFH_GET_ROW_LENGTH_MACRO(brptr))
			{
			    partial_row = bytes_read;
			    MEcopy(bptr, partial_row, (PTR) hbase->hsh_workrow);
			    break;	/* out of inner loop */
			}
			/* get length of non-split row for below */
			rowsz = QEFH_GET_ROW_LENGTH_MACRO(brptr);
			bytes_used = rowsz;
		    }
		    else
		    {
			/* A row was started in workrow.  qee assures that the
			** buffer is as large as a row, which implies that
			** the remainder of the row has to be in the buffer.
			*/
			brptr = hbase->hsh_workrow;
			if (partial_row < QEF_HASH_DUP_SIZE)
			{
			    /* The header itself was split, complete the
			    ** header so that we can get the row size.
			    */
			    MEcopy(bptr, QEF_HASH_DUP_SIZE-partial_row,
					(PTR) brptr + partial_row);
			    bptr += QEF_HASH_DUP_SIZE-partial_row;
			    bytes_read -= QEF_HASH_DUP_SIZE-partial_row;
			    partial_row = QEF_HASH_DUP_SIZE;
			}
			rowsz = QEFH_GET_ROW_LENGTH_MACRO(brptr);
			bytes_used = rowsz - partial_row;
			MEcopy(bptr, bytes_used, (PTR) brptr + partial_row);
			partial_row = 0;	/* Not split now */
		    }
		    /* Advance read buffer position */
		    bptr += bytes_used;
		    bytes_read -= bytes_used;

		    /* Unless we're reading out of the last block(s) in the
		    ** partition buffer, we need to copy the row into the
		    ** hash build area, which might involve finding a place
		    ** where it will fit.
		    */
		    if (!in_part_buf || brptr == hbase->hsh_workrow)
		    {
			if (chunkleft < rowsz)
			{
			    /* This row doesn't fit, but if the chunk has
			    ** useful space left, return what's left to the
			    ** free list.  (Obviously this only happens
			    ** when RLL-compressing rows.)
			    */
			    frptr = free;
			    frprev = NULL;
			    if (chunkleft >= min_useful_size)
			    {
				((FREE_CHUNK *)chunkptr)->free_next = free;
				free = (FREE_CHUNK *) chunkptr;
				free->free_bytes = chunkleft;
				frprev = free;
				if (dsh->dsh_hash_debug)
				{
				    TRdisplay("n%d t%d putback %d bytes %p..%p\n",
					hbase->hsh_nodep->qen_num,
					dsh->dsh_threadno, chunkleft,
					chunkptr, chunkptr+chunkleft-1);
				}
			    }
			    /* Search for a free chunk that's large enough. */
			    while (frptr != NULL && frptr->free_bytes < rowsz)
			    {
				frprev = frptr;
				frptr = frptr->free_next;
			    }
			    if (frptr != NULL)
			    {
				/* Take the winner off the free list */
				if (frprev == NULL)
				    free = frptr->free_next;
				else
				    frprev->free_next = frptr->free_next;
				chunkptr = (PTR) frptr;
				chunkleft = frptr->free_bytes;
				if (dsh->dsh_hash_debug)
				{
				    TRdisplay("n%d t%d use free %d bytes %p..%p\n",
					hbase->hsh_nodep->qen_num,
					dsh->dsh_threadno, chunkleft,
					chunkptr, chunkptr+chunkleft-1);
				}
			    }
			    else if (emptyps > 0)
			    {
				/* Use a partition which is totally empty and
				** never had a buffer allocated.
				*/
				emptyps--;
				status = qen_hash_palloc(qcb, hbase,
					empty_size,
					&chunkptr,
					&dsh->dsh_error, "xtr_part_buf+bv");
				if (status != E_DB_OK)
				    return (status);
				chunkleft = empty_size;

				/* Now find empty buffer slot to point to 
				** this buffer. */
				for (i = 0; i < hbase->hsh_pcount; i++)
				{
				    if (hlink->hsl_partition[i].hsp_bptr == NULL)
				    {
					hlink->hsl_partition[i].hsp_bptr = chunkptr;
					/* bit filter follows partition buffer */
					hlink->hsl_partition[i].hsp_bvptr =
					    &hlink->hsl_partition[i].hsp_bptr[hbase->hsh_csize];
					break;
				    }
				}
			    }
			    else if (extras != NULL)
			    {
				/* Use the next existing "extra" chunk, of
				** usable size hsh_csize (not empty_size).
				*/
				chunkptr = extras + sizeof(PTR);
				extras = *(PTR *)extras;
				chunkleft = hbase->hsh_csize;
				if (dsh->dsh_hash_debug)
				{
				    TRdisplay("n%d t%d use extra %d bytes %p..%p\n",
					hbase->hsh_nodep->qen_num,
					dsh->dsh_threadno, chunkleft,
					chunkptr, chunkptr+chunkleft-1);
				}
			    }
			    else
			    {
				/* The inability to split rows in the hash
				** table has caused our calculations to be a
				** little off.  Allocate an "extra" chunk of
				** usable size hsh_csize, linked onto the extra
				** list via a pointer at its start.
				*/
				status = qen_hash_palloc(qcb, hbase,
						hbase->hsh_csize + sizeof(PTR),
						&chunkptr,
						&dsh->dsh_error, "extra");
				if (status != E_DB_OK)
				    return (status);
				*(PTR *)chunkptr = hbase->hsh_extra;
				hbase->hsh_extra = chunkptr;
				++ hbase->hsh_extra_count;
				chunkptr += sizeof(PTR);
				chunkleft = hbase->hsh_csize;
			    }
			}
			/* Ok, we have someplace to put the row, move it */
			MEcopy((PTR) brptr, rowsz, chunkptr);
			brptr = (QEF_HASH_DUP *) chunkptr;
			chunkptr += rowsz;
			chunkleft -= rowsz;
		    }
		    /* Finally, we have a whole row in hash space.  Insert
		    ** it into the hash table and count it off.
		    */
		    status = qen_hash_insert(dsh, hbase, brptr);
		    if (status != E_DB_OK)
			return (status);
		    hashrows++;
		    rowcnt--;
		} while (rowcnt > 0 && bytes_read > 0);

		/* If we fall out of the row insertion loop, either we
		** emptied the read buffer, or we're done.  Loop back for
		** another buffer unless we're finished.
		*/
	    } while (rowcnt > 0);

	    /* If there is space left in the chunk we were loading into,
	    ** release it back to the free list in case we load more
	    ** partitions into this hash table.
	    */
	    if (chunkleft >= min_useful_size)
	    {
		((FREE_CHUNK *)chunkptr)->free_next = free;
		free = (FREE_CHUNK *) chunkptr;
		free->free_bytes = chunkleft;
		if (dsh->dsh_hash_debug)
		{
		    TRdisplay("n%d t%d putback-at-end %d bytes %p..%p\n",
			hbase->hsh_nodep->qen_num,
			dsh->dsh_threadno, chunkleft,
			chunkptr, chunkptr+chunkleft-1);
		}
	    }

	    /* Done this partition - close file & change flags accordingly. */
	    i = CLOSE_RELEASE;
	    if (hlink->hsl_level == 0)
		i = CLOSE_DESTROY;
	    status = qen_hash_halfclose(hbase, hpptr, i);
	    if (status != E_DB_OK) 
		return(status);
	    hpptr->hsp_flags &= ~(spillmask | candidate_mask | flushmask);
	    hpptr->hsp_flags |= inhashmask;
		
	}	/* end of loop to cram disk partitions into hash table */

    }	/* end of big loop */
    while (hbase->hsh_flags & HASH_NOHASHT);

    hbase->hsh_currows_left = 0;

    /* Flush any buffers on the hash table side that we didn't include
    ** in the hash table.
    */
    status = qen_hash_flush(dsh, hbase, FALSE);

/* qen_hash_verify(hbase); */
    if (dsh->dsh_hash_debug)
    {
	/* Print some stuff */
	TRdisplay ("%@ hjoin %s htbuild done, node: %d, t%d, level %d\n      Loaded %u rows total, using %d hash ptrs\n",
		reverse ? "probe" : "build",
		hbase->hsh_nodep->qen_num, dsh->dsh_threadno,
		hbase->hsh_currlink->hsl_level,hashrows,
		hbase->hsh_htsize);
	for (i = 0; i < hbase->hsh_pcount; i++)
	{
	    hpptr = &hbase->hsh_currlink->hsl_partition[i];
	    /* note, byte counts are meaningless here, just output rows */
	    TRdisplay ("t%d: P[%d]: b:%u  p:%u (rows), flags %v\n",
		dsh->dsh_threadno, i,
		hpptr->hsp_brcount,
		hpptr->hsp_prcount,
		"BSPILL,PSPILL,BINHASH,PINHASH,BFLUSHED,PFLUSHED,BALLSAME,PALLSAME,DONE,BCANDIDATE,PCANDIDATE",hpptr->hsp_flags);
	}
    }


    return(status);
}


/*{
** Name: QEN_HASH_INSERT	- insert row into hash table
**
** Description:
**	Rows of build source are simply inserted into correct bucket
**	chain of hash table.
**
** Inputs:
**	dsh		- Pointer to thread's DSH
**	hbase		- ptr to base hash structure
**	brptr		- Pointer to build row to insert
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	5-mar-99 (inkdo01)
**	    Written.
**	8-may-01 (inkdo01)
**	    Changed to sort on hashno, then join keys.
**	21-Nov-2006 (kschendel) SIR 122512
**	    Flag same-key entries.
**	23-Jun-2008 (kschendel) SIR 122512
**	    Don't need to pass in htable pointer, get from hbase
*/
 
static DB_STATUS
qen_hash_insert(
QEE_DSH		*dsh,
QEN_HASH_BASE	*hbase,
QEF_HASH_DUP	*brptr)

{
    u_i4	hashno;
    i4		hbucket, cmpres;
    QEF_HASH_DUP **htable;
    QEF_HASH_DUP *hchain, *hprev;


    /* Compute bucket number (from hash number). Then insert into 
    ** bucket's chain in sorted sequence. */

    htable = hbase->hsh_htable;
    hashno = brptr->hash_number;	/* load hash key from row */
    hbucket = hashno % hbase->hsh_htsize;
    brptr->hash_same_key = 0;		/* Usual case, set it now */
    if (htable[hbucket] == NULL)
    {
	/* if 1st row in bucket */
	htable[hbucket] = brptr;
	brptr->hash_next = (QEF_HASH_DUP *)NULL;
    }
    else
    {
	ADF_CB *adfcb = dsh->dsh_adf_cb;
	DB_CMP_LIST *keylist = hbase->hsh_nodep->node_qen.qen_hjoin.hjn_cmplist;

	/* Add row to list of dups in this bucket, but do it
	** in sorted sequence on join key.  This allows two
	** optimizations:
	** 1. when probing the hash chain, we can stop with
	** nomatch as soon as a larger join key is found in the chain;
	** 2. when multiple rows have identical join keys in the chain,
	** they are all together and probe-time knows how many there
	** are without redoing the join-key comparisons.
	*/
	adfcb->adf_errcb.ad_errcode = 0;
	hchain = htable[hbucket];
	hprev = NULL;
	while (hchain != NULL)
	{
	    cmpres = 1;
	    if (hchain->hash_number == hashno)
	    {
		/* Hash number match, now do key compares. */
		cmpres = adt_sortcmp(adfcb, keylist,
		    hbase->hsh_kcount, &(brptr->hash_key[0]), 
		    &(hchain->hash_key[0]), 0);
		if (adfcb->adf_errcb.ad_errcode != 0)
		{
		    dsh->dsh_error.err_code = adfcb->adf_errcb.ad_errcode;
		    return(E_DB_ERROR);
		}
	    }
	    else if (hchain->hash_number > hashno)
		cmpres = -1;
	    if (cmpres <= 0)	/* found our spot? */
	    {
		/* Keep track of key-duplicated entries.  As a special
		** case, if this is a (CO) join, throw the extra row
		** away.  (This only works if there's no post-join
		** qualification;  don't just check kuniq!)
		*/
		if (cmpres == 0)
		{
		    if (hbase->hsh_flags & HASH_CO_KEYONLY)
			return (E_DB_OK);
		    brptr->hash_same_key = 1;
		}
		brptr->hash_next = hchain;
		if (hprev) hprev->hash_next = brptr;
		else htable[hbucket] = brptr;
		return (E_DB_OK);
	    }
	    /* Skip to the next entry with a different hash join key.
	    ** i.e. skip all same-key entries, then one more (the last
	    ** one with that key).
	    */
	    do
	    {
		hprev = hchain;
		hchain = hchain->hash_next;
	    } while (hprev->hash_same_key);
	}

	if (hchain == NULL)
	{
	    /* We fell off chain - our guy must belong at end. */
	    brptr->hash_next = NULL;
	    if (hprev) hprev->hash_next = brptr;
	    else htable[hbucket] = brptr;
				/* else shouldn't happen */
	}
    }	/* end of hash table insertion */

    return(E_DB_OK);
}


/*{
** Name: QEN_HASH_RECURSE - recursively partitions a single partition
**			too large to fit the hash table
**
** Description:
**	Another partitioning recursion level is pushed onto the "link
**	stack" and the rows of the source partition are repartitioned.
**
** Inputs:
**	dsh		- Pointer to thread's DSH
**	hbase		- ptr to base hash structure
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	7-apr-99 (inkdo01)
**	    Written.
**	jan-00 (inkdo01)
**	    Changes for cartprod logic, bug fixes.
**	28-jan-00 (inkdo01)
**	    Tidy up hash memory management.
**	1-Jun-2005 (schka24)
**	    When recursing on a CO join, we must repartition the build
**	    side, not the smaller side.
**	    Copy partition bufs from previous level, not top.
**	31-may-07 (hayke02 for schka24)
**	    Split HCP_BOTH into HCP_IALLSAME and HCP_OALLSAME. This change
**	    fixes bug 118200.
**
*/
 
static DB_STATUS
qen_hash_recurse(
QEE_DSH		*dsh,
QEN_HASH_BASE	*hbase)
{

    QEN_HASH_LINK	*hlink = hbase->hsh_currlink;
    QEN_HASH_LINK	*prevlink;
    QEN_HASH_PART	*hpptr;
    i4			start;
    i4			i;
    QEF_CB		*qcb = dsh->dsh_qefcb;
    DB_STATUS		status;


    /* Locate next partition in current recursion level bigger than
    ** hash table. Build new recursion level (if not already there)
    ** open partition file and repartition its rows into the new 
    ** partition set. */

    start = (hlink->hsl_pno == -1) ? 0 : hlink->hsl_pno+1;
				/* start of partition search */

    for (i = start; i < hbase->hsh_pcount; i++)
    {
	if (hlink->hsl_partition[i].hsp_flags & (HSP_BSPILL | HSP_PSPILL))
	    break;
    }

    if (i >= hbase->hsh_pcount)
    {
	/* A recurseable partition wasn't found. This is an error. */
	TRdisplay("%@ qen_hash_recurse: no spilled partition found\n");
	dsh->dsh_error.err_code = E_QE0002_INTERNAL_ERROR;
	return(E_DB_ERROR);
    }

    /* This should be the partition we recurse on. First pick which
    ** (build source or probe source) to repartition, then add 
    ** and initialize new recursion level (if necessary). 
    **
    ** NOTE: both sources (build and probe) will be bigger 
    ** than the hash table capacity for this partition - hence 
    ** the decision as to which to repartition (the smaller, to
    ** increase the likelihood of a hashable repartitioning). */

    hlink->hsl_pno = i;		/* Store the partition to process */
    hpptr = &hlink->hsl_partition[i];

    /* First, check for one having all rows with same hashno. This
    ** is the ugly case which requires cross product joining on 
    ** the partition pair. */
    if ((hpptr->hsp_flags & HSP_BALLSAME && hpptr->hsp_brcount) ||
	(hpptr->hsp_flags & HSP_PALLSAME && hpptr->hsp_prcount) )
    {
	QEN_HASH_CARTPROD	*hcpptr;
	bool	leftas, rightas, leftjoin, rightjoin;

	hbase->hsh_flags |= HASH_CARTPROD;
	hbase->hsh_ccount++;

	/* Setup cross product structure to save stuff which would
	** otherwise clog up other hash join structures. If memory
	** has already been allocated for the structure, just reuse it. 
	** Otherwise, allocate the memory first. */

	if ((hcpptr = hbase->hsh_cartprod) == NULL)
	{
	    status = qen_hash_palloc(qcb, hbase,
			    sizeof(QEN_HASH_CARTPROD),
			    &hcpptr,
			    &dsh->dsh_error, "cartprod");
	    if (status != E_DB_OK)
		return (status);
	    hbase->hsh_cartprod = hcpptr;
	}
	hcpptr->hcp_flags = HCP_FIRST;
	hcpptr->hcp_boffset = hcpptr->hcp_poffset = 0;
	hcpptr->hcp_bcurrbn = hcpptr->hcp_pcurrbn = 0;
	/* Rather than allocate another read buffer, use this partition's
	** partition buffer for reading the other side of the cart-prod.
	** Set the read/write size to the smaller of the two sizes.
	*/
	hcpptr->hcp_rwsize = hbase->hsh_rbsize;
	if (hbase->hsh_csize < hbase->hsh_rbsize)
	    hcpptr->hcp_rwsize = hbase->hsh_csize;

	/* Both sides should be flushed if they are nonempty.  If one
	** side was nonempty and not flushed it would form a hash table.
	** If check-only (which prevents reversal), a flushco should have
	** been done.
	*/

	/* Determine which source is "outer" in the join, then assign
	** structure fields accordingly.
	** If it's a check-only join, make the probe side the outer
	** since, since it's easier to run the cart-prod special casing
	** that way.  If this is not an outer join (left, right, full),
	** and only one of the sources is ALLSAME, the non-ALLSAME
	** is outer (outer rows which don't match the ALLSAME hash key
	** can be skipped, reducing the number of passes of the
	** ALLSAME partition).  If both are ALLSAME and this is 
	** an outer join, the outer join outer becomes the cartprod
	** outer. Otherwise, the build source is the outer. 
	*/ 
	leftas = (hpptr->hsp_flags & HSP_BALLSAME) != 0;
	rightas = (hpptr->hsp_flags & HSP_PALLSAME) != 0;
	leftjoin = (hbase->hsh_flags & HASH_WANT_LEFT) != 0;
	rightjoin = (hbase->hsh_flags & HASH_WANT_RIGHT) != 0;

	if (hbase->hsh_nodep->node_qen.qen_hjoin.hjn_kuniq
	  || !rightas || (rightjoin && !leftjoin))
	{
	    /* These conditions lead to role reversal (build source 
	    ** is inner of the cartprod). This is different from the
	    ** role reversal recorded in hsh_flags, as it applies only 
	    ** to this partition. */
	    hcpptr->hcp_flags |= HCP_REVERSE;
	    if (rightas)
		hcpptr->hcp_flags |= HCP_OALLSAME;
	    if (leftas)
		hcpptr->hcp_flags |= HCP_IALLSAME;
	    hcpptr->hcp_bstarts_at = hpptr->hsp_pstarts_at;
	    hcpptr->hcp_pstarts_at = hpptr->hsp_bstarts_at;
	    hcpptr->hcp_browcnt = hpptr->hsp_prcount;
	    hcpptr->hcp_prowcnt = hpptr->hsp_brcount;
	    hcpptr->hcp_bbptr = hpptr->hsp_bptr;
	    hcpptr->hcp_pbptr = hbase->hsh_rbptr;
	}
	else
	{
	    /* Cartprod is of outer to inner. */
	    hcpptr->hcp_flags &= ~HCP_REVERSE;
	    if (rightas)
		hcpptr->hcp_flags |= HCP_IALLSAME;
	    if (leftas)
		hcpptr->hcp_flags |= HCP_OALLSAME;
	    hcpptr->hcp_bstarts_at = hpptr->hsp_bstarts_at;
	    hcpptr->hcp_pstarts_at = hpptr->hsp_pstarts_at;
	    hcpptr->hcp_browcnt = hpptr->hsp_brcount;
	    hcpptr->hcp_prowcnt = hpptr->hsp_prcount;
	    hcpptr->hcp_pbptr = hpptr->hsp_bptr;
	    hcpptr->hcp_bbptr = hbase->hsh_rbptr;
	}
	
	hcpptr->hcp_prowsleft = hcpptr->hcp_prowcnt;
	hcpptr->hcp_partition = hpptr;
	hcpptr->hcp_file = hpptr->hsp_file;
	return(E_DB_OK);
    }

    /* Repartition the smaller, except for CO join where we
    ** repartition the build side.
    */
    if (hbase->hsh_nodep->node_qen.qen_hjoin.hjn_kuniq
      || hpptr->hsp_bbytes <= hpptr->hsp_pbytes)
    {
	/* Repartition build source partition. */
	hbase->hsh_reverse = FALSE;
    }
    else
    {
	/* Repartition probe source partition & do role reversal. */
	hbase->hsh_reverse = TRUE;
    }

    /* We have a partition to recurse on. Now setup the link structure. */

    if (dsh->dsh_hash_debug)
    {
	TRdisplay("%@ qen_hash_recurse node: %d, t%d newlevel %d on pno %d\n",
		hbase->hsh_nodep->qen_num,
		dsh->dsh_threadno, hlink->hsl_level+1, hlink->hsl_pno);
    }
    if (hlink->hsl_next == NULL)
    {
	/* First visit to this recursion level. Allocate/format link
	** structure and corresponding partition descriptor array. */
	status = qen_hash_palloc(qcb, hbase,
		sizeof(QEN_HASH_LINK) + hbase->hsh_pcount * sizeof(QEN_HASH_PART),
		&hlink->hsl_next,
		&dsh->dsh_error, "partition_desc_array");
	if (status != E_DB_OK)
	    return (status);

	prevlink = hlink;
	hlink = hlink->hsl_next;	/* hook 'em together */
	hlink->hsl_prev = prevlink;
	hlink->hsl_next = (QEN_HASH_LINK *) NULL;
	hlink->hsl_level = prevlink->hsl_level + 1;
	hlink->hsl_partition = (QEN_HASH_PART *)&((char *)hlink)
						[sizeof(QEN_HASH_LINK)];
    }
    else
    {
	prevlink = hlink;
	hlink = hlink->hsl_next;	/* new link level */
    }

    /* hlink now points to the NEW recursion level */

    /* reset or initialize new level's partition descriptor array */
    for (i = 0; i < hbase->hsh_pcount; i++)
    {
	QEN_HASH_PART   *hpart = &hlink->hsl_partition[i];

	hpart->hsp_bptr = prevlink->hsl_partition[i].hsp_bptr;
	hpart->hsp_bvptr = prevlink->hsl_partition[i].hsp_bvptr;
	hpart->hsp_file = (QEN_HASH_FILE *) NULL;
	hpart->hsp_bbytes = 0;
	hpart->hsp_pbytes = 0;
	hpart->hsp_bstarts_at = 0;
	hpart->hsp_pstarts_at = 0;
	hpart->hsp_buf_page = 0;
	hpart->hsp_offset = 0;
	hpart->hsp_brcount = 0;
	hpart->hsp_prcount = 0;
	hpart->hsp_flags  = (HSP_BALLSAME | HSP_PALLSAME);
    }

    /* Record highest recursion level since reset */
    if (hlink->hsl_level > hbase->hsh_maxreclvl)
	hbase->hsh_maxreclvl = hlink->hsl_level;

    hlink->hsl_pno = -1;
    hlink->hsl_flags = 0;
    hbase->hsh_currlink = hlink;	/* tell the world */
    
    /* Finally, call qen_hash_repartition to read rows from source
    ** partition and repartition them into new recursion level. */

    status = qen_hash_repartition(dsh, hbase, hpptr);
    return(status);

}	/* end of qen_hash_recurse */


/*{
** Name: QEN_HASH_REPARTITION - reads rows and repartitions from 
**			partition just subjected to recursion.
**
** Description:
**	With recursion structures in place, this function opens 
**	partition file of partition to be recursed on, reads its rows
**	and repartitions them to new level.
**
** Inputs:
**	dsh		- Pointer to thread's DSH
**	hbase		- ptr to base hash structure
**	hpptr		- ptr to partition structure of partition being
**			recursed  (in the parent level)
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	8-apr-99 (inkdo01)
**	    Written.
**	19-apr-02 (inkdo01)
**	    Compute oldojp more accurately to reflect non-OJ queries (107539).
**	10-Nov-2005 (schka24)
**	    Restate ojpartflag as filterflag.
**	27-Nov-2006 (kschendel)
**	    Call CRC hasher explicitly.
**	18-Jun-2008 (kschendel) SIR 122512
**	    Rework for variable row sizes.
**	31-Mar-2010 (smeke01 b123516
**	    Add BCANDIDATE/PCANDIDATE to trace displays.
**
*/
 
static DB_STATUS
qen_hash_repartition(
QEE_DSH		*dsh,
QEN_HASH_BASE	*hbase,
QEN_HASH_PART	*hpptr)
{

    QEN_HASH_LINK	*hlink = hbase->hsh_currlink;
    QEF_HASH_DUP	*rowptr;
    i4			offset, rowcount, i;
    i4			bytes_left;
    DB_STATUS		status;
    u_i4		hashno;
    bool		reverse = hbase->hsh_reverse;
    bool		filter, filterflag;

    ++ hbase->hsh_rpcount;		/* For stats */

    /* Set up size and count constants (remembering possible role
    ** reversal). Close/open partition file, read its rows and 
    ** repartition using qen_hash_partition. */

    if (dsh->dsh_hash_debug)
    {
	/* Print some stuff */
	i = hpptr - &hlink->hsl_prev->hsl_partition[0];
	TRdisplay ("%@ Repartition of %s node: %d, t%d pno %d into level %d\n",
		reverse ? "probe" : "build",
		hbase->hsh_nodep->qen_num,
		dsh->dsh_threadno, i, hlink->hsl_level);
	TRdisplay ("t%d: P[%d]: b:%lu (%u)  p:%lu (%u), flags %v\n",
		dsh->dsh_threadno, i,
		hpptr->hsp_bbytes, hpptr->hsp_brcount,
		hpptr->hsp_pbytes, hpptr->hsp_prcount,
		"BSPILL,PSPILL,BINHASH,PINHASH,BFLUSHED,PFLUSHED,BALLSAME,PALLSAME,DONE,BCANDIDATE,PCANDIDATE",hpptr->hsp_flags);
    }

    /* We can filter rows if we're loading the probe side and we aren't
    ** doing outer join on the side we're loading.  Loading the "probe"
    ** side here means that BOTH-LOADED is set in the link flags (the
    ** caller turns that on BEFORE calling repartition for the second
    ** side, when both sides of a partition have to be repartitioned).
    ** Filtering is excluded when outer-joining on this side, because
    ** we're not in a position to deal properly with the nomatch row.
    ** We'll consider oj a little below.
    */
    filter = (hlink->hsl_flags & HSL_BOTH_LOADED) != 0;

    /* Reset spill to start for the appropriate side */
    hpptr->hsp_file->hsf_currbn = reverse ? hpptr->hsp_pstarts_at
					  : hpptr->hsp_bstarts_at;
    rowptr = NULL;
    offset = hbase->hsh_rbsize;

    if (reverse)
    {
	rowcount = hpptr->hsp_prcount;
	if (hbase->hsh_flags & HASH_WANT_RIGHT) filter = FALSE;
    }
    else
    {
	rowcount = hpptr->hsp_brcount;
	if (hbase->hsh_flags & HASH_WANT_LEFT) filter = FALSE;
    }

    while (rowcount > 0)
    {
	/* Loop to read blocks of partition file, address individual
	** rows and partition them in new level. */

	bytes_left = hbase->hsh_rbsize - offset;
	rowptr = (QEF_HASH_DUP *)&((char *)hbase->hsh_rbptr)[offset];
	/* Make sure entire header is there before getting row size! */
	if (bytes_left >= QEF_HASH_DUP_SIZE
	  && bytes_left >= QEFH_GET_ROW_LENGTH_MACRO(rowptr))
	{
	    /* We have a whole row in block. Just update offset and
	    **  fall through to partition call. */
	    offset += QEFH_GET_ROW_LENGTH_MACRO(rowptr);
	}
	else
	{
	    /* Read new bufferful, possibly construct split row in workrow */
	    hpptr->hsp_offset = offset;	/* Where readrow expects it */
	    status = qen_hash_readrow(dsh, hbase, hpptr, &rowptr);
	    if (status != E_DB_OK)
		return (status);
	    offset = hpptr->hsp_offset;	/* Retrieve updated offset */
	}

	/* Copy old hashno, rehash it and repartition row. */
	hashno = rowptr->hash_number;
	rowptr->hash_number = -1;
	HSH_CRC32((char *)&hashno, sizeof(u_i4), &rowptr->hash_number);

	filterflag = filter;		/* copy (hash_part changes it) */
	status = qen_hash_partition(dsh, hbase, rowptr, &filterflag, FALSE);
	if (status != E_DB_OK) 
			return(status);
	rowcount--;
    }	/* end of partitioning loop */

    /* Close file and add to free chain.  Remember that hlink is for
    ** the new level - drop file if it's at the top level.
    */
    i = CLOSE_RELEASE;
    if (hlink->hsl_level <= 1)
	i = CLOSE_DESTROY;
    status = qen_hash_halfclose(hbase, hpptr, i);
    return(status);
	
}	/* end of qen_hash_repartition */


/*{
** Name: QEN_HASH_PROBE1	- phase I of probe
**
** Description:
**	If the original build source scan resulted in a hash table, rows
**	from original probe source are sent to qen_hash_probe to attempt
**	hash joins. If the original build partitions were all too big to
**	generate a hash table, the probe rows are sent directly to
**	qen_hash_partition for storage in the partition buffers (or
**	probe spill files).  With luck, after all probes are read,
**	we may find that one or more probe partitions are small enough to
**	create a hash table (resulting in an immediate role reversal).
**	That isn't our problem here, though.
**
**	At this point we're working with rows from the real inner, can't
**	be in role reversal.  (REVERSE can be set if the driver knows
**	that there's no hash table.)
**
**	If the row is stored into a partition buffer, it may get compressed.
**	If we return with a "gotone" indication from probe1, the probe row
**	cannot have been compressed (although the matching build row,
**	if any, may have been compressed).
**
** Inputs:
**	dsh		- Pointer to thread's DSH
**	hbase		- ptr to base hash structure
**	buildptr	- ptr to addr of build row buffer (to return row)
**	probeptr	- addr of probe row
**	ojmask		- ptr to mask word for OJs
**	gotone		- addr of flag set to indicate successful join
**
** Outputs:
**	*buildptr	- addr of matching build row returned here if any
**	*ojmask		- OJ-RIGHT flag set if right-OJ on this probe row
**	*gotone		- set TRUE if found build row, or right-OJ and probe
**			  didn't/can't match anything
**
** Side Effects:
**
** History:
**	17-mar-99 (inkdo01)
**	    Written.
**	jan-00 (inkdo01)
**	    Changes to oj handling.
**	29-aug-04 (inkdo01)
**	    Support global base array to reduce build row movement.
**	22-Dec-08 (kibro01) b121373
**	    Unset ojmask so we don't get erroneous full outer join results
*/
 
DB_STATUS
qen_hash_probe1(
QEE_DSH		*dsh,
QEN_HASH_BASE	*hbase,
QEF_HASH_DUP	**buildptr,
QEF_HASH_DUP	*probeptr,
QEN_OJMASK	*ojmask,
bool		*gotone)

{

    DB_STATUS	status;
    bool	filterflag;
    bool	try_compress;

    /* If build phase didn't create a hash table (all partitions were
    ** too big), just load probe rows directly into partitions, too. 
    ** Maybe we can make a hash table from probe source (immediate role
    ** reversal). Otherwise, call qen_hash_probe to probe for joins. */

    *gotone = FALSE;			/* init return flag */
    *ojmask = 0;
    try_compress = (hbase->hsh_flags & HASH_PROBE_RLL) != 0;

    if (hbase->hsh_flags & HASH_NOHASHT)
    {
	/* Driver already set REVERSE to direct rows into the probe
	** side of the partitions.
	*/
	filterflag = TRUE;
	status = qen_hash_partition(dsh, hbase, probeptr,
			&filterflag, try_compress);
	/* Just return unless row filtered and we want right OJ */
	if (status == E_DB_OK
	  && filterflag && hbase->hsh_flags & HASH_WANT_RIGHT)
	{
	    /* This is outer join and current row has no match in the other 
	    ** partition (because the bit filter is 0 or there are no rows
	    ** in the partition).  Note that the row cannot have been
	    ** compressed in this situation.
	    */
	    *ojmask |= OJ_HAVE_RIGHT;
	    *gotone = TRUE;
	}
	return(status);
    }
    else
    {
	return(qen_hash_probe(dsh, hbase, buildptr, probeptr,
				ojmask, gotone, try_compress));
    }

}


/*{
** Name: QEN_HASH_PROBE		- probe hash table for join
**
** Description:
**	Rows of current probe source (either actual build or actual probe
**	source, depending on current state of join) are targeted to hash 
**	table (those probe rows corresponding to the partitions which make 
**	up the hash table, that is). Probe rows targeting disk-resident 
**	partitions are themselves written to disk for phase II. If all 
**	partitions generated by preceding build phase of other source were 
**	too big to create a hash table, the probe rows are sent to 
**	qen_hash_partition in hope that one or more probe partitions 
**	are small enough to generate a hash table, resulting in an immediate 
**	role reversal. Otherwise, a recursion is done to break down the
**	build/probe partitions.
**
** Inputs:
**	dsh		- Pointer to thread's DSH
**	hbase		- ptr to base hash structure
**	buildptr	- ptr to addr of build row buffer (to return row)
**	probeptr	- addr of probe row
**	ojmask		- ptr to mask word for OJs
**	gotone		- addr of flag set to indicate successful join
**	try_compress	- TRUE if we should attempt RLL-compression (only
**			  if row is saved to a partition buffer)
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	17-mar-99 (inkdo01)
**	    Written.
**	7-may-01 (inkdo01)
**	    Support hash chains sorted first on hashno, then on join keys.
**	22-aug-01 (inkdo01)
**	    Added code to precheck for null join cols (to avoid "null = null"
**	    joins).
**	7-sep-01 (inkdo01)
**	    Set HASH_PROBE_STORED flag for probe rows partitioned because their
**	    build rows are on disk - avoids premature OJ logic.
**	23-aug-01 (devjo01)
**	    Change probekey calculation to be friendly to 64bit platforms.
**	29-aug-04 (inkdo01)
**	    Support global base array to reduce build row movement.
**	6-apr-05 (inkdo01)
**	    Add hsh_prevpr to test OJ's.
**	10-Nov-2005 (schka24)
**	    fix browsz type, rows can be larger then u_i2 now.
**	    Remove prevpr, use NOJOIN flag to test probe OJ-ness.
**	23-Dec-2005 (kschendel)
**	    Check hash before chasing keys when using firstbr optimization.
**	21-Nov-2006 (kschendel) SIR 122512
**	    Use knowledge of rows-with-same-key gained during insert to
**	    avoid additional key comparisons here.
*/
 
static DB_STATUS
qen_hash_probe(
QEE_DSH		*dsh,
QEN_HASH_BASE	*hbase,
QEF_HASH_DUP	**buildptr,
QEF_HASH_DUP	*probeptr,
QEN_OJMASK	*ojmask,
bool		*gotone,
bool		try_compress)

{

    QEN_HASH_PART	*hpptr;
    QEN_HASH_LINK	*hlink = hbase->hsh_currlink;
    i4			hbucket, pno;
    u_i4		hashno;
    i4			inhashmask;
    DB_STATUS		status;
    QEN_OJMASK		joinflag = hbase->hsh_pojmask;
    bool		reverse = hbase->hsh_reverse;
    bool		filterflag;
    QEF_HASH_DUP	*hchain;
    QEF_HASH_DUP	*hnext;

    *gotone = FALSE;			/* init to no return yet */

    /* If previous call returned a matched pair of rows, AND if it
    ** saw that more rows have the same join key, the immediately
    ** following build row will join too and we don't need to compare.
    */

    if (hbase->hsh_flags & HASH_PROBE_MATCH)
    {
	QEF_HASH_DUP    *nextrow = hbase->hsh_nextbr;

	/* Got a match - copy to build row buffer & set up next
	** row in chain if there are more rows with the same key.
	*/
	*gotone = TRUE;
	hnext = NULL;
	if (nextrow->hash_same_key)
	    hnext = nextrow->hash_next;
	hbase->hsh_nextbr = hnext;
	if (hnext == NULL)
	    hbase->hsh_flags &= ~HASH_PROBE_MATCH;

	*buildptr = nextrow;	/* save matched row addr */

	return(E_DB_OK);
    }

    /* We get here if this is a new probe row. Compute partition it 
    ** hashes to. If partition is in hash table, compute hash bucket
    ** and search chain. */

    inhashmask = HSP_BINHASH;
    if (reverse)
	inhashmask = HSP_PINHASH;

    /* Get probe row hash, figure out probe partition */
    hashno = probeptr->hash_number;
    pno = hashno % hbase->hsh_pcount;
    hpptr = &hlink->hsl_partition[pno]; /* target partition */

    /* If target partition is in current hash table, locate the bucket
    ** and proceed from there. Actually, first we check if current 
    ** probe row has same key as the last hash table hit. If not, we
    ** look for the bucket. */
    if (hpptr->hsp_flags & inhashmask)
    {
	ADF_CB		*adfcb = dsh->dsh_adf_cb;
	char		*probekey;
	DB_CMP_LIST	*keylist = hbase->hsh_nodep->node_qen.qen_hjoin.hjn_cmplist;
	i4		cmpres;
	QEF_HASH_DUP    *firstrow = hbase->hsh_firstbr;
	QEF_HASH_DUP	*hprev;

	adfcb->adf_errcb.ad_errcode = 0;
	probekey = &probeptr->hash_key[0];

	if (firstrow && hashno == firstrow->hash_number)
	{
	    /* Check if current has same key as last one. */
	    if (hbase->hsh_flags & HASH_CHECKNULL &&
		    qen_hash_checknull(hbase, probekey))
		cmpres = 1;		/* can't match */
	    else cmpres = adt_sortcmp(adfcb, keylist, hbase->hsh_kcount,
			probekey, &firstrow->hash_key[0], 0);
	    if (adfcb->adf_errcb.ad_errcode != 0)
	    {
		dsh->dsh_error.err_code = adfcb->adf_errcb.ad_errcode;
		return(E_DB_ERROR);
	    }

	    if (cmpres == 0)
	    {
		*gotone = TRUE;
		hchain = firstrow;
	    }
	}

	/* If previous logic didn't find one, compute hash bucket and
	** search the chain. */

	if (!(*gotone))
	{
	    hbucket = hashno % hbase->hsh_htsize;
	    hchain = hbase->hsh_htable[hbucket];
	    while (hchain != NULL)
	    {
		if (hchain->hash_number > hashno)
		    break;		/* No match */
		if (hchain->hash_number == hashno)
		{
		    /* Matched hash number, now compare join keys. */
		    if (hbase->hsh_flags & HASH_CHECKNULL &&
			    qen_hash_checknull(hbase, probekey))
			cmpres = 1;
		    else cmpres = adt_sortcmp(adfcb, keylist, 
			    hbase->hsh_kcount, &hchain->hash_key[0], 
			    probekey, 0);
		    if (adfcb->adf_errcb.ad_errcode != 0)
		    {
			dsh->dsh_error.err_code = adfcb->adf_errcb.ad_errcode;
			return(E_DB_ERROR);
		    }
		    if (cmpres == 0)
		    {			/* got match */
			*gotone = TRUE;
			hbase->hsh_firstbr = hchain;  /* save for next call */
			break;
		    }
		    else if (cmpres > 0)
			break;		/* no match */
		    /* else cmpres < 0, fall thru */
		}
		/* row hashno < this hashno, or row key < this key;
		** Skip to the next entry with a different hash join key.
		** i.e. skip all same-key entries, then one more (the last
		** one with that key).
		*/
		do
		{
		    hprev = hchain;
		    hchain = hchain->hash_next;
		} while (hprev->hash_same_key);
	    }	/* end of bucket chain loop */
	}

	if (! (*gotone) )
	{
	    /* Check for possible probe-side outer join. */
	    if (joinflag != 0)
	    {
		*ojmask |= joinflag;
		*gotone = TRUE;
	    }
	}
	else
	{
	    /* Got a match - copy to build row buffer & set up next
	    ** row in chain if there are more rows with the same key.
	    */
	    if (hchain->hash_same_key)
	    {
		hbase->hsh_nextbr = hchain->hash_next;
		hbase->hsh_flags |= HASH_PROBE_MATCH;
	    }
	    else
	    {
		hbase->hsh_nextbr = NULL;
		hbase->hsh_flags &= ~HASH_PROBE_MATCH;
	    }
	    *buildptr = hchain;	/* return matched row addr */
	}
	return(E_DB_OK);

    }	/* end of probe partition in hash table */

    else
    {
	/* Probe row hashes to build partition on disk. Simply call 
	** qen_hash_partition to repartition it. 
	** Note that the role reversal logic reverses for this 
	** partition call, since we're partitioning a probing
	** row.  hash-partition understands REVERSE in an absolute
	** sense;  so if we're role reversed, our "probe" row is really
	** a build side row and we'd better unreverse so that partition
	** sends it to the build side!
	*/
	hbase->hsh_reverse = ! hbase->hsh_reverse;
	filterflag = TRUE;
	status = qen_hash_partition(dsh, hbase, probeptr,
			&filterflag, try_compress);
	hbase->hsh_reverse = ! hbase->hsh_reverse;

	/* See if row had no match in build partition - 0 rows or
	** bit filter is off. If so, check for OJ, else row was dropped. */
	if (status == E_DB_OK && filterflag && joinflag)
	{
	    *ojmask |= joinflag;
	    *gotone = TRUE;
	}
	return(status);
    }

}


/*{
** Name: QEN_HASH_PROBE2	- phase II of probe
**
** Description:
**	This routine oversees the processing of joins not performed
**	during the phase I probe (as rows are actually materialized 
**	from the probe source). It coordinates the building of hash
**	tables from partitions not yet processed (either build or probe
**	partitions). And if no remaining partitions are small enough
**	to fit a hash table, it drives recursive partitioning until
**	they are small enough.
**
** Inputs:
**	dsh		- Pointer to thread's DSH
**	hbase		- ptr to base hash structure
**	buildptr	- ptr to addr of build row buffer (to return row)
**	probeptr	- ptr to addr of probe row (to return row)
**	ojmask		- ptr to returned mask word for OJs
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	30-mar-99 (inkdo01)
**	    Written.
**	6-dec-99 (inkdo01)
**	    Added logic for recursion processing.
**	jan-00 (inkdo01)
**	    Changes for cartprod logic, various bug fixes.
**	2-feb-00 (inkdo01)
**	    Logic for first non-OJ call to probe2, to tidy up phase1 pass.
**	9-aug-02 (inkdo01)
**	    Added "indptr" local to pass the hash table row indicator address to 
**	    qen_hash_probe (as determined by reverse flag).
**	27-june-03 (inkdo01)
**	    Dropped "*ojmask == 0" test at loop's end - it prevented OJ
**	    processing on hash table rows (bug 110499).
**	29-aug-04 (inkdo01)
**	    Support global base array to reduce build/probe  row movement.
**	8-Jun-2005 (schka24)
**	    "End" partitions that have no probe rows if between phases
**	    1 and 2 (non-OJ only).
**	17-Jun-2005 (schka24)
**	    Fix read-too-far when a partition ends exactly at the end of
**	    a block.
**	14-Nov-2005 (schka24)
**	    Tidy up indptr stuff, set NOJOIN when fetching a new probe.
**	28-nov-05 (inkdo01)
**	    Fix bug handling outer joins in which inner side is build source
**	    and some partitions have no build rows, but some probe rows 
**	    (which are immediately OJ candidates).
**	6-dec-05 (inkdo01)
**	    Quick fix to cover an edge case of above change.
**	15-May-2007 (kschendel)
**	    Repair allojs logic:  set it each time in, as we process the
**	    all-oj probes, else it wastes time in probing;  and don't let the
**	    return logic kick in until we've actually fetched a probing row.
**	23-Aug-2007 (kschendel)
**	    Fix the last bug in hash join again.  :)  ojset was being used
**	    in two different senses: build OJ in the build-oj's-flush loop,
**	    or probe OJ in the main loop when the partitioning bit-filter
**	    guarantees a probing nomatch.  Unfortunately it was only set
**	    for the former sense, causing bogus OJ returns from the latter.
**	    The OQUAL or JQUAL in the caller would usually catch the bad
**	    oj and throw it out, but not always.
**	    Break up ojset so that it's clear which OJ is meant.
**	    (Note that this only happens in "recurse" mode and then only if
**	    some recursed partitions fit and some didn't;  very hard to
**	    reproduce!)
**	18-Jun-2008 (kschendel) SIR 122512
**	    Rework for variable row sizes.
**	30-Jun-2008 (kschendel) SIR 122512
**	    Minor code bumming for small speed and clarity improvements.
**	    Add a "fast path" for the most common case of probing from a
**	    current-level partition.
**	24-Sep-2008 (inkdo01) b120656
**	    Initialise pojset on each loop to avoid erroneous hash join results
**	22-Dec-08 (kibro01) b121373
**	    Unset ojmask so we don't get erroneous full outer join results
**	24-Jun-10 (smeke01) b123968
**	    Move the initialisation of allojs to the top of the main for-loop.
*/
 
DB_STATUS
qen_hash_probe2(
QEE_DSH		*dsh,
QEN_HASH_BASE	*hbase,
QEF_HASH_DUP	**buildptr,
QEF_HASH_DUP	**probeptr,
QEN_OJMASK	*ojmask)

{

    QEN_HASH_LINK  *hlink = hbase->hsh_currlink;
    QEN_HASH_PART  *hpptr;
    QEF_HASH_DUP *brptr;
    QEF_HASH_DUP *rowptr;
    QEF_HASH_DUP **buildp, **probep;
    DB_STATUS	status = E_DB_OK;
    i4		size, i;
    i4		inhashmask;
    u_i4	hashno;
    QEN_OJMASK	pojset;
    bool	recurse;
    bool	newrow;
    bool	reverse = hbase->hsh_reverse;
    bool	gotone = FALSE;
    bool	filterflag, allojs;


    /* Main function of this routine is to read rows from a disk
    ** resident partition and probe a hash table with them. If there
    ** is no hash, it calls qen_hash_build to make one. Rows from
    ** probe partitions which match partitions loaded into hash table
    ** are read in turn for the join process. */

    /* Big loop forces rebuild of hash table, if necessary, before 
    ** partitions are read to be joined. Returns are done from inside 
    ** loop when joins are made. */

    *ojmask = 0;
    if (hbase->hsh_flags & HASH_FAST_PROBE2)
    {
	/* The fast-path loop is: fetch a new row if we don't have one
	** already, do a probe, if gotone return, and keep looping until
	** probe says gotone or we run out of rows in the partition.
	** If we run out, just fall through, and all of the standard logic
	** will kick in; it will try to get another row, see that it can't,
	** and "end" the partition in whatever way is appropriate.
	**
	** The fastpath flag is only on for vanilla non-cartprod,
	** non-allojs, non-recurse, non-build-oj partitions.
	** It's meant to be a more or less temporary hack until the entire
	** probe2 process is rearranged to be a bit more orderly, if that
	** is possible.
	*/
	hpptr = &hlink->hsl_partition[hbase->hsh_currpart];
	if (reverse)	/* encapsulate build/probe sources */
	{
	    buildp = probeptr;
	    probep = buildptr;
	}
	else
	{
	    buildp = buildptr;
	    probep = probeptr;
	}
	for (;;)
	{
	    if ((hbase->hsh_flags & HASH_PROBE_MATCH) == 0)
	    {
		if (--hbase->hsh_currows_left < 0)
		    break;
		/* Rows left in current partition. Get the next. */

		/* Copy the bits of the probe row. */
		rowptr = (QEF_HASH_DUP *) (&hbase->hsh_rbptr[hpptr->hsp_offset]);
		size = hbase->hsh_rbsize - hpptr->hsp_offset;
		if (size >= QEF_HASH_DUP_SIZE
		  && size >= QEFH_GET_ROW_LENGTH_MACRO(rowptr))
		{
		    /* Whole row in buffer - just copy ptr. */
		    *probep = rowptr;
		    hpptr->hsp_offset += QEFH_GET_ROW_LENGTH_MACRO(rowptr);
		}
		else
		{
		    status = qen_hash_readrow(dsh, hbase, hpptr, probep);
		    if (status != E_DB_OK)
			return (status);
		}
		hbase->hsh_flags |= HASH_PROBE_NOJOIN;
	    }
	    /* Probe to see what's up. */
	    status = qen_hash_probe(dsh, hbase, buildp, *probep, 
					ojmask, &gotone, FALSE);
	    if (status != E_DB_OK) 
		return(status);
	    if (gotone) return(E_DB_OK);
	}
	/* If we fall out of the loop, the partition has no more probes,
	** turn off the fast flag and creep through the standard logic
	** which will eventually "end" this partition and move on.
	*/
	hbase->hsh_flags &= ~HASH_FAST_PROBE2;
    }

    /* Some additional setup that wasn't needed for the fastpath... */

    recurse = (hlink->hsl_level > 0 &&
		(hlink->hsl_flags & HSL_BOTH_LOADED) == 0);
    for ( ; ; )
    {
   	allojs = FALSE;

	/* Before executing the big loop, check if we've just finished 
	** all the probe1 calls (and subsequent left join calls). If so,
	** execute logic to flush probe rows which split to partitions not 
	** in the phase 1 hash table (if there was one).
	** If we just finished probe1, but need left joins, outer loops
	** will whirl until all the left-OJ rows are sucked out,
	** then we'll do this bit.
	*/
	if (hbase->hsh_flags & HASH_BETWEEN_P1P2 && 
	    !(hbase->hsh_flags & HASH_BUILD_OJS))
	{
	    bool	flush = FALSE;

	    hbase->hsh_flags &= ~HASH_BETWEEN_P1P2;
	    hlink->hsl_flags |= HSL_BOTH_LOADED;

	    /* Loop over partitions - "end"ing those in hash table. 
	    ** (Left OJ for in-hash partitions are all done now.)
	    ** Also "end" not-in-hash partitions with no probe rows,
	    ** unless we need them for left OJ.
	    ** If there are any nonempty non-ended probe partitions,
	    ** we'll need to flush them if they spilled.
	    */
	    for (i = 0; i < hbase->hsh_pcount; i++)
	    {
		hpptr = &hlink->hsl_partition[i];
		if ((hpptr->hsp_flags & HSP_BINHASH) || 
		    (hpptr->hsp_prcount == 0 &&
		    !(hbase->hsh_flags & HASH_WANT_LEFT)))
		{
		    status = qen_hash_close(hbase, &hpptr->hsp_file, CLOSE_DESTROY);
		    if (status != E_DB_OK) 
			return(status);
		    hpptr->hsp_flags = HSP_DONE;
		}
		else if (hpptr->hsp_prcount > 0) flush = TRUE;
	    }

	    if (flush) 
	    {
		/* Flush probes, that's what was in the buffers */
		hbase->hsh_reverse = TRUE;
		status = qen_hash_flush(dsh, hbase, FALSE);
		if (status != E_DB_OK) 
		    return(status);
		reverse = FALSE;
		hbase->hsh_reverse = FALSE;
	    }
	}

	newrow = FALSE;
	if (hbase->hsh_flags & HASH_NOHASHT && 
			!(hbase->hsh_flags & HASH_CARTPROD))
	{
	    status = qen_hash_build(dsh, hbase);
	    if (status != E_DB_OK) 
		return(status);		/* Might be normal "done" exit */
	    /* Set stuff to fall thru mainline and set up first partition
	    ** after standard hash table build.  (recursion special case
	    ** may change this.)
	    */
	    hbase->hsh_currows_left = -1;
	    hbase->hsh_currpart = -1;
	    /* Hash build could have caused role reversal, or recursion */
	    reverse = hbase->hsh_reverse;
	    hlink = hbase->hsh_currlink;	/* refresh, in case of
						** recursion */

	    /* Reset probing OJ mask for this hash table build */
	    hbase->hsh_pojmask = 0;
	    if (reverse && hbase->hsh_flags & HASH_WANT_LEFT)
		hbase->hsh_pojmask = OJ_HAVE_LEFT;
	    else if (!reverse && hbase->hsh_flags & HASH_WANT_RIGHT)
		hbase->hsh_pojmask = OJ_HAVE_RIGHT;

	    /* See if hash build caused recursion. If so, must prepare
	    ** to read rows of corresponding partition in other source.
	    ** Its rows probe hash table when they rehash to partitions
	    ** in table, and rest are repartitioned to disk.
	    **
	    ** This logic is only applied if one source was repartitioned
	    ** in the recursion. If both sources were repartitioned 
	    ** (because the first repartitioned source was still too large
	    ** to create a hash table), regular probe2 logic is applied. */

	    recurse = FALSE;
	    if (hlink->hsl_level > 0 && !(hlink->hsl_flags & HSL_BOTH_LOADED))
	    {
		hlink = hbase->hsh_currlink;
		recurse = TRUE;
		hpptr = &hlink->hsl_prev->hsl_partition
			[hlink->hsl_prev->hsl_pno];
				/* partition to recurse */

		/* Reset to read the OTHER side of the prev level
		** partition, we've repartitioned THIS side.
		*/
		hpptr->hsp_file->hsf_currbn = reverse ? hpptr->hsp_bstarts_at
						      : hpptr->hsp_pstarts_at;

		hpptr->hsp_offset = hbase->hsh_rbsize;
		hbase->hsh_currows_left = (reverse) ? hpptr->hsp_brcount :
				hpptr->hsp_prcount;
	    }
	}
/* qen_hash_verify(hbase); */

	/* If we're recursing, set up repartitioning of recurse partition. */
	hpptr = NULL;		/* if newly built hash table */
	if (recurse)
	{
	    hpptr = &hlink->hsl_prev->hsl_partition
		[hlink->hsl_prev->hsl_pno];
				/* partition to recurse */
	}
	else if (hbase->hsh_currpart >= 0)
	    hpptr = &hlink->hsl_partition[hbase->hsh_currpart];

	/* Check for role reversal and set appropriate flags. */

	pojset = hbase->hsh_pojmask;
	if (reverse)	/* encapsulate build/probe sources */
	{
	    buildp = probeptr;
	    probep = buildptr;
	    inhashmask = HSP_PINHASH;
	    if (!recurse && hbase->hsh_flags & HASH_WANT_LEFT
	      && hpptr != NULL && hpptr->hsp_prcount == 0)
		allojs = TRUE;
	}
	else
	{
	    buildp = buildptr;
	    probep = probeptr;
	    inhashmask = HSP_BINHASH;
	    if (!recurse && hbase->hsh_flags & HASH_WANT_RIGHT
	      && hpptr != NULL && hpptr->hsp_brcount == 0)
		allojs = TRUE;
	}

	/* We have a hash table. Enter loop to produce a join. */
	while (!gotone)
	{
	    /* If we're doing a "cartprod" partition, call special func. and
	    ** get out of the way!  Let it handle all OJ testing, etc.
	    */
	    if (hbase->hsh_flags & HASH_CARTPROD)
	    {
		status = qen_hash_cartprod(dsh, hbase, buildptr, probeptr,
				ojmask, &gotone);
		if (status == E_DB_OK && !gotone) break;
					/* cartprod is done, next hash build */
		return(status);
	    }

	    /* See if we're in the midst of producing outer 
	    ** joins from previous hash table. */
	    if (hbase->hsh_flags & HASH_BUILD_OJS)
	    {
		QEN_OJMASK bojset;

		/* Loop over all hash buckets, then all rows within
		** each bucket, looking for rows with "0" in last 
		** byte (indicator byte). They're the oj candidates.
		**
		** First determine whether we're looking for left
		** or right joins (which depends on what source 
		** is in the hash table).  */
		if (reverse) 
		{
		    if (!(hbase->hsh_flags & HASH_WANT_RIGHT)) break;
		    bojset = OJ_HAVE_RIGHT;
		}
		else 
		{
		    if (!(hbase->hsh_flags & HASH_WANT_LEFT)) break;
		    bojset = OJ_HAVE_LEFT;
		}

		if (hbase->hsh_currbucket == -1)
		{
		    /* First call for this hash table. Set up 
		    ** following loops. */
		    hbase->hsh_currbucket = 0;
		    hbase->hsh_nextbr = NULL;
		}

		for (i = hbase->hsh_currbucket; i < hbase->hsh_htsize; i++)
		{
		    if (hbase->hsh_nextbr == NULL) 
			hbase->hsh_nextbr = hbase->hsh_htable[i];
		    else
			hbase->hsh_nextbr = hbase->hsh_nextbr->hash_next;
		    for (brptr = hbase->hsh_nextbr;
			brptr != NULL; brptr = brptr->hash_next)
		    {
			if (brptr->hash_oj_ind == 0)
			{
			    /* This is an outer join candidate. */
			    *buildp = brptr;
			    (*ojmask) |= bojset;
			    hbase->hsh_nextbr = brptr;
			    hbase->hsh_currbucket = i;
					/* note where we left off */
			    return(E_DB_OK);
			}
		    }
		    hbase->hsh_nextbr = NULL;
		}
		if (i == hbase->hsh_htsize) break;
				/* no more oj candidates - leave loop */
	    }

	    /* If last trip thru produced a fresh probe row, init the
	    ** probe-side OJ indicator flag.
	    */
	    if (newrow)
	    {
		hbase->hsh_flags |= HASH_PROBE_NOJOIN;

		/* allojs indicates a probe row from a partition with no rows 
		** in the hash table and an outer join candidate. We don't
		** need to call qen_hash_probe(). */
		if (allojs)
		{
		    *ojmask |= pojset;
		    gotone = TRUE;
		    return(E_DB_OK);
		}
	    }

	    if (hbase->hsh_flags & HASH_PROBE_MATCH || newrow)
	    {
		status = qen_hash_probe(dsh, hbase, buildp, *probep, 
					ojmask, &gotone, FALSE);
		if (status != E_DB_OK) 
				return(status);
		if (gotone) return(E_DB_OK);
		newrow = FALSE;
	    }

	    /* Dropping to here means a new probe row is required
	    ** from the disk partition. Copy to probep and loop. */

	    /* If we are in a recursion level, the probe row is rehashed
	    ** in new level first. If probes to a partition in the current
	    ** hash table, process as normal. Else, repartition the row and 
	    ** loop back for another. */

	    if (recurse)
	    {
		while (hbase->hsh_currows_left > 0)
		{
		    QEF_HASH_DUP *target;

/* qen_hash_verify(hbase); */
		    /* Following code extracts next row from selected disk 
		    ** partition and is identical to the code in the
		    ** non-recursion logic which follows. */
		    /* Copy the bits of the probe row. */
		    rowptr = (QEF_HASH_DUP *) (&hbase->hsh_rbptr[hpptr->hsp_offset]);
		    size = hbase->hsh_rbsize - hpptr->hsp_offset;
		    if (size >= QEF_HASH_DUP_SIZE
		      && size >= QEFH_GET_ROW_LENGTH_MACRO(rowptr))
		    {
			/* Whole row in buffer - just copy ptr. */
			*probep = target = rowptr;
			hpptr->hsp_offset += QEFH_GET_ROW_LENGTH_MACRO(rowptr);
		    }
		    else
		    {
			status = qen_hash_readrow(dsh, hbase, hpptr, probep);
			if (status != E_DB_OK)
			    return (status);
			target = *probep;
		    }

/* qen_hash_verify(hbase); */
		    /* Got a row - rehash to determine partition in new 
		    * recursion level. */
		    hbase->hsh_currows_left--;	/* one less row */
		    hashno = target->hash_number;
		    target->hash_number = -1;
		    HSH_CRC32((char *) &hashno, sizeof(u_i4),
				&target->hash_number);

		    hashno = target->hash_number;	/* new hash number */
		    if (hlink->hsl_partition
			[hashno % hbase->hsh_pcount].hsp_flags & inhashmask)
		    {
			/* This row hashes to the current hash table. */
			newrow = TRUE;
			break;
		    }

		    /* New hashno splits to a disk partition - repartition it.
		    ** Note that the role reversal logic reverses for this 
		    ** partition call, since we're partitioning a probing
		    ** row. */
		    hbase->hsh_reverse = ! hbase->hsh_reverse;
		    filterflag = TRUE;
		    status = qen_hash_partition(dsh, hbase, 
						target, &filterflag, FALSE);
		    hbase->hsh_reverse = ! hbase->hsh_reverse;
		    if (pojset && filterflag && status == E_DB_OK) 
		    {
			/* This is outer join and current row has no match in
			** the other partition (because the bit filter is 0 or
			** there are no rows in the partition). */
			*ojmask |= pojset;
			return(E_DB_OK);
		    }

		    if (status != E_DB_OK) 
			return(status);
		}	/* end of while currows_left (recurse) */

		if (newrow) continue;		/* got a row - loop to probe */

		/* We get here when all probing rows in the recursed partition
		** have been processed.  Break out of the !gotone loop  
		** to mark partitions that participated in this hash
		** table, do hash-table OJ's, and process any rehashed
		** partitions that didn't participate this time around.
		**
		** If we spilled at the new level, it's now loaded on
		** both sides.  (or, perhaps we're done, in which case
		** the next hash-table-build call will exit the new level.)
		*/
		hlink->hsl_flags |= HSL_BOTH_LOADED;
		break;		/* from gotone loop, recurse still on */
	    } /* if recurse */

	    /* Regular join with no recursion - get the next probe row.
	    ** In the typical case this only happens for the first attempt
	    ** at extracting a probe.  Once we successfully extract and
	    ** deliver a row, the fast-path at the top takes over.
	    ** (the old style loop remains in use if allojs, too.)
	    */

	    if (--hbase->hsh_currows_left >= 0)
	    {
		/* Rows left in current partition. Get the next. */
		newrow = TRUE;

		/* Copy the bits of the probe row. */
		rowptr = (QEF_HASH_DUP *) (&hbase->hsh_rbptr[hpptr->hsp_offset]);
		size = hbase->hsh_rbsize - hpptr->hsp_offset;
		if (size >= QEF_HASH_DUP_SIZE
		  && size >= QEFH_GET_ROW_LENGTH_MACRO(rowptr))
		{
		    /* Whole row in buffer - just copy ptr. */
		    *probep = rowptr;
		    hpptr->hsp_offset += QEFH_GET_ROW_LENGTH_MACRO(rowptr);
		}
		else
		{
		    status = qen_hash_readrow(dsh, hbase, hpptr, probep);
		    if (status != E_DB_OK)
			return (status);
		}
		continue;		/* loop back to probe again */
	    }	/* end if hsh_currows_left */

	    /* We get here if we've used all the rows in the current 
	    ** partition or we're just starting to process a newly
	    ** built hash table. Look for the next partition in the hash 
	    ** table, open its file and start reading its rows. But
	    ** first, close and release previous partition file (if
	    ** this isn't the first partition in the hash table). */

	    if (hbase->hsh_currpart >= 0)
	    {
		hpptr = &hlink->hsl_partition[hbase->hsh_currpart];
		i = CLOSE_RELEASE;
		if (hlink->hsl_level == 0)
		    i = CLOSE_DESTROY;
		status = qen_hash_close(hbase, &hpptr->hsp_file, i);
		if (status != E_DB_OK) 
		    return(status);
		hpptr->hsp_flags = HSP_DONE;
	    }

	    allojs = FALSE;		/* reset for next partition */
	    hbase->hsh_flags &= ~HASH_FAST_PROBE2;
	    for (i = hbase->hsh_currpart+1; i < hbase->hsh_pcount; i++)
	    {
		/* Get next partition that maps to the hash table, or
		** has 0 rows in the hash table but may contribute
		** outer join rows. */

		hpptr = &hlink->hsl_partition[i];
		if (hpptr->hsp_flags == HSP_DONE ||
		    (hpptr->hsp_brcount == 0 && hpptr->hsp_prcount == 0))
		    continue;
		if (hpptr->hsp_flags & inhashmask)
		{
		    /* We'll get the first row out of this partition the
		    ** usual loopy way, but after delivering it and returning,
		    ** we can take the quick path for the rest.
		    */
		    hbase->hsh_flags |= HASH_FAST_PROBE2;
		    break;
		}
		if ( (reverse && hpptr->hsp_prcount == 0
			&& hbase->hsh_flags & HASH_WANT_LEFT)
		  || (!reverse && hpptr->hsp_brcount == 0
			&& hbase->hsh_flags & HASH_WANT_RIGHT) )
		{
		    allojs = TRUE;
		    break;
		}
	    }
	    if (i >= hbase->hsh_pcount) break;
					/* finished hash table - loop
					** back and make another */
	    hbase->hsh_currpart = i;

	    /* Reset to read the probing side of the spill file */
	    hpptr->hsp_file->hsf_currbn = reverse ? hpptr->hsp_bstarts_at
						  : hpptr->hsp_pstarts_at;

	    hpptr->hsp_offset = hbase->hsh_rbsize;
	    hbase->hsh_currows_left = (reverse) ? hpptr->hsp_brcount :
				hpptr->hsp_prcount;
	}	/* end of !gotone */

	/* Current hash table has now been processed. Loop marking
	** partitions done, for partitions in the hash table. */
	for (i = 0; i < hbase->hsh_pcount; i++)
	{
	    hpptr = &hlink->hsl_partition[i];
	    if (hpptr->hsp_flags & inhashmask)
	    {
		status = qen_hash_close(hbase, &hpptr->hsp_file, CLOSE_DESTROY);
		if (status != E_DB_OK) 
			return(status);
		hpptr->hsp_flags = HSP_DONE;
	    }
	}

	/* If we've finished a one-sided recursion, we might have loaded rows
	** into partitions that spilled when the original side's repartitioning
	** was done.  Flush everything that's nonempty on the other (probing)
	** side, so that the next hash table build can do the right thing.
	** (Defer this flush until after all processed partitions are
	** marked done, so that we don't pointlessly spill them.)
	*/
	if (recurse)
	{
	    hbase->hsh_reverse = ! hbase->hsh_reverse;
	    status = qen_hash_flush(dsh, hbase, TRUE);
	    hbase->hsh_reverse = ! hbase->hsh_reverse;
	    if (status != E_DB_OK) 
		return(status);
	    recurse = FALSE;		/* Turn this off now */
	}

	/* See if hash table is potential source of outer join candidates.
	** If so, set up OJ pass and loop back. If not (or if OJ pass
	** has now been completed), loop back to rebuild hash table. */

	/* (There used to be a test for *ojmask == 0 as well, but it 
	** inhibited the OJ search - search never got off the ground - 
	** and was dropped. Comment is here in case the test had another 
	** purpose.) */
	if (hbase->hsh_flags & HASH_BUILD_OJS || 
	    (reverse && !(hbase->hsh_flags & HASH_WANT_RIGHT)) || (!reverse &&
	    !(hbase->hsh_flags & HASH_WANT_LEFT)))
	{
	    hbase->hsh_flags &= ~HASH_BUILD_OJS;
	    hbase->hsh_flags |= HASH_NOHASHT;
	}
	else
	{
	    hbase->hsh_flags |= HASH_BUILD_OJS;
	    hbase->hsh_currbucket = -1;
	}

    }	/* end of outer for-loop */
}


/*
** Name: qen_hash_readrow - Read new spill bufferful with possibly split row
**
** Description:
**	When one of the repartition or probe2 loops needs to read a row
**	that isn't already in the read buffer in its entirety, this routine
**	is called to read a new buffer-load.
**
**	If the last row in the previous bufferful was split, the complete
**	row is reconstructed in the workrow area.  Otherwise we just need
**	to read a new bufferful.
**
**	Since there's a fair amount of fiddling around here, due to
**	having to read the entire row header before we know the row length,
**	this (comparatively rare) case has been extracted from the
**	probe2 inline code for clarity.
**
**	Note that readrow isn't appropriate for standard hash table build
**	because it doesn't know about the in-lastbuf business;  nor is it
**	used for cart-prod because cart-prod needs to handle two work rows.
**
** Inputs:
**	dsh			Thread's data segment header
**	hbase			Hash join base info structure
**	hpptr			Pointer to hash partition being read
**	probep			An output
**
** Outputs:
**	probep			Set to address of row, either in read
**				buffer, or in work row area
**	Returns E_DB_OK or error status
**
** History:
**	23-Jun-2008 (kschendel) SIR 122512
**	    Extract from probe2 common code, to ensure correctness.
*/

static DB_STATUS
qen_hash_readrow(QEE_DSH *dsh, QEN_HASH_BASE *hbase,
	QEN_HASH_PART *hpptr, QEF_HASH_DUP **probep)
{
    DB_STATUS status;		/* The usual status thing */
    i4 size;			/* Bytes left in read buffer */
    QEF_HASH_DUP *target;	/* Where new row will be */

    size = hbase->hsh_rbsize - hpptr->hsp_offset;
    /* We already know the next row isn't in the buffer, so if there
    ** is data left in the buffer, it must be a split row.
    */
    if (size > 0)
    {
	target = hbase->hsh_workrow;
	MEcopy(&hbase->hsh_rbptr[hpptr->hsp_offset], size, (PTR) target);
    }
    else
	target = (QEF_HASH_DUP *) hbase->hsh_rbptr;
    *probep = target;
    status = qen_hash_read(hbase, hpptr->hsp_file,
	hbase->hsh_rbptr, hbase->hsh_rbsize);
    if (status != E_DB_OK) 
	return(status);
    if (size == 0)
    {
	/* Just move offset past full row in buffer */
	hpptr->hsp_offset = QEFH_GET_ROW_LENGTH_MACRO(target);
    }
    else
    {
	i4 bytes_to_copy;
	PTR source = hbase->hsh_rbptr;

	/* Finish reconstructing a split row */
	target = (QEF_HASH_DUP *) ((char *)target + size);
	if (size < QEF_HASH_DUP_SIZE)
	{
	    /* Finish off the header first */
	    MEcopy(source, QEF_HASH_DUP_SIZE-size, (PTR)target);
	    source += QEF_HASH_DUP_SIZE-size;
	    target = (QEF_HASH_DUP *)
		    ((char *)target + QEF_HASH_DUP_SIZE - size);
	    size = QEF_HASH_DUP_SIZE;
	}
	bytes_to_copy = QEFH_GET_ROW_LENGTH_MACRO(hbase->hsh_workrow) - size;
	MEcopy(source, bytes_to_copy, (PTR) target);
	hpptr->hsp_offset = source + bytes_to_copy - hbase->hsh_rbptr;
    }
    return (E_DB_OK);
} /* qen_hash_readrow */

/*{
** Name: QEN_HASH_CARTPROD	- cross product of ALLSAME partitions
**
** Description:
**	This routine performs a cross product join of the build/probe
**	rows from a partition in which both sources won't fit the 
**	memory resident hash table and all the rows of at least one
**	source have the same hashkeys. This last factor makes 
**	recursion useless, since all rows would again be assigned to
**	the same partition and the partition would not be reduced in
**	size. 
**
**	This hopefully rare phenomenon leaves no alternative but to
**	perform a cross-product of the rows of each source for this
**	partition. One source is chosen to drive the join (the "outer"
**	source), and for each of its rows, every row is read and matched
**	from the other ("inner") source. Logic is also required to 
**	handle outer joins with such data.
**
**	In the case of a check-only cart-prod situation, the cart-prod
**	setup has arranged for the check-only side (probe input) to be
**	the "outer" of the cross-product.  No true cross-product is done
**	in this case;  we arrange to ask for a new outer each time.
**
**	For outer joins, we'll handle all the OJ checking logic here.
**	All that the top level has to do is update the OJ indicator
**	bytes properly (i.e. if the "on" clause CX says that the rows
**	really do join).
**
**	*Caution* the guy who sets up cross-producting (qen_hash_recurse)
**	can set local reversal for this one partition.  This reversal
**	flag is set in the HCP control block flags, NOT the usual
**	hashjoin hsh_flags.  Be very careful about calling utility
**	routines that might expect role reversal to be set in hsh_flags!
**
** Inputs:
**	dsh		- DSH execution control block for thread
**	hbase		- ptr to base hash structure
**	buildptr	- ptr to addr of build row buffer (to return row)
**	probeptr	- ptr to addr of probe row
**	ojmask		- ptr to returned mask word for OJs
**	gotone		- ptr to flag indicating whether a (joined) row is 
**			being returned
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	7-jan-00 (inkdo01)
**	    Written.
**	22-aug-01 (inkdo01)
**	    Added code to precheck for null join cols (to avoid "null = null"
**	    joins).
**	15-jan-03 (inkdo01)
**	    Minor fixes to full/right join logic to correct failing calls
**	    to qen_hash_write.
**	24-jan-03 (inkdo01)
**	    One last change to fix full/right joins.
**	22-apr-03 (inkdo01)
**	    Really one last change to fix full/right joins.
**	29-aug-04 (inkdo01)
**	    Add logic for global base array to reduce data movement for 
**	    build/probe rows.
**	17-may-05 (inkdo01)
**	    Destroy files at end - not just free them.
**	7-Jun-2005 (schka24)
**	    cartprod during check-only join didn't work, make it work.
**	    Also, clean out partition file ptrs, since we destroy the
**	    files per the previous edit.
**	14-Nov-2005 (schka24)
**	    Don't bother passing OJ indptrs around.
**	17-Jun-2006 (kschendel)
**	    Deal with one side being empty (OJ case).
**	    from returning too few rows. This change fixes bug 118200.
**	5-Apr-2007 (kschendel)
**	    Fix some bugs, mostly for outer join:
**	    - slip-ups in row reading logic could leave pointers aimed at
**	    the wrong place instead of at the just-read row.
**	    - if OJ outer is not allsame but inner is, and if first outer's
**	    hash didn't match the inner, we weren't setting READ_OUTER before
**	    returning the OJ, and the first row was fetched twice.
**	    - When rows split read buffers (either side), the OJ indicator
**	    byte is in one of the work rows.  On the left side we were
**	    clearing the wrong thing, garbaging data.  On the right side,
**	    a possibly set OJ indicator wasn't being written back to the
**	    buffer for writing back to the inner spill file.
**	31-may-07 (hayke02 for schka24)
**	    Only continue/return from the outer row FOR loop if HCP_IALLSAME
**	    is set. This prevents reversed cart prod (check only) hash joins
**	20-Jun-2008 (kschendel) SIR 122512
**	    Extract out read-a-row bits to cater to variable length rows.
**	24-Sep-2008 (kschendel) SIR 122512
**	    NULL key component wasn't inhibiting the key compare as it should
**	    have been;  thus, rows would join that shouldn't.  Fix.
*/
 
static DB_STATUS
qen_hash_cartprod(
QEE_DSH		*dsh,
QEN_HASH_BASE	*hbase,
QEF_HASH_DUP	**buildptr,
QEF_HASH_DUP	**probeptr,
QEN_OJMASK	*ojmask,
bool		*gotone)

{

    QEN_HASH_CARTPROD	*hcpptr = hbase->hsh_cartprod;
    QEN_HASH_FILE *fptr = hcpptr->hcp_file;
    QEN_HASH_PART *hpptr;
    DB_CMP_LIST	*keylist = hbase->hsh_nodep->node_qen.qen_hjoin.hjn_cmplist;
    DB_STATUS	status;
    QEF_HASH_DUP *target;
    QEN_OJMASK	bojmask = 0, pojmask = 0;
    ADF_CB	*adfcb = dsh->dsh_adf_cb;
    u_i4	bhashno;
    i4		cmpres;
    bool	alldone = FALSE;
    bool	read_outer = (hcpptr->hcp_flags & HCP_READ_OUTER) != 0;
    bool	rewrite_inner;

    if (hcpptr->hcp_flags & HCP_REVERSE)
    {
	QEF_HASH_DUP **otherptrp;

	otherptrp = probeptr;
	probeptr = buildptr;
	buildptr = otherptrp;

	if (hbase->hsh_flags & HASH_WANT_RIGHT) 
				bojmask = OJ_HAVE_RIGHT;
	if (hbase->hsh_flags & HASH_WANT_LEFT) 
				pojmask = OJ_HAVE_LEFT;
    }
    else
    {
	if (hbase->hsh_flags & HASH_WANT_LEFT) 
				bojmask = OJ_HAVE_LEFT;
	if (hbase->hsh_flags & HASH_WANT_RIGHT) 
				pojmask = OJ_HAVE_RIGHT;
    }
    rewrite_inner = (pojmask != 0);

    /* If previous call left a probe row in workrow2, and if we're
    ** doing probe side OJ, make sure we copy the OJ indicator flag
    ** (possibly set by driver) from the work row back into the disk
    ** buffer.  Otherwise we'll lose it and think the row didn't join.
    **
    ** This is almost certainly a major pain in the butt, since it's
    ** entirely likely that the OJ indicator is in the block preceding
    ** the one currently in the probe buffer (because we had to read the
    ** tail of the row, and the OJ indicator is in the qef-hash-dup header).
    ** Rather than try to get cute, just assume that we have to back up
    ** to the prior block;  and stuff the row that's in workrow2 back
    ** in, writing it;  then read the current block and stuff the rest
    ** of workrow2 back into the front.  This avoids tedious portability
    ** issues with trying to take the address of a bit-field.
    **
    ** Because this IS such a PITA, the copy-oj flag is only set if the
    ** OJ indicator was originally off, and we only copy if the OJ indicator
    ** is now on.
    */
    if (hcpptr->hcp_flags & HCP_COPY_WORK_OJ)
    {
	hcpptr->hcp_flags &= ~HCP_COPY_WORK_OJ;
	if (hbase->hsh_workrow2->hash_oj_ind == 1)
	{
	    PTR p;
	    i4 size;

	    /* Roll back to start of prior bufferload, re-read */
	    fptr->hsf_currbn = hcpptr->hcp_pojrbn;
	    status = qen_hash_read(hbase, fptr,
			hcpptr->hcp_pbptr,
			hcpptr->hcp_rwsize);
	    if (status != E_DB_OK) return(status);
	    p = hcpptr->hcp_pbptr + hcpptr->hcp_pojoffs;
	    size = hcpptr->hcp_rwsize - hcpptr->hcp_pojoffs;
	    MEcopy((PTR) hbase->hsh_workrow2, size, p);
	    p = (PTR) hbase->hsh_workrow2 + size;
	    /* Roll back again, re-write, read the next one */
	    fptr->hsf_currbn = hcpptr->hcp_pojrbn;
	    status = qen_hash_write(dsh, hbase, 
			fptr, hcpptr->hcp_pbptr,
			hcpptr->hcp_rwsize);
	    if (status != E_DB_OK) return(status);
	    status = qen_hash_read(hbase, fptr,
			hcpptr->hcp_pbptr,
			hcpptr->hcp_rwsize);
	    if (status != E_DB_OK) return(status);
	    /* Copy the rest of the row back into the buffer, just in
	    ** case the OJ flag is in this buffer and not the prev one.
	    */
	    size = QEFH_GET_ROW_LENGTH_MACRO(hbase->hsh_workrow2) - size;
	    MEcopy(p, size, hcpptr->hcp_pbptr);
	}
    }

    /* If first call for this cartprodable partition, read 
    ** 1st blocks of each source, set some flags and do 
    ** other initialization stuff. */
    if (hcpptr->hcp_flags & HCP_FIRST)
    {
	u_i4 bhashno, phashno;

	read_outer = TRUE;
	hcpptr->hcp_flags |= HCP_READ_OUTER;
	hcpptr->hcp_flags &= ~HCP_FIRST;

	/* This is only for write stats.  During a cart-prod we can only
	** (re)write the inner side, under right-OJ conditions.  Indicate
	** which side writes come from, in the unlikely situation that we
	** have them.  Normally inner == probe.
	*/
	fptr->hsf_isbuild = (hcpptr->hcp_flags & HCP_REVERSE) != 0;

	/* We work bhashno and phashno such that if one side is empty,
	** the two are forced not-equal so that we don't attempt to read
	** an empty build or probe.
	*/
	bhashno = 0;  phashno = 1;	/* Shouldn't both be empty but... */
	if (hcpptr->hcp_browcnt > 0)
	{
	    fptr->hsf_currbn = hcpptr->hcp_bstarts_at;
	    status = qen_hash_read(hbase, fptr,
  		hcpptr->hcp_bbptr, hcpptr->hcp_rwsize);
	    if (status != E_DB_OK) return(status);
	    hcpptr->hcp_bcurrbn = fptr->hsf_currbn;
	    bhashno = ((QEF_HASH_DUP *)hcpptr->hcp_bbptr)->hash_number;
	    phashno = ~bhashno;		/* Something not equal */
	}
  
	if (hcpptr->hcp_prowcnt > 0)
	{
	    fptr->hsf_currbn = hcpptr->hcp_pstarts_at;
	    status = qen_hash_read(hbase, fptr,
  		hcpptr->hcp_pbptr, hcpptr->hcp_rwsize);
	    if (status != E_DB_OK) return(status);
	    hcpptr->hcp_pcurrbn = fptr->hsf_currbn;
	    phashno = ((QEF_HASH_DUP *)hcpptr->hcp_pbptr)->hash_number;
	    if (hcpptr->hcp_browcnt == 0)
		bhashno = ~phashno;	/* Don't pass the BOTH test */
	}
	hcpptr->hcp_hashno = phashno;	/* For main loop */
  	
  	/* If hashno's of 2 ALLSAME sources don't match, 
  	** we're done with this partition, unless this is 
	** an outer join.
	** If one partition is empty, fail the test as well.  (Note
	** that all-same is set if a partition is empty!)
	*/
	if ((hcpptr->hcp_flags & (HCP_OALLSAME | HCP_IALLSAME)) == (HCP_OALLSAME | HCP_IALLSAME)
	  && bhashno != phashno)
	{
	    if (bojmask || pojmask) hcpptr->hcp_flags |= HCP_ONLYOJ;
	    else alldone = TRUE;
	}
	if (dsh->dsh_hash_debug)
	{
	    i4 i;
	    QEN_HASH_LINK *hlink = hbase->hsh_currlink;

	    hpptr = hcpptr->hcp_partition;
	    i = hpptr - &hlink->hsl_partition[0];
	    TRdisplay("%@ node: %d, t%d: P[%d]: start cartprod; b:%u  p:%u, flags %v\n",
		hbase->hsh_nodep->qen_num,
		dsh->dsh_threadno, i, hcpptr->hcp_browcnt, hcpptr->hcp_prowcnt,
		"FIRST,REVERSE,OALLSAME,IALLSAME,ONLYOJ,READ_OUTER,COPY_WORK_OJ",
		hcpptr->hcp_flags);
	}
    }

    /* Big loop, waiting for gotone to turn on. Successes
    ** return from inside loop. If we're out of rows, we just
    ** break and clean up. */
    *gotone = FALSE;
    adfcb->adf_errcb.ad_errcode = 0;

    while (!(*gotone) && !alldone)
    {
	/* First check for outer joins - left join, then right
	** join. Then it falls through to inner join logic. */
	if (hcpptr->hcp_flags & HCP_ONLYOJ)
	{
	    if (hcpptr->hcp_browcnt && bojmask)
	    {
		/* Only outer OJ's around, read a row and return it */
		status = qen_hash_cpreadrow(dsh, hbase, hcpptr,
				buildptr, TRUE, FALSE);
		if (status != E_DB_OK)
		    return (status);

		hcpptr->hcp_browcnt--;
		*ojmask = bojmask;
		*gotone = TRUE;
		return(E_DB_OK);
	    }	/* end of "left" join logic */

	    else if (hcpptr->hcp_prowsleft && pojmask)
	    {
		/* Only inner OJ's left, read rows and return any that
		** haven't already been marked as used by an inner join
		*/
		while (!(*gotone) && hcpptr->hcp_prowsleft)
		{
		    /* No need to rewrite now, this is the last pass */
		    status = qen_hash_cpreadrow(dsh, hbase, hcpptr,
				probeptr, FALSE, FALSE);
		    if (status != E_DB_OK)
			return (status);
		    hcpptr->hcp_prowsleft--;
		    if ((*probeptr)->hash_oj_ind == 0)
		    {
			*ojmask = pojmask;
			*gotone = TRUE;
			return(E_DB_OK);
		    }
		    /* else row already joined, read another one */
		}
	    }	/* end of "right" join logic */

	    /* We get here if we're in OJ mode, and they're all done. */
	    alldone = TRUE;
	    continue;
	}	/* end of outer join processing */

	/* Inner join processing starts here. It's quite a bit simpler.
	** First read an "outer" row, if necessary. */
	for ( ; ; )	/* big loop to break from */
	{
	    if (read_outer)
	    {
		if (hcpptr->hcp_browcnt == 0)
		{
		    if (pojmask) hcpptr->hcp_flags |= HCP_ONLYOJ;
				/* just OJs left */
		    else alldone = TRUE;
				/* otherwise, we're really done */
		    break;
		}

		/* Get the next outer row */
		status = qen_hash_cpreadrow(dsh, hbase, hcpptr,
				buildptr, TRUE, FALSE);
		if (status != E_DB_OK)
		    return (status);
		target = *buildptr;
		hcpptr->hcp_browcnt--;
		bhashno = target->hash_number;
		target->hash_oj_ind = 0;	/* Init OJ indicator */
		if (hcpptr->hcp_flags & HCP_IALLSAME
		  && bhashno != hcpptr->hcp_hashno)
		{
		    /* Outer row hashno doesn't match inners. "Left"
		    ** join, or reject the row and loop for another. */
		    if (bojmask)
		    {
			*ojmask = bojmask;
			*gotone = TRUE;
			return(E_DB_OK);
		    }
		    else continue;
		}

		read_outer = FALSE;
		hcpptr->hcp_flags &= ~HCP_READ_OUTER;
	    }

	    /* We have an outer row in buildptr. Now get a matching 
	    ** inner. */
	    for ( ; ; )	/* another loop to break from */
	    {
		if (hcpptr->hcp_prowsleft == 0) break;

		/* If any inners left, loop 'til we find one. */
		status = qen_hash_cpreadrow(dsh, hbase, hcpptr,
			(QEF_HASH_DUP **) probeptr, FALSE, rewrite_inner);
		if (status != E_DB_OK)
		    return (status);
		hcpptr->hcp_prowsleft--;
		/* (note: "target" not set here, use *probeptr) */
		
		/* Now have outer and inner rows. Compare keys. */
		if (hbase->hsh_flags & HASH_CHECKNULL &&
			qen_hash_checknull(hbase, &(*buildptr)->hash_key[0]))
		    cmpres = -1;
		else
		{
		    cmpres = adt_sortcmp(adfcb, keylist,
			hbase->hsh_kcount, 
			&(*buildptr)->hash_key[0],
			&(*probeptr)->hash_key[0], 0);
		    if (adfcb->adf_errcb.ad_errcode != 0)
		    {
			dsh->dsh_error.err_code = adfcb->adf_errcb.ad_errcode;
			return(E_DB_ERROR);
		    }
		}

		if (cmpres == 0)
		{
		    *gotone = TRUE;
		    /* If a check-only join, don't really do a cross-product,
		    ** just ask for the next outer (which is the probe
		    ** side in this case) and reset the inner.
		    */
		    if (hbase->hsh_nodep->node_qen.qen_hjoin.hjn_kuniq)
		    {
			hcpptr->hcp_flags |= HCP_READ_OUTER;
			status = qen_hash_reset_cpinner(dsh, hbase, pojmask);
			if (status != E_DB_OK) return(status);
		    }
		    return(E_DB_OK);
		}
	    }	/* end of inner source for-loop */
	
	    /* To get here, inner rows are all used and we have
	    ** no match. Reset to start at top of inner file and
	    ** check outer row for outer join possibility. */

	    hcpptr->hcp_flags |= HCP_READ_OUTER;
	    read_outer = TRUE;

	    if (hcpptr->hcp_browcnt || pojmask)
	    {
		/* Only reset inner file if there are
		** remaining outers or inners may be in OJ. */
		status = qen_hash_reset_cpinner(dsh, hbase, pojmask);
		if (status != E_DB_OK) return(status);
	    }

	    if (bojmask && (*buildptr)->hash_oj_ind == 0)
	    {
		/* Outer join. */
		*ojmask = bojmask;
		*gotone = TRUE;
		return(E_DB_OK);
	    }
			
	    if (hcpptr->hcp_browcnt == 0) break;
				/* if remaining outers, stay in loop */
	}	/* end of outer source for-loop */
    }	/* end of !gotone && !alldone loop */

    /* We get here when the Cartesian product is thankfully over. 
    ** Just close the files, set/reset flags and leave. */

    hpptr = hcpptr->hcp_partition;
    if (dsh->dsh_hash_debug)
    {
	i4 i;
	QEN_HASH_LINK *hlink = hbase->hsh_currlink;

	i = hpptr - &hlink->hsl_partition[0];
	TRdisplay("%@ n%d t%d: P[%d]: finished cartprod\n",
	    hbase->hsh_nodep->qen_num, dsh->dsh_threadno, i);
    }
    status = qen_hash_close(hbase, &hpptr->hsp_file, CLOSE_DESTROY);
    if (status != E_DB_OK) return(status);
    hpptr->hsp_flags = HSP_DONE;
    hcpptr->hcp_file = NULL;

    hbase->hsh_flags &= ~HASH_CARTPROD;
    return(E_DB_OK);

}


/*
** Name: qen_hash_cpreadrow - Fetch next row for cart-prod
**
** Description:
**	When doing a nested-loop join on an allsame partition (cartprod),
**	we're reading both halves of the spill file.  In order to
**	make sure split rows are handled properly, this routine is called
**	to either point to the next row in a buffer, or read the next
**	bufferful and deal with a possibly split row.  Outer-side split
**	rows are returned in workrow, and inner-side split rows are
**	returned in workrow2.
**
**	If the inner side is outer-joining, the current bufferful has to
**	(usually) be written back out to keep the OJ indicators on disk
**	up-to-date until the join is complete.  This write-back can be
**	skipped if we're on the final outer-join read pass.  Things get
**	even more complicated when doing an OJ on a split row, as the row
**	in workrow2 might have to be written back.  For this condition, a
**	flag in the cart-prod control structure is set so that the driver
**	can deal with it.
**
**	Since speed is not as much at issue with cart-prod (the join is
**	already in deep trouble speed-wise), cpreadrow does everything;
**	unlike the normal readrow which expects the caller to handle the
**	no-read case.
**
** Inputs:
**	dsh			Thread's data segment header
**	hbase			Hash join base info structure
**	hcpptr			Pointer to cart-prod control structure
**	rowpp			An output
**	readouter		TRUE if reading outer side, else inner
**	rewrite_inner		TRUE if doing inner side and need to rewrite
**				inners (for OJ flag purposes).  Must only be
**				true if readouter is false!
**
** Outputs:
**	rowpp			Set to address of row, either in appropriate
**				buffer, or in one of the work row areas
**	Returns E_DB_OK or error status
**
** History:
**	23-Jun-2008 (kschendel) SIR 122512
**	    Extract from cartprod common code, to ensure correctness.
*/

static DB_STATUS
qen_hash_cpreadrow(QEE_DSH *dsh, QEN_HASH_BASE *hbase,
	QEN_HASH_CARTPROD *hcpptr, QEF_HASH_DUP **rowpp,
	bool readouter, bool rewrite_inner)
{
    DB_STATUS status;		/* The usual status thing */
    i4 oj_poffset;		/* Offset to start of (split) row */
    i4 size;			/* Bytes left in read buffer */
    i4 *offsetp;		/* hcp_boffset or poffset */
    PTR bufbase;		/* hcp_bbptr or pbptr */
    PTR target;
    QEF_HASH_DUP *rowptr;	/* Pointer to row to be returned */
    QEN_HASH_FILE *fptr;	/* Spill file for cart-prod */
    u_i4 oj_oldrbn;		/* First page originally in the buffer */
    u_i4 *currbnp;		/* hcp_bcurrbn or pcurrbn */

    if (readouter)
    {
	offsetp = &hcpptr->hcp_boffset;
	bufbase = hcpptr->hcp_bbptr;
    }
    else
    {
	offsetp = &hcpptr->hcp_poffset;
	bufbase = hcpptr->hcp_pbptr;
    }

    rowptr = (QEF_HASH_DUP *) (bufbase + *offsetp);
    size = hcpptr->hcp_rwsize - *offsetp;
    if (size >= QEF_HASH_DUP_SIZE
      && size >= QEFH_GET_ROW_LENGTH_MACRO(rowptr))
    {
	/* Row is entirely within read buffer, just use it in place.
	** This ought to be the most common case.
	*/
	*rowpp = rowptr;
	*offsetp += QEFH_GET_ROW_LENGTH_MACRO(rowptr);
	return (E_DB_OK);
    }

    /* We'll need to read a new buffer, possibly with a row split across */
    fptr = hcpptr->hcp_file;
    if (readouter)
    {
	target = (PTR) hbase->hsh_workrow;
	currbnp = &hcpptr->hcp_bcurrbn;
    }
    else
    {
	target = (PTR) hbase->hsh_workrow2;
	currbnp = &hcpptr->hcp_pcurrbn;
    }

    if (size > 0)
    {
	MEcopy((PTR) rowptr, size, target);
	rowptr = (QEF_HASH_DUP *) target;
	oj_poffset = *offsetp;
    }
    else
    {
	/* Row isn't split, just need a new bufferful */
	rowptr = (QEF_HASH_DUP *) bufbase;
    }
    *rowpp = rowptr;

    /* Before reading the next bufferful, if this is the inner side and
    ** a right-outer-join, we need to write the bufferful back in case
    ** any OJ indicator flags were changed this pass.
    */
    if (rewrite_inner)
    {
	/* back up over this bufferful and rewrite it */
	oj_oldrbn = hcpptr->hcp_pcurrbn - (hcpptr->hcp_rwsize / hbase->hsh_pagesize);
	fptr->hsf_currbn = oj_oldrbn;
	status = qen_hash_write(dsh, hbase, fptr,
			bufbase,
			hcpptr->hcp_rwsize);
	if (status != E_DB_OK)
	    return (status);
    }

    /* Need to read from proper half of the spill file... */
    fptr->hsf_currbn = *currbnp;
    status = qen_hash_read(hbase, fptr, bufbase, hcpptr->hcp_rwsize);
    if (status != E_DB_OK) 
	return(status);
    *currbnp = fptr->hsf_currbn;
    if (size == 0)
    {
	/* Just move offset past full row in buffer */
	*offsetp = QEFH_GET_ROW_LENGTH_MACRO(rowptr);
    }
    else
    {
	i4 bytes_to_copy;
	PTR source = bufbase;

	/* Finish reconstructing a split row */
	target += size;		/* Skip bit copied from first bufferful */
	if (size < QEF_HASH_DUP_SIZE)
	{
	    /* Finish off the header first */
	    MEcopy(source, QEF_HASH_DUP_SIZE-size, target);
	    source += QEF_HASH_DUP_SIZE-size;
	    target += QEF_HASH_DUP_SIZE-size;
	    size = QEF_HASH_DUP_SIZE;
	}
	bytes_to_copy = QEFH_GET_ROW_LENGTH_MACRO(rowptr) - size;
	MEcopy(source, bytes_to_copy, target);
	*offsetp = source + bytes_to_copy - bufbase;
	/* If we're doing a right OJ, and the OJ flag isn't already set
	** showing an inner-join, light a flag for the next entry into
	** the cart-prod driver.  The join driver might set the OJ
	** flag (in the split row) and it has to get copied back to
	** the disk file, else we'll lose it.
	*/
	if (rewrite_inner && (*rowpp)->hash_oj_ind == 0)
	{
	    hcpptr->hcp_flags |= HCP_COPY_WORK_OJ;
	    /* Save row-start position info to help out */
	    hcpptr->hcp_pojoffs = oj_poffset;
	    hcpptr->hcp_pojrbn = oj_oldrbn;
	}
    }
    return (E_DB_OK);
} /* qen_hash_cpreadrow */

/*
** Name: QEN_HASH_RESET_CPINNER - Reset cross-product inner source
**
** Description:
**	When doing a cross-product on "all same hash" partitions,
**	after reaching the end of the inner source, we need to
**	reset the inner source to the start, and ask for a fresh
**	outer source row.
**
**	The reason for making this a routine is that we also need to
**	reset the inner source if we're doing a check-only join.
**	In that case, the cartprod loop resets the inner and signals
**	for a fresh outer as soon as a match is found for the current
**	outer row.  (I.e. we don't want to do a true cross product.)
**
** Inputs:
**	dsh		- Data segment header for thread
**	hbase		- ptr to base hash structure
**	pojmask		- nonzero if doing outer-join on "inner" source
**
** Outputs:
**	Returns E_DB_OK or error status
**
** History:
**	7-Jun-2005 (schka24)
**	    Extract common code from qen_hash_cartprod.
**
*/

static DB_STATUS
qen_hash_reset_cpinner(
	QEE_DSH *dsh, QEN_HASH_BASE *hbase,
	QEN_OJMASK pojmask)

{

    DB_STATUS status;
    i4 currbn;
    i4 pages_per_buf;
    QEN_HASH_CARTPROD *hcpptr = hbase->hsh_cartprod;
    QEN_HASH_FILE *fptr = hcpptr->hcp_file;

    pages_per_buf = hcpptr->hcp_rwsize / hbase->hsh_pagesize;

    /* Start over with the inner side, but first, consider writeback */
    hcpptr->hcp_prowsleft = hcpptr->hcp_prowcnt;
    hcpptr->hcp_poffset = 0;
    currbn = hcpptr->hcp_pcurrbn - pages_per_buf;
    if (pojmask)
    {

	/* If probe is "outer" of an OJ, write buffer
	** back, c/w indicator bytes for later OJ 
	** pass. */
	fptr->hsf_currbn = currbn;
	status = qen_hash_write(dsh, hbase, fptr,
			hcpptr->hcp_pbptr,
			hcpptr->hcp_rwsize);
	if (status != E_DB_OK) return(status);
    }
    /* Read block 0 unless that's what's in there now (unlikely
    ** unless it's a CO join)
    */
    if (currbn > hcpptr->hcp_pstarts_at)
    {
	fptr->hsf_currbn = hcpptr->hcp_pstarts_at;
	status = qen_hash_read(hbase, hcpptr->hcp_file,
		hcpptr->hcp_pbptr,
		hcpptr->hcp_rwsize);
	if (status != E_DB_OK) return(status);
    }
    /* Next probe read is at start + buffer size */
    hcpptr->hcp_pcurrbn = hcpptr->hcp_pstarts_at + pages_per_buf;
    /* if browcnt == 0, must be "right" OJ */
    if (hcpptr->hcp_browcnt == 0) 
	hcpptr->hcp_flags |= HCP_ONLYOJ;

    /* Caller will ask for a new outer */
    return (E_DB_OK);

} /* qen_hash_reset_cpinner */

/*{
** Name: QEN_HASH_BVINIT	- initialize bit filter
**
** Description:
**	This function zeroes a bit filter and then sets bits for
**	each row currently in the partition buffer.  It's not necessary
**	to worry about any split row at the end.
**
** Inputs:
**	hbase		- ptr to base hash structure
**	hpptr		- ptr to partition descriptor
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	29-dec-99 (inkdo01)
**	    Written.
**	22-Jun-2005 (schka24)
**	    Bit filtering is within partition, calculate it that way
**	    for better selectivity.
**	10-Nov-2005 (schka24)
**	    Tighten up check to make sure we don't look at a hash number
**	    that falls off the end of the buffer.  qee's layout prevents
**	    this at the moment, but belt and suspenders...
**	17-Jun-2008 (kschendel) SIR 122512
**	    Rework for variable row sizes.
**
*/
 
static VOID
qen_hash_bvinit(
QEN_HASH_BASE	*hbase,
QEN_HASH_PART	*hpptr)

{
    char	*rowend;
    i4		pcount;
    QEF_HASH_DUP *rowptr;
    u_i4	bvsize = hbase->hsh_bvsize * BITSPERBYTE - 1;
    u_i4	hashno;

    /* First, loop over vector and zero all the bits.  Note that qee
    ** always chooses bvsize to be a multiple of 8, so hopefully the
    ** memset intrinsic that's probably behind MEfill will do the most
    ** optimal thing.
    */
    MEfill(hbase->hsh_bvsize, 0, hpptr->hsp_bvptr);

    /* Now loop over partition buffer, setting the appropriate bit
    ** for each row.  It's OK to not look at a row whose header
    ** isn't entirely in the buffer, that must be the row that caused
    ** the spill and the caller will handle it.
    */
    rowptr = (QEF_HASH_DUP *) hpptr->hsp_bptr;
    rowend = (char *) rowptr + hbase->hsh_csize - QEF_HASH_DUP_SIZE;
    pcount = hbase->hsh_pcount;
    while ((char *)rowptr < rowend)
    {
	hashno = rowptr->hash_number;
	BTset((hashno/pcount) % bvsize, hpptr->hsp_bvptr);
	rowptr = (QEF_HASH_DUP *) ((char *)rowptr
				+ QEFH_GET_ROW_LENGTH_MACRO(rowptr));
    }

}


/*{
** Name: QEN_HASH_CHECKNULL	- check for null join keys
**
** Description:
**	This function looks for null valued join columns. If one
**	is found, subsequent key compare is unnecessary because 
**	"null = null" joins are NOT allowed.
**
** Inputs:
**	hbase		- ptr to base hash structure
**	bufptr		- ptr to key buffer (probe or build - makes no
**			  difference)
**
** Outputs:
**	returns		TRUE - if a join column is NULL
**			FALSE - if no join column value is NULL
**
** Side Effects:
**
**
** History:
**	22-aug-01 (inkdo01)
**	    Written to prevent "null = null" joins.
**
*/
 
static bool
qen_hash_checknull(
QEN_HASH_BASE	*hbase,
PTR		bufptr)

{
    DB_CMP_LIST	*cmp = &hbase->hsh_nodep->node_qen.qen_hjoin.hjn_cmplist[0];
    i4		i;
    PTR		nullp;

    /* Loop over key compare-list entries, checking null indicators of
    ** the nullable ones for non-zero. */

    i = hbase->hsh_kcount;
    while (--i >= 0)
    {
	if (cmp->cmp_type < 0)
	{
	    nullp = (PTR)(bufptr + (cmp->cmp_offset+cmp->cmp_length-1));
	    if (nullp[0])
		return(TRUE);
	}
	++cmp;
    }

    /* If we fall off loop ... */
    return(FALSE);

}


/*
** Name: qen_hash_htalloc -- (Re)Allocate hash pointer table
**
** Description:
**	This routine is called the first time that a new hash table
**	is about to be constructed.  It verifies that a hash pointer
**	table exists, and is appropriately sized.  If not, a (new)
**	hash pointer table is built.  In either case, the hash pointer
**	table is zeroed out.
**
**	One situation to guard against is a pointer table that exists
**	but is much too small.  This can occur with unevenly distributed
**	PC-joins, or somewhat less commonly when hash-join partitions
**	are unevenly distributed.  If an existing pointer table is too
**	small, long chains build up in the hash table, leading to
**	quadratic insert behaviour and much CPU time wastage.  In a
**	worst case, the first PCjoin group might have one row, and the
**	next might have millions!
**
**	"Too small" is a tough call, especially when it's impossible to
**	know a priori how many rows will actually get inserted;  but
**	as an estimate, assume a row count based on use of all partition
**	buffers as hash space, and a row width between minimum and
**	maximum (weighted towards the minimum).  A too-large pointer
**	table is better than a too-small one;  on the other hand,
**	don't allocate megabytes for 10 rows.
**
**	The call to htalloc should be deferred until hash table build
**	has actually decided how to load the hash table, so that we
**	can use row counts/sizes from the proper side.
**
** Inputs:
**	dsh			Pointer to thread's data segment heaer
**	hbase			Pointer to hash join base structure
**
** Outputs:
**	Returns E_DB_OK or error status
**
** Side effects:
**	Sets hsh_htable and hsh_htsize (rows) in hash join structure.
**
** History:
**	23-Jun-2008 (kschendel) SIR 122512
**	    Refine jgramling's original hash-pointer-table fix slightly.
**	11-Sep-2009 (kschendel) SIR 122513
**	    Nested PC-join integrated, fix here.
*/

static DB_STATUS
qen_hash_htalloc(QEE_DSH *dsh, QEN_HASH_BASE *hbase)
{
    DB_STATUS status;
    i4 i;
    i4 numrows;
    i4 min_rowsz, max_rowsz;	/* Min and max row sizes, hash side */
    i8 actualrows;		/* Actual rows this side all partitions */

    if (hbase->hsh_reverse)
    {
	actualrows = hbase->hsh_prcount;
	min_rowsz = hbase->hsh_min_prowsz;
	max_rowsz = hbase->hsh_prowsz;
    }
    else
    {
	actualrows = hbase->hsh_brcount;
	min_rowsz = hbase->hsh_min_browsz;
	max_rowsz = hbase->hsh_browsz;
    }
    /* Fiddle the row size to bias it towards the minimum. */
    if (min_rowsz != max_rowsz)
    {
	/* Use minimum + 1/4 the variation */
	i = (max_rowsz - min_rowsz) / 4;
	max_rowsz = DB_ALIGNTO_MACRO((min_rowsz+i), QEF_HASH_DUP_ALIGN);
    }
    numrows = ((hbase->hsh_csize + hbase->hsh_bvsize) * hbase->hsh_pcount) / max_rowsz;

    /* Clip to actual rows, except for PC joins.  The PC join case
    ** is one where reallocation is rather likely and it would
    ** be better to not waste the old pointer table memory.
    ** For PC joins, stick with a pointer table that can potentially
    ** cover the entire set of hashed rows.
    */
    if ((hbase->hsh_nodep->qen_flags & (QEN_PART_SEPARATE | QEN_PART_NESTED)) == 0
      && numrows > actualrows)
	numrows = actualrows;	/* Clip to actual rows on this side */

    /* Reconstruct a pointer table if an existing one is less than 70%
    ** of what we think we'll need this time.
    */
    if (hbase->hsh_htable == NULL
      || hbase->hsh_htsize <= (7*numrows)/10)
    {
	if (numrows < 100)
	    numrows = 100;	/* Enforce at least some minimum size */
	else if (numrows == actualrows)
	{
	    /* If we are (probably) sizing the hash table accurately,
	    ** allow a SMALL extra for hash dispersion.  Normally the
	    ** number of rows is an over-estimate which will include
	    ** the extra by its nature.
	    */
	    numrows = numrows + numrows/25;
	}

	if (dsh->dsh_hash_debug && hbase->hsh_htable != NULL)
	{
	    TRdisplay("node:%d t%d realloc htable from %d ptrs to %d ptrs\n",
		hbase->hsh_nodep->qen_num,
		dsh->dsh_threadno, hbase->hsh_htsize, numrows);
	}
	status = qen_hash_palloc(dsh->dsh_qefcb, hbase,
			(numrows+1) * sizeof(PTR),
			&hbase->hsh_htable,
			&dsh->dsh_error, "ht_ptrs");
	if (status != E_DB_OK)
	    return (status);
	hbase->hsh_htsize = numrows;
    }
    MEfill((hbase->hsh_htsize+1) * sizeof(PTR), 0, (PTR) hbase->hsh_htable);

    return (E_DB_OK);
} /* qen_hash_htalloc */

/*
** Name: qen_hash_palloc
**
** Description:
**	Allocate memory from the hash stream, making sure that we don't
**	exceed the session limit.  If the hash stream isn't open, we'll
**	open it.
**
**	This function can nap (CSms_thread_nap) if the pool is full.
**	Don't call it while holding a mutex or critical resource!
**
** Inputs:
**	qefcb			QEF session control block
**	hbase			Hash join control base
**	psize			Size of request in bytes
**	pptr			Put allocated block address here
**	dsh_error		Where to put error code if error
**	what			Short name for request for tracing
**
** Outputs:
**	Returns E_DB_OK or error status
**	If error, dsh_error->err_code filled in.
**
** History:
**	27-May-2005 (schka24)
**	    Wrote to help trace memory problem with 100Gb TPC-H.
**	24-Oct-2005 (schka24)
**	    (from Datallegro) Nap if we hit the pool limit (but NOT
**	    the session limit).  Don't nap indefinitely, though, we
**	    might be holding locks etc.
**	25-Sep-2007 (kschendel) SIR 122512
**	    Allow somewhat more than qef_hash_mem before preemptively
**	    announcing out of memory.  qee now figures hash joins and
**	    hash aggregations a bit more closely, and for a query to
**	    exceed qef_hash_mem probably implies a complex query and a
**	    too-small qef_hash_mem setting.
**	    (later) actually, take the test out altogether.
*/

#ifdef xDEBUG
GLOBALDEF i4 hash_most_ever = 0;
#endif

static DB_STATUS
qen_hash_palloc(QEF_CB *qefcb, QEN_HASH_BASE *hbase,
	SIZE_TYPE psize, void *pptr,
	DB_ERROR *dsh_error, char *what)

{
    DB_STATUS status;		/* Usual status thing */
    i4 naptime;			/* Total time napped so far */
    ULM_RCB ulm;		/* ULM request block */

#ifdef xDEBUG
    if (qefcb->qef_h_used + psize > hash_most_ever)
    {
	if (qefcb->qef_h_used + psize > qefcb->qef_h_max*10/11)
	    TRdisplay("%@ Most ever was %d, new used=%d, this req=%d\n",
			hash_most_ever, qefcb->qef_h_used+psize, psize);
	hash_most_ever = qefcb->qef_h_used+psize;
    }
#endif

    /* Use the sorthash pool */
    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_s_ulmcb, ulm);

    /* If stream not allocated, do that now */
    if (hbase->hsh_streamid == NULL)
    {
	ulm.ulm_streamid_p = &hbase->hsh_streamid;
	naptime = 0;
	for (;;)
	{
	    status = ulm_openstream(&ulm);
	    if (status == E_DB_OK || naptime >= Qef_s_cb->qef_max_mem_nap)
		break;
	    CSms_thread_nap(500);		/* Half a second */
	    naptime += 500;
	}
	if (status != E_DB_OK)
	{
	    TRdisplay("%@ QEF sorthash pool: node: %d open of %s (%lu) failed, pool full, used is %d\n",
			hbase->hsh_nodep->qen_num,
			what, psize, qefcb->qef_h_used);
	    if (ulm.ulm_error.err_code == E_UL0005_NOMEM)
		dsh_error->err_code = E_QE000D_NO_MEMORY_LEFT;
	    else
		dsh_error->err_code = ulm.ulm_error.err_code;
	    return (E_DB_ERROR);
	}
        if (naptime > 0)
	{
	    TRdisplay("%@ QEF sorthash pool: node: %d open of %s (%lu) delayed %d ms, pool full, used is %d\n",
		    hbase->hsh_nodep->qen_num,
		    what, psize, naptime, qefcb->qef_h_used);
	}
	/* FIXME pass dsh instead of qefcb but fix later */
	if (((QEE_DSH *)(qefcb->qef_dsh))->dsh_hash_debug)
	{
	    TRdisplay("%@ QEF sorthash pool: node: %d open of %s (%lu) complete: used is %d\n",
			hbase->hsh_nodep->qen_num,
			what, psize, qefcb->qef_h_used);
	}

    }

    ulm.ulm_streamid = hbase->hsh_streamid;
    ulm.ulm_psize = psize;
    naptime = 0;
    for (;;)
    {
	status = ulm_palloc(&ulm);
	if (status == E_DB_OK || naptime >= Qef_s_cb->qef_max_mem_nap)
	    break;
	CSms_thread_nap(500);			/* Half a second */
	naptime += 500;
    }
    if (status != E_DB_OK)
    {
	TRdisplay("%@ QEF sorthash pool: node: %d alloc of %s (%lu) failed, pool full, used is %d\n",
		hbase->hsh_nodep->qen_num,
		what, psize, qefcb->qef_h_used);
	if (ulm.ulm_error.err_code == E_UL0005_NOMEM)
	    dsh_error->err_code = E_QE000D_NO_MEMORY_LEFT;
	else
	    dsh_error->err_code = ulm.ulm_error.err_code;
	return (E_DB_ERROR);
    }
    if (naptime > 0)
    {
	TRdisplay("%@ QEF sorthash pool: node: %d alloc of %s (%lu) delayed %d ms, pool full, used is %d\n",
		hbase->hsh_nodep->qen_num,
		what, psize, naptime, qefcb->qef_h_used);
    }
    hbase->hsh_size += psize;
    CSadjust_counter(&qefcb->qef_h_used, psize);	/* thread-safe */
    *(void **) pptr = ulm.ulm_pptr;
    if (((QEE_DSH *)(qefcb->qef_dsh))->dsh_hash_debug)
    {
	TRdisplay("%@ hashalloc n:%d alloc %lu %s at %p, new hsh_size %u used %d\n",
		hbase->hsh_nodep->qen_num,
		psize, what, ulm.ulm_pptr, hbase->hsh_size, qefcb->qef_h_used);
    }

    return (E_DB_OK);

} /* qen_hash_palloc */

/*{
** Name: QEN_HASH_WRITE		- write a block to a partition file
**
** Description:
**	This function writes blocks to partition files. 
**	The first block number to write is in hsf_currbn.
**
** Inputs:
**	dsh		- DSH execution control block
**	hbase		- ptr to base hash structure
**	hfptr		- ptr to file descriptor
**	buffer		- ptr to buffer being written
**	nbytes		- number of bytes to write
**
** Outputs:
**
** Side Effects:
**	currbn is advanced by nbytes / hsh_pagesize, the number of
**	hashfile pages written.
**
**
** History:
**	23-mar-99 (inkdo01)
**	    Written.
**	28-jan-00 (inkdo01)
**	    Tidy up hash memory management.
**	22-Jun-2005 (schka24)
**	    Keep I/O stats.
**	15-May-2008 (kschendel) SIR 122512
**	    Move current block number to file struct.
**
*/
 
static DB_STATUS
qen_hash_write(
    QEE_DSH		*dsh,
    QEN_HASH_BASE	*hbase,
    QEN_HASH_FILE	*hfptr,
    PTR			buffer,
    u_i4		nbytes)

{

    DMH_CB	*dmhcb;
    DB_STATUS	status;
    i4		kbytes;
    QEF_CB	*qcb = dsh->dsh_qefcb;

    /* If we're writing a block, we'll need to read it back sooner
    ** or later. So allocate read buffer here. */
    if (hbase->hsh_rbptr == NULL)
    {

	/* Partition read buffer has not yet been allocated. Do
	** it now. */
	status = qen_hash_palloc(qcb, hbase,
			hbase->hsh_rbsize,
			&hbase->hsh_rbptr,
			&dsh->dsh_error, "io_part_buf");
	if (status != E_DB_OK)
	    return (status);
    }

    /* Keep stats */
    kbytes = nbytes >> 10;	/* 1K is 1024 bytes = 2^10 */
    if (hfptr->hsf_isbuild)
	hbase->hsh_bwritek += kbytes;
    else
	hbase->hsh_pwritek += kbytes;

    /* Load up DMH_CB and issue the call. */
    dmhcb = (DMH_CB *)hbase->hsh_dmhcb;
    dmhcb->dmh_locidxp = &hfptr->hsf_locidx;
    dmhcb->dmh_file_context = hfptr->hsf_file_context;
    dmhcb->dmh_rbn = hfptr->hsf_currbn;
    dmhcb->dmh_buffer = buffer;
    dmhcb->dmh_blksize = hbase->hsh_pagesize;
    dmhcb->dmh_nblocks = nbytes / hbase->hsh_pagesize;
    dmhcb->dmh_func = DMH_WRITE;
    dmhcb->dmh_flags_mask = 0;
    hfptr->hsf_currbn += dmhcb->dmh_nblocks;
    if (hfptr->hsf_filesize < hfptr->hsf_currbn)
	hfptr->hsf_filesize = hfptr->hsf_currbn;

    status = dmh_write(dmhcb);
    if (status != E_DB_OK)
	status = status;
    return(status);

}


/*{
** Name: QEN_HASH_OPEN 		- determines if open file is available
**				or creates (and opens) a new one
**
** Description:
**	Open a spill file for writing.
**
**	If the partition has no spill file, get one from a free chain,
**	or if none, ask DMF for a new spill file.
**
**	If the partition already has a spill file, we can assume that
**	the "other" side has spilled already.  "This" side starts where
**	the current spill-filesize is, so record that in the partition
**	info structure.
**
**	There was originally a call to open for reading, but it
**	turns out that neither DMF nor the low level SR code needed
**	it.  Reading just requires proper positioning.
**
**	If called with ROLE-REVERSE on, we're opening the probe (right) side.
**	Otherwise we're opening the build (left) side.
**
** Inputs:
**	dsh		- DSH execution control block
**	hbase		- ptr to base hash structure
**	hpptr		- ptr to partition descriptor
**
** Outputs:
**
** Side Effects:
**	hsp_file, hsp_bstarts_at or _pstarts_at set in partition struct.
**
**
** History:
**	30-mar-99 (inkdo01)
**	    Written.
**	28-jan-00 (inkdo01)
**	    Tidy up hash memory management.
**	11-Jun-2008 (kschendel) SIR 122512
**	    Open now implies write, not needed for read.
**
*/
 
static DB_STATUS
qen_hash_open(
QEE_DSH		*dsh,
QEN_HASH_BASE	*hbase,
QEN_HASH_PART	*hpptr)

{

    DMH_CB	*dmhcb;
    QEN_HASH_FILE  *fptr;
    DB_STATUS	status;
    bool	reverse = hbase->hsh_reverse;

    fptr = hpptr->hsp_file;

    /* Maybe the spill file is already open.  If so, we're appending
    ** this side to it.
    */
    if (fptr != NULL)
    {
	if (reverse)
	    hpptr->hsp_pstarts_at = fptr->hsf_filesize;
	else
	    hpptr->hsp_bstarts_at = fptr->hsf_filesize;
	fptr->hsf_currbn = fptr->hsf_filesize;
	fptr->hsf_isbuild = !reverse;
	return (E_DB_OK);
    }

    /* Creating or re-using... */

    if (reverse)
	hpptr->hsp_pstarts_at = 0;
    else
	hpptr->hsp_bstarts_at = 0;

    /* Check free list and reuse existing file, if possible. 
    ** Otherwise, allocate a QEN_HASH_FILE + file context block
    ** and create the file, too. */
    if (hbase->hsh_freefile != NULL)
    {
	fptr = hbase->hsh_freefile;
	hbase->hsh_freefile = fptr->hsf_nextf;
	hpptr->hsp_file = fptr;
	fptr->hsf_nextf = NULL; /* avoid stale ptr */
	fptr->hsf_isbuild = !reverse;
	/* Don't bother truncating any file that is there, just overwrite */
	fptr->hsf_filesize = 0;
	fptr->hsf_currbn = 0;
	return(E_DB_OK);
    }
    else
    {
	QEF_CB		*qcb = dsh->dsh_qefcb;

	/* No existing file, but maybe there's an available context
	** block -- they aren't that big, but no point in wasting them.
	*/
	if (hbase->hsh_availfiles != NULL)
	{
	    fptr = hbase->hsh_availfiles;
	    hbase->hsh_availfiles = fptr->hsf_nextf;
	    /* Fall thru to reinit the block */
	}
	else
	{
	    /* Allocate memory for the file context (and containing
	    ** QEN_HASH_FILE block). */
	    status = qen_hash_palloc(qcb, hbase,
			sizeof(QEN_HASH_FILE) + sizeof(SR_IO),
			&fptr,
			&dsh->dsh_error, "hash_file");
	    if (status != E_DB_OK)
		return (status);
	}

	fptr->hsf_nextf = NULL;
	fptr->hsf_nexto = hbase->hsh_openfiles;
	hbase->hsh_openfiles = fptr;		/* link to open list */
	hpptr->hsp_file = fptr;
	fptr->hsf_isbuild = !reverse;
	fptr->hsf_filesize = 0;
	fptr->hsf_currbn = 0;
	fptr->hsf_file_context = (PTR)&((char *)fptr)
					[sizeof(QEN_HASH_FILE)];
    }
    ++ hbase->hsh_fcount;

    /* Load up DMH_CB and issue the call. */
    dmhcb = (DMH_CB *)hbase->hsh_dmhcb;
    dmhcb->dmh_file_context = fptr->hsf_file_context;
    dmhcb->dmh_locidxp = &fptr->hsf_locidx;
    dmhcb->dmh_blksize = hbase->hsh_pagesize;
    dmhcb->dmh_func = DMH_OPEN;
    dmhcb->dmh_flags_mask = DMH_CREATE;

    status = dmh_open(dmhcb);
    if (status != E_DB_OK)
	status = status;
    return(status);

}


/*{
** Name: QEN_HASH_READ 		- read a block from a partition file
**
** Description:
**	This function reads blocks from partition files.
**
**	It's OK to ask for a number of bytes that runs past the actual
**	written EOF of the file, but NOTE that the file struct's currbn
**	pointer is advanced as if the entire request were read.  This
**	is bogus, but it simplifies some situations in cart-prod where
**	we need to back up and rewrite a buffer that was previously read.
**	We expect callers to use rowcounts or some similar indication
**	to decide when the end of actual data has been reached.
**
** Inputs:
**	hbase		- ptr to base hash structure
**	hfptr		- ptr to file descriptor
**	buffer		- ptr to buffer being written
**	nbytes		- Number of bytes to read
**
** Outputs:
**
** Side Effects:
**	currbn is advanced by nbytes / hsh_pagesize, the number of
**	hashfile pages requested.
**
**
** History:
**	30-mar-99 (inkdo01)
**	    Written.
**	15-May-2008 (kschendel) SIR 122512
**	    Move current block number to file struct.
**
*/
 
static DB_STATUS
qen_hash_read(
    QEN_HASH_BASE	*hbase,
    QEN_HASH_FILE	*hfptr,
    PTR			buffer,
    u_i4		nbytes)

{

    DMH_CB	*dmhcb;
    DB_STATUS	status;

    /* Load up DMH_CB and issue the call. */
    dmhcb = (DMH_CB *)hbase->hsh_dmhcb;
    dmhcb->dmh_locidxp = &hfptr->hsf_locidx;
    dmhcb->dmh_file_context = hfptr->hsf_file_context;
    dmhcb->dmh_rbn = hfptr->hsf_currbn;
    dmhcb->dmh_buffer = buffer;
    dmhcb->dmh_blksize = hbase->hsh_pagesize;
    dmhcb->dmh_nblocks = nbytes / hbase->hsh_pagesize;
    dmhcb->dmh_func = DMH_READ;
    dmhcb->dmh_flags_mask = 0;
    hfptr->hsf_currbn += dmhcb->dmh_nblocks;
    if (hfptr->hsf_currbn > hfptr->hsf_filesize)
	dmhcb->dmh_nblocks -= (hfptr->hsf_currbn - hfptr->hsf_filesize);

    status = dmh_read(dmhcb);
    if (status != E_DB_OK) 
{
status = status;			/* For breakpointing */
}
    return(status);

}


/*{
** Name: QEN_HASH_FLUSH		- write out contents of partition buffers
**				and close files
**
** Description:
**	This function writes remaining buffer of all disk resident 
**	partition files, then closes them.
**
** Inputs:
**	dsh		- DSH execution control block
**	hbase		- ptr to base hash structure
**	nonspilled_too	- TRUE - allocate & write file for non-spilled
**			partitions, as well as those already spilled
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	8-apr-99 (inkdo01)
**	    Written.
**	18-apr-02 (inkdo01)
**	    Added support to flush non-spilled p's, too, to fix 107539.
**	7-Jun-2005 (schka24)
**	    non-spilled-too case has to watch out for empty partitions.
**	    It's a bit unusual, but can happen when the input is large with
**	    a few discrete values (some partitions fill up and others stay
**	    empty).
**	17-Jun-2006 (kschendel)
**	    Break out flusher for one partition.
**	11-Jun-2008 (kschendel) SIR 122512
**	    Make empty partition test more accurate.
**
*/
 
static DB_STATUS
qen_hash_flush(
QEE_DSH		*dsh,
QEN_HASH_BASE	*hbase,
bool		nonspilled_too)

{

    QEN_HASH_LINK  *hlink = hbase->hsh_currlink;
    QEN_HASH_PART  *hpptr;
    i4		i, spillmask, flushmask;
    i4		rowcount;
    DB_STATUS	status;
 

    /* Search partition array for "spilled" partitions. Write last
    ** block for each, then close the file. */

    if (hbase->hsh_reverse)
    {
	spillmask = HSP_PSPILL;
	flushmask = HSP_PFLUSHED;
    }
    else
    {
	spillmask = HSP_BSPILL;
	flushmask = HSP_BFLUSHED;
    }

    for (i = 0; i < hbase->hsh_pcount; i++)
    {
	hpptr = &hlink->hsl_partition[i];
	/* Skip flushed, done, or nonexistent partitions */
	if (hpptr->hsp_flags & (flushmask | HSP_DONE) || hpptr->hsp_bptr == NULL)
	    continue;
	/* Skip partitions with nothing on this side at all.  This turns
	** out to be essential;  a partition that is both empty and
	** spilled will confuse the hash-build selection loops.
	*/
	rowcount = hpptr->hsp_brcount;
	if (hbase->hsh_reverse)
	    rowcount = hpptr->hsp_prcount;
	if (rowcount == 0)
	    continue;
	/* Flush if spilled, or nonspilled and caller requests */
	if ( (hpptr->hsp_flags & spillmask) || nonspilled_too)
  	{
	    status = qen_hash_flushone(dsh, hbase, hpptr);
  	    if (status != E_DB_OK)
		return (status);
  	}
    }

    return(E_DB_OK);

} /* qen_hash_flush */

/*{
** Name: QEN_HASH_FLUSHONE	- write out contents of one partition buffer
**				and close its file
**
** Description:
**	This routine writes the specified partition buffer to the
**	spill file (build, or probe if reverse set).  If the partition
**	hasn't spilled yet, we force it out assuming that the caller
**	knows what's up.  Do not call if there is no partition buffer
**	at all!
**
** Inputs:
**	dsh		- Thread execution control header
**	hbase		- ptr to base hash structure
**	hpptr		- ptr to partition to spill
**
** Outputs:
**
** Side Effects:
**	File's current block number is reset to start.
**
** History:
**	17-Jun-2006 (kschendel)
**	    Split off from qen_hash_flush, because I thought I would need
**	    this separately.  Apparently not, but seems better anyway.
**
*/
 
static DB_STATUS
qen_hash_flushone(
QEE_DSH		*dsh,
QEN_HASH_BASE	*hbase,
QEN_HASH_PART	*hpptr)

{
    i4		spillmask, flushmask;
    DB_STATUS	status;
 

    /* Set up masks based on reversal */

    if (hbase->hsh_reverse)
    {
	spillmask = HSP_PSPILL;
	flushmask = HSP_PFLUSHED;
    }
    else
    {
	spillmask = HSP_BSPILL;
	flushmask = HSP_BFLUSHED;
    }

    if ((hpptr->hsp_flags & spillmask) == 0)
    {
	/* Forced spill, open the appropriate spill file */
	status = qen_hash_open(dsh, hbase, hpptr);
	if (status != E_DB_OK)
  	    return(status);
	hpptr->hsp_flags |= spillmask;
    }

    /* There is always data in the buffer of a spilled partition.
    ** The flag isn't set (and a write isn't done) until there's
    ** enough data to partially fill the next block. */

    /* FIXME if hsh_pagesize < hsh_csize, attempt to back off the write
    ** size to the next pagesize boundary past hsp_offset.  Need to
    ** double-check that hsp_offset is OK in all cases here.  The idea
    ** is to avoid writing the unfilled portion of the buffer.
    */

    status = qen_hash_write(dsh, hbase, hpptr->hsp_file, hpptr->hsp_bptr, 
				hbase->hsh_csize);
    if (status != E_DB_OK) 
	return(status);

    hpptr->hsp_flags |= flushmask;
    hpptr->hsp_offset = 0;

    return(E_DB_OK);
  
} /* qen_hash_flushone */

/*{
** Name: QEN_HASH_FLUSHCO	- special version of qen_hash_flush for
**				use in CO joins (hjn_kuniq == 1)
**
** Description:
**	This function allocates files and writes the buffer for each
**	probe partition of a CO join that hasn't already spilled to
**	disk. This is necessary because the corresponding build partition
**	must be reloaded and will overlay the probe rows. 
**	Probe partitions that are spilled but not flushed, are flushed.
**
**	Ordinarily the probe partitions that fit in memory would be 
**	loaded in the hash table and processed with role reversal, 
**	but CO joins can't handle role reversal.
**
** Inputs:
**	dsh		- DSH execution control block
**	hbase		- ptr to base hash structure
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	25-oct-01 (inkdo01)
**	    Written to fix CO join problem.
**	29-may-03 (inkdo01)
**	    Exclude partitions already finished (bug 110329).
*/
 
static DB_STATUS
qen_hash_flushco(
QEE_DSH		*dsh,
QEN_HASH_BASE	*hbase)

{

    QEN_HASH_LINK  *hlink = hbase->hsh_currlink;
    QEN_HASH_PART  *hpptr;
    i4		i;
    DB_STATUS	status;
    

    /* Search partition array for partitions with nonempty probe
    ** side that isn't probe-flushed.  If the partition isn't probe-
    ** spilled, open a file for it.  Then, flush the last or only block,
    ** and close the file.
    */

    hbase->hsh_reverse = TRUE;		/* temporarily set for 
					** our callees */

    for (i = 0; i < hbase->hsh_pcount; i++)
    {
	hpptr = &hlink->hsl_partition[i];
	/* Reinit. these guys, regardless. */
	if ((hpptr->hsp_flags & (HSP_PFLUSHED | HSP_DONE)) ||
		hpptr->hsp_prcount == 0) continue;

	/* Force it out */
	status = qen_hash_flushone(dsh, hbase, hpptr);
	if (status != E_DB_OK) 
	    return(status);
    }

    hbase->hsh_reverse = FALSE;		/* extinguish flag */

    return(E_DB_OK);

}


/*
** Name: qef_hash_halfclose -- Close one side of partition spillage
**
** Description:
**
**	This routine is called when one side of partition spill is
**	completely processed and is no longer of any interest.
**	After marking it thus (by adjusting the flags),
**	if the spill file is still in use by the other side, just
**	return.  If the spill file is now unused, call the file close
**	to either release the file for reuse, or destroy the file.
**
**	The build/left side is closed if the role-reversal flag is
**	off;  the probe/right side is closed if it's on.
**
** Inputs:
**	hbase		- ptr to base hash structure
**	hpptr		- ptr to partition descriptor ptr
**	flag		- flag: CLOSE_RELEASE or CLOSE_DESTROY
**
** Outputs:
**	Returns E_DB_OK or error status
**
** Side Effects:
**
** History:
**	11-Jun-2008 (kschendel) SIR 122512
**	    Write prelude to full close, for unified spill files.
*/

static DB_STATUS
qen_hash_halfclose(
QEN_HASH_BASE	*hbase,
QEN_HASH_PART	*hpptr,
i4		flag)

{

    QEN_HASH_FILE  *fptr;

    fptr = hpptr->hsp_file;
    if (fptr == NULL)
	return (E_DB_OK);	/* Already gone... */

    if (hbase->hsh_reverse)
    {
	hpptr->hsp_pbytes = 0;		/* No probe spill now. */
	hpptr->hsp_flags &= ~(HSP_PSPILL | HSP_PFLUSHED);
	if (hpptr->hsp_flags & HSP_BSPILL)
	    return (E_DB_OK);		/* but build spill still there */
    }
    else
    {
	hpptr->hsp_bbytes = 0;		/* No build spill now. */
	hpptr->hsp_flags &= ~(HSP_BSPILL | HSP_BFLUSHED);
	if (hpptr->hsp_flags & HSP_PSPILL)
	    return (E_DB_OK);		/* but probe spill still there */
    }

    /* Neither side still using file, release or destroy it.
    ** Pass file pointer addr to get it nulled out too.
    */
    return (qen_hash_close(hbase, &hpptr->hsp_file, flag));

} /* qen_hash_halfclose */

/*{
** Name: QEN_HASH_CLOSE		- closes an existing partition file and
**				places it on free chain
**
** Description:
**
**	This function "closes" a spill file that is no longer needed.
**	A flag controls what happens to the disk file.  CLOSE_RELEASE
**	says to keep the file but put its file context struct on a free-list
**	for re-use.   CLOSE_DESTROY says to close and delete the file;
**	the file context block is placed on a (different) avail-list
**	for potential re-use with a brand new disk file.
**
**	There's no need for a "close" after writing a spill file in
**	order to re-read it.  Close only happens when the spill has
**	been completely processed.
**
** Inputs:
**	hbase		- ptr to base hash structure
**	fptrp		- ptr to ptr to spill file context
**	flag		- flag: CLOSE_RELEASE or CLOSE_DESTROY
**
** Outputs:
**
** Side Effects:
**	*fptrp is nulled out.
**	Destroy case removes file from hsh_openfiles chain.
**
** History:
**	9-apr-99 (inkdo01)
**	    Written.
**	17-may-05 (inkdo01)
**	    Added code to remove destroyed files from open list.
**	3-Jun-2005 (schka24)
**	    Put destroyed file contexts on an avail list.
**	16-Jan-2007 (kiria01) b119767
**	    Belt & braces check on lists to reduce chance of
**	    list corruption.
**	11-Jun-2008 (kschendel) SIR 122512
**	    Unify build/probe spill into one file.
*/
 
static DB_STATUS
qen_hash_close(
QEN_HASH_BASE	*hbase,
QEN_HASH_FILE	**fptrp,
i4		flag)

{

    DMH_CB	*dmhcb;
    QEN_HASH_FILE *fptr;
    QEN_HASH_FILE  **wfptr;

    fptr = *fptrp;
    if (fptr == NULL)
	return (E_DB_OK);		/* Already closed... */
    *fptrp = NULL;			/* Null out caller's pointer */

    /* Make sure we're not on free chain already and if we are get off it.
    ** This is just a safety check.
    */
    for (wfptr = &hbase->hsh_freefile; *wfptr && *wfptr != fptr;
		    wfptr = &(*wfptr)->hsf_nextf)
	/*SKIP*/;
    if (*wfptr)
	*wfptr = fptr->hsf_nextf;
    else
    {
	/* Likewise if on available chain */
	for (wfptr = &hbase->hsh_availfiles; *wfptr && *wfptr != fptr;
			wfptr = &(*wfptr)->hsf_nextf);
	    /*SKIP*/
	if (*wfptr)
	    *wfptr = fptr->hsf_nextf;
    }
    fptr->hsf_nextf = NULL;

    if (flag & CLOSE_RELEASE)
    {
	/* Add file context to free list. */
	fptr->hsf_nextf = hbase->hsh_freefile;
	hbase->hsh_freefile = fptr;
	return(E_DB_OK);
    }

    /* Search for file on open chain and remove it. */
    for (wfptr = &hbase->hsh_openfiles; *wfptr && *wfptr != fptr;
		    wfptr = &(*wfptr)->hsf_nexto)
	/*SKIP*/;
    if (*wfptr)
	*wfptr = fptr->hsf_nexto;

    /* Add file block to re-use list */
    fptr->hsf_nextf = hbase->hsh_availfiles;
    hbase->hsh_availfiles = fptr;

    /* Load up DMH_CB and issue the call. */
    dmhcb = (DMH_CB *)hbase->hsh_dmhcb;
    dmhcb->dmh_locidxp = &fptr->hsf_locidx;
    dmhcb->dmh_file_context = fptr->hsf_file_context;
    dmhcb->dmh_func = DMH_CLOSE;
    dmhcb->dmh_flags_mask = DMH_DESTROY;

    return(dmh_close(dmhcb));
}

/*{
** Name: QEN_HASH_CLOSEALL	- closes and destroys all files on hash
**				control block's file chain
**
** Description:
**	This function loops over the hsh_openfiles chain, issuing 
**	qen_hash_close calls with the DESTROY option to free them all.
**	It is called at the end of hash join processing.
**
** Inputs:
**	hbase		- ptr to base hash structure
**
** Outputs:
**
** Side Effects:
**	The open and free file lists are cleared, and all the file
**	blocks are left on the "available" list.
**
**
** History:
**	3-dec-99 (inkdo01)
**	    Written.
**	16-Jan-2007 (kiria01) b119767
**	    Simplify list traversal of openfile list and keep deleting
**	    regardless of status return.
*/
 
DB_STATUS
qen_hash_closeall(
QEN_HASH_BASE	*hbase)

{
    DB_STATUS		status = E_DB_OK;
    QEN_HASH_FILE	*fptr;

    /* While there are open files in the list - close them */
    while ((fptr = hbase->hsh_openfiles) != NULL)
    {
	DB_STATUS lcl_status;
	lcl_status = qen_hash_close(hbase, &fptr, CLOSE_DESTROY);
	/* We won't get a further chance to close these files
	** so on error, keep deleting & just report first error returned.
	*/
	if (status == E_DB_OK && lcl_status != E_DB_OK)
	    status = lcl_status;
    }
    hbase->hsh_freefile = NULL;
    return(status);

}

static VOID qen_hash_verify(QEN_HASH_BASE *hbase)
{
    i4 	htsize = hbase->hsh_htsize;
    QEF_HASH_DUP **htable = hbase->hsh_htable;
    QEF_HASH_DUP *hchain;
    i4 	i;

    if ( htable == NULL) return;

    for (i = 0; i < htsize; i++)
    {
	for (hchain = htable[i]; hchain; hchain = hchain->hash_next);
    }
}
