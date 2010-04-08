/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <cm.h>
#include    <tm.h>
#include    <tmtz.h>
#include    <si.h>
#include    <st.h>
#include    <lo.h>
#include    <nm.h>
#include    <er.h>
#include    <pm.h>


/**
**
**  name: iizck.c - Program to check the content of a timezone information 
**                  file.
**
**  synopsis
**
**   iizck [ -n[ame][=]timezone_name ] [ -f[ile][=]timezone_filename ] 
**
**  description
**
**   iizck display content of timezone information file specifies by
**   II_TIMEZONE_NAME unless -n[ame] option is used in which case 
**   the specified name will be used.  Normally timezone information
**   file is looked up from PM configuration file config.dat unless -f[ile] 
**   option is specified, in which case the specified filename will be used.
**
**  history:
**	03-mar-1993 (stevet)
**          Created.
**      13-sep-1993 (stevet)
**          Replaced reference to internal structure of LOCATION.  Also 
**          fixed LO call sequence so that it can support timezone file
**          to be on sub-directories.
**	21-oct-96 (mcgem01)
**	    Remove hard-coded references to iizck so that Jasmine may
**	    share this executable.
**      21-apr-1999 (hanch04)
**          Replace STindex with STchr
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-mar-2002 (somsa01)
**	    Added NEEDLIBS MING hint for successful 64-bit compilation.
**      16-May-2002 (hanch04)
**          Added lp64 directory for 32/64 bit builds.
**	20-Jul-2004 (hanje04)
**	    Set build directory to $ING_BUILD/utility
**	20-Jul-2004 (lakvi01)
**          SIR #112703, cleaned-up warnings. 
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**      07-Mar-2005 (hanje04)
**          SIR 114034
**          Add support for reverse hybrid builds. First part of much wider
**          change to allow support of 32bit clients on pure 64bit platforms.
**          Changes made here allow application statically linked against
**          the standard 32bit Intel Linux release to run against the new
**          reverse hybrid ports without relinking.
**	 3-may-07 (hayke02)
**	    In main(), initialize c to '\0'. This prevents an unintialized
**	    value of 'n' or 'f' triggering the -name or -filename code. This
**	    change fixes bug 118239.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
*/

/*
NEEDLIBS = COMPAT
DEST = utility
*/



struct timevect
{
    i4    tm_sec;
    i4    tm_min;
    i4    tm_hour;
    i4    tm_mday;
    i4    tm_mon;
    i4    tm_year;
    i4    tm_wday;
    i4    tm_yday;
    i4    tm_isdst;
};


static STATUS
TMtz_getopt(i4	  nargc,
	    char  **nargv, 
	    char  *ostr,
	    i4    *ioptind,
	    char  *ioptarg,
	    char  *ioptopt);

static VOID
TMtz_usage();

static VOID
TMtz_cvtime(
i4	           time,
struct timevect    *t);

static char * appname;



i4
main(
i4	argc,
char *	argv[])
{
    char      c='\0';
    char      *tz_file=NULL;
    char      *tz_name=NULL;
    char      *tz_def;
    char      tzname[TM_MAX_TZNAME+1];
    char      tzfile[MAX_LOC+1];
    char      ioptarg[MAX_LOC + 1];
    char      tz_pmvalue[MAX_LOC + 1];
    char      tz_pmname[MAX_LOC + 1];
    char      *p, *ip, *pm_value;
    char      *tm_tztype;
    char      chr='/';
    char      *out_file;
    i4        ioptind=1, i;
    char      *tm_tztime;
    i4        timecnt, tempi, temptype, temptz;
    char      buf[sizeof(TM_TZ_CB)+TM_MAX_TIMECNT*(sizeof(i4)+1)];
    FILE      *fid;
    LOCATION  loc_root, tmp_loc;
    STATUS    status=OK;
    TM_TZ_CB  *tm_tz_cb;
    struct timevect time_v;

    appname = argv[0];

    if( TMtz_getopt(argc, argv, "n:name:f:file", &ioptind, ioptarg, &c) == OK)
    {
	switch (c) 
	{
	  case 'f':
	    tz_file = ioptarg;
	    break;
	  case 'n':
	    tz_name = ioptarg;
	    break;
	  default:
	    break;
	}
    }
    else
    {
	TMtz_usage();
	PCexit(FAIL);
    }

    if( tz_file == NULL)
    {
	if( tz_name == NULL)
	{
	    /* Get II_TIMEZONE_NAME value */
	    NMgtAt(ERx("II_TIMEZONE_NAME"), &tz_def);
	    if (!tz_def || !(*tz_def))
	    {
		SIprintf("%s: %s_TIMEZONE_NAME is not set\n", 
				appname, SystemVarPrefix);
		PCexit(FAIL);
	    }
	    STncpy(tzname, tz_def, TM_MAX_TZNAME);
	    tzname[ TM_MAX_TZNAME ] = EOS;
	}
	else
	{
	    STncpy(tzname, tz_name, TM_MAX_TZNAME);
	    tzname[ TM_MAX_TZNAME ] = EOS;
	}
	PMinit();	
	if( PMload( NULL, (PM_ERR_FUNC *)NULL) != OK)
	{
	    SIprintf("%s: Error loading PM %s_CONFIG/config.dat file\n", 
			appname, SystemVarPrefix);
	    PCexit(FAIL);	    
	}
	/* Get timezone file name */
	STprintf( tz_pmname, ERx("%s.*.tz.%s"), SystemCfgPrefix, tzname);
	if( PMget( tz_pmname, &pm_value) != OK)
	{
	    SIprintf("%s: Error locating %s in PM config.dat file\n",
		     appname, tz_pmname);
    	    PCexit(FAIL);	    
	}
	do
	{
	    if((status = NMloc(FILES, PATH, ERx("zoneinfo"), &loc_root)) != OK)
		break;
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
	{
	    SIprintf("%s: Error composing timezone file name for %s\n",
		     appname, tz_pmvalue);
	    PCexit(FAIL);
	}
    }
    else
    {
	STcopy("<unknown>", tzname);
	STncpy( tzfile, tz_file, MAX_LOC);
	tzfile[ MAX_LOC ] = EOS;
	if( LOfroms(FILENAME&PATH, tzfile, &loc_root) != OK)
	{
	    SIprintf("%s: Error composing timezone file name for %s\n",
		     appname, tz_pmvalue);
	    PCexit(FAIL);
	}
    }

    /*
    ** Now open the timezone information file
    */
    do
    {
	if((status = SIfopen( &loc_root, ERx("r"), SI_VAR, sizeof buf, &fid)) 
	             != OK)
	    break;
	status = SIread(fid, sizeof buf, &i, buf);
	status = SIclose(fid);
    } while(FALSE);

    if( status != OK)
    {
	LOtos( &loc_root, &out_file);
	SIprintf("%s: Error opening %s for timezone %s\n", 
		 appname, out_file, tzname);
	PCexit(FAIL);
    }

    tm_tz_cb = (TM_TZ_CB *)&buf;
    I4ASSIGN_MACRO( tm_tz_cb->timecnt, timecnt);

    /* Make sure the input file has correct file size */
    if( timecnt > TM_MAX_TIMECNT || timecnt < 0
        || i != sizeof(TM_TZ_CB) + timecnt*(sizeof(i4)+1))
    {
	LOtos( &loc_root, &out_file);
	SIprintf(
	  "%s: Invalid file format for timezone file %s for timezone %s\n",
          appname, out_file, tzname);
	SIprintf(
	  "         File size: %d, Expected file size: %d, time periods: %d\n",
	  i, sizeof(TM_TZ_CB) + timecnt*(sizeof(i4)+1), 
	  timecnt);
	PCexit(FAIL);
    }

    /* Now we are all set to display the content of timezone information file */
    LOtos( &loc_root, &out_file);    
    SIprintf("\n\n");
    SIprintf("timezone name:     %s\n", tzname);
    SIprintf("timezone file:     %s\n", out_file);    
    SIprintf("-------------------------------------");
    SIprintf("-------------------------------------\n");

    if(timecnt == 0)
    {
	I4ASSIGN_MACRO( tm_tz_cb->tzinfo[0].gmtoff, tempi);
	SIprintf("     Fixed GMT offset (secs): %d\n", tempi);      
    }
    else
    {
	SIprintf("\tPeriod Begin");
	SIprintf("\t\tGMT offset\n");
	SIprintf("\t(YYYY_MM_DD HH:MM)");
	SIprintf("\t(Minute)\n\n");
	tm_tztype = buf + sizeof(TM_TZ_CB);
	tm_tztime = tm_tztype + timecnt;
	i=0;
	while( i < timecnt)
	{
	    I4ASSIGN_MACRO( *tm_tztime, tempi);
	    /* Adjust for timezone */
	    if( i == 0)
		temptype = (i4)tm_tztype[i+1];
	    else
		temptype = (i4)tm_tztype[i-1];

	    I4ASSIGN_MACRO( tm_tz_cb->tzinfo[temptype].gmtoff, temptz);
	    /* Get real timezone */
	    tempi += temptz;
	    temptype = (i4)tm_tztype[i];
	    I4ASSIGN_MACRO( tm_tz_cb->tzinfo[temptype].gmtoff, temptz);
	    TMtz_cvtime( tempi, &time_v);
	    SIprintf("\t%04d_%02d_%02d %02d:%02d\t%d\t%s\n", 
		     time_v.tm_year+1900,
		     time_v.tm_mon+1, 
		     time_v.tm_mday,
		     time_v.tm_hour,
		     time_v.tm_min,
		     temptz/60,
		     tm_tz_cb->tzlabel + tm_tz_cb->tzinfo[temptype].abbrind);  
	    tm_tztime += sizeof(i4);
	    i++;
	}
    }
    PCexit(OK);
}


static STATUS
TMtz_getopt(
i4	nargc,
char	**nargv, 
char    *ostr,
i4      *ioptind,
char    *ioptarg,
char    *ioptopt)    
{
    char	*place="";      /* option letter processing */
    char	*oli;		/* option letter list index */

    if (*ioptind >= nargc || *(place = nargv[*ioptind]) != '-' ||
	!*++place)
	return(OK);
    if (*place == '-') 
    {		        /* found "--" */
	*ioptind += 1;
	return(OK);
    }
    if ((*ioptopt = *place++) == ':' ||
	((oli = STchr(ostr, *ioptopt)) == NULL)) 
    {
	if (!*place)
	    *ioptind += 1;
	SIprintf("%s: illegal option -- \n", appname);
	return(FAIL);
    }
    if (*++oli != ':') 
    {			                /* don't need argument */
	ioptarg = NULL;
	if (!*place)
	    *ioptind += 1;
    }
    else 
    {					/* need an argument */
	if (*place)			
	{
	    while(*place != '=' && *place != ' ')
	    {
		if( *place == EOS)
		{
		    if (nargc > (*ioptind += 1)) 
			STcopy(nargv[*ioptind], ioptarg);
		    else
		    {
			*ioptopt = ' ';
			ioptarg = "";
		    }
		    break;
		}
		place++;
	    }
	    STcopy(place+1, ioptarg);
	}
	else 
	    if (nargc <= (*ioptind += 1)) 
	    {	                        /* no arg */
		place = "";
		ioptarg = "";
		*ioptopt = ' ';
	    }
	    else				/* white space */
		STcopy(nargv[*ioptind], ioptarg);
	place = "";
	*ioptind += 1;
    }
    return(OK);				/* dump back option letter */
}



static VOID
TMtz_usage()
{
    (VOID) SIprintf(
    "%s: usage is %s [-n[ame][=]timezone_name ] [-f[ilename][=]timezone_filename ]\n", appname, appname);
}




static VOID
TMtz_cvtime(
i4	           time,
struct timevect    *t)
{
    i4              mo,
		    yr;
    i4		    mdays,
		    ydays;
    i4	    noyears;
    i4	    nodays;
    i4	    i;

    /* get seconds, minutes, hours */
    t->tm_sec   = time % 60;
    time        /= 60;
    if( t->tm_sec < 0)
    {
	t->tm_sec += 60;
	time--;
    }
    t->tm_min    = time % 60;
    time        /= 60;
    if( t->tm_min < 0)
    {
	t->tm_min += 60;
	time--;
    }
    t->tm_hour   = time % 24;
    time        /= 24;
    if( t->tm_hour < 0)
    {
	t->tm_hour += 24;
	time--;
    }

    /* get day of week */
    t->tm_wday  = (time + 4) % 7;

    /* at this point, time is number of days since 1-jan-1970 */

    /* get the year. first, add enough days to get us to 1/1/1601.
    ** this date is special in that it is after a leapyear where
    ** the year is divisible by 400 and 100. The formula is good till
    ** the year 3012 or so.
    */
#define DAYS_SINCE_1601 134774		/* 1-1-1970 - 1-1-1601 */
    /* number of days since 1601 */
    i = time + DAYS_SINCE_1601;  
    /* number of years since 1601 */
    noyears = (i-i/1460+i/36500-i/146000)/365;	
    /* number of days to first day of current year */
    nodays = noyears*365 + noyears/4 - noyears/100 + noyears/400 
	- DAYS_SINCE_1601;

    time -= nodays;

    yr		= noyears + 1601;	/* xxxx year */
    t->tm_year  = yr - 1900;		/* offset from 1900 . why?? */
    t->tm_yday  = time;

    /* get month */
    TMyrsize(yr, &ydays);
    for (mo = 0; mo < 12; mo++)
    {
	if ((mo == 1) && (ydays == LEAPYEAR))
	    mdays = 29;
	else
	{
	    switch( mo)
	    {
	      case 1:
		mdays = 28;
		break;
	      case 3:
	      case 5:
	      case 8:
	      case 10:
		mdays = 30;
	        break;
	      default:
		mdays = 31;
		break;
	    }
	}

	if (time >= mdays)
	    time -= mdays;
	else
	    break;
    }

    if (mo > 11)
	mo = 11;

    t->tm_mon   = mo;
    t->tm_mday  = time + 1;

    return;
}

