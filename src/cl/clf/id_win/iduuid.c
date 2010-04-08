/*
** Copyright (c) 1998 Ingres Corporation
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
# include	<cm.h>

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
**Name: IDuuid_create_seq
**      For security reasons, UuidCreate was modified so that it no longer uses
**      a machine's MAC address to generate UUIDs. UuidCreateSequential was
**      introduced to allow creation of UUIDs using the MAC address of a
**      machine's Ethernet card.
**      In Windows XP/2000, the UuidCreate function generates a UUID that
**      cannot be traced to the ethernet/token ring address of the computer on
**      which it was generated. It also cannot be associated with other UUIDs
**      created on the same computer. If you do not need this level of security
**      your application can use the UuidCreateSequential function, which
**      behaves exactly as the UuidCreate function does on all other versions
**      of the operating system.
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
** History:
**	15-oct-1998 (canor01)
**	    Created.
**	18-nov-1998 (canor01)
**	    Add IDuuid_from_string.
**      08-dec-1998 (canor01)
**          Correct logic in IDuuid_from_string.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	21-Oct-2004 (shaha03)
**          Added function IDuuid_substr_validate to validate the UUID string.
**	11-feb-2005 (raodi01)
**	    Added IDuuid_create_seq definition
*/

# define NODESIZE 6 /* 6 bytes in address */

# define HNANOPERSEC ((longlong)10000000)
# define HNANOPERUSEC (HNANOPERSEC / 1000000)

# define PRE1970DAYS 141427  /* from 15-oct-1582 to 1-jan-1970 */
# define PRE1970MINS ((longlong)(PRE1970DAYS * 24 * 60))
# define PRE1970SECS ((longlong)(PRE1970MINS * 60))
# define PRE1970HNANO ((longlong)(PRE1970SECS * HNANOPERSEC))

static bool IDuuid_init = FALSE;
static u_char uuid_node[NODESIZE] ZERO_FILL;

# ifndef IDuuid_create

STATUS
IDuuid_create(
          UUID *uuid
)
{
    /* [...] use Microsoft function */
}

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
*/
static
longlong
IDuuid_time()
{
    [...]
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
*/
static
u_i2
IDuuid_sequence(i4 seed)
{
    [...]
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
*/
static
STATUS
IDuuid_node(u_char *nodeid)
{
    [...]
}
# endif /* ! defined(IDuuid_create) */

# ifndef IDuuid_create_seq

STATUS
IDuuid_create_seq(
          UUID *uuid
)
{
    /* [...] use Microsoft function */
}
# endif
 
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
    STprintf( string, 
	      "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
              uuid->Data1,
              uuid->Data2,
              uuid->Data3,
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
**		Validates the seperator '-' and the string termination '\0' for the substrings as well.
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
**      28-Oct-2004 (shaha03)
**          Replaced Hexa decimal checking with "CMhex" macro.
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
**      07-dec-1998 (canor01)
**          Source string must be 36 with NULL termination.
**		21-Oct-2004 (Shaha03)
**			Added calls to IDuuid_substr_validate to validate the substrings of each subpart.
*/
# ifndef IDuuid_from_string
STATUS
IDuuid_from_string( char *string, UUID *uuid )
{
    char *str;
    i4  i;
    int ii;
    i4  lint;

	if ( !string )
		return (FAIL);
    if ( STlength( string ) != 36 )
        return (FAIL);

    str = STindex( string, "-", 0 );

    for ( i = 0; i < 5 ; i++ )
    {
        if ( str )
            *str = EOS;
	    switch ( i )
        {
            case 0: 
				if(IDuuid_substr_validate(string, 8) != OK)
                        return (FAIL);
                STscanf( string, "%08lx", &lint );
                uuid->Data1 = lint;
                break;
            case 1: 
                if(IDuuid_substr_validate(string, 4) != OK)
                        return (FAIL);
                STscanf( string, "%04x",  &ii );
                uuid->Data2 = ii;
                break;
            case 2: 
                if(IDuuid_substr_validate(string, 4) != OK)
                        return (FAIL);
                STscanf( string, "%04x",  &ii );
                uuid->Data3 = ii;
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
            str = STindex( string, "-", 0 );
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
*/

# ifndef IDuuid_compare
i4 
IDuuid_compare( UUID *uuid1, UUID *uuid2 )
{
    i4  res;

    if ( uuid1->Data1 != uuid2->Data1 )
	return ( uuid1->Data1 < uuid2->Data1 ? -1 : 1 );
    if ( uuid1->Data2 != uuid2->Data2 )
	return ( uuid1->Data2 < uuid2->Data2 ? -1 : 1 );
    if ( uuid1->Data3 != uuid2->Data3 )
	return ( uuid1->Data3 < uuid2->Data3 ? -1 : 1 );
    if ( (res = MEcmp(uuid1->Data4, uuid2->Data4, sizeof(uuid1->Data4))) == 0 )
	return 0;
    else
	return ( res < 0 ? -1 : 1 );
}
# endif /* !defined(IDuuid_compare) */
