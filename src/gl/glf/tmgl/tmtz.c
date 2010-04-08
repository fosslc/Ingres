/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <tm.h>
#include    <tmtz.h>
#include    <st.h>
#include    <me.h>
#include    <si.h>
#include    <lo.h>
#include    <nm.h>
#include    <er.h>
#include    <pm.h>
#include    <cs.h>
#include    <cv.h>

static  PTR TM_tz_default = 0;


/**
** Name: TMTZ.C - Timezone handling routines
**
** Description:
**      This file contains the following TMtz routines:
**    
**      TMtz_init()        -  Returns the pointer to the default TM_TZ_CB
**      TMtz_lookup()      -  Find the timezone data structure TM_TZ_CB for
**                            a given timezone name.
**      TMtz_load()        -  Load the timezone information file.
**      TMtz_search()      -  Find the timezone offset for a given time value.
**	TMtz_year_cutoff() -  Find the value of II_DATE_CENTURY_BOUNDARY.
**
**      25-sep-1992 (stevet)
**          Initial Creation.
**      01-dec-1992 (stevet)
**          Modified the TMtz_init and TMtz_load routines which now lookup
**          timezone information file using the PM resource file 
**          'config.dat'.
**      15-sep-1993 (stevet)
**          Replace LOaddpath() with LOfaddpath() to support timezone
**          files in zoneinfo sub-directories.
**	13-nov-1995 (nick)
**	    Add TMtz_year_cutoff().
**	22-apr-1996 (lawst01)
**	   Windows 3.1 port changes - added casts of fcn arguments.
**      03-jun-1996 (canor01)
**          New ME for operating-system threads does not need external
**          semaphores. Removed ME_stream_sem. Removed NMloc() semaphore.
**	12-sep-96 (nick)
**	    Altered TMtz_year_cutoff() to return the setting of 
**	    II_DATE_CENTURY when we can convert it to an integer but it
**	    falls outside the permissible range.
**      28-feb-1997 (kitch01)                                                
**          Bug 79975. Fix in TMtz_search for DST start/end problems. 
**          Bug 59988 is also corrected by this fix.
**	10-oct-1996 (canor01)
**	    Use generic SystemCfgPrefix instead of 'ii'.
**      29-Aug-97 (hanal04)
**              Load the config.dat file in II_CONFIG_LOCAL if     
**              II_CONFIG_LOCAL is set (Re: b78563).
**      21-Jul-98 (hanal04)
**          Moved the above change into PMmLoad(). b91480.
**	14-aug-1998 (canor01)
**	    Initialize the tz_name buffer to prevent access violation when
**	    timezone name not set.
**      21-apr-1999 (hanch04)
**          Replace STindex with STchr
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	06-Jan-2000 (kitch01)
**		Bug 99917. If the time is exactly the time that DST begins the
**		the incorrect GMT offset is returned.
**	07-aug-2001 (somsa01)
**	    In TMtz_Load(), perform alignment based upon ALIGN_RESTRICT.
**	25-oct-2001 (somsa01)
**	    In TMtz_Load(), perform alignment based upon ALIGN_RESTRICT
**	    ONLY if LP64 is set.
**	07-nov-2001 (devjo01)
**	    Rework previous logic to make Tru64 happy.  New code
**	    is generic removing need for LP64 conditional section.
**	09-Jan-2002 (hanje04)
**	    In TMtz_Load(), ALIGN_RESTRICT should not be used to cast lp,
**	    (causes problem on IA64 Linux) use long instead.
**      16-May-2002 (hanch04)
**          Added lp64 directory for 32/64 bit builds.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      07-Mar-2005 (hanje04)
**          SIR 114034
**          Add support for reverse hybrid builds. First part of much wider
**          change to allow support of 32bit clients on pure 64bit platforms.
**          Changes made here allow application statically linked against
**          the standard 32bit Intel Linux release to run against the new
**          reverse hybrid ports without relinking.
*/


/*{
** Name: TMtz_init - Returns the pointer to the default TM_TZ_CB.
**
** Description:
**    	This routine returns a pointer to the default TM_TZ_CB 
**     	structure.  If the default TM_TZ_CB is not loaded, this
**    	routine gets the II_TIMEZONE_NAME value and pass to 
**    	TMtz_load() to load the the timezone information file into memory.
**
** Inputs:
**      none
**
** Outputs:
**      tz_cb                         PTR to the default TM_TZ_CB.
**
**	Returns:
**    	    TM_NO_TZNAME
**    	    TM_NO_TZFILE
**    	    TM_TZFILE_OPNERR
**    	    TM_TZFILE_BAD
**    	    TM_TZFILE_NOMEM
**          TM_PMFILE_OPNERR
**          TM_PMFILE_BAD
**          TM_PMNAME_BAD
**          TM_PMVALUE_BAD
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-sep-1992 (stevet)
**          Initial creation
**      02-dec-1992 (stevet)
**          Added initialization for PM.  Added support for IIGMT,
**          which set timezone offset to zero and bypassing the PM
**          resource lookup.  
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      29-Aug-97 (hanal04)
**              Load the config.dat file in II_CONFIG_LOCAL if     
**              II_CONFIG_LOCAL is set (Re: b78563).
**      26-Jul-98 (hanal04)
**          Moved the above change into PMmLoad(). b91480.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
STATUS
TMtz_init(PTR *cb)
{
    PTR    tz_def;
    char   tz_name[TM_MAX_TZNAME+1];
    STATUS status=OK, status2=OK;
    
    tz_name[0] = '\0';

    if( TM_tz_default == NULL)
    {
	do
	{
	    NMgtAt(ERx("II_TIMEZONE_NAME"), &tz_def);
	    if (!tz_def || !(*tz_def))
	    {
		status =TM_NO_TZNAME;
		break;
	    }
	    STncpy(tz_name, tz_def, TM_MAX_TZNAME);
	    tz_name[ TM_MAX_TZNAME ] = EOS;

	    /* Initialize the PM resource file for timezone */
	    PMinit();

	    status = PMload( NULL, (PM_ERR_FUNC *)NULL);

	    /*
	    ** Load the PM resource file for mappings of timezone name and 
	    ** and timezone information files
	    */
	    switch(status)
	    {
	      case OK:
		/* loaded successfully */
		status = TMtz_load(tz_name, &TM_tz_default);
		break;
	      case PM_FILE_BAD:
		status = TM_PMFILE_BAD;
		break;
	      default:
		status = TM_PMFILE_OPNERR;
		break;
	    }
	} while( FALSE);


	/*
	** If II_TIMEZONE_NAME="IIGMT", we should ignore error and
	** allow to move on even we got a PM error
	*/
	if( TM_tz_default == NULL &&
	   STcasecmp( ERx("IIGMT"), tz_name ) == 0)
	{
	    /*
	    ** Set up IIGMT timezone structure
	    */
	    TM_tz_default = MEreqmem( 0, sizeof(TM_TZ_CB) , FALSE, &status2);
	    if( status2 == OK)
	    {
		TM_TZ_CB  *tz_cb=(TM_TZ_CB *)TM_tz_default;

		tz_cb->timecnt = 0;
		tz_cb->tzinfo[0].gmtoff = 0;
		tz_cb->next_tz_cb = NULL;
		STcopy(ERx("IIGMT"), tz_cb->tzname);
		status = OK;
	    }
	    else
		status = status2;
	}
    }
    *cb = TM_tz_default;
    return(status);
}



/*{
** Name: TMtz_lookup - Find the timezone data structure TM_TZ_CB for a 
**                     given timezone name.
**
** Description:
**    	This routine lookup the internal TM_TZ_CB link list 
**    	structure and returns the PTR to the TM_TZ_CB if timezone name
**    	is found.  Otherwise, it returns TM_TZLKUP_FAIL.  No read semaphore 
**    	is required since the TM_TZ_CB link list structure is guaranteed
**    	to be in good order during the load operation.
**
** Inputs:
**      tz_name                    NULL terminate string that contains
**                                 the timezone name.
**
** Outputs:
**      cb                         PTR to the TM_TZ_CB.
**
**	Returns:
**    	    TM_NO_TZNAME
**          TM_TZLKUP_FAIL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-sep-1992 (stevet)
**          Initial creation.
**      02-dec-1992 (stevet)
**          Changed STbcompare() to pass zero lengths so that
**          full string will be compared.
*/
STATUS
TMtz_lookup( char *tz_name, PTR *cb)
{
    TM_TZ_CB *tz_cb;

    if( TM_tz_default == NULL)
	return(TM_NO_TZNAME);

    tz_cb = (TM_TZ_CB *)TM_tz_default;

    while(tz_cb != NULL)
    {
	*cb = (PTR)tz_cb;
	if( STcasecmp( tz_cb->tzname, tz_name ) == 0)
	    return(OK);
	else
	    tz_cb = (TM_TZ_CB *)tz_cb->next_tz_cb;
    }
    return(TM_TZLKUP_FAIL);
}


/*{
** Name: TMtz_load - Load the timezone information file.
**
** Description:

**    	This routine finds the corresponding timezone information file
**    	for the given timezone name by calling the PM routine.  TM_tz_load 
**    	then open and read the timezone information file using SI 
**    	routines and allocates the required memory using MEreqmem() before
**    	populating and return the pointer to the TM_TZ_CB structure.  
**
**    	This routine guarantees that the internal TM_TZ_CB link list 
**    	structure will be safe for read operation without the need
**    	for read semaphore (i.e. no pointer to un-initialized TM_TZ_CB
**    	structure during the load operation).  In addition, no duplicate
**    	version of timezone information file will be loaded even
**    	in highly concurrent environment since this routine lookup
**    	the internal TM_TZ_CB link list structure for matching 
**    	timezone name before the load operation. 
**
**    	Since this routine updates the internal TM_TZ_CB link list 
**    	structure, calling client have to make sure semaphore
**    	protection is in place in a multi-tread environment.
**
** Inputs:
**      tz_name                    NULL terminate string that contains
**                                 the timezone name.
**
** Outputs:
**      cb                         PTR to the TM_TZ_CB structure.
**
**	Returns:
**    	    TM_NO_TZFILE
**    	    TM_TZFILE_OPNERR
**    	    TM_TZFILE_BAD
**    	    TM_TZFILE_NOMEM
**          TM_PMNAME_BAD
**          TM_PMVALUE_BAD
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-sep-1992 (stevet)
**          Initial creation
**      01-dec-1992 (stevet)
**          Lookup PM resource file for the name of timezone information
**          file.
**      03-mar-1993 (stevet)
**          Put directory name to location buffer right after NMloc call
**          to reduce the risk of the unlikely event of being switch out 
**          and end up with corrupted buffer.
**      29-mar-93 (swm)
**          Corrected assignment of buf address to tm_tz_cb (ie. assign
**          pointer to buf, not address of pointer).
**	07-aug-2001 (somsa01)
**	    Perform alignment based upon ALIGN_RESTRICT.
**	07-nov-2001 (devjo01)
**	    Rework previous logic to make Tru64 happy.  New code
**	    is generic removing need for LP64 conditional section.
**      07-Mar-2005 (hanje04)
**          SIR 114034
**          Add support for reverse hybrid builds. First part of much wider
**          change to allow support of 32bit clients on pure 64bit platforms.
**          Changes made here allow application statically linked against
**          the standard 32bit Intel Linux release to run against the new
**          reverse hybrid ports without relinking.
**      07-Mar-2005 (fanch01)
**          BUG 114996
**          Fix memory stomping bug with for some combinations of timezones
**          and pointer values.  Remove LP64 conditional, rely on provided
**          alignment macros for alignment.
**	17-Feb-2006 (jenjo02)
**	    Fix to fanch01 fix, above. "buf[0]" may be unaligned,
**	    yielding BUS errors on assignment of
**	    timecnt = tm_tz_cb->timecnt.
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
*/
STATUS
TMtz_load( char *tz_name, PTR *cb)
{
    char        buf[sizeof(TM_TZ_CB)+TM_MAX_TIMECNT*(sizeof(i4)+1)];
    char        tz_pmvalue[MAX_LOC + 1];
    char        tz_pmname[MAX_LOC + 1];
    char        *p, *ip, *pm_value;
    char        chr='/';
    char        loc_buf[MAX_LOC + 1];
    FILE        *fid;
    LOCATION    loc_root, tmp_loc;
    i4          i, sz_tzcb, sz_tztype, sz_tzdate;
    i4          timecnt;
    i4          slop;
    TM_TZ_CB    *tm_tz_cb;
    PTR         last_ptr=NULL;
    STATUS      status=OK;

    /* 
    ** If the default timezone structure already set up, call TMtz_lookup 
    ** to get the last TM_TZ_CB structure
    */
    if( TM_tz_default != NULL)
    {
	status = TMtz_lookup(tz_name, &last_ptr);
	if( status == OK)
	{
	    /* TZ already loaded */
	    *cb = last_ptr;
	    return( OK);
	}
	else if( status != TM_TZLKUP_FAIL)
	{
	    return( status);
	}
    }

    /*
    ** Use PMget to look up the TZ file from TZ name 
    */
    STprintf( tz_pmname, ERx("%s.*.tz.%s"), SystemCfgPrefix, tz_name);
    if( PMget( tz_pmname, &pm_value) != OK)
	return( TM_PMNAME_BAD);

    /*
    ** Create the full path and directory name for the timezone information
    ** file
    */

    do
    {
	status = NMloc(FILES, PATH, ERx("zoneinfo"), &loc_root);
	if ( status != OK )
	{
	   break;
	}
#if defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH64)
	if((status = LOfaddpath(&loc_root, ERx("lp64"), &loc_root)) != OK)
	    break;
#endif
#if defined(conf_BUILD_ARCH_64_32) && defined(BUILD_ARCH32)
	{
	    /*
	    ** Reverse hybrid support must be available in ALL
	    ** 32bit binaries
	    */
	    char    *rhbsup;
	    NMgtAt("II_LP32_ENABLED", &rhbsup);
	    if ( (rhbsup && *rhbsup) &&
	    ( !(STbcompare(rhbsup, 0, "ON", 0, TRUE)) ||
	    !(STbcompare(rhbsup, 0, "TRUE", 0, TRUE))))
	    status = LOfaddpath(&loc_root, "lp32", &loc_root);
	}
#endif /* reverse hybrid */


        LOcopy( &loc_root, loc_buf, &loc_root);
	STcopy( pm_value, tz_pmvalue);

	/*
	** Compose the directory path 
        */
        for( p = tz_pmvalue, ip = tz_pmvalue; 
	     (p = STchr(ip, chr)) != NULL;)
	{
	    *p = EOS;
	    if((status = LOfaddpath(&loc_root, ip, &loc_root)) != OK)
		break;
	    ip = CMnext(p);
	}
	   
	/* 
	** Add file name to the directory path
	*/
	if((status = LOfroms(FILENAME, ip, &tmp_loc)) != OK)
	    break;
	status = LOstfile( &tmp_loc, &loc_root);
    } while( FALSE);

    if( status != OK)
	return( TM_PMVALUE_BAD);

    /*
    ** Now open the timezone information file
    */
    if( SIfopen( &loc_root, ERx("r"), SI_VAR, sizeof buf, &fid) != OK)
	return(TM_TZFILE_OPNERR);

    status = SIread(fid, sizeof buf, &i, buf);
    if( SIclose(fid) != OK || i < sizeof(TM_TZ_CB))
	return(TM_TZFILE_BAD);

    tm_tz_cb = (TM_TZ_CB *)&buf[0];
    /* &buf[0] may be unaligned, avoid BUS */
    I4ASSIGN_MACRO( tm_tz_cb->timecnt, timecnt );

    /* Make sure the input file has correct file size */
    if( timecnt > TM_MAX_TIMECNT || timecnt < 0
        || i != (i4)(sizeof(TM_TZ_CB) + timecnt*(sizeof(i4)+1)))
	return(TM_TZFILE_BAD);

    sz_tzcb = sizeof(TM_TZ_CB);
    if( sz_tzcb % sizeof(ALIGN_RESTRICT))
	sz_tzcb += (sizeof(ALIGN_RESTRICT) - (sz_tzcb % sizeof(ALIGN_RESTRICT)));

    sz_tztype = timecnt;
    if( sz_tztype % sizeof(ALIGN_RESTRICT))
	sz_tztype += (sizeof(ALIGN_RESTRICT) - (sz_tztype % sizeof(ALIGN_RESTRICT)));

    sz_tzdate = sizeof(i4) * timecnt;

    tm_tz_cb = (TM_TZ_CB *)(MEreqmem( 0, sz_tzcb + sz_tztype + sz_tzdate, FALSE, &status));

    if( status != OK)
	return(TM_TZFILE_NOMEM);
    
    /* Memory is aligned so no alignment is needed */
    MEcopy( (PTR)buf, sizeof(TM_TZ_CB), (PTR)tm_tz_cb); 

    if( timecnt > 0)
    {
	p = buf + sizeof(TM_TZ_CB);
	ip = (char *)tm_tz_cb + sizeof(TM_TZ_CB);

	/* Copy the type array to memory */
	MEcopy( (PTR)p, timecnt, (PTR)ip);
	tm_tz_cb->tm_tztype = (char *)ip;

	/* Align buffer before moving time value */
	ip = ME_ALIGN_MACRO(ip + timecnt, sizeof(ALIGN_RESTRICT));
	p += timecnt;
	MEcopy( (PTR)p, timecnt * sizeof(i4), (PTR)ip);
	tm_tz_cb->tm_tztime = (i4 *)ip;
    }
    else
    {
	tm_tz_cb->tm_tztype = NULL;
	tm_tz_cb->tm_tztime = NULL;
    }
    tm_tz_cb->next_tz_cb = NULL;
    STcopy(tz_name, tm_tz_cb->tzname);

    *cb = (PTR)tm_tz_cb;
    /* Set up the link list structure */
    
    if(last_ptr != NULL)
	((TM_TZ_CB *)last_ptr)->next_tz_cb = (PTR)tm_tz_cb; 
    return(OK);
}


#define TM_tz_adj(val) (tm_timetype == TM_TIMETYPE_GMT ? 0 : tm_tz_cb->tzinfo[type[val]].gmtoff)


/*{
** Name: TMtz_search - Find the timezone offset for a given time value.
**
** Description:
**    	This routine returns the number of seconds from GMT by 
**    	searching the timezone data structure TM_TZ_CB point to by 
**    	tz_cb according to input timevalue and the timetype (TM_TIMETYPE_GMT 
**    	or TM_TIMETYPE_LOCAL).  
**
** Inputs:
**      cb                         TM_TZ_CB to be used for the search.
**      tm_timetype                Input time type:
**                                   TM_TIMETYPE_GMT   - input time is GMT 
**                                   TM_TIMETYPE_LOCAL - input time is LOCAL time
**      tm_time                    Time value in number of second since
**                                 1/1/70 00:00 GMT.
**
** Outputs:
**
**	Returns:
**    	    Timezone offset from GMT.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-sep-1992 (stevet)
**          Initial creation
**      09-dec-1992 (stevet)
**          Return 0 when TM_TZ_CB is NULL
**	28-feb-1997 (kitch01)
**	    Bug 79975. In the hour before DST ends date_gmt('now') returns
**	    an incorrect value. This is because we were using the wrong
**	    offset value during comparison. For DST end we were using the 
**	    non-DST offset which meant the DST was ending 1 hour earlier
**	    than it should. I used the offset calculation/comparison that
**	    is used by iizck to generate the start/end times.
**	    Further corrections were required to compensate for the 'missing'
**	    hour when DST starts.
**	    Bug 59988 is also corrected by this fix.
**	06-Jan-2000 (kitch01)
**		Bug 99917. If the time is exactly the time that DST begins the
**		the incorrect GMT offset is returned.
*/
i4
TMtz_search( PTR cb, TM_TIMETYPE tm_timetype, i4 tm_time)
{
    TM_TZ_CB     *tm_tz_cb=(TM_TZ_CB *)cb;
    i4           i;
    i4           *dt; 
    char         *type;
    i4           tztype = 0, low=0, mid=0;
    i4           high;
    i4           defoff, count;

    if(cb == NULL)
	return(0);

    i = tm_tz_cb->timecnt;
    dt = tm_tz_cb->tm_tztime;
    type = tm_tz_cb->tm_tztype;
    high = tm_tz_cb->timecnt-1;

    /* If fix offset, just return the fix offset value */
    if(tm_tz_cb->timecnt == 0)
	return(tm_tz_cb->tzinfo[0].gmtoff);

    /* If out of bound.  Return standard offset */
    if(tm_time < dt[low] + TM_tz_adj(low) 
       || tm_time > dt[high] + TM_tz_adj(high))
    {
	for(count=0; tm_tz_cb->tzinfo[count].isdst != '\0' 
	    && count < TM_MAX_TZTYPE; count++)
	    ;
	return(tm_tz_cb->tzinfo[count].gmtoff);
    }

    /* Binary search */
    while(low <= high)
    {
	mid = (low+high)/2;
	defoff = mid ? mid-1 : 0;
	if(tm_time < dt[mid] + TM_tz_adj(defoff))
	    high = mid - 1;
	else
	{
	    if (tm_time > dt[mid+1] + TM_tz_adj(mid))
		low = mid + 1;
	    else
	    {
		/* Solely to check for the 'missing' hour at the start
		** of DST. If the local date given is in this hour then
		** adjust it to DST time. Any local time in this hour
		** does not, in reality, exist!
		** This is required because the fix for 79975 'broke' this
		** adjustment.
		*/
		/* Minor adjustment from bug 99917, 
		** make the test greater-than-or-equal-to
		*/
		if (tm_timetype &&
		    (tm_time < dt[mid] + TM_tz_adj(mid)
		     && tm_time >= dt[mid] + TM_tz_adj(defoff)) )
		{
		    tztype = type[mid+1];
		}
		else
		{
		   /* Take care of boundary conditions */
		   if( tm_time != dt[mid+1] + TM_tz_adj(mid))
		       tztype = type[mid];
		   else
		   {
		     /* To maintain compatibility with dates prior to my
		     ** fix for 79775, this check is required to leap forward
		     ** the time that is execatly on the DST boundary.
		     */
		       if (tm_timetype && tm_tz_cb->tzinfo[type[mid+1]].isdst)
			   tztype = type[mid];
		       else
		           tztype = type[mid+1];
		   }
		}
		break;
	    }
	}
    }

    return( tm_tz_cb->tzinfo[tztype].gmtoff);
}

/*{
** Name: TMtz_year_cutoff - Find the value of II_DATE_CENTURY_BOUNDARY.
**
** Description:
**
** Inputs:
**
** Outputs:
**
**	year_cutoff	The date_century boundary if set.
**
**	Returns:
**	    OK
**	    TM_YEARCUT_BAD
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    First call sets the server's value.
**
** History:
**      13-nov-1995 (nick)
**	    Created.
**	12-sep-96 (nick)
**	    Altered logic slightly so that a valid integer but outside the
**	    permissible range is reported back along with the fail status
**	    so that it gets printed correctly in the subsequent error message.
**	    Previously, we'd always send back TM_DEF_YEAR_CUTOFF on failure.
*/
STATUS
TMtz_year_cutoff(i4 *year_cutoff)
{
    STATUS  status = OK;

    static  bool year_cutoff_set = FALSE;
    static  i4   year_cutoff_default = TM_DEF_YEAR_CUTOFF;

    if (!year_cutoff_set)
    {
    	char	*date_century_boundary;

    	NMgtAt(ERx("II_DATE_CENTURY_BOUNDARY"), &date_century_boundary);
    	if (date_century_boundary && *date_century_boundary)
    	{
	    i4  date_century;

	    if (CVan(date_century_boundary, &date_century) == OK)
	    {
		year_cutoff_default = date_century;
		if ((date_century <= TM_DEF_YEAR_CUTOFF) ||
		    (date_century > TM_MAX_YEAR_CUTOFF))
		{
		    status = TM_YEARCUT_BAD;
		}
	    }
	    else
	    {
	    	status = TM_YEARCUT_BAD;
	    }
    	}
	year_cutoff_set = TRUE;
    }


    *year_cutoff = year_cutoff_default;
    return(status);
}

/*{
** Name: TMtz_isdst	- determine if date lies in daylight savings.
**
** Description:
**    	This function returns 1 if the date passed is in daylight savings
**	for its timezone, otherwise 0.
**
** Inputs:
**      cb                         TM_TZ_CB to be used for the search.
**      tm_time                    Time value in number of second since
**                                 1/1/70 00:00 GMT.
**
** Outputs:
**
**	Returns:
**    	    1 if tm_time is in DST, otherwise 0.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	13-apr-06 (dougi)
**	    Written to support isdst() scalar function (though largely
**	    cloned from TMtz_search()).
*/
i4
TMtz_isdst( PTR cb, i4 tm_time)
{
    TM_TZ_CB     *tm_tz_cb=(TM_TZ_CB *)cb;
    TM_TIMETYPE	 tm_timetype = TM_TIMETYPE_LOCAL;
    i4           i;
    i4           *dt; 
    char         *type;
    i4           tztype = 0, low=0, mid=0;
    i4           high;
    i4           defoff, count;

    if(cb == NULL)
	return(0);

    i = tm_tz_cb->timecnt;
    dt = tm_tz_cb->tm_tztime;
    type = tm_tz_cb->tm_tztype;
    high = tm_tz_cb->timecnt-1;

    /* If fix offset, just return 0. */
    if(tm_tz_cb->timecnt == 0)
	return(0);

    /* If out of bounds (i.e., past 2037 or before 1907), return 0. */
    if(tm_time < dt[low] + TM_tz_adj(low) 
       || tm_time > dt[high] + TM_tz_adj(high))
	return(0);

    /* Binary search for tm_tztime entry that our date falls in. The
    ** result of this loop is a tztype value that is used in the tzinfo
    ** array to determine whether we're in DST. */
    while(low <= high)
    {
	mid = (low+high)/2;
	defoff = mid ? mid-1 : 0;
	if(tm_time < dt[mid] + TM_tz_adj(defoff))
	    high = mid - 1;
	else
	{
	    if (tm_time > dt[mid+1] + TM_tz_adj(mid))
		low = mid + 1;
	    else
	    {
		/* Solely to check for the 'missing' hour at the start
		** of DST. If the local date given is in this hour then
		** adjust it to DST time. Any local time in this hour
		** does not, in reality, exist!
		** This is required because the fix for 79975 'broke' this
		** adjustment.
		*/
		/* Minor adjustment from bug 99917, 
		** make the test greater-than-or-equal-to
		*/
		if (tm_timetype &&
		    (tm_time < dt[mid] + TM_tz_adj(mid)
		     && tm_time >= dt[mid] + TM_tz_adj(defoff)) )
		{
		    tztype = type[mid+1];
		}
		else
		{
		   /* Take care of boundary conditions */
		   if( tm_time != dt[mid+1] + TM_tz_adj(mid))
		       tztype = type[mid];
		   else
		   {
		     /* To maintain compatibility with dates prior to my
		     ** fix for 79775, this check is required to leap forward
		     ** the time that is execatly on the DST boundary.
		     */
		       if (tm_timetype && tm_tz_cb->tzinfo[type[mid+1]].isdst)
			   tztype = type[mid];
		       else
		           tztype = type[mid+1];
		   }
		}
		break;
	    }
	}
    }

    /* Got our type - return 1, 0 according to isdst flag. */
    return( (tm_tz_cb->tzinfo[tztype].isdst) ? 1 : 0);
}
