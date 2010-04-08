/*
** Name: gcblpack.c - BLAST pack/unpack data for use with GCC ASYNC Protocol
**
** Description:
**	This source file was provided as part of 'BLAST' protocol
**	support.  It is included as support routines for the GCC 'ASYNC'
**	protocol driver.
**
** History:
**	29-Jan-92 (alan)
**	    Added header and renamed.
*/

/***************************************************************** 
 *
 *  pack.c - pack and unpack data in either 7-bit (3/4th's format)
 *		   or 8-bit (7/8th's) printable ascii text forms.
 *
 *  roussel 5/17/85
 *
 *  7-bit printable ASCII mapping:
 *
 *  3 8-bit values are mapped into four 6-bit values using the
 *  following scheme:
 *
 *  8-bit bytes:	   76543210  |  76543210  |  76543210
 *  6-bit bytes:   xx765432  xx10|7654  xx3210|76  xx543210
 *
 *  the 6-bit values are then represented by a printable ASCII character
 *  by indexing into the first half of pktab.
 *
 *
 *  8-bit printable ASCII mapping:
 *
 *  7 8-bit bytes are mapped into 8 7-bit values using the following
 *  scheme:
 *
 *   76543210  76543210  76543210  76543210  76543210  76543210  76543210
 *  x7654321|x0765432|x1076543|x2107654|x3210765|x4321076|x5432107|x6543210
 *
 *
 *  pk34() and pk78() operate on an entire blk of data at one time
 *  so that blks can be written to the comm port in large chunks.
 *
 *	upkf34() and upkf78() operate on a byte at a time, so that recv
 *	data processing can be interspersed among recv bytes.
 */


/* pktab
	this table is used to get a printable ASCII representation
	for a binary value.  In 3/4th's mode, 6-bit binary values 
	index into the first half of the table.  In 7/8th's mode,
	the whole table is used.  Note that the second half of the
	table is merely a copy of the first half with the high bit
	set.
*/

static char pktab[128] = {
	0060,0061,0062,0063,0064,0065,0066,0067,	/* 000 01234567 */
	0070,0071,0101,0102,0103,0104,0105,0106,	/* 010 89ABCDEF */
	0107,0110,0111,0112,0113,0114,0115,0116,	/* 020 GHIJKLMN */
	0117,0120,0121,0122,0123,0124,0125,0126,	/* 030 OPQRSTUV */
	0127,0130,0131,0132,0141,0142,0143,0144,	/* 040 WXYZabcd */
	0145,0146,0147,0150,0151,0152,0153,0154,	/* 050 efghijkl */
	0155,0156,0157,0160,0161,0162,0163,0164,	/* 060 mnopqrst */
	0165,0166,0167,0170,0171,0172,0042,0047,	/* 070 uvwxyz"' */

	0260,0261,0262,0263,0264,0265,0266,0267,	/* 100 01234567 */
	0270,0271,0301,0302,0303,0304,0305,0306,	/* 110 89ABCDEF */
	0307,0310,0311,0312,0313,0314,0315,0316,	/* 120 GHIJKLMN */
	0317,0320,0321,0322,0323,0324,0325,0326,	/* 130 OPQRSTUV */
	0327,0330,0331,0332,0341,0342,0343,0344,	/* 140 WXYZabcd */
	0345,0346,0347,0350,0351,0352,0353,0354,	/* 150 efghijkl */
	0355,0356,0357,0360,0361,0362,0363,0364,	/* 160 mnopqrst */
	0365,0366,0367,0370,0371,0372,0242,0247		/* 170 uvwxyz"' */
};


/* upktab
	this table performs the reverse transformation on
	incoming data that was packed using pktab
*/

static char upktab[256] = {
	0200,0200,0200,0200,0200,0200,0200,0200, 	/* ........ */
	0200,0200,0200,0200,0200,0200,0200,0200, 	/* ........ */
	0200,0200,0200,0200,0200,0200,0200,0200,   	/* ........ */
	0200,0200,0200,0200,0200,0200,0200,0200, 	/* ........ */
	0200,0200,0076,0200,0200,0200,0200,0077,   	/* .."....' */
	0200,0200,0200,0200,0200,0200,0200,0200, 	/* ........ */
	0000,0001,0002,0003,0004,0005,0006,0007, 	/* 01234567 */
	0010,0011,0200,0200,0200,0200,0200,0200, 	/* 89...... */
	0200,0012,0013,0014,0015,0016,0017,0020, 	/* .ABCDEFG */
	0021,0022,0023,0024,0025,0026,0027,0030, 	/* HIJKLMNO */
	0031,0032,0033,0034,0035,0036,0037,0040, 	/* PQRSTUVW */
	0041,0042,0043,0200,0200,0200,0200,0200, 	/* XYZ..... */
	0200,0044,0045,0046,0047,0050,0051,0052, 	/* .abcdefg */
	0053,0054,0055,0056,0057,0060,0061,0062, 	/* hijklmno */
	0063,0064,0065,0066,0067,0070,0071,0072, 	/* pqrstuvw */
	0073,0074,0075,0200,0200,0200,0200,0200, 	/* xyz..... */

	0200,0200,0200,0200,0200,0200,0200,0200, 	/* ........ */
	0200,0200,0200,0200,0200,0200,0200,0200, 	/* ........ */
	0200,0200,0200,0200,0200,0200,0200,0200,   	/* ........ */
	0200,0200,0200,0200,0200,0200,0200,0200, 	/* ........ */
	0200,0200,0176,0200,0200,0200,0200,0177,   	/* .."....' */
	0200,0200,0200,0200,0200,0200,0200,0200, 	/* ........ */
	0100,0101,0102,0103,0104,0105,0106,0107, 	/* 01234567 */
	0110,0111,0200,0200,0200,0200,0200,0200, 	/* 89...... */
	0200,0112,0113,0114,0115,0116,0117,0120, 	/* .ABCDEFG */
	0121,0122,0123,0124,0125,0126,0127,0130, 	/* HIJKLMNO */
	0131,0132,0133,0134,0135,0136,0137,0140, 	/* PQRSTUVW */
	0141,0142,0143,0200,0200,0200,0200,0200, 	/* XYZ..... */
	0200,0144,0145,0146,0147,0150,0151,0152, 	/* .abcdefg */
	0153,0154,0155,0156,0157,0160,0161,0162, 	/* hijklmno */
	0163,0164,0165,0166,0167,0170,0171,0172, 	/* pqrstuvw */
	0173,0174,0175,0200,0200,0200,0200,0200, 	/* xyz..... */

};


/*{
** Name: pk34
**  	
** Description: 
**  	pack data into 3/4 format
**	
** Inputs:
**  	pf -	    address of data to be packed 
**  	cnt -	    length of data
**  	pt -	    address of buffer to place packed data
**
** Outputs:
**	none
**	
** Returns:
**	length of packed data
**
** Exceptions:
**	none
**
** History:
**	06-Feb-91 (cmorris) 
**  	    	Reformatted
**		
*/
int
pk34( pf, cnt, pt )
char *pf;
char *pt;
int cnt;	
{
    unsigned int i,j;
    char *phold = pt;

    while( 1 )
    {
	*pt++ = pktab[ (i= (*pf++ & 0377)) >> 2 ];
	if( !--cnt ) 
    	    j = 0;
	else 
    	    j = *pf++ & 0377;
	*pt++ = pktab[ ((i<<4) & 060) | (j>>4) ];
	if( !cnt ) 
    	    break;
	if( !--cnt )
    	    i = 0;
	else 
    	    i = *pf++ & 0377;
	*pt++ = pktab[ ((j<<2) & 074) | (i>>6) ];
	if( !cnt )
    	    break;
	*pt++ = pktab[ i & 077 ];
	if( !--cnt ) 
    	    break;
    }
	return( pt - phold );	/* return size of packed data */
} /* end of pk34() */

/*{
** Name: upkf34
**  	
** Description: 
**  	unpack byte into 3/4 format
**	
** Inputs:
**  	byte -	    byte to unpack
**
** Outputs:
**	none
**	
** Returns:
**	-1 if byte is un-unpackable, unpacked byte otherwise
**
** Exceptions:
**	none
**
** History:
**	06-Feb-91 (cmorris) 
**  	    	Reformatted
**		
*/
int
upkf34( byte )
int byte;	
{
    static int state = 0;
    static unsigned last_byte;
    unsigned tmp, ub;

    if( !byte ) 
    {	    	    	    	    	/* init */
	state = 0;
	return( -1 );
    }

    byte &= 0177;	    	    	/* ignore parity */

    if(( ub = upktab[byte] ) & 0200 ) 	/* byte not valid */
	return( -1 );

    tmp = last_byte;


    switch( state ) 
    {
    case 0:
	last_byte = ub<<2;
	++state;
	return( -1 );
    case 1:
	ub = tmp |(( last_byte = ub ) >> 4 );
	++state;
	break;
    case 2:
	ub = ( tmp<<4 )|(( last_byte = ub ) >> 2 );
	++state;
	break;
    case 3:
	ub = ( tmp << 6 ) | ub;
	state = 0;
	break;
    }

    return( ub & 0377 );
} /* end of upkf34() */

/*{
** Name: pk78
**  	
** Description: 
**  	pack data into 7/8 format
**	
** Inputs:
**  	pf -	    address of data to be packed 
**  	cnt -	    length of data
**  	pt -	    address of buffer to place packed data
**
** Outputs:
**	none
**	
** Returns:
**	length of packed data
**
** Exceptions:
**	none
**
** History:
**	06-Feb-91 (cmorris) 
**  	    	Reformatted
**		
*/
int
pk78( pf,cnt,pt )	/* return the size of packed data */
char *pf;		/* data to pack */
int cnt;
char *pt;		/* destination buffer */
{
    unsigned int i,j;
    char *pthold = pt;

    while( cnt )
    {
	*pt++ = pktab[(i = (*pf++ & 0377))>>1];
	if( !--cnt )
    	    j = 0;
	else
    	    j = (*pf++ & 0377);
	*pt++ = pktab[((i << 6)|(j >> 2))&0177];
	if( !cnt ) 
    	    break;
	if( !--cnt )
    	    i = 0;
	else 
    	    i = (*pf++ & 0377);
	*pt++ = pktab[((j << 5)|(i >> 3))&0177];
	if( !cnt ) 
    	    break;
	if( !--cnt ) 
    	    j = 0;
	else 
    	    j = (*pf++ & 0377);
	*pt++ = pktab[((i << 4)|(j >> 4))&0177];
	if( !cnt ) 
    	    break;
	if( !--cnt )
    	    i = 0;
	else
    	    i = (*pf++ & 0377);
	*pt++ = pktab[((j << 3)|(i >> 5))&0177];
	if( !cnt )
    	    break;
	if( !--cnt ) 
    	    j = 0;
	else 
    	    j = (*pf++ & 0377);
	*pt++ = pktab[((i << 2)|(j >> 6))&0177];
	if( !cnt ) 
    	    break;
	if( !--cnt ) 
    	    i = 0;
	else 
    	    i = (*pf++ & 0377);
	*pt++ = pktab[((j << 1)|(i >> 7))&0177];
       	if( !cnt ) 
    	    break;
	*pt++ = pktab[ i & 0177 ];
	--cnt;
    }

    return( pt - pthold );
}	/* end of pk78() */



/*{
** Name: upkf78
**  	
** Description: 
**  	unpack byte into 7/8 format
**	
** Inputs:
**  	bye - 	    byte to unpack
**
** Outputs:
**	none
**	
** Returns:
**	-1 if byte is un-unpackable, unpacked byte otherwise
**
** Exceptions:
**	none
**
** History:
**	06-Feb-91 (cmorris) 
**  	    	Reformatted
**		
*/
int
upkf78( byte )	
int byte;
{
    static int state = 0;
    static unsigned last_byte;
    unsigned tmp, ub;

    if( !byte )	    			/* if called with 0, initialize */
    {
	state = 0;
	return( -1 );
    }

    byte &= 0377;

    if(( ub = upktab[ byte ] )& 0200 )	/* not a valid byte */
	return( -1 );

    tmp = last_byte;

    switch( state ) 
    {
    case 0:
	last_byte = ub << 1;
	++state;
	return( -1 );
    case 1:
	ub = tmp | (( last_byte = ub ) >> 6 );
	++state;
	break;
    case 2:
	ub = ( tmp << 2 ) | (( last_byte = ub ) >> 5 );
	++state;
	break;
    case 3:
	ub = ( tmp << 3 ) | (( last_byte = ub ) >> 4 );
	++state;
	break;
    case 4:
	ub = ( tmp << 4 ) | (( last_byte = ub ) >> 3 );
	++state;
	break;
    case 5:
	ub = ( tmp << 5 ) | (( last_byte = ub ) >> 2 );
	++state;
	break;
    case 6:
	ub = ( tmp << 6 ) | (( last_byte = ub ) >> 1 );
	++state;
	break;
    case 7:
	ub = ( tmp << 7 ) | ub;
	state = 0;
	break;
    }

    return( ub & 0377 );
}	/* end of upkf78() */
/* end of pack.c */



