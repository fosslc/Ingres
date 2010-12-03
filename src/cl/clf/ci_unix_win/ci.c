/*
**  Copyright (c) 1985, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<clconfig.h>
# include	<bt.h>
# include	<ci.h>
# include	<cv.h>
# include	<er.h>
# include	<gl.h>
# include	<me.h>
# include	<nm.h>
# include	<st.h>
# include	<tm.h>
# include	<pm.h>
# include	<pc.h>
# include	<si.h>
# include	"cilocal.h"

/*
** Name:    CI.c    - This module is used for the coding and decoding of 
**		information.
**
** Description:
**        This module was originally written for encoding and decoding
**	authorization strings in the Ingres Distribution Keys project.
**	  The encryption and decryption algorithms are intended to be general
**	purpose enough to be used by the rest of the system where encoding
**	and decoding routines are required.
**
**	  The following routines are intended to be used by RTI system code:
**
**	CIchksum()	A function that computes the checksum of a binary
**			byte stream.
**      CIdecode()	This routine decodes cipher text to plain text using
**			a key string.
**      CIencode()      This routine encodes plain text to cipher text
**			using a key string.
**	CItobin()	This routine maps a string of characters to a block of
**			binary bytes.  The string of characters must have been
**			originally created using CItotext().
**	CItotext()	This routine maps binary data to a printable alphabet 
**			consisting of upper case letters excluding 'I' and 'O' and
**			digits excluding '0' and '1'.
**
** History:
 * Revision 1.2  88/10/25  14:00:23  mikem
 * some spec changes
 * 
 * Revision 1.1  88/08/05  13:26:16  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.5  88/01/07  13:00:11  mikem
**      disable ci for now.
**      
**      Revision 1.4  87/11/30  17:05:59  mikem
**      return OK all the time.
**      
**      Revision 1.3  87/11/17  15:37:19  mikem
**      changed name of local hdr to cilocal.h from ci.h
**      
**      Revision 1.2  87/11/09  15:07:25  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.2  87/07/27  11:10:46  mikem
**      Updated to meet jupiter coding standards.
**      
**	10-sep-86 (ericj)
**		written.
**	13-mar-86 (ericj)
**		Fix call to CVal() in CIcapability.
**	14-mar-86 (ericj)
**		Temporarily turn off cluster checking in CIcapability routine.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**      30-jan-1989 (roger)
**          Reinstated CIcapability().
**	28-jul-89 (jrb)
**	    Check for special authorization string which is now disallowed
**	    because it was mistakenly distributed in the I&O guide.
**      08-mar-91 (rudyw)
**          Comment out text following #endif
**	04-oct-92 (ed)
**	    add bt.h
**	23-dec-92 (mikem)
**	    su4_us5 port.  Added a cast to remove acc warning about "semantics 
**	    of ">=" change in ANSI C;"
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      11-aug-93 (ed)
**          added missing includes
**	25-Jan-1994 (jpk)
**	    As per Authorization String Plan for 6.5, get rid of
**	    nearly-useless processor ID check, which had degenerated
** 	    to a processor model check only for VAX processors only.
**	    In return get back 32 more capabbility bits.
**	8 April 1994 (jpk)
**		Change II_AUTHORIZATION to II_AUTH_STRING.
**	14-apr-95 (emmag)  
**	    NT, OS/2 and Netware do not support Authorization Strings.
**	    Use xCL_NO_AUTH_STRING_EXISTS.
**	04-apr-1996 (thoda04)
**	    Added i4  as the return type to CIcapability()
**	    and CImk_capability_block().  They were missing altogether.
**	07-feb-1997 (canor01)
**	    Remove special authorization string that was mistakenly
**	    distributed in the I&O guide, since it is no longer valid
**	    with this new major release and subsequent DIST_KEY.
**	14-may-97 (mcgem01)
**	    Clean up compiler warnings.
**	06-may-1998 (canor01)
**	    Allow use of ca_license_check().
**	27-may-1998 (canor01)
**	    Include clconfig.h. Return error if requesting invalid capability
**	    bit.
**	28-may-1998 (canor01)
**	    Load the CA Licensing API shared library dynamically to avoid 
**	    having to explicitly link it in with every executable.
**	08-jun-1998 (musro02)
**	    Add sqs_ptx to platforms using the CA Licensing API (above)
**	08-Jun-1998 (allmi01)
**	    Add dgi_us5 to list of platforms to use the dynamic linking
**	    to the CA license code.
**	19-jun-1998 (muhpa01)
**	    Add code to dynamically load CA Licensing with shl_load/shl_findsym
**	    on hp8_us5.  Also, incorporate Orlan's 19-jun-1998 changes for
**	    dlopen into hp8_us5's use of shl_load().
**	24-Jun-1998 (allmi01)
**	    Corrected change by canor01 which mutexed the licensing code for
**	    all platforms instead of those which use OS threads only.
**	24-jun-1998 (canor01)
**	    Add include of cs.h.
**	25-Jun-1998 (merja01)
**		Add axp_osf to the list of platforms who load licensing
**		dynamically with dlopen.
**	07-jul-1998 (popri01)
**	    Enable Orlan's licensing code for Unixware (usl us5).
**	08-Jul-1998 (walro03)
**	    Add dr6_us5 to list of platforms to use the dynamic linking
**	    to the CA license code.
**	04-aug-98 (mcgem01)
**	    Add OpenROAD Development package for Enterprise Edition.
**      01-sep-98 (mcgem01)
**          Replace ca_license_check with ca_license_check_m and write
**	    all license messages to the errlog.
**	02-sep-98 (mcgem01)
**	    Don't report license success to the errlog.  Also check for
**	    a null license string.
**	02-sep-98 (mcgem01)
**	    Don't print empty license messages to the log.  Also, fix
**	    problem whereby we were truncating the messages prematurely.
**	11-sep-98 (mcgem01)
**	    Add a licensing bit for ICE.
**      11-Nov-98 (linke01)
**          Added pym_us5 and rmx_us5 to the list of platforms which load
**          licensing dynamically with dlopen.     
**	30-nov-98 (toumi01)
**	    For the free Linux version hack the II_AUTH_STRING code to
**	    check only the date in the string, so that we can ship with
**	    a string with NO products authorized (useless for other
**	    Ingres versions).  Thus, II_AUTH_STRING becomes just a "time
**	    bomb" enabler.
**	07-jan-1999 (canor01)
**	    Add new license component code for TNG_EDITION.
**      18-jan-1999 (matbe01)
**          Add nc4_us5 to list of platforms to use the dynamic linking
**          to the CA license code.
**	07-Feb-1999 (kosma01)
**	    Enable Orlan's licensing code for IRIX64 (sgi_us5).
**	29-jun-1999 (somsa01 for canor01)
**	    Added CIvalidate() to return the co. name and site-id from CA-lic.
**	    Moved license component code from CIcapability to CI_getcompcode.
**      03-jul-99 (podni01)
**          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**	07-jul-1999 (somsa01)
**	    Changed references from nat to i4 introduced by previous cross
**	    integration.
**      21-aug-1999 (mcgem01)
**          Correct the parameters passed to the workgroup license
**          check routine.
**          License failure in the Workgroup Edition should be a soft
**          fail.
**	01-Oct-1999 (toumi01)
**	   Added lnx_us5 header and load ifdefs for CA licensing.
**	06-oct-1999 (toumi01)
**	   Change Linux config string from lnx_us5 to int_lnx.
**	04-nov-1999 (toum01)
**	   Modify Workgroup Edition licensing to support Linux (int_lnx).
**	11-nov-1999 (mcgem01)
**	    The black-box version of Ingres built for ACCPAC should
**	    not do any license enforcement.
**	09-may-2000 (thoda04)
**	    Disable CI check for XA (CI_XA_TUXEDO, CI_XA_CICS, CI_XA_ENCINA).
**	31-mar-2000 (mcgem01)
**	   Add 'Embedded Ingres' component codes for all products, not just
**	   the backend.
**      05-Apr-2000 (hweho01)
**          Add ris_u64 to list of platforms to use the dynamic linking
**          to the CA license code.
**	19-may-2000 (mcgem01)
**	    The calls to PMinit in CIcapablity and CIvalidate should be 
**	    followed by calls to PMload to load the PM context.  Also
**	    generecised this code - there should be no hard-coded references 
**	    to ii.
**	19-May-2000 (hanch04)
**	    Don't exit if PMload fails.  ingbuild checks to see if licensing
**	    is installed, but there is no config.dat yet.
**	22-may-2000 (somsa01)
**	    The recent adds of PMget() calls on embed_installation will
**	    cause a memory leak, as it will be done on any connect. Also,
**	    on HP-UX 11.0 the admin thread in the DBMS server SEGVs in
**	    PMload() for some reason during a RUN_ALL run. Therefore, we
**	    will now do this just one and save the value as a static.
**	    Also, in CIvalidate(), CI_getcompcode() was passing FALSE in as
**	    the second parameter, but it should now be passing
**	    embed_installation.
**	22-May-2000 (ahaal01)
**	    Add rs4_us5 to list of platforms to use the dynamic linking
**	    to the CA license code.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	14-jun-2000 (hanje04)
**	   Added ibm_lnx to free Linux version II_AUTH_STRING hack; and ifdefs 
**	   for CA licensing
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	7-Sep-2000 (hanje04)
**	    Added axp_lnx (Alpha Linux) header and load ifdefs for licensing,
**	    including WG support modifications.
**	09-sep-2000 (somsa01)
**	    PMinit() can also return PM_NO_INIT, which means that it's
**	    already been initialized.
**	30-jan-2001 (shust01)
**	    PMinit() has previously been changed to return PM_DUP_INIT,
**	    instead of PM_NO_INIT (change #448380).  Change code here to
**	    reflect this. PMinit() was returning PM_DUP_INIT, and we 
**	    skipped check for embed_installation.  So, for an Embedded 
**	    install, we were using the wrong license string. bug 103802.
**	22-sep-2000 (holgl01) bug 102653
**      The replicator non-embedded value for licensing needs to be 2JG2 instead
**      of 1JG2.  Final fix to bug 101325.
**	22-sep-2000 (somsa01)
**	    Amended previous change with proper multiple status check.
**	20-apr-1999 (hanch04)
**	    Load lic98_lp64.so for 64 bit OS
**	08-feb-2001 (somas01)
**	    Do not go through the LP64 code if NT_GENERIC is defined, as the
**	    look of an NT DLL does not match the look of a UNIX shared
**	    library.
**	22-Feb-2001 (hweho01)
**          Added liblic98.so as the shared lib name for dlopen() to search 
**          after lic98.so. The name of license library is lic98.o  
**          (provided by licensing group) for AIX (rs4_us5 and ris_u64)  
**          platforms.   
**	20-jul-2001 (stephenb)
**	    Add support for i64_aix
**	07-Sep-2001 (hanch04)
**	    Look for the lic98.so in /opt/CA/CAlib
**	07-Sep-2001 (hanch04)
**	    Added too many else in last change.
**	25-oct-2001 (devjo01)
**	    Make axp_osf an exception to the 64 bit platforms using
**	    lic98_lp64.so as their licensing library.
**	01-nov-2001 (devjo01)
**	    Per new licensing policy, CIcapability will always
**	    return OK, even if LIC_TERMINATE returned.
**	09-nov-2001 (devjo01)
**	    Hook in ca_license_release, and ca_license_log_usage.
**	12-Nov-2001 (hanje04)
**	    Removed dump of license information from CIcapability as it break
**	    iisudbms on UNIX.
**    03-Dec-2001 (hanje04)
**	  Added support for IA64 Linux (i64_lnx)
**	14-Dec-2001 (somsa01)
**	    Corrected retrieval of license checking function from license
**	    DLL for i64_win. Also, do not use ci_release_licenses() for now
**	    on Win64. In Licensing, ca_license_release() does nothing for
**	    now, and the invocation of ci_release_licenses() causes a SEGV
**	    in LIC98_64.DLL.
**	22-May-2000 (ahaal01)
**	    Add rs4_us5 to list of platforms to use the dynamic linking
**	    to the CA license code.
**	14-may-2002 (devjo01)
**	    Coerce user count to zero for CI_BACKEND also.
**	22-may-2000 (holgl01) bug 102653
**      The replicator non-embedded value for licensing needs to be 2JG2 instead
**      of 1JG2.  Final fix to bug 101325.
**      26-jul-2002 (mcgem01)
**          Add a licensing component code 2SLS for the spatial object
**          library.
**	31-jul-2002 (devjo01) bug 108428
**	    Modified change 455847 (addresses EDBC b107177, no 
**	    license component codes), to surpress unwanted CI error
**	    codes, and add desired timestamps.
**	08-aug-2002(somsa01)
**	    Changes for HP-UX 64-bit.
**	18-Sep-2002(hweho01)
**	    Changes for AIX 64-bit.
**	07-Oct-2002 (hanje04)
**	    As part of AMD x86-64 Linux port replace various linux defines
**	    with generic LNX define where apropriate.
**	22-nov-2002(mcgem01)
**	    Added a licensing component code of 2AOT for Advantage OpenROAD
**	    Transformation Runtime.
**	18-jun-2003 (somsa01)
**	    For HP PA-RISC 32-bit, updated the location of CAlib in
**	    CIcapability().
**	02-oct-2003 (somsa01)
**	    Ported to NT_AMD64.
**	14-may-2004 (somsa01)
**	    Corrected licensing file location for 64-bit Windows.
**	07-jun-2004 (somsa01)
**	    Updated call to RegQueryValueEx() such that we initialize dwSize
**	    in CIcapability().
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**      29-Nov-2010 (frima01) SIR 124685
**          Removed forward declarations - they reside in ci.h.
*/

/* # defines */
# define	BIT_CNT		5
#define CI_i4swap(x) ((((x) & 0x000000ff) << 24) | (((x) >> 24) & 0x000000ff) \
				| (((x) & 0x0000ff00) << 8) | (((x) & 0x00ff0000) >> 8))
# define CI_i2swap(x) ((((x) & 0x00ff) << 8) | (((x) >> 8) & 0x00ff))
# define	CCITT_INIT	0x0ffff
# define	CCITT_MASK	0102010

/* statics */
/*
**   Define a bit mask which will be used by CItotext and CItobin to clear
** all bits in the transformation register, other than the bits that haven't
** yet been processed.
*/
static	i4	bit_mask[] = {
		0, 01, 03, 07, 017
};


/*{
** Name:	CIchksum	- Compute a check sum for a stream of bytes.
**
** Description:
**	   This procedure computes a non-unique checksum for a stream of bytes.
**	Actually this routine produces a CCITT cyclic redundancy code.
**
** Inputs:
**	block	pointer to a stream of binary bytes whose checksum is to be
**		computed.
**	size	number of bytes in the binary byte stream.
**
** Outputs:
**	Returns:
**		a BITFLD, which is the computed checksum.
**
** History:
**	19-sep-85 (ericj) - written.
**	5-may-86 (ericj)
**		Made parameters, size and inicrc, i4s as requested by
**		CL committee.
*/

BITFLD
CIchksum(u_char *input, i4 size, i4 inicrc)
{
			i4	i;
	register	BITFLD	crc;

	crc = inicrc;
	while (size--)
	{
		crc ^= *input++;
		for (i = 8; i; i--)
		{
			if (crc & 01)
			{
				crc >>= 1;
				crc ^= CCITT_MASK;
			}
			else {
				crc >>= 1;
			}
		}
	}
	return(crc);
}



/*{
** Name:	CIdecode - Decode cipher text to plain text using key.
**
** Description:
**	  This is a general purpose encryption routine designed to decrypt
**	binary cipher text into binary plain text using a block cipher and key
**	schedule to perform the transformation.  IMPORTANT NOTE, This is a block
**	cipher that operates on CRYPT_SIZE bytes of data at a time and expects
**	the size of the cipher text to be evenly divisible by CRYPT_SIZE.
**
** Inputs:
**	cipher_text	pointer to binary cipher text to be decoded.  This is
**			interpreted as a bit stream.
**	size		size in bytes of cipher text to be decoded.  This size
**			should be evenly divisible by CRYPT_SIZE.
**	ks		key schedule used to transform cipher text back to plain
**			text.  Must be the same key that was used to originally
**			encrypt the cipher text.
** Outputs:
**	plain_text	pointer to byte array to hold decoded cipher text.
**			Must be at least as big as cipher text being decoded.
**	
** History:
**	10-sep-85 (ericj) - written.
**	5-may-86 (ericj)
**		Made size parameter a i4 as requested by CL committee.
*/
VOID
CIdecode(PTR cipher_text, i4 size, CI_KS ks, PTR plain_text)
{
	/* Define an array to hold CRYPT_SIZE bytes worth of unpacked bits */
	char	bit_map[BITS_IN_CRYPT_BLOCK];

	/* for length of cipher text */
	while (size > 0)
	{
		/*
		** expand 8 cipher text bytes to 64 bit representation,
		** update pointer to next byte to be deciphered, and
		** update count of bytes left to be decoded
		*/
		CIexpand(cipher_text, bit_map);
		cipher_text += 8;
		size -= 8;

		/* decrypt an 8 byte block of cipher text */
		CIencrypt(ks, TRUE, bit_map);

		/*
		** pack 64 decoded bits back to 8 byte representation and
		** update pointer to next plain text position to be filled.
		*/
		CIshrink(bit_map, plain_text);
		plain_text += 8;
	}
	return;
}



/*{
** Name:	CIencode	- encode plain text to cipher text.
**
** Description:
**	  This routine encodes binary plain text to a binary cipher text using
**	a block cipher and key string to perform the transformation.  IMPORTANT
**	NOTE, this routine uses a block cipher that operates on CRYPT_SIZE bytes of data
**	at a time.  Thus, the size of the cipher text being operated on should
**	be padded so that it is evenly divisible by CRYPT_SIZE.
**
** Inputs:
**	plain_text	pointer to binary plain text to be encripted.
**	size		number of bytes in plain text to be encripted.  This
**			should be evenly divisible by CRYPT_SIZE.
**	ks		key schedule to be used in the encryption.
**
** Outputs:
**	cipher_text	pointer to byte array which will hold the encoded
**			binary text.  This array must be at least as big
**			as the cipher text being encrypted.
**
** History:
**	10-sep-85 (ericj) - written.
**	5-may-86 (ericj)
**		Made size parameter a i4 as requested by CL committee.
*/
VOID
CIencode(PTR plain_text, i4 size, CI_KS ks, PTR cipher_text)
{
	/* Define an array to hold CRYPT_SIZE bytes worth of unpacked bits */
	char	bit_map[CRYPT_SIZE * BITSPERBYTE];

	/* while there's still plain text to be encoded */
	while (size > 0)
	{
		/*
		** unpack 8 bytes of plain text bits to 64 byte
		** representation, update pointer to next byte to
		** be encrypted, and count of bytes left to be
		** encrypted.
		*/
		CIexpand(plain_text, bit_map);
		plain_text += 8;
		size -= 8;

		/* encrypt the bit map */
		CIencrypt(ks, FALSE, bit_map);
		/*
		** pack the bits back into a encrypted 8 byte
		** representaion and update pointer to next empty
		** byte in cipher text array.
		*/
		CIshrink(bit_map, cipher_text);
		cipher_text += 8;
	}
	return;
}

/*{
** Name:	CItobin	- map printable characters to binary data.
**
** Description:
**	    This routine maps a printable string defined over the alphabet
**	of upper case letters excluding 'I' and 'O' and digits excluding '0'
**	and '1' to a sequence of binary bytes.  Note, this routine is 
**	intended to be used only with text strings that were originally
**	generated using CItobin.
**
** Inputs:
**	textbuf	pointer to printable characters to be mapped to binary bytes.
**
** Outputs:
**	size	pointer to i4  for returning resulting binary block size.
**	block	pointer to sequence of bytes to hold mapped binary bytes.
**		  IMPORTANT NOTE, there must be at least
**		CI_TOBIN_SIZE_MACRO(STlength(textbuf)) number of bytes available.
**		If this argument is going to be passed to CIdecode or CIencode, its
**		size in bytes should be evenly divisible by CRYPT_SIZE.
**
** History:
**	16-sep-85 (ericj) - written.
**	05-jan-86 (ericj) - modified to map from a different printable alphabet as
**			generated by CItotext.
**	14-apr-95 (emmag)   
**		Desktop compilers insist that register variables be 
**		accompanied by a type name.
**		
*/

VOID
CItobin(char *textbuf, i4 *size, u_char *block)
{
	register	i4 t_reg = 0;	/* transformation register */
	register	i4 c;		/* character being upmapped */
	i4		i;
	i4		bit_cnt = 0;	/* # of bits in t_reg to be mapped. */

	*size = 0;
	/* while there's still text to be mapped to binary data */
	while (*textbuf) {
		/* get up to 4 printable chars to map to three binary bytes */
		while (bit_cnt < 3 * BITSPERBYTE)
		{
			if (*textbuf)
			{
				c = *textbuf++;
				if (c > 'O')
					c--;
				if (c > 'I')
					c--;
				if (c > '9')
					c -= 7;
				c -= '2';
				t_reg <<= BIT_CNT;
				t_reg |= (c & 037);
				bit_cnt += BIT_CNT;
			}
			else
			{
				break;
			}
		}
		/* write out as many integral bytes as there are in t_reg */
		for (i = BITSPERBYTE; bit_cnt - i >= 0; i += BITSPERBYTE)
		{
			*block++ = (u_char)((t_reg >> (bit_cnt - i)) & 0377);
			(*size)++;
		}
		bit_cnt -= (i - BITSPERBYTE);
		t_reg &= bit_mask[bit_cnt];
	}
	return;
}



/*{
** Name:	CItotext	- map binary bytes to printable alphabet.
**
** Description:
**	    This routine maps a sequence of binary bytes to the characters
**	in a printable alphabet.  The printable alphabet consists of upper
**	case letters excluding 'O' and 'I' and digits excluding '0' and '1'.
**
** Inputs:
**	block	pointer to sequence of binary bytes to be mapped.
**	size	the number of binary bytes to be mapped.
**
** Outputs:
**	textbuf	pointer of output array to hold mapped printable characters.
**		IMPORTANT NOTE, this array's size must be greater than or equal
**		to CI_TOTEXT_SIZE_MACRO(size).  
**
** History: 
**	16-sep-85 (ericj) - written.
**	05-jan-86 (ericj) - modified to map to a new printable alphabet that
**			consists of characters that are easily distinguished on
**			our laser printer.
**	14-apr-95 (emmag)   
**		Desktop compilers insist that register variables be 
**		accompanied by a type name.
*/
VOID
CItotext(u_char *block, i4 size, char *textbuf)
{
	register i4	t_reg = 0;	/* transformation register */
	register i4	c;		/* a printable character */
	i4		i;
	i4		bit_cnt = 0;	/* # of bits to transform */

	/* while there are still bytes to transform or bits to be processed */
	while (size > 0 || bit_cnt)
	{
		/* fill transformation register with up to three bytes */
		for (i = 3; i && bit_cnt < 3 * BITSPERBYTE; size--, i--)
		{
			if (size > 0)
			{
				t_reg <<= BITSPERBYTE;
				t_reg = ((*block++) & 0377) | t_reg;
				bit_cnt += BITSPERBYTE;
			}
			else {
				if (bit_cnt % BIT_CNT)
				{
					t_reg <<= (BIT_CNT - (bit_cnt % BIT_CNT));
					bit_cnt += (BIT_CNT - (bit_cnt % BIT_CNT));
				}
				break;
			}
		}
		/* map as many integral BIT_CNT units in t_reg to printable characters */
		for (i = BIT_CNT; bit_cnt - i >= 0; i += BIT_CNT)
		{
			/* map BIT_CNT bits to a printable character */
			c = (t_reg >> (bit_cnt - i)) & 037;
			c += '2';
			if (c > '9')
				c += 7;
			if (c >= 'I')
				c ++;
			if (c >= 'O')
				c++;
			*textbuf++ = (char) c;
		}
		/*
		** count how many bits weren't processed and clear the bits
		** that were.
		*/
		bit_cnt -= (i - BIT_CNT);	
		t_reg &= bit_mask[bit_cnt];
	}
	*textbuf = '\0';
}
