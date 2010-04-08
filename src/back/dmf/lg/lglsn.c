/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <me.h>
#include    <nm.h>
#include    <pc.h>
#include    <tm.h>
#include    <sl.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tr.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <lg.h>
#include    <lk.h>
#include    <cx.h>
#include    <lgdef.h>
#include    <lgkdef.h>

/**
**
**  Name: LGLSN.C - Implements the Log Sequence Number support for the LG system
**
**  Description:
**	This file contains routines which participate in the implementation of
**	Log Sequence Number support for VMSClusters.
**
**	    ------------------------------------------------------------
**
**	THIS IMPLEMENTATION IS NOT COMPLETE!!! This file is being checked in
**	in its current state because it mostly sort of works and checking it in
**	like this lets us get some sort of basic cluster support up and running
**	while we continue to develop improved cluster support.
**
**	    The primary things that are known to be broken and will be worked on
**	    include:
**		1) Initial value generation is unclear. Our algorithm picks a
**		    big number, but we don't think it's the right big number.
**		1a) Initial value generation should be primed in restart cases
**		    by having recovery code provide the highest LSN written to
**		    the log file(s) before the crash.
**		2) Performance is horrible, because we make a synchronous
**		    lock request/release on every single LGwrite call.
**		3) Concurrency is horrible, because we do this while holding
**		    the LG mutex.
**		4) Handling of the VMS VALUE-NOT-VALID condition is not yet
**		    present. (well, it's present, but not correct)
**		5) No ICL/GoldRush support yet exists.
**		6) During situations such as cold restart recovery, where the
**		    entire installation is known to be offline, LSN generation
**		    can be (and probably should be) done by simple local
**		    incrementing of the LSN value -- no need to involve the
**		    VMS lock manager until the installation goes online.
**
**	    ------------------------------------------------------------
**
**	Documentation of LSNs follows:
**
**	Ingres 6.5 Recovery mechanisms require distinguishing between Log
**	Addresses and Log Sequence Numbers.
**
**	Log Addresses:
**
**	Each record written to the Ingres Log File is given a Log Address.  A
**	Log Address is an 8 byte integer.  The last 4 bytes (least significant
**	portion of the Log Address) give the byte offset into the log file to
**	where the record is written.  The first 4 bytes we call the sequence
**	number and guarantees that log addresses are always increasing in value
**	even as we complete a circle around the physical file.  Each time the
**	log file wraps around, the sequence number is incremented.
**
**	Log Sequence Numbers:
**
**	Each record written to the Log File is assigned an LSN - Log Sequence
**	Number.  Like Log Addresses, Log Sequence Numbers are 8 byte integers.
**	Each log record is given an LSN that is guaranteed to be unique and
**	guaranteed to be greater than the LSN of the previous log record.
**	Sequential log records need not (and generally will not) be given
**	sequential LSN's.
**
**	On single Log File systems (currently everywhere but the VAXCluster),
**	Ingres simply uses the Log Address of a log record as its LSN.  On
**	multiple Log File systems (currently only the VAXCluster)
**	installation-wide unique LSN's must be generated on each log write
**	call.
**
**	To assist recovery algorithms, whenever a database page is updated, the
**	LSN of the log record describing the change is written onto the page.
**	The recovery routines can always tell whether a given update is present
**	on a page by comparing the page's LSN to that of the update's log
**	record.
**
**	Note that the ordering of log records in the Log File implies an order
**	in which the updates were actually performed on the database.  A log
**	record with a LSN less than that of a second log record must describe
**	an update which was performed prior to the second update.
**
**	Because LSN's are written to the database, Log Sequence Numbers
**	returned from the Logging System must always increase in value, even
**	following a system shutdown and restart.  When an installation restarts
**	or is reinitialized, the Logging System must set its initial LSN to a
**	value which is guaranteed to be greater than any returned previously.
**	In some cases (simple restart) this is done by checking the Log File
**	for the greatest LSN logged.  When this method cannot be used, a new
**	initial LSN is generated from a timestamp to guarantee an increasing
**	value.
**
**	The database is always treated as moving forward by performing undo
**	changes similar to normal database updates.  Undo operations are logged
**	using Compensation Log Records (CLR's).  Compensation Log Records are
**	assigned Log Sequence Numbers just like any log record.  When a
**	Compensation action is performed on a page to rollback an update, the
**	LSN of the CLR is written to the page.  The fact that the LSN's on the
**	Ingres pages increase in value we define as meaning that the database
**	moves forward.
**
**	The redo routines can check for the need to reapply an update by
**	comparing the Page LSN field in the page header to the LSN
**	of the update log record.  If the Page LSN is greater
**	(more recent) then no update is required, otherwise the update is
**	redone.
**
**	As a side effect of forcing the Log file, the Logging System returns
**	the LSN of the log record to which the Log File was last forced.  The
**	Buffer Manager uses this capability to optimize its page tossing
**	mechanism.  Whenever a page must be written from the cache, the Buffer
**	Manager compares the LSN of the page in the buffer with the known
**	forced LSN.  If the comparison indicates that the Log File may not be
**	forced up to this LSN, then the Buffer Manager calls the Logging System
**	to request that a force be done up to the LSN of the page in the
**	buffer.  The forced LSN from this call is saved for future reference in
**	the Buffer Manager Control Block (BMCB).
**
**	(Note that the BMCB's forced LSN value is not guaranteed to be
**	current, it is only guaranteed to be less than or equal to the real
**	LSN.)
**
**	On non-cluster systems, we will continue to equate Log Addresses and
**	LSN's inside LG.  Thus while the higher-level DMF code will think they
**	are different entities, LG will use the Log Address of a record for its
**	LSN, and return that value from LGwrite calls. (In actual fact, LGwrite
**	computes something that sort of looks like a log address, but cannot
**	actually be used for LGposition or LGread calls. This helps to ensure
**	that higher-level code maintains the distinction properly.)
**	
**	On cluster systems, LG will be required to generate unique cluster LSN's
**	on each LGwrite call.  THIS WILL LIKELY COST A CLUSTER LOCK CALL ON EACH
**	LGWRITE CALL IN A CLUSTER INSTALLATION.
**
**	Rollforward
**
**	Rollforward merges the journal streams together before executing
**	rollforward.  The LSN's are merged to produce a time-based (sort of)
**	stream of journal records to apply to the database.
**
**	There is an assumption that some records may be out of actual
**	chronological order, but it won't be possible to distiguish this
**	because any two updates which occur almost concurrently must use
**	disjoint data sets.
**
**	Performance Implications
**
**	In the current implementation, we actually generate LSN values
**	at LGwrite time for each log record.  The LSN is written to the
**	page header and into the log record header.
**
**	The ICL group may not be extremely happy as it will increase cost on
**	the cluster rather than decrease.  On the other hand, it makes cluster
**	algorithms work better and furthers the possibilities that multi-node
**	fast commit could be built in the future (we don't think multi-node
**	fast commit can be built without having unique and increasing LSN's for
**	each log record).
**
**	This is a simpler system, but is more expensive, requiring a cluster
**	lock request/release (to generate the LSN) for each LGwrite call rather
**	than each write_block.
**
**	There may be ways to optimize this sequence similar to the current
**	algorithms, by noting that most concurrent updates must access disjoint
**	sections of the database to which time ordering is not important.  We
**	would have to be very careful about generating log records in correct
**	order whenever it matters.
**
**	We may be able to request the cluster lock only once per block (or some
**	other interval), but need to do it BEFORE writing log records rather than
**	at commit time. This is because the LA needs to be returned to the
**	caller to be written onto the page header.
**
**	Could we generate them at allocate_lbb or in Begin Transaction?
**	No, because the LSN has to be generated AFTER the object(s)
**	which are to be logged are locked.  Otherwise, there is no guarantee
**	that we will serialize log records correctly.
**	We need the LSN when the log records are written.
**
**	Perhaps something could be made of knowledge of node ownership of data.
**
**	The local LG system could carry around some LSN that it updates
**	periodically (like when log buffers are written).
**
**	When a new data object is read into the server and locked, then a new
**	LSN needs to be generated since we cannot guarantee that some other node
**	has not just updated that object.
**
**	But if we have the object in our cache and it is still valid, then we
**	know that no other node has just updated the object and we need no new
**	LSN to update it.
**
**	This might need some new interface into LG - or a flag to LGwrite.
**	It also means that LSN's have to be generated in groups rather than
**	one at a time.  Each node must allocate a group of LSNs to be used by 
**	several log records.
**
**	Perhaps a allocate_lsn routine in LG
**	that updates the value of the LSN lock and always increments
**	it by some range - LSN_GROUP - say 100.  This would allow a maximum
**	of 100 LGwrite calls before having to update the LSN value.
**
**	There would be a get_lsn call that would return the next local lsn -
**	that is it would return an LSN and increment the local LSN value - not
**	the one held in the LSN lock.
**
**	The allocate_lsn routine would be called:
**	
**	Whenever the get_lsn routine used the last LSN in an allocated
**	LSN group. That is, in LGwrite:
**
**	if ((cluster ) && 
**	    (its time to get a new cluster LSN because we used all LSN_GROUP))
**	{
**	    call routine to make cluster lock call
**	    (note that this is done without mutex)
**	}
**	
**	LGmutex()
**	get local LSN by just incrementing local LSN value
**	do normal LGwrite work
**
**	Whenever LGalter(LG_LSN_UPDATE) is called.  This alter call
**	should be made by the server whenever data is accessed that is
**	not currently owned by the local cache.  These places would
**	have to be identified (any fault_page, validate_page refresh,
**	direct DI read, as well as some logical operations like modify,
**	.... (may be others too - what about CP's, opendb, ....)
**
**	Whenever LGbegin is called, whenever allocate_lbb is called???? 
**	(Updating the LSN at this time might prevent us from having
**	LSN's wildly out of range on two different nodes)
**
**	LGalter(LG_LSN_UPDATE) would need to be
**	called whenever a server was doing an operation that was dependent
**	on sequencing of LSN's across a cluster and could not guarantee that
**	no update was performed on the same or dependent object on a different
**	node:
**
**	Whenever a page is faulted into the cache (either by fault_page,
**	gfault_page, or validate_page).
**
**	Whenever a DDL statement is performed.
**
**	whenever direct DIreads are made to get pages from the db that will
**	then be updated - this is included in the list of DDL operations.
**
**	whenever an update was made that has ordering dependencies with other
**	udpates in a non-page oriented manner.  (calling it whenever a page
**	fault is done resolves all page-oriented ordering issues).
**
**	Things that are not page oriented are operations like MODIFY.  These
**	need to make sure they get a new LSN.  Also, since they serve to
**	invalidate pages in other caches, they will guarantee that new updates
**	to those pages in other nodes will also get new LSN's.  If there
**	is any type of operation that is DDL-like (table or db oriented
**	rather than page oriented) that does not invalidate pages in caches
**	and which the ordering of that update and the udpates to the
**	non-invalidated pages is order-dependent, then we have a problem - 
**	but that we would have a problem anyway with the current
**	recovery scheme.
**
**	Then on top of these optimizations, we might include the following
**	rules:
**
**	When a database is locked exclusively by a server or node
**	(like sole-cache), then even though we fault a page into
**	the cache, we would not have to generate a new LSN.  We
**	would have to generate a new LSN at the time we exclusively
**	lock the database.
**
**	When a table is locked exclusively by a server or node
**	(like X table operation), then we could avoid getting new
**	LSN's when new pages were faulted in for that table.  Not
**	sure that this is very useful now.  (Again, an LSN would
**	have to be requested when the table was locked).
**
**	LSN Initial Value Generation
**
**	In non-cluster mode, the log sequence number (log address) is generated
**	from a timestamp at installation startup to ensure that it goes forward
**	even when the logfile is re-initialized.  This will be an absolute
**	necessity to the new rollforwarddb algorithms.
**
**	In Cluster mode, the LSN also needs to be initialized from a timestamp
**	at installation startup.  It is probably enough to initialize it only
**	when the master starts (we could re-initialize it any time we start a
**	new node if we wanted to (assuming of course that any re-initialization
**	from a timestamp will move its value FORWARD rather than backward)).
**
**	Assumption is that we cannot allocate LSN's faster than the timestamp
**	grows.  Not sure what the guarantee is here.  Also we are assuming that
**	no funny system call stuff (like daylight savings) will cause the
**	timestamp to move backwards.
**
**	If system moves between cluster and non-cluster, then the LSN's must
**	always move forward.  This is guaranteed if the high portion of the
**	LSN (which is the log sequence number in the non-cluster) is chosen
**	the same way.  The log file will have to be re-inited to include/exclude
**	the extra LSN field in the log record format.
**
**	The initialization of the global LSN is still a little unclear:
**
**	If we initialize it using a TMget_stamp, then we have the
**	problem that the timestamps on different nodes might be
**	out of sync.  Also the checking of the LSN against what is
**	stored in the local log file does not tell us what the LSN's
**	might be in other log files.
**
**	If we are using a lock for the LSN values, if a node fails
**	while holding the lock, then the value of the lock will be
**	lost and the next getstamp call will get back VALNOTVALID.
**	In this case we have to generate a new LSN on the fly.
**
**	We should continue to use the TMget_stamp mechanisms,
**	but perhaps we need to make more guarantees about out-of-date
**	timestamps accross the nodes.
**
**	(we could poll the nodes for the highest valued timestamp
**	on each node, but this requires that all the nodes
**	be up - and we don't currently require that all nodes be
**	joined into the ingres cluster in order to start the installation.)
**
**	We should probably look at continuing to use the timer mechanism
**	for our cluster product.  In this mechanism, each node keeps its
**	notion of the current LSN up to date at each 10 second interval.
**	So even if a node is inactive, it will generally tend to keep
**	in sync (as long as it is alive).  So if a node dies while
**	holding the LSN lock and we get valnotvalid on the next update
**	request, we know that we cannot be more than 10 seconds out of date.
**	So we should just make sure that the new TMget_stamp return is
**	at least 10 seconds greater than the previous LSN.
**
**	On system restart, we have to make sure to pick an LSN that will
**	be greater than the last LSN used on ANY INGRES NODE.  It may
**	be that by just using the TMget_stamp call, we will be making
**	the assumption that two nodes cannot get so wildly out of date
**	that the Ingres installation could be brought down and then
**	restarted in less time than the out-of-date interval.  Not sure
**	if this is a bad assumption.
**
**	---- End description as of pre-103715 ----
**
**	It is believed that the above discription is still generally valid,
**	especially that part of the discussion reguarding handling of
**	initial LSN generation, and effect of invalid lock values.
**
**	Implementation of synchronization mechanism is now encapsulated
**	within CX CMO, which allows alternate implementations where are
**	practical.  (E.g.  IMC under Tru64 UNIX).
**
**
**  History:
**	26-apr-1993 (bryanp/keving)
**	    Created as part of building Cluster Recovery.
**	    There are many problems in this code still. At the top of this file
**	    is a list.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Added preliminary handling of VALNOTVALID. Just enough to allow
**		continued testing. "real" generation of new values doesn't work.
**	    Include <tr.h>
**	17-jul-1995 (duursma)
**	    Set the LCKM$_SYSTEM attribute on the SYS$ENQW() call so that the
**	    lock block joins the system resource domain.  This avoids the 
**	    situation observed in bug #69615 where the installation owner 
**	    creates a lock in his group's resource domain; later locks requested
**	    by the user during rollforwarddb processing are in a different
**	    resource domain, causing LSN's to get out of sync, which causes
**	    certain REDO operations to be ignored, leading to the symptoms
**	    seen in bug #69615.
**	    To ensure uniquess at the installation level, we append the two
**	    character installation ID to the original lock name, turning
**	    IICLUSTERSTAMP into eg. IICLUSTERSTAMP_XX.  
**	    As an aside, note that the dsc$w_length field had always been
**	    set incorrectly to 13 instead of 14.  This caused the actual
**	    lock name to show up as IICLUSTERSTAM instead of IICLUSTERSTAMP.
**	    Avoided further such problems by taking the length from the
**	    string literal definition.
**	20-jul-1995 (duursma)
**	    Fixed typo in previous change.
**	    Also, added paranoid LGlsn_init() call to LGlsn_next()
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	01-may-2001 (devjo01)
**	    LSN generation now uses CX CMO.   Remove all VMS specific
**	    code.   LSN algorithms kept intact with all their alarming
**	    cavaets.
**	17-sep-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	06-oct-2003 (devjo01)
**	    Unsubmitted change of 01-may-2001 trumped change of 17-sep-2003,
**	    sorry Joe.
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
*/


# define LG_LSN_CMO		(CX_CMO_RESERVED + 0)
# define LG_LSN_MEM_MARK	(7 << 24)
# define LG_LSN_MARK_MASK	(0xff << 24)
# define LG_LSN_BUMP		1000000

static void	fabricate( LG_LSN *value );

static STATUS	lsn_update( CX_CMO *oldval, CX_CMO *pnewval,
  PTR punused, bool invalidin );


/*
** Name: LGlsn_init	    - Initialize the Log Sequence Number system
**
** Description:
**	This routine is called when a process starts up, to initialize its
**	connection to the Log Sequence Number system.
**
** Inputs:
**	None
**
** Outputs:
**	sys_err		    - system-specific error code
**
** Returns:
**	STATUS
**
** History:
**	26-apr-1993 (bryanp)
**	    Created.
**	17-jul-1995 (duursma)
**	    Set the LCKM$_SYSTEM attribute on the SYS$ENQW() call so that the
**	    lock block joins the system resource domain.  This avoids the 
**	    situation observed in bug #69615 where the installation owner 
**	    creates a lock in his group's resource domain; later locks requested
**	    by the user during rollforwarddb processing are in a different
**	    resource domain, causing LSN's to get out of sync, which causes
**	    certain REDO operations to be ignored, leading to the symptoms
**	    seen in bug #69615.
**	    To ensure uniquess at the installation level, we append the two
**	    character installation ID to the original lock name, turning
**	    IICLUSTERSTAMP into eg. IICLUSTERSTAMP_XX.  
**	    As an aside, note that the dsc$w_length field had always been
**	    set incorrectly to 13 instead of 14.  This caused the actual
**	    lock name to show up as IICLUSTERSTAM instead of IICLUSTERSTAMP.
**	    Avoided further such problems by computing the length using 
**	    STlength(), since it is no longer a constant.
**	20-jul-1995 (duursma)
**	    Added paranoid LGlsn_init() call
**	01-may-2001 (devjo01)
**	    LSN generation now uses CX CMO, so this function is now a dummy.
*/
STATUS
LGlsn_init(CL_ERR_DESC *sys_err)
{
    CL_CLEAR_ERR(sys_err);
    return (OK);
}

/*
** Name: LGlsn_term	    - Terminate the Log Sequence Number system
**
** Description:
**	This routine is called when a process shuts down, to terminate its
**	connection to the Log Sequence Number system.
**
** Inputs:
**	None
**
** Outputs:
**	sys_err		    - system-specific error code
**
** Returns:
**	STATUS
**
** History:
**	26-apr-1993 (bryanp)
**	    Created.
**	01-may-2001 (devjo01)
**	    LSN generation now uses CX CMO, so this function is now a dummy.
*/
STATUS
LGlsn_term(CL_ERR_DESC *sys_err)
{
    CL_CLEAR_ERR(sys_err);
    return (OK);
}

/*
** Name: LGlsn_next	    - Get the next Log Sequence Number
**
** Description:
**	This routine returns the next Log Sequence Number.
**
**	If VMS tells us that the lock value is not valid, we generate a new lock
**	value and use it for the LSN. Note that we do this using our LOCAL
**	clock, which may be buggy in the cluster. A better solution would
**	involve some sort of timer-based scheme by which we can track a "recent"
**	LSN, and then generate a value using that -- see comments in the top of
**	this file. Another possibility would be to read log files.
**
**	Currently, as a minimal sort of check against out-of-sync clocks, we
**	check the new value that we generate to see if it's less than the value
**	that VMS gave us in the lock (even though VMS says value-not-value, it
**	still often gives us its best guess at the last-known-good value). If
**	our new value is less than the current VMS lock value, we use the
**	current VMS lock value plus one million.
**
** Inputs:
**	None
**
** Outputs:
**	lsn		- set to contain the next Log Sequence Number
**	sys_err		- set if an error occurs. (now unused.)
**
** Returns:
**	OK		- All is well, new LSN is valid.
**	E_DMA484	- LSN update failure.
**
** History:
**	16-feb-1993 (bryanp)
**	    Created.
**	26-apr-1993 (bryanp)
**	    Tuned up code, got ready for first integration. Added comments.
**	    Added error-checking. Log errors that occur.
**	26-jul-1993 (bryanp)
**	    Added preliminary handling of VALNOTVALID. Just enough to allow
**		continued testing. "real" generation of new values doesn't work.
**		The problem with the current code is that we generate a new
**		LSN value using our local clock. However, clocks on the cluster
**		are not kept in sync, and hence our clock may be sufficiently
**		behind that we end up generating a new LSN which has a lower
**		value than some previously used LSN. At some point, we need to
**		fix this.
**	20-jul-1995 (duursma)
**	    Fixed typo in previous change
**	01-may-2001 (devjo01)
**	    LSN generation now uses CX CMO.   Much of the logic which
**	    was here is now moved into lsn_update, which is invoked 
**	    by CX CMO once exclusive access is obtained on CMO value,
**	    which may be implemented as a shared lock value, as under VMS,
**	    or a special memory region like IMC under tru64.
*/
STATUS
LGlsn_next(LG_LSN   *lsn, CL_ERR_DESC *sys_err)
{
    STATUS	status;
    union 
    {
	CX_CMO		cmo;
	LG_LSN		lsn;
    }		value;
    i4	err_code;

    CL_CLEAR_ERR(sys_err);

    status = CXcmo_update( LG_LSN_CMO, &value.cmo, lsn_update, NULL );
    if ( status )
    {
	uleFormat(NULL, status, NULL, ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0 );
	status = E_DMA484_LG_LSN_NEXT_FAIL;
    }
    else
    {
	lsn->lsn_high = value.lsn.lsn_high;
	lsn->lsn_low = value.lsn.lsn_low;
    }
    return (status);
}

/*
** Name: lsn_update	    - CMO callback to do actual update.
**
** Description:
**	This routine generaes the new LSN based on the current LSN
**	while CMO holding LSN is under exclusive control.  Will also
**	handle cases of LSN becoming invalidated, or LSN initialization.
**
** Inputs:
**	poldval		- Address of CMO holding existing LSN.
**	invalidin	- TRUE if value of LSN in poldval is suspect
**			  because an updater died mid-update.
**
** Outputs:
**	*pnewval 	- Filled with new LSN value.
**
** Returns:
**	OK		- Always.
**
** History:
**	01-may-2001 (devjo01)
**	    Created.
*/
static STATUS
lsn_update( CX_CMO *poldval, CX_CMO *pnewval, PTR punused, bool invalidin )
{
    LG_LSN	*poldlsn, *pnewlsn;

    poldlsn = (LG_LSN *)poldval;
    pnewlsn = (LG_LSN *)pnewval;

    if ( invalidin )
    {
	/*
	** LSN value was invalidated, currently only possible with DLM CMO. 
	**
	** Generate a new value.
	*/
	fabricate(pnewlsn);

	/* Double check against "invalid" value.  Must ALWAYS increase. */

	/* s103715 note:  I've added a sanity check to make sure lock
	** value block is "reasonable".  On VMS & Tru64, it seems that
	** value will still reflect the latest "committed" value, but
	** if it were possible for "junk" to get in here, we could
	** wind up generating a ridiculously high LSN, which could
	** cause trouble during the next cold start.
	*/
	if ( LG_LSN_MEM_MARK == (poldlsn->lsn_high & LG_LSN_MARK_MASK) )
	{
	    /* old value passed simplistic sanity check. */
	    if ( pnewlsn->lsn_high < poldlsn->lsn_high ||
		 (pnewlsn->lsn_high == poldlsn->lsn_high &&
		  pnewlsn->lsn_low <= poldlsn->lsn_low) )
	    {
		/*
		** New value, may be too low, generate a new value
		** based on old value plus a bunch.
		*/
		pnewlsn->lsn_high = poldlsn->lsn_high;
		if ( (poldlsn->lsn_low + LG_LSN_BUMP) < poldlsn->lsn_low )
		{
		    /* Adding a million causes a carry, bump lsn_high */
		    pnewlsn->lsn_high ++;
		}
		pnewlsn->lsn_low = poldlsn->lsn_low + LG_LSN_BUMP;
	    }
	}
    }
    else if ( 0 == poldlsn->lsn_high && 0 == poldlsn->lsn_low )
    {
	/* LSN unitialized, set initial value. */
	fabricate(pnewlsn);
    }
    else 
    {
	/* Just increment value */
	pnewlsn->lsn_high = poldlsn->lsn_high;
	if ( 0 == ( pnewlsn->lsn_low = poldlsn->lsn_low + 1 ) )
	    pnewlsn->lsn_high++;
    }
    return OK;
} /*lsn_update*/


/*
** Name: fabricate			- initial LSN value fabricator.
**
** Description:
**	This routine fabricates an 8 byte integer. If we let byte 0 be the
**	most significant byte, and byte 7 be the least significant byte, then
**	the current LSN value fabricator forms the 8 byte integer as follows:
**
**	 byte 0      bytes 1-4                    bytes 5-7
**	+------+--------------------------------+-----------+
**	| 0x07 | seconds since Jan 1, 1970      |  0x000000 |
**	+------+--------------------------------+-----------+
**
**	The 0x07 in the top byte just makes LSN's look "interesting", and makes
**	it much less likely that you'll confuse them with local log addresses.
**
**	We have 24 bits at the bottom of the LSN: this means that we are
**	asserting that we could never generate more than 2^24 (16 million)
**	LSN's per second. That's probably more than safe -- 20 bits (1 million)
**	is probably fine.
**
**	If all the clocks on the cluster were in sync, this algorithm would
**	allow any node to confidently generate a new LSN value at any time,
**	which is important since the VMS lock manager may re-master the lock and
**	lose the lock value at any time. However, the cluster clocks are NOT
**	in sync, which means that the current algorithm doesn't always work
**	properly.
**
** History:
**	26-jul-1993 (bryanp)
**	    Broke into its own routine, documented.
**	01-may-2001 (devjo01)
**	    Make fabricate operate on LG_LSN type.
*/
static void
fabricate( LG_LSN *value )
{
    u_i4	timestamp;

    /*
    ** If lock value is no good, fabricate a new 
    ** large enough number (more than the number
    ** of LGwrites possible in time interval to generate 
    ** new largest number).
    */
    /* For now, make lsns distinctive by setting top byte
    ** to 7
    */
    timestamp = TMsecs();

    TRdisplay("Assign new LSN: Time stamp is <%x>\n", timestamp);

    value->lsn_high = ((timestamp >> 8) & ~LG_LSN_MARK_MASK) + LG_LSN_MEM_MARK;

    value->lsn_low = (timestamp << 24) & 0xff000000;

    TRdisplay("Initial LSN: <%x,%x>\n", value->lsn_high, value->lsn_low );

    return;
}
