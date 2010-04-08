/*
** Copyright (c) 1998, 2008 Ingres Corporation
**
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
#include <dcdef.h>
# include  	<cs.h>
#include <descrip.h>
#include <devdef.h>
#include <dvidef.h>
#include <dvsdef.h>
#include <efndef.h>
#include <ints.h>
#include <iledef.h>
#include <iodef.h>
#include <iosbdef.h>
#include <nmadef.h>
#include <ssdef.h>
#include <ucbdef.h>
#include <starlet.h>
#include <vmstypes.h>

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
**	15-oct-1999 (kinte01)
**	    Created from Unix CL version.
**	06-Apr-2001 (bonro01)
**	    Modified the UUID time field to be in Big Endian format for
**	    all platforms to make it consistent, and allow cross-platform
**	    UUID comparison.
**	    Added time check to insure that the current time value is
**	    always incrementing to insure a unique uuid value.
**	04-jun-2001 (kinte01)
**	   Replace STindex with STchr
**	19-oct-2001 (devjo01)
**	    In IDuuid_compare don't assume MEcmp, or what it resolves to,
**	    returns only -1, 0, and 1.  b106088.
**	17-dec-2003 (abbjo03)
**	    Implement IDuuid_node properly on Alpha VMS. Integrated changes
**	    449888 (Apr 2001) and 453828 (Oct 2001) from Unix CL.
**	28-apr-2005 (raodi01)
**	    Following the changes made for b110668,  which was a
**	    windows-specific one
**	    added changes to handle the change on unix and VMS
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
**	28-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
**      6-Nov-2009 (maspa05) b122642
**          Protect update of ID_uuid_last_time which is used to generate uuid 
**          time component. Previously this could allow duplicates.
**          In order to do this across all servers a 'last time' variable
**          was added to LGK shared memory and a mutex to protect it. 
**          However ID gets linked to frontend programs so we use pointers
**          which if not initialized in LGKinitialize to the shared memory 
**          version, are set to a process-specific variable here. 
*/

# define NODESIZE 6 /* 6 bytes in address */

# define HNANOPERSEC ((longlong)10000000)

# define PRE1970DAYS 141427  /* from 15-oct-1582 to 1-jan-1970 */
# define PRE1970MINS ((longlong)(PRE1970DAYS * 24 * 60))
# define PRE1970SECS ((longlong)(PRE1970MINS * 60))
# define PRE1970HNANO ((longlong)(PRE1970SECS * HNANOPERSEC))

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
     /* Not to be defined for Unix (or VMS) */
        return IDuuid_create(uuid);
 }
# endif

/* Name: IDuuid_time()
**
** Description:
**	Number of 100-nanosecond intervals since 15-oct-1582 00:00:00.00
**
**	"The timestamp is a 60 bit value. For UUID version 1, this is
**	represented by Coordinated Universal Time (UTC) as a count of 100-
**	nanosecond intervals since 00:00:00.00, 15 October 1582 (the date of
**	Gregorian reform to the Christian calendar.
**
**	"For systems that do not have UTS available, but do have local time, 
**	they MAY use local time instead of UTC, as long as they do so 
**	consistently throughout the system.  This is NOT RECOMMENDED, however,
**	and it should be noted that ll that is needed to generate UTC, given
**	local time, is a time zone offset."
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
    longlong hnanosec;

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
    if( cur.tv_sec < ID_uuid_last_time_ptr->tv_sec ||
	(cur.tv_sec == ID_uuid_last_time_ptr->tv_sec &&
	(cur.tv_nsec - ID_uuid_last_time_ptr->tv_nsec) < 100) )
    {
	cur = *ID_uuid_last_time_ptr;
	cur.tv_nsec += 100; /* 100 is the minimum that we can add */
	if(cur.tv_nsec >= NANOPERSEC)
	{
	    cur.tv_sec++;
	    cur.tv_nsec = 0;
	}
    }
    *ID_uuid_last_time_ptr=cur;

    ID_UUID_SEM_UNLOCK(ID_uuid_sem_ptr);

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
** History:
**	17-dec-2003 (abbjo03)
**	    Replace Unix code by proper VMS implementation.
*/
static
STATUS
IDuuid_node(u_char *nodeid)

{
# define have_uuid_node_code
    int status;
    bool found = FALSE;
    char device_name[64+1];
    struct dsc$descriptor_s devnam =
	{ sizeof(device_name) - 1, DSC$K_DTYPE_T, DSC$K_CLASS_S, device_name };
    struct dsc$descriptor_s srch_devnam =
	{ 1, DSC$K_DTYPE_T, DSC$K_CLASS_S, "*" };
    int devclass_code = DC$_SCOM;
    int dev_status;
    int dev_char;
    uint64 contxt = 0;
    ILE3 devclass[2];
    ILE3 devinfo[3];
    unsigned short reslen[3];
    IOSB iosb;
    II_VMS_CHANNEL chan;
    char dev_attribs[256];
    struct { int buflen; void *bufadr; } attdsc =
	{ sizeof(dev_attribs), dev_attribs };
#pragma member_alignment __save
#pragma nomember_alignment
    struct nma_attrib
    {
	short type;
	union
	{
	    int intval;
	    struct { short len; u_char str[6]; } strval;
	} value;
    } *nma_attr;
#pragma member_alignment __restore

    /* initialize item list for sys$device_scan() */
    devclass[0].ile3$w_length = sizeof(devclass_code);
    devclass[0].ile3$w_code = DVS$_DEVCLASS;
    devclass[0].ile3$ps_bufaddr = &devclass_code;
    devclass[0].ile3$ps_retlen_addr = &reslen[0];
    devclass[1].ile3$w_length = devclass[1].ile3$w_code = 0;
    /* initialize item list for sys$getdviw() */
    devinfo[0].ile3$w_length = sizeof(dev_status);
    devinfo[0].ile3$w_code = DVI$_STS;
    devinfo[0].ile3$ps_bufaddr = &dev_status;
    devinfo[0].ile3$ps_retlen_addr = &reslen[1];
    devinfo[1].ile3$w_length = sizeof(dev_char);
    devinfo[1].ile3$w_code = DVI$_DEVCHAR;
    devinfo[1].ile3$ps_bufaddr = &dev_char;
    devinfo[1].ile3$ps_retlen_addr = &reslen[2];
    devinfo[2].ile3$w_length = devinfo[2].ile3$w_code = 0;

    while ((status = sys$device_scan(&devnam, &devnam.dsc$w_length,
	    &srch_devnam, devclass, &contxt)) == SS$_NORMAL)
    {
	device_name[devnam.dsc$w_length] = EOS;
	status = sys$getdviw(EFN$C_ENF, 0, &devnam, devinfo, &iosb, 0, 0, 0); 
	if (status != SS$_NORMAL || iosb.iosb$w_status != SS$_NORMAL)
	    break;

	/* Only process non-mailbox network template devices */
	if ((dev_status & UCB$M_TEMPLATE) &&
	    (dev_char & DEV$M_MBX | DEV$M_NET) == DEV$M_NET)
	{
	    if (sys$assign(&devnam, &chan, 0, 0) != SS$_NORMAL)
		break;

	    status = sys$qiow(EFN$C_ENF, chan, IO$_SENSEMODE | IO$M_CTRL, &iosb,
		              0, 0, 0, &attdsc, 0, 0, 0, 0);
	    if (status == SS$_NORMAL && iosb.iosb$w_status == SS$_NORMAL)
	    {
		nma_attr = (struct nma_attrib *)dev_attribs;
		while ((char *)nma_attr < dev_attribs + sizeof(dev_attribs))
		{
		    if ((nma_attr->type & 0xfff) == NMA$C_PCLI_HWA)
		    {
			MEcopy(nma_attr->value.strval.str, 6, nodeid);
			found = TRUE;
			break;
		    }
		    else if ((nma_attr->type & 0x1000) == 0)
		    {
			nma_attr = (struct nma_attrib *)((char *)nma_attr +
			    sizeof(short) + sizeof(int));
		    }
		    else
		    {
			nma_attr = (struct nma_attrib *)((char *)nma_attr +
			    sizeof(short) * 2 + nma_attr->value.strval.len);
		    }
		}
	    }
	    status = sys$dassgn(chan);
	    if (found)
		break;
	}
    }

    return (found ? OK : FAIL);

# ifndef have_uuid_node_code
# error Missing OS-specific support for IDuuid_node.
# endif
}
# endif /* ! defined(IDuuid_create) */

/* 
** Name: IDuuid_to_string
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
*/
# ifndef IDuuid_from_string
STATUS
IDuuid_from_string( char *string, UUID *uuid )
{
    char *str;
    i4 i;
    int ii;
    UUID_TIME uuidtime;

    if ( STlength( string ) != 36 )
        return (FAIL);

    str = STchr( string, '-');

    for ( i = 0; i < 5 && str != NULL; i++ )
    {
        if ( str )
            *str = EOS;

        switch ( i )
        {
            case 0:
                STscanf( string, "%08lx", &ii );
		uuidtime.I1.c[4] = (ii >> 24) & 0xff;
		uuidtime.I1.c[5] = (ii >> 16) & 0xff;
		uuidtime.I1.c[6] = (ii >> 8) & 0xff;
		uuidtime.I1.c[7] = ii & 0xff;
		uuid->Data1 = uuidtime.I4.low;
                break;
            case 1:
                STscanf( string, "%04x",  &ii );
		uuidtime.I1.c[2] = (ii >> 8) & 0xff;
		uuidtime.I1.c[3] = ii & 0xff;
		uuid->Data2 = uuidtime.I2.high2;
                break;
            case 2:
                STscanf( string, "%04x",  &ii );
		uuidtime.I1.c[0] = (ii >> 8) & 0xff;
		uuidtime.I1.c[1] = ii & 0xff;
		uuid->Data3 = uuidtime.I2.high1;
                break;
            case 3:
                STscanf( string, "%02x",  &ii );
                uuid->Data4[0] = ii;
                string += 2;
                STscanf( string, "%02x",  &ii );
                uuid->Data4[1] = ii;
                break;
            case 4:
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
	res = MEcmp(&uuid1->Data1, &uuid2->Data1, sizeof(uuid1->Data1));
    else if ( uuid1->Data2 != uuid2->Data2 )
	res = MEcmp(&uuid1->Data2, &uuid2->Data2, sizeof(uuid1->Data2));
    else if ( uuid1->Data3 != uuid2->Data3 )
	res = MEcmp(&uuid1->Data3, &uuid2->Data3, sizeof(uuid1->Data3));
    else if ( (res = MEcmp(uuid1->Data4, uuid2->Data4, sizeof(uuid1->Data4)))
		== 0 )
	return 0;
    return ( res < 0 ? -1 : 1 );
}
# endif /* !defined(IDuuid_compare) */
