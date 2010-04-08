/*
** Copyright (c) 1998, 2008 Ingres Corporation
** All Rights Reserved
*/

# include	<compat.h>
# include	<cv.h>
# include	<id.h>
# include       <lo.h>
# include       <me.h>
# include       <mh.h>
# include       <nm.h>
# include       <pc.h>
# include       <si.h>
# include       <st.h>
# include       <tm.h>
# include  	<cm.h>
# include  	<cs.h>
#ifdef sqs_ptx
#include <stdio.h>
#endif /* sqs_ptx */

#if defined(rmx_us5) || defined(nc4_us5)
# include <sys/sockio.h>
# include <stropts.h>
# include <net/if.h>
# include <fcntl.h>
#endif

#if defined(dgi_us5) 
#include <sys/file.h>
#include <netinet/dg_inet_info.h>
#endif



/*
** Name: IDuuid_create
** 
** Description:
**	Generate a Universally Unique ID (UUID) based on the
**	Internet-Draft by Paul J. Leach (Microsoft) and Rich Salz
**	(Certco) of the Internet Engineering Task Force (IETF)
**	and dated 4-feb-1998.
**
**	"A UUID is an identifier that is unique across both space and time,
**	with respect to the space of all UUIDs. To be precise, the UUID
**	consists of a finite bit space. Thus the time value used for 
**	constructing a UUID is limited and will roll over in the future
**	(approximately at A.D. 3400, based on the specified algorithm). A
**	UUID can be used for multiple purposes, from tagging objects with an
**	extremely short lifetime, to reliably identifying very persistent
**	objects across a network."
**
**	The basic format of the 128-bit UUID is:
**
**	Field			Data type	Octet	Note
**						#
**
**	time_low		unsigned 32	0-3	The low field of the
**				bit integer		timestamp.
**
**	time_mid		unsigned 16	4-5	The middle field of the
**				bit integer		timestamp.
**
**	time_hi_and_version	unsigned 16	6-7	The high field of the
**				bit integer		timestamp multiplexed
**							with the version number.
**	
**	clock_seq_hi_and_rese	unsigned 8	8	The high field of the
**	rved			bit integer		clock sequence
**							multiplexed with the
**							variant.
**
**	clock_seq_low		unsigned 8	9	The low field of the
**				bit integer		clock sequence.
**
**	node			unsigned 48	10-15	The spatially unique
**				bit integer		node identifier.
**
** Quotes from the Leach/Salz document are:
**	"Copyright (C) The Internet Society 1997. All Rights Reserved.
**
**	This document and translations of it may be copied and furnished to
**	others, and derivative works that comment on or otherwise explain it
**	or assist in its implementation may be prepared, copied, published
**	and distributed, in whole or in part, without restriction of any
**	kind, provided that the above copyright notice and this paragraph are
**	included on all such copies and derivative works.  However, this
**	document itself may not be modified in any way, such as by removing
**	the copyright notice or references to the Internet Society or other
**	Internet organizations, except as needed for the purpose of 
**	developing Internet standards in which case the procedures for 
**	copyrights defined in the Internet Standards process must be 
**	followed, or as required to translate it into languages other than
**	English."
**
** Input:
**	None.
**
** Output:
**	uuid		A unique UUID.
**
** Returns:
**	RPC_S_OK		The UUID is globally unique.
**
**	RPC_S_UUID_LOCAL_ONLY	The UUID is valid only on the machine where
**				it was generated (this usually means that
**				there was no network card detected).
**
** History:
**	15-oct-1998 (canor01)
**	    Created.
**	05-nov-1998 (canor01)
**	    Use clock_gettime().
**	09-nov-1998 (canor01)
**	    Add return values.
**      18-nov-1998 (canor01)
**          Add IDuuid_from_string.
**	08-dec-1998 (canor01)
**	    Correct logic in IDuuid_from_string.
**	18-jan-1999 (muhpa01)
**	    Fix longlong constants for hp8_us5, and add hp8_us5 to su4_us4's
**	    use of arp command in IDuuid_node().
**	16-feb-1990 (rosga02)
**       Added sui_us5 to the list of supported platforms
**	03-mar-1999 (walro03)
**	    Added sqs_ptx to the list of supported platforms.
**	15-mar-1999 (walro03)
**	    Typo in 03-mar-1999 change: vertical bars "||" mistyped as
**	    bangs "!!"
**	18-Mar-1999 (popri01)
**	    Added usl_us5 to the list of supported platforms.
**	    Implement TMhrnow (unintentionally implemented in previous integration).
**	    Remove HNANOPERUSEC which is no longer used.
**	05-Apr-1999 (bonro01)
**	    Replaced longlong integer division with bit shifting.
**	    This will eliminate problems with large integer constants
**	    for all platforms.  This should also produce more efficient
**	    code when compiled.  On DGUX the shift was done with one
**	    machine instruction whereas the divide called a library function.
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**	07-Apr-1999 (schte01)
**       Added axp_osf to use arp command for MAC address.
**      22-Jun-1999 (musro02)
**          Added nc4_us5 to the list of supported platforms.
**	14-Jul-1999 (ahaal01)
**	    Added rs4_us5 to the list of supported platforms.
**      08-Dec-1999 (podni01)
**          Added rux_us5 to the list of supported platforms.
**      09-Dec-1999 (hweho01)
**          1) To pass the compiler, add ris_u64 to the list
**             of platforms that have the uuid node codes.
**          2) Changed the format type for lint in STscanf from long to
**             int, because long is 8 bytes on 64-bit platform.
**	14-Jul-2000 (hanje04)
**	    Add int_lnx to list of platforms using arp in IDuuid_node
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	15-aug-2000 (hanje04)
**	    Added ibm_lnx as supported on this platform.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-mar-2001 (mosjo01)
**	    Lets use ethstat or trstat if sqs_ptx
**	08-Sep-2000 (hanje04)
**	    Add axp_lnx to list of platforms using arp in IDuuid_node
**	20-feb-2001 (somsa01)
**	    In IDuuid_node(), the usage of "arp" on HP and Linux platforms
**	    is inconsistent. Therefore, new code has been added to grab
**	    the value properly from these platforms.
**	23-Mar-2001 (wansh01)
**	    Add IDuuid_node for sgi_us5 
**	28-Mar-2001 (ahaal01)
**	    In IDuuid_node(), the usage of "arp on AIX (rs4_us5) is
**	    inconsistent.  Therefore, new code has been added to grap the
**	    value properly for this platform.
**	06-Apr-2001 (bonro01)
**	    Added rmx_us5 specific code to retrieve the hardware address.
**	    Added 64-bit math routines to calculate time value for
**	    platforms without 64-bit integers.
**	    Modified the UUID time field to be in Big Endian format for
**	    all platforms to make it consistent, and allow cross-platform
**	    UUID comparison.
**	    Added time check to insure that the current time value is
**	    always incrementing to insure a unique uuid value.
**      24-Apr-2001 (musro02)
**          Added nc4_us5 to the rmx_us5 changes mentioned above.
**          Removed nc4_us5 from the list of platforms that use the arp command
**      24-Apr-2001 (musro02)
**          Corrected ifdefs that I "messed up" propagating change 450014
**      25-Apr-2001 (hweho01)
**          For axp_osf platform, setup nodeid value from the result   
**          of command "netstat -i".
**      26-Apr-2001 (hweho01/ahaal01)
**          Used /usr/sbin/lscfg command to grap the proper value   
**          of MAC address for ris_u64 and rs4_us5 platforms.
**	31-may-2001 (bonro01/legru01)
**	    Added IDuuid_node routine for sos_us5.
**      22-Jun-2001 (linke01)
**          UUID value was not complete for usl_us5.(Same as bug 104625)
**          changed on IDuuid_node() to return correct value. 
**      26-Jun-2001 (linke01)
**          when submitted change to piccolo, it was propagated incorrectly,
**          re-submit the change. (this is to modify change#451142).
**	23-jul-2001 (stephenb)
**	    Add support fir i64_aix
**	19-oct-2001 (devjo01)
**	    In IDuuid_compare don't assume MEcmp, or what it resolves to,
**	    returns only -1, 0, and 1.  b106088.
**	30-Oct-2001 (bonro01)
**	    Fixed incorrectly submitted code.  Someone manually added the
**	    same update to ingres25 and main, then forgot to ignore the
**	    changes in the other codelines.  Then a 2nd person propagated
**	    the ingres25 changes to main without realizing that the change
**	    already existed a few lines higher in the code.
**	04-Dec-2001 (hanje04)
**	    Add IA64 to linux get mac address routine.
**      08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate.
**      28-apr-2005 (raodi01)
**          Following the changes made for b110668,  which was a 
**	    windows-specific one
**          added changes to handle the change on unix and VMS
**	21-Oct-2004 (shaha03)
**	    Fixed the bug #112890 (Improper conversion of UUID string to UUID).
**	    Added function IDuuid_substr_validate to validate the UUID string.
**	14-Dec-2004 (bonro01)
**	    Added support for AMD64 Solaris a64_sol
**    18-Apr-2005 (raodi01) b113846 INGSRV3145
**        Added the check for big endian or little endian and initialize
**        version field accordingly.
**	06-May-2005 (hanje04)
**	    Remove X-Integ tags
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Based on initial changes by utlia01 and monda07.
**	1-Jun-2006 (bonro01)
**	    Fix uuid_from_char() bug caused by previous change.
**	    uuid_from_char() fails to convert the node section
**	    of the uuid correctly.
**	08-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
**      12-dec-2008 (joea)
**          Replace BYTE_SWAP by BIG/LITTLE_ENDIAN_INT.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**      6-Nov-2009 (maspa05) b122642
**          Protect update of ID_uuid_last_time which is used to generate uuid 
**          time component. Previously this could allow duplicates.
**          In order to do this across all servers a 'last time' variable
**          was added to LGK shared memory and a mutex to protect it. 
**          However ID gets linked to frontend programs so we use pointers
**          which if not initialized in LGKinitialize to the shared memory 
**          version, are set to a process-specific variable here. 
*/

/* UnDefine TYPE_LONGLONG_OK for platforms that */
/* don't have longlong C variable types        */
#if !defined(rmx_us5) && !defined(sos_us5)
#define TYPE_LONGLONG_OK
#endif

# define NODESIZE 6 /* 6 bytes in address */

#if defined(TYPE_LONGLONG_OK)
# define HNANOPERSEC ((longlong)10000000)

# define PRE1970DAYS 141427  /* from 15-oct-1582 to 1-jan-1970 */
# define PRE1970MINS ((longlong)(PRE1970DAYS * 24 * 60))
# define PRE1970SECS ((longlong)(PRE1970MINS * 60))
# define PRE1970HNANO ((longlong)(PRE1970SECS * HNANOPERSEC))
#else
typedef u_i4 S64[4];
static void add64(S64 a, S64 b, S64 ans);
static void copy64(S64 a, S64 ans);
static void mul64(S64 a, S64 b, S64 ans);
static void ItoS64( u_i4 in, S64 ans);
#endif /* TYPE_LONGLONG_OK */

# define IDUUID_NO_INIT		0
# define IDUUID_OK		1
# define IDUUID_NO_NODE		2
static i4 IDuuid_init	= 	IDUUID_NO_INIT;

static u_char uuid_node[NODESIZE] ZERO_FILL;

typedef union uuid_time {
	struct {
		u_i4 high;
		u_i4 low;
	} I4;
        struct {
		u_i2 high1;
		u_i2 high2;
		u_i2 low1;
		u_i2 low2;
	} I2;
        struct {
                u_i1 c[8];
        } I1;
} UUID_TIME;

GLOBALDEF ID_UUID_SEM_TYPE  ID_uuid_sem ZERO_FILL;
GLOBALDEF ID_UUID_SEM_TYPE  *ID_uuid_sem_ptr=NULL;
GLOBALDEF HRSYSTIME ID_uuid_last_time ZERO_FILL;
GLOBALDEF HRSYSTIME *ID_uuid_last_time_ptr=NULL;
GLOBALDEF u_i2 ID_uuid_last_cnt=0;
GLOBALDEF u_i2 *ID_uuid_last_cnt_ptr=NULL;

# ifndef IDuuid_create

static UUID_TIME IDuuid_time(void);
static u_i2 IDuuid_sequence(void);
static STATUS IDuuid_node(u_char *nodeid);

STATUS
IDuuid_create(
          UUID *uuid
)
{
    UUID_TIME time_all;
    u_i2 clock_seq;
    u_i4 version = 0x1000;
    STATUS status = RPC_S_OK;
    #ifdef BIG_ENDIAN_INT
      version = 0x1000;
    #else
      version = 0x0010;
    #endif
    /* initialization and one-time stuff */
    if ( IDuuid_init == IDUUID_NO_INIT )
    {
        IDuuid_init = IDUUID_OK;
        if (IDuuid_node(uuid_node) != OK)
	    IDuuid_init = IDUUID_NO_NODE;
    }

    time_all = IDuuid_time();

    uuid->Data1 = time_all.I4.low;
    uuid->Data2 = time_all.I2.high2;
    uuid->Data3 = time_all.I2.high1;
    uuid->Data3 |= version;

    clock_seq = IDuuid_sequence();
    
    /*
    ** "Set the clock_seq_low field to the 8 least significant bits (bits
    ** numbered 0 to 7 inclusive) of the clock sequence in the same order of
    ** significance."
    */
    uuid->Data4[1] = clock_seq & 0xff;
    /*
    ** "Set the 6 least significat bits (bits numbered 0 to 5 inclusive) 
    ** of the clock_seq_hi_and_reserved field to the 6 most significant bits
    ** (bits numbered 8 to 13 inclusive) of the clock sequence in the same
    ** order of significance."
    */
    uuid->Data4[0] = (clock_seq >> 8) & 0x3f;
    /*
    ** "Set the 2 most significant bits (bits numbered 6 and 7) of the
    ** clock_seq_hi_and_reserved to 0 and 1, respectively."
    */
    uuid->Data4[0] |= 0x80;

    /*
    ** "Set the node field to the 48-bit IEEE address in the same order of 
    ** significance as the address."
    */

    MEcopy(uuid_node, NODESIZE, &uuid->Data4[2]);

    if ( IDuuid_init == IDUUID_NO_NODE )
	status = RPC_S_UUID_LOCAL_ONLY;

    return (status);
}
# ifndef IDuuid_create_seq
 
 STATUS
 IDuuid_create_seq(
           UUID *uuid
 )
 {
     /* Not to be defined for Unix */
	return IDuuid_create(uuid);
 }
# endif

/* Name: IDuuid_time()
**
** Description:
**	Number of 100-nanosecond intervals since 15-oct-1582 00:00:00.00
**	"The timestamp is a 60 bit value. For UUID version 1, this is
**	represented by Coordinated Universal Time (UTC) as a count of 100-
**	nanosecond intervals since 00:00:00.00, 15 October 1582 (the date of
**	Gregorian reform to the Christian calendar.
**
**      "For systems that do not have UTS available, but do have local time,
**      they MAY use local time instead of UTC, as long as they do so
**      consistently throughout the system.  This is NOT RECOMMENDED, however,
**      and it should be noted that ll that is needed to generate UTC, given
**      local time, is a time zone offset."
**
**      6-Nov-2009 (maspa05) b122642
**          Protect update of ID_uuid_last_time which is used to generate uuid 
**          time component
*/
static
UUID_TIME
IDuuid_time()
{
    UUID_TIME time_hnano;
    HRSYSTIME cur;


    /* Need to protect getting of time to nearest 100 nanosecs
     * otherwise we could get duplicate UUIDs
     */

    if (!ID_uuid_sem_ptr )
    {
	    ID_uuid_sem_ptr = &ID_uuid_sem;
	    ID_UUID_SEM_INIT(ID_uuid_sem_ptr,CS_SEM_SINGLE,"uuid sem");
    }

    if (!ID_uuid_last_time_ptr)
	    ID_uuid_last_time_ptr=&ID_uuid_last_time;

    ID_UUID_SEM_LOCK(TRUE,ID_uuid_sem_ptr);

    /* get current time in seconds and nanoseconds */
    TMhrnow(&cur);

    /* Make sure time is unique; minimum increment is 100 nanoseconds */
    if ( cur.tv_sec < ID_uuid_last_time_ptr->tv_sec ||
      (cur.tv_sec == ID_uuid_last_time_ptr->tv_sec &&
       (cur.tv_nsec - ID_uuid_last_time_ptr->tv_nsec) < 100) )
    {
        cur = *ID_uuid_last_time_ptr;
        cur.tv_nsec += 100;  /* 100 is the minimum that we can add */
        if(cur.tv_nsec >= NANOPERSEC)
        {
            cur.tv_sec++;
            cur.tv_nsec=0;
        }
    } 
    *ID_uuid_last_time_ptr=cur;

    ID_UUID_SEM_UNLOCK(ID_uuid_sem_ptr);

#if defined(TYPE_LONGLONG_OK)
    {
    longlong hnanosec;

    /* 
    ** use seconds and microseconds from cur
    ** use just the hundred-nanosecond interval from cur_nano
    ** this yields 100-nano intervals since 1-jan-1970
    */
    hnanosec = (longlong) cur.tv_sec * (HNANOPERSEC) +
                    (longlong) cur.tv_nsec / 100;

    /* now get the intervals since 15-oct-1582 */
    hnanosec += PRE1970HNANO;

    /* Store time in Big Endian order for all platforms */
    time_hnano.I1.c[0] = (hnanosec >> 56) & 0xff;
    time_hnano.I1.c[1] = (hnanosec >> 48) & 0xff;
    time_hnano.I1.c[2] = (hnanosec >> 40) & 0xff;
    time_hnano.I1.c[3] = (hnanosec >> 32) & 0xff;
    time_hnano.I1.c[4] = (hnanosec >> 24) & 0xff;
    time_hnano.I1.c[5] = (hnanosec >> 16) & 0xff;
    time_hnano.I1.c[6] = (hnanosec >> 8) & 0xff;
    time_hnano.I1.c[7] = hnanosec & 0xff;
    }

#else
    {
    S64 hnanosec, op1, op2, ans;

    /* pre-1970 nanoseconds constant */
    /*  hnanosec = 141427 * 24 * 60 * 60 * 10,000,000 */
    /*  nnanosec = 122,192,928,000,000,000 */
    /*  hnanosec = 0x01b21dd213814000 */
    hnanosec[3] = 0x01b2;
    hnanosec[2] = 0x1dd2;
    hnanosec[1] = 0x1381;
    hnanosec[0] = 0x4000;

    /*   hnanosec += cur.tv_sec * 10,000,000   */
    ItoS64(cur.tv_sec,op1);
    ItoS64(10000000,op2);
    mul64(op1,op2,ans);
    add64(ans,hnanosec,op1);
    copy64(op1,hnanosec);

    /*   hnanosec += tv_nsec / 100  */
    ItoS64(cur.tv_nsec / 100,op1);
    add64(op1,hnanosec,ans);
    copy64(ans,hnanosec);

    /* Store time in Big Endian order for all platforms */
    time_hnano.I1.c[0] = (hnanosec[3] >> 8) & 0xff;
    time_hnano.I1.c[1] = hnanosec[3] & 0xff;
    time_hnano.I1.c[2] = (hnanosec[2] >> 8) & 0xff;
    time_hnano.I1.c[3] = hnanosec[2] & 0xff;
    time_hnano.I1.c[4] = (hnanosec[1] >> 8) & 0xff;
    time_hnano.I1.c[5] = hnanosec[1] & 0xff;
    time_hnano.I1.c[6] = (hnanosec[0] >> 8) & 0xff;
    time_hnano.I1.c[7] = hnanosec[0] & 0xff;
    }
#endif /* TYPE_LONGLONG_OK */

    return (time_hnano);
}

/* Name: IDuuid_sequence
**
** Description:
**	"For UUID version 1, the clock sequence is used to help avoid
**	duplicates that could arise when the clock is set backwards in time
**	or if the node ID changes.
**
**	"If the clock is set backwards, or even might have been set
**	backwards (e.q., while the system was powered off), and the UUID
**	generator can not be sure that no UUIDs were generated with timestamps
**	larger than the value to which the clock was set, then the clock
**	sequence has to be changed.  If the previous value of the clock
**	sequence is known, it can be just incremented; otherwise it should
**	be set to a random or high-quality pseudo random value.
**
**	"Similarly, if the node ID changes (e.g. because a network card has
**	been moved between machines), setting the clock sequence to a random
**	number minimizes the probability of a duplicate due to slight
**	differences in the clock settings of the machines. (If the value of
**	clock sequence associated with the changed node ID were known, then
**	the clock sequence could just be incremented, but that is unlikely.)
**
**	"The clock sequence MUST be originally (i.e., once in the lifetime of
**	a system) initialized to a random number to minimize the correlation
**	across systems.  This provides maximum protection against node
**	identifiers that may move or switch from system to system rapidly.
**	The initial value MUST NOT be correlated to the node identifier."
**
**  23-nov-2009 (maspa05)
**      Instead of a random number use a counter. Since only bits 0-13 are
**      in fact used, limit the size of the counter to 12bits and use the
**      extra bit as an indicator for Frontend processes. This means a
**      backend will never create a UUID that a Frontend has. Clashes could
**      still occur between different FEs but there's nothing we can do 
**      about that.
*/
static
u_i2
IDuuid_sequence()
{
    u_i2 seq;
    static bool frontend=FALSE;

    /* if we have to set the ptr here it wasn't init'd in lgkinit which
     * means we're a front-end */

    if (!ID_uuid_last_cnt_ptr)
    {
	    frontend=TRUE;
	    ID_uuid_last_cnt_ptr=&ID_uuid_last_cnt;
    }

    ID_UUID_SEM_LOCK(TRUE,ID_uuid_sem_ptr);

    seq=*ID_uuid_last_cnt_ptr;
    if (seq == 0x0fff)
	    seq=0;
    else
	    seq++;

    *ID_uuid_last_cnt_ptr=seq;

    ID_UUID_SEM_UNLOCK(ID_uuid_sem_ptr);

    if (frontend)
	    seq|=0x1000;
    
    return (seq);
}

#if defined(rmx_us5) || defined(nc4_us5)
/*
** Name: IDstroictl
** 
** Description:
**	Support routine for rmx_us5 IDuuid_node routine.
*/
static STATUS
IDstrioctl(i4 so, i4 cmd, char *buffer, i4 bufsize, i4 *buflen)
{
    struct strioctl ic;
    i4     rc;

    ic.ic_cmd = cmd;
    ic.ic_timout = 0;
    switch (cmd)
    {
    case SIOCGIFCONF:
        /* Fit as many structures as possible in the buffer */
        ic.ic_len = ( bufsize / sizeof(struct ifreq) ) * sizeof(struct ifreq);
        break;
    case SIOCGIFFLAGS:
    case SIOCGENADDR:
        ic.ic_len = sizeof(struct ifreq);
        break;
    default:
        ic.ic_len = 1;
    }
    ic.ic_dp = buffer;
    rc = ioctl(so, I_STR, &ic);
    *buflen = ic.ic_len;
    return rc;
}
#endif

/*
** Name: IDuuid_node
** 
** Description:
**	"For UUID version 1, the node field consists of the IEEE address,
**	usually the host address.  For systems with multiple IEEE 802
**	addresses, any available address can be used. The lowest addressed
**	octet (octet number 10) contains the global/local bit and the
**	unicast/multicast bit, and is the first octet of the address
**	transmitted on an 802.3 LAN."
**
** 	Most systems support getting the MAC address from the "netstat"
**	command (with some parameters or other).  Using ioctl() would
**	be more efficient.
**
**
** History:
**	20-feb-2001 (somsa01)
**	    The usage of "arp" on HP and Linux platforms is inconsistent.
**	    Therefore, new code has been added to grab the value properly
**	    from these platforms.
*/
static
STATUS
IDuuid_node(u_char *nodeid)
{
#if defined(sparc_sol) || defined(sui_us5) || defined(a64_sol)

  /* we'll use arp command */
# define have_uuid_node_code

    STATUS status;
    char hostname[128];
    char buffer[2048];
    i4 wordcount = 4;
    char *wordarray[4];
    char *cmdline = buffer;
    char *cp;
    LOCATION temploc;
    char tempbuf[MAX_LOC+1] = "";
    CL_ERR_DESC err_desc;
    FILE *infile;
    u_i4 result;
    i4 i;

    if ( gethostname(hostname, sizeof(hostname)) != 0 )
	return (FAIL);

    STprintf( cmdline, "/usr/sbin/arp %s", hostname );

    LOfroms( PATH, tempbuf, &temploc );
    NMloc( TEMP, PATH, NULL, &temploc );
    LOuniq( "arp", "tmp", &temploc );

    status = PCcmdline( NULL, cmdline, PC_WAIT, &temploc, &err_desc );

    if ( status != OK )
	goto badreturn;

    if ( (status = SIopen(&temploc, "r", &infile)) != OK )
	goto badreturn;
    if ( (status = SIgetrec( buffer, sizeof(buffer), infile )) != OK )
	goto badreturn;
    STgetwords( buffer, &wordcount, (char **)&wordarray );

    /* MAC address is in 4th word */
    for ( cp = wordarray[3], i = 0; i < NODESIZE; i++ )
    {
        char *divider = cp;
        int term = 0;
        while ( *divider != ':' && *divider != EOS )
	    divider++;
        if ( *divider == EOS )
 	    term = 1;
        *divider = EOS;
	if ( (status = CVahxl( cp, &result )) != OK )
	    goto badreturn;
	nodeid[i] = (u_char) result;
  	if ( term )
	    break;
	cp = divider + 1;
    }

    status = OK;

badreturn:
    LOdelete(&temploc);

    return (status);

# endif /* MACADDR_FROM_ARP */

# if defined(sqs_ptx)
# define have_uuid_node_code

# define NOHOSTID       0
# define HOSTID_ETHER   2
# define HOSTID_TOKEN   3
static char ethstat[32] = "/usr/bin/ethstat -a ";
static char trstat[32]  = "/usr/bin/trstat -a ";
char ethbuf[128];
STATUS	status;

FILE *f;
char *e;
i2  id_type = NOHOSTID;

f = popen(ethstat, "r");
if (f)
    {
    if (fgets(ethbuf, sizeof(ethbuf), f) )
        {
        for (e = ethbuf; *e && *e != ':'; e++);
	if ( sscanf(e-2, "%2x:%2x:%2x:%2x:%2x:%2x",&nodeid[0],&nodeid[1],
            &nodeid[2],&nodeid[3],&nodeid[4],&nodeid[5]) == 6)
	    id_type = HOSTID_ETHER;
        }
    pclose(f);
    }

if (id_type == NOHOSTID)
    {   /* didn't get a valid hostid from ethstat, try trstat */
    f = popen(trstat, "r");
    if (f)
        {
        if (fgets(ethbuf, sizeof(ethbuf), f) )
            {
            for (e = ethbuf; *e && *e != ':'; e++);
	    if ( sscanf(e-2, "%2x:%2x:%2x:%2x:%2x:%2x",&nodeid[0],&nodeid[1],
                 &nodeid[2],&nodeid[3],&nodeid[4],&nodeid[5]) == 6)
                 id_type = HOSTID_TOKEN;
            }
        pclose(f);
        }
    }

if (id_type == NOHOSTID)
    status = FAIL;
else
    status = OK;

return (status);

# endif /* MACADDR_FROM_ETHSTAT */

#if defined(any_aix)
# define have_uuid_node_code

char ethbuf[256];
FILE *f;
char *e, *env_name, outbuf[30];
int temp[6];
int i;
int rc;


  env_name = (char*)getenv("LANG");
  rc = putenv("LANG=C");
  if ( rc != 0)
        perror("Can't set enviornment variable LANG\n");

  f = popen("/usr/sbin/lscfg -v", "r");
  if (f)
  {
    while (fgets(ethbuf, sizeof(ethbuf), f) != NULL)
    {
      if (strstr(ethbuf,"Network Address"))
      {
        for (e = ethbuf; *e && *e != ':'; e++);
        e = strrchr(ethbuf,'.');
        if (e)
        {
          sscanf(e+1, "%2x%2x%2x%2x%2x%2x",&temp[0],&temp[1],
                           &temp[2],&temp[3],&temp[4],&temp[5]);
          for(i=0;i<6;i++)
            nodeid[i]=temp[i];
        }
      }
    }
    pclose(f);
  }
  memset(outbuf, sizeof(outbuf), 0);
  strcpy(outbuf, "LANG=");
  strcat(outbuf, env_name);
  rc = putenv(outbuf);
  if ( rc != 0)
        perror("Can't set enviornment variable LANG\n");
 
#endif /* MACADDR for aix */

#if defined(usl_us5)
#define have_uuid_node_code

#include <stdio.h>

char ethbuf[256];
FILE *f;
char *e;
int temp[6],i;

    f = popen( "/usr/sbin/ifconfig -a", "r");
    if (f)
    {
        while (fgets(ethbuf, sizeof(ethbuf), f) != NULL)
        {
            if (strstr(ethbuf,"ether"))
            {
                for (e = ethbuf; *e && *e != ':'; e++);
                sscanf(e-2, "%2x:%2x:%2x:%2x:%2x:%2x",&temp[0],&temp[1],
                  &temp[2],&temp[3],&temp[4],&temp[5]);            
                for(i=0;i<6;i++)
                  nodeid[i] = temp[i];
            }
        }
        pclose(f);
    }

#endif  /* MACADDR for usl_us5 */                        
# if defined(any_hpux)
# define have_uuid_node_code

    STATUS	status;
    char	buffer[2048];
    char	*cmdline = buffer;
    LOCATION	temploc;
    char	tempbuf[MAX_LOC+1] = "";
    CL_ERR_DESC	err_desc;
    FILE	*infile;
    char	ethbuf[128], hdigit[3];
    char	*bptr = ethbuf;
    u_i4	result;
    i4		i;

    STprintf(cmdline, "/etc/lanscan -a");

    LOfroms(PATH, tempbuf, &temploc);
    NMloc(TEMP, PATH, NULL, &temploc);
    LOuniq("lanscan", "tmp", &temploc);

    status = PCcmdline(NULL, cmdline, PC_WAIT, &temploc, &err_desc);

    if (status != OK)
	goto badreturn;

    if ((status = SIopen(&temploc, "r", &infile)) != OK)
	goto badreturn;
    if ((status = SIgetrec(ethbuf, sizeof(ethbuf), infile)) != OK)
	goto badreturn;
    SIclose(infile);

    /* Now, let's translate the address we got. */
    bptr += 2;
    hdigit[2] = '\0';

    for (i = 0; i < NODESIZE; i++)
    {
	MEcopy(bptr, 2, &hdigit);
	bptr += 2;

	if ((status = CVahxl(hdigit, &result)) != OK)
	    goto badreturn;

	nodeid[i] = (u_char) result;
    }

    status = OK;

badreturn:
    LOdelete(&temploc);

    return(status);

# endif /* MACADDR_FROM_LANSCAN */

# if defined(LNX) || defined(OSX)
# define have_uuid_node_code

    STATUS	status;
    char	buffer[2048];
    char	*cmdline = buffer;
    LOCATION	temploc;
    char	tempbuf[MAX_LOC+1] = "";
    CL_ERR_DESC	err_desc;
    FILE	*infile;
    char	*cp;
    u_i4	result;
    i4		i;
    bool	NoMACAddr = FALSE;
    i4		wordcount = 4;
    char	*wordarray[4];

    STprintf(cmdline, "/sbin/ifconfig");

    LOfroms(PATH, tempbuf, &temploc);
    NMloc(TEMP, PATH, NULL, &temploc);
    LOuniq("ifconfig", "tmp", &temploc);

    status = PCcmdline(NULL, cmdline, PC_WAIT, &temploc, &err_desc);

    if (status != OK)
	goto badreturn;

    if ((status = SIopen(&temploc, "r", &infile)) != OK)
	goto badreturn;

    do
    {
	if ((status = SIgetrec(buffer, sizeof(buffer), infile)) != OK)
	{
	    if (status == ENDFILE)
	    {
		NoMACAddr = TRUE;
		break;
	    }

	    goto badreturn;
	}
    }
    while ((cp = STstrindex(buffer, "HWaddr", 0, FALSE)) == NULL);

    SIclose(infile);

    if (NoMACAddr)
    {
	char	tempbuf[2048];

	/*
	** Since we didn't find a MAC address, we need to "generate"
	** one from the first IP address registered.
	*/
	if ((status = SIopen(&temploc, "r", &infile)) != OK)
	    goto badreturn;

	do
	{
	    if ((status = SIgetrec(tempbuf, sizeof(tempbuf), infile)) != OK)
		goto badreturn;
	}
	while ((cp = STstrindex(tempbuf, "addr:", 0, FALSE)) == NULL);

	SIclose(infile);

	STcopy("HWaddr 80", buffer);
	cp += 5;
	for (i = 0; i < 4; i++)
	{
	    char	*divider = cp;
	    int		term = 0;
	    char	hexnum[9];

	    while (*divider != '.' && *divider != EOS && *divider != ' ')
		divider++;
	    if (*divider == EOS || *divider == ' ')
		term = 1;
	    *divider = EOS;

	    if ((status = CVal(cp, &result)) != OK)
		goto badreturn;
	    CVlx(result, hexnum);
	    STcat(buffer, ":");
	    STcat(buffer, hexnum);

	    if (term)
		break;
	    cp = divider + 1;
	}

	STcat(buffer, ":00");
	cp = buffer;
    }

    /* Now, let's translate the MAC address we got. */
    STgetwords(cp, &wordcount, (char **)&wordarray);
    for (cp = wordarray[1], i = 0; i < NODESIZE; i++)
    {
        char	*divider = cp;
        int	term = 0;

        while (*divider != ':' && *divider != EOS)
	    divider++;
        if (*divider == EOS)
 	    term = 1;
        *divider = EOS;
	if ((status = CVahxl(cp, &result)) != OK)
	    goto badreturn;
	nodeid[i] = (u_char)result;
  	if (term)
	    break;
	cp = divider + 1;
    }

    status = OK;

badreturn:
    LOdelete(&temploc);

    return(status);
# endif /* MACADDR_FROM_IFCONFIG */

#if defined(sgi_us5) 
# define have_uuid_node_code 
char ethbuf[256];
FILE *f;
char *e;
int temp[6];
int i;

  f = popen("/usr/etc/netstat -ia", "r");
  if (f)
  {
    while (fgets(ethbuf, sizeof(ethbuf), f) != NULL)
    {
      if (strstr(ethbuf,":"))
      {
        for (e = ethbuf; *e && *e != ':'; e++);
        sscanf(e-2, "%2x:%2x:%2x:%2x:%2x:%2x",&temp[0],&temp[1],
                          &temp[2],&temp[3],&temp[4],&temp[5]);
        for(i=0;i<6;i++)
          nodeid[i]=temp[i];
      }
    }
    pclose(f);
  }
  else
    return(FAIL);

  return(OK);
#endif    /* MACADDR for sgi_us5 */  

#if defined(rmx_us5) || defined(nc4_us5)
# define have_uuid_node_code

    i4     ifcnt, so, buflen;
    char   buffer[1024];
    struct ifreq *ifr;

    if ((so = open("/dev/ip", O_RDONLY)) == -1)
        return(FAIL);

    if (IDstrioctl(so, SIOCGIFCONF, buffer, sizeof(buffer), &buflen) == -1)
        return(FAIL);

    ifr = (struct ifreq *) buffer;
    ifcnt = buflen / sizeof(struct ifreq);

    for ( ;ifcnt--; ifr++)
    {
        if (IDstrioctl(so, SIOCGIFFLAGS, (char *) ifr, sizeof(buffer), &buflen) == -1)
            return(FAIL);
        if ((ifr->ifr_flags & (IFF_UP|IFF_LOOPBACK)) == IFF_UP)
        {
            if (IDstrioctl(so, SIOCGENADDR, (char *) ifr, sizeof(buffer), &buflen) == -1)
                return(FAIL);
            MEcopy(ifr->ifr_enaddr, sizeof(ifr->ifr_enaddr), nodeid) ;
        }
    }
    if (so != -1)
    close(so);

    return (OK);

#endif /* MACADDR for rmx_us5 */

#if defined(dgi_us5) 
# define have_uuid_node_code
    i4 i, status=FAIL;
    i4 stream_fd = -1;
    char * stream_dev = "/dev/udp";
    struct dg_inet_info dg_inet_info;
    struct dg_inet_if_entry dg_inet_if_entry;


    stream_fd = open (stream_dev,O_RDWR);
    if (stream_fd >= 0)
    {
        dg_inet_info.key = 0;
        dg_inet_info.version = DG_INET_INFO_VERSION;
        dg_inet_info.selector = DG_INET_IF_ENTRY;

        do
        {
            dg_inet_info.buff_len = sizeof (dg_inet_if_entry);
            dg_inet_info.buff_ptr = (caddr_t)&dg_inet_if_entry;

            if (ioctl (stream_fd,SIOCGINFO,&dg_inet_info) == -1)
                break;

            if (dg_inet_info.buff_len) 
            {
                if (strstr (dg_inet_if_entry.ifName,"loop")) continue;
                if (dg_inet_if_entry.ifHwAddrLen != 6) continue;
      
                  for (i = 0; i < 6; i++)
                    nodeid[i] = dg_inet_if_entry.ifHwAddr[i];
                status=OK;
                break;
            }
        }
        while (dg_inet_info.buff_len);
	close(stream_fd);
    }

    return(status);
#endif 

#if defined(axp_osf)
# define have_uuid_node_code

STATUS      status;
    char        buffer[2048];
    char        *cmdline = buffer;
    LOCATION    temploc;
    char        tempbuf[MAX_LOC+1] = "";
    CL_ERR_DESC err_desc;
    FILE        *infile;
    int         temp[6];
    int         i;
    static char ethbuf[128], eaddr[128];

    STprintf(cmdline, "/usr/sbin/netstat -i");  
    LOfroms(PATH, tempbuf, &temploc);
    NMloc(TEMP, PATH, NULL, &temploc);
    LOuniq("netstat", "tmp", &temploc);
    
    status = PCcmdline(NULL, cmdline, PC_WAIT, &temploc, &err_desc);
    if (status != OK)
        goto badreturn;
    if ((status = SIopen(&temploc, "r", &infile)) != OK)
        goto badreturn;

      while (1) 
        {
           if ((status = SIgetrec(ethbuf, sizeof(ethbuf), infile)) != OK)
             {
               if (status == ENDFILE)
                 {
                   SIclose(infile);
                   goto badreturn;
                 }
             }
           if ( STindex(ethbuf, ":", sizeof(ethbuf) ) != NULL )
             {
                /* got the entry */
                STscanf( ethbuf+24, "%s", eaddr );
                STscanf( eaddr, "%2x:%2x:%2x:%2x:%2x:%2x",
                       &temp[0], &temp[1], &temp[2], &temp[3],   
                       &temp[4], &temp[5] );
                for( i = 0; i < 6; i++)
                  nodeid[i] = (u_char) temp[i]; 
                status = OK;
                SIclose(infile);
                break; 
             }  
        }  /* end of while() */

 badreturn:
    LOdelete(&temploc);

    return( status );

# endif /* MACADDR for axp_osf */



#if defined(sos_us5)
# define have_uuid_node_code

 FILE *pipepointer;                          /* pointer to a pipe    */
 char commandbuf[1024];                      /* ifconfig command     */
 char *e;                                    /* pointer ether string */
 i4 temp[6];                                 /* holds ether string   */
 i4 i;                                       /* loop control         */

 pipepointer = popen("/etc/ifconfig -a 2> /dev/null", "r");
 if(pipepointer)
   {
   while (fgets(commandbuf, sizeof(commandbuf), pipepointer) != NULL)
     {
     if (strstr(commandbuf, "ether"))
        {
        for (e = commandbuf; *e && *e != ':'; e++)
        sscanf (e-2,"%2x:%2x:%2x:%2x:%2x:%2x",&temp[0],&temp[1],\
               &temp[2],&temp[3],&temp[4],&temp[5]);
        }
     }
     for (i=0;i<6;i++){
         nodeid[i] = temp[i];
         }
     pclose(pipepointer);
   }
   else
      return (FAIL);

   return (OK);
#endif /* sos_us5 */


# ifndef have_uuid_node_code
# error Missing OS-specific support for IDuuid_node.
# endif
}
# endif /* ! defined(IDuuid_create) */

/* 
** Name: IDuuid_to_string
**
** Description:
**	Format a UUID into a string:
**		aaaaaaaa-bbbb-cccc-dddd-eeeeeeee
**	aaaaaaaa	time_low
**	bbbb		time_mid
**	cccc		time_hi_and_version
**	dddd		clock_seq_hi_and_reserved
**	eeeeeeee	clock_seq_low (2) + node (6)
**
** Inputs:
**	uuid		pointer to the 128-bit UUID
**
** Outputs:
**	string		caller must have allocated at least 37 bytes
**			for the resulting string.
**
** History:
**	27-oct-1998 (canor01)
**	    Created.
*/
# ifndef IDuuid_to_string
STATUS
IDuuid_to_string( UUID *uuid, char *string )
{
    UUID_TIME uuidtime;

    uuidtime.I4.low = uuid->Data1;
    uuidtime.I2.high2 = uuid->Data2;
    uuidtime.I2.high1 = uuid->Data3;

    STprintf( string, 
	      "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
              uuidtime.I1.c[4],
              uuidtime.I1.c[5],
              uuidtime.I1.c[6],
              uuidtime.I1.c[7],
              uuidtime.I1.c[2],
              uuidtime.I1.c[3],
              uuidtime.I1.c[0],
              uuidtime.I1.c[1],
              uuid->Data4[0],
              uuid->Data4[1],
              uuid->Data4[2],
              uuid->Data4[3],
              uuid->Data4[4],
              uuid->Data4[5],
              uuid->Data4[6],
              uuid->Data4[7] );

    return (OK);

}
# endif /* IDuuid_to_string */

/*
** Name: IDuuid_substr_validate
**
** Description:
**		Validates the substring of the uuid to be having Hexadecimal values.
**		Validates the seperator '-' and the string termination '\0' for the 
**		substrings as well.
**      Allowed chanrecters in Substring: '0' to '9'
**										  'a' to 'f'
**										  'A' to 'F'
**										   Seperator '-' and terminator '\0'
** Inputs:
**      str, size of the subfield to be considered.		
**
** Outputs:
**      OK if valid
**		FAIL if invalid
**
** History:
**      21-Oct-2004 (shaha03)
**          Created.
**	28-Oct-2004 (shaha03)
**	    Replaced Hexa decimal checking with "CMhex" macro.
**	06-Nov-2006 (kiria01) b117042
**	    Conform CMhex macro call to relaxed bool form
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

# ifndef IDuuid_substr_validate
static STATUS
IDuuid_substr_validate(char *str, i4    size)
{
		i4 loop_cnt;
        for(loop_cnt=0; loop_cnt<size; loop_cnt++)
		{
			if(CMhex(&str[loop_cnt]))
				continue;
			else
				return (FAIL);
		}
		if( str[size] == EOS)
			return (OK);
		else
			return (FAIL);
}
#endif /*IDuuid_substr_validate*/

/*
** Name: IDuuid_from_string
**
** Description:
**      Convert a string into a UUID, where the string is of the format:
**              aaaaaaaa-bbbb-cccc-dddd-eeeeeeee
**      aaaaaaaa        time_low
**      bbbb            time_mid
**      cccc            time_hi_and_version
**      dddd            clock_seq_hi_and_reserved
**      eeeeeeee        clock_seq_low (2) + node (6)
**
** Inputs:
**      string
**
** Outputs:
**      uuid
**
** History:
**      17-nov-1998 (canor01)
**          Created.
**	07-dec-1998 (canor01)
**	    Source string must be 36 with NULL termination.
**	21-Oct-2004 (Shaha03)
**		Added calls to IDuuid_substr_validate to validate the substrings 
**		of each subpart.
*/
# ifndef IDuuid_from_string
STATUS
IDuuid_from_string( char *string, UUID *uuid )
{
    char *str;
    i4  i;
    i4  ii;
    UUID_TIME uuidtime;

	if ( !string )
		return (FAIL);
    if ( STlength( string ) != 36 )
        return (FAIL);

    str = STchr( string, '-' );

    for ( i = 0; i < 5 ; i++ )
    {
        if ( str )
            *str = EOS;

        switch ( i )
        {
            case 0:
				if(IDuuid_substr_validate(string, 8) != OK)
                        return (FAIL);
#if defined(axp_osf) || defined(LP64)
                STscanf( string, "%08x", &ii );
#else
                STscanf( string, "%08lx", &ii );
#endif
                uuidtime.I1.c[4] = (ii >> 24) & 0xff;
                uuidtime.I1.c[5] = (ii >> 16) & 0xff;
                uuidtime.I1.c[6] = (ii >> 8) & 0xff;
                uuidtime.I1.c[7] = ii & 0xff;
                uuid->Data1 = uuidtime.I4.low;
                break;
            case 1:
				if(IDuuid_substr_validate(string, 4) != OK)
                        return (FAIL);
                STscanf( string, "%04x",  &ii );
                uuidtime.I1.c[2] = (ii >> 8) & 0xff;
                uuidtime.I1.c[3] = ii & 0xff;
                uuid->Data2 = uuidtime.I2.high2;
                break;
            case 2:
				if(IDuuid_substr_validate(string, 4) != OK)
                        return (FAIL);
                STscanf( string, "%04x",  &ii );
                uuidtime.I1.c[0] = (ii >> 8) & 0xff;
                uuidtime.I1.c[1] = ii & 0xff;
                uuid->Data3 = uuidtime.I2.high1;
                break;
            case 3:
				if(IDuuid_substr_validate(string, 4) != OK)
                        return (FAIL);
                STscanf( string, "%02x",  &ii );
                uuid->Data4[0] = ii;
                string += 2;
                STscanf( string, "%02x",  &ii );
                uuid->Data4[1] = ii;
                break;
            case 4:
				if(IDuuid_substr_validate(string, 12) != OK)
                        return (FAIL);
                STscanf( string, "%02x",  &ii );
                uuid->Data4[2] = ii;
                string += 2;
                STscanf( string, "%02x",  &ii );
                uuid->Data4[3] = ii;
                string += 2;
                STscanf( string, "%02x",  &ii );
                uuid->Data4[4] = ii;
                string += 2;
                STscanf( string, "%02x",  &ii );
                uuid->Data4[5] = ii;
                string += 2;
                STscanf( string, "%02x",  &ii );
                uuid->Data4[6] = ii;
                string += 2;
                STscanf( string, "%02x",  &ii );
                uuid->Data4[7] = ii;
                break;
        }
        if ( i < 4 )
        {
			if(str == NULL)
				return (FAIL);
	        string = str + STlength(str) + 1;
       		str = STchr( string, '-' );
        }
    }

    return (OK);

}
# endif /* IDuuid_from_string */

/*
** Name: IDuuid_compare
**
** Description:
**	Perform a "lexical" comparison of two UUIDs.
**
** Inputs:
**	uuid1, uuid2	UUIDs to compare
**
** Returns:
**	-1	uuid1 < uuid2
**	0	uuid1 == uuid2
**	1	uuid1 > uuid2
**
** History:
**      17-nov-1998 (canor01)
**          Created.
**	19-oct-2001 (devjo01)
**	    Don't assume MEcmp, or what it resolves to, returns
**	    only -1, 0, and 1.  b106088.
*/

# ifndef IDuuid_compare
i4
IDuuid_compare( UUID *uuid1, UUID *uuid2 )
{
    i4 res;

    if ( uuid1->Data1 != uuid2->Data1 )
	res = ( MEcmp(&uuid1->Data1, &uuid2->Data1, sizeof(uuid1->Data1)) );
    else if ( uuid1->Data2 != uuid2->Data2 )
	res = ( MEcmp(&uuid1->Data2, &uuid2->Data2, sizeof(uuid1->Data2)) );
    else if ( uuid1->Data3 != uuid2->Data3 )
	res = ( MEcmp(&uuid1->Data3, &uuid2->Data3, sizeof(uuid1->Data3)) );
    else if ( (res = MEcmp(uuid1->Data4, uuid2->Data4, sizeof(uuid1->Data4)))
	      == 0 )
	return 0;
    return ( res < 0 ? -1 : 1 );
}
# endif /* !defined(IDuuid_compare) */

#if !defined(TYPE_LONGLONG_OK)
static void ItoS64( u_i4 in, S64 ans)
{
   i4 i;
   for (i=0; i < 2; i++)
   {
      ans[i] = in & 0xffff;
      in >>= 16;
   }
   ans[2]=ans[3]=0;
}

static void add64(S64 a, S64 b, S64 ans)
{
   u_i4 intermed;
   u_i4 carry;
   i4 i;
   carry = 0;
   for (i=0; i < 4; i++)
   {
      intermed = a[i]+b[i] + carry;
      if (intermed > 65535L)
         carry = 1;
      else
         carry = 0;
      ans[i] = (u_i4)intermed & 0xffff;
   }
}

static void copy64(S64 a, S64 ans)
{
   i4 i;
   for (i=0; i < 4; i++)
      ans[i] = a[i];
}

static void mul64(S64 a, S64 b, S64 ans)
{
   u_i4 temp[7];
   u_i4 intermed;
   i4 i,j,k;

   for (i=0; i < 8; i++)
      temp[i] = 0;

   for (i=0; i < 4; i++)
      for (j=0; j < 4; j++)
      {
         intermed = a[i] * b[j];

         temp[i+j] += intermed & 0xffff;
         temp[i+j+1] += intermed >> 16;
         for(k=i+j;k < 7;k++)
         {
             if(temp[k] > 65535L)
                 temp[k] = temp[k] & 0xffff, temp[k+1]++;
         }
      }

   for (i=0; i < 4 ; i++)
      ans[i] = temp[i];

}
#endif /* TYPE_LONGLONG_OK */
