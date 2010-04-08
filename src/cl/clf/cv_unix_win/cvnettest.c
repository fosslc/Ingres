/*
**Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <me.h>
# include <cv.h>
# include <tr.h>
# include <pc.h>
# include <cvgcc.h>

/*
** Name: cvnettest
**
** Description:
**	Primitive cvnet routine test.  Convert a few numbers back and
**	forth.
**
** History:
**		2/18/87 (daveb)	 -- Call to MEadvise, added MALLOCLIB hint.
**	08-Nov-89 (GordonW)
**		revamped for clearer output. Also added the standard bit pattern
**		to data value testing.
**	11-may-90 (blaise)
**		Integrated ingresug changes into 63p library.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	20-may-97 (mcgem01)
**	    Clean up compiler warnings.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	20-Apr-04 (gordy)
**	    Completely reworked.  Network byte patterns and associated
**	    known numeric values are now passed through net conversion
**	    routines and tested for validity.  Platform configuration
**	    also validated.
**      30-Apr-2004 (hanje04)
**          Replace BIG_ENDIAN and LITTLE_ENDIAN with BIG_ENDIAN_INT and
**          LITTLE_ENDIAN_INT because the former are already defined in
**          /usr/include/endian.h on Linux.
**	21-May-04 (gordy)
**	    Added remaining tests for IEEE floats.
**	29-Sep-2004 (drivi01)
**	    Removed MALLOCLIB from NEEDLIBS
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

/*
** Ming hints:
NEEDLIBS = COMPAT 
*/

/*
** For each atomic type, the network bit pattern and
** known associated numeric value for a number of
** 'interesting' values are defined.  Network/Local
** conversions may be tested by converting the lcl/net
** value and comparing with associated net/lcl value.
*/
static struct
{
    i1		lcl;
    u_i1	net[ CV_N_I1_SZ ];
} test_i1[] =
{
    { 0,	{ 0x00 } },
    { 1,	{ 0x01 } },
    { -1,	{ 0xFF } },
    { 0x12,	{ 0x12 } },
};

static struct
{
    i2		lcl;
    u_i1	net[ CV_N_I2_SZ ];
} test_i2[] =
{
    { 0,	{0x00, 0x00} },
    { 1,	{0x01, 0x00} },
    { -1,	{0xFF, 0xFF} },
    { 0x1234,	{0x34, 0x12} },
};

static struct
{
    i4		lcl;
    u_i1	net[ CV_N_I4_SZ ];
} test_i4[] =
{
    { 0,	{0x00, 0x00, 0x00, 0x00} },
    { 1,	{0x01, 0x00, 0x00, 0x00} },
    { -1,	{0xFF, 0xFF, 0xFF, 0xFF} },
    { 0x12345678, {0x78, 0x56, 0x34, 0x12} },
};

static struct
{
    i8		lcl;
    u_i1	net[ CV_N_I8_SZ ];
} test_i8[] =
{
    { 0LL,	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },
    { 1LL,	{0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },
    { -1LL,	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} },
    { 0x123456789ABCDEF0LL,
		{0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12} },
};

static struct
{
    f4		lcl;
    u_i1	net[ CV_N_F4_SZ ];
} test_f4[] =
{
    { 0,	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },
    { 1,	{0xFF, 0x3F, 0x00, 0x00, 0x00, 0x00} },
    { -1,	{0xFF, 0xBF, 0x00, 0x00, 0x00, 0x00} },

    /*
    ** 1234.0 2**0 => .12340 2**13 => 23400000 400B
    **
    ** Sign:			0 (positive)
    ** Exponent unbiased:	13 (3 MSB are 0)
    ** Exponent NTS biased:	0x400B
    ** Mantissa unnormalized:	0x1234
    ** Mantissa normalized:	0x23400000 (drop MSB)
    */
    { 0x1234,	{0x0B, 0x40, 0x40, 0x23, 0x00, 0x00} },
};

static struct
{
    f8		lcl;
    u_i1	net[ CV_N_F8_SZ ];
} test_f8[] =
{
    { 0,	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },
    { 1,	{0xFF, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },
    { -1,	{0xFF, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },

    /*
    ** 12345678.0 2**0 => .12345678.0 2**29 => 23456780 400B
    **
    ** Sign:			0 (positive)
    ** Exponent unbiased:	29 (3 MSB are 0)
    ** Exponent NTS biased:	0x401B
    ** Mantissa unnormalized:	0x12345678
    ** Mantissa normalized:	0x23456780 (drop MSB)
    */
    { 0x12345678,	
		{0x1B, 0x40, 0x45, 0x23, 0x80, 0x67, 0x00, 0x00, 0x00, 0x00} },
};

# define ARRAY_SIZE(array)	(sizeof(array)/sizeof((array)[0]))

static void	hex_display( u_i1 *, i4 );


/*
** Name: main
**
** Description:
**	Test platform configuration definitions and net/local conversions.
**
** Input:
**	argc	Number of command line parameters.
**	argv	Command line parameters.
**
** History:
**	20-Apr-04 (gordy)
**	    Rewrote.
*/

i4
main( i4 argc, char **argv )
{
    bool	pass = TRUE;
    i1		lcl_i1;
    u_i1	net_i1[ CV_N_I1_SZ ];
    i2		lcl_i2;
    u_i1	net_i2[ CV_N_I2_SZ ];
    i4		lcl_i4;
    u_i1	net_i4[ CV_N_I4_SZ ];
    i8		lcl_i8;
    u_i1	net_i8[ CV_N_I8_SZ ];
    f4		lcl_f4;
    u_i1	net_f4[ CV_N_F4_SZ ];
    f8		lcl_f8;
    u_i1	net_f8[ CV_N_F8_SZ ];
    i4		i;
    STATUS	status;
    CL_ERR_DESC	syserr;

    /* Use the ingres allocator instead of malloc/free default (daveb) */
    MEadvise( ME_INGRES_ALLOC );

    (void)TRset_file( TR_T_OPEN, "stdio", 5, &syserr );

    /* Check platform configuration */
# ifndef CV_NCSJ_TYPE
    
    TRdisplay( "CV_NCSJ_TYPE is not defined!\n" );
    pass = FAIL;

# endif /* CV_NCSJ_TYPE */

# ifndef CV_NCSN_TYPE
    
    TRdisplay( "CV_NCSN_TYPE is not defined!\n" );
    pass = FAIL;

# endif /* CV_NCSN_TYPE */

# ifndef CV_INT_TYPE
    
    TRdisplay( "CV_INT_TYPE is not defined!\n" );
    pass = FAIL;

# endif /* CV_INT_TYPE */

# ifndef CV_FLT_TYPE
    
    TRdisplay( "CV_FLT_TYPE is not defined!\n" );
    pass = FAIL;

# endif /* CV_FLT_TYPE */

# ifdef NO_64BIT_INT

    if ( sizeof(f8) >= 8 )
    {
	TRdisplay( "Configured for no 64 bit integer but sizeof(f8) = %d!\n",
		sizeof(f8) );
	pass = FALSE;
    }

# else

    if ( sizeof(f8) < 8 )
    {
	TRdisplay( "Configured for 64 bit integers but sizeof(f8) = %d!\n",
		sizeof(f8) );
	pass = FALSE;
    }

# endif /* NO_64BIT_INT */

# if	defined(LITTLE_ENDIAN_INT)

    lcl_i4 = 0x01020304;

    if ( 
	 ((u_i1 *)&lcl_i4)[0] != 0x04  ||
	 ((u_i1 *)&lcl_i4)[1] != 0x03  ||
	 ((u_i1 *)&lcl_i4)[2] != 0x02  ||
	 ((u_i1 *)&lcl_i4)[3] != 0x01
       )
    {
	TRdisplay( "Configured for little-endian integers\n" );
	TRdisplay( "\tExpected order 4321, found %d%d%d%d\n",
		((u_i1 *)&lcl_i4)[0], ((u_i1 *)&lcl_i4)[1],
		((u_i1 *)&lcl_i4)[2], ((u_i1 *)&lcl_i4)[3] );
	pass = FALSE;
    }

# elif	defined(BIG_ENDIAN_INT)

    lcl_i4 = 0x01020304;

    if ( 
	 ((u_i1 *)&lcl_i4)[0] != 0x01  ||
	 ((u_i1 *)&lcl_i4)[1] != 0x02  ||
	 ((u_i1 *)&lcl_i4)[2] != 0x03  ||
	 ((u_i1 *)&lcl_i4)[3] != 0x04
       )
    {
	TRdisplay( "Configured for little-endian integers\n" );
	TRdisplay( "\tExpected order 1234, found %d%d%d%d\n",
		((u_i1 *)&lcl_i4)[0], ((u_i1 *)&lcl_i4)[1],
		((u_i1 *)&lcl_i4)[2], ((u_i1 *)&lcl_i4)[3] );
	pass = FALSE;
    }

# else

    /*
    ** Currently, all Unix platforms are little or big endian.
    */
    TRdisplay( "Platform not configured for LITTLE/BIG ENDIAN integers!\n" );
    pass = FAIL;

# endif /* LITTLE_ENDIAN */

# ifdef IEEE_FLOAT

    /*
    ** Test for IEEE float by comparing the bit patterns
    ** for a known floating point value.  Note, this is
    ** byte order independent since floating point value
    ** is treated as an atomic integer value rather than
    ** testing individual bytes.
    **
    ** Value: -1.625
    ** Sign: 1
    ** Unbiased exponent: 0
    ** Biased exponent: 127 (0x7F)
    ** Mantissa: 1.10100...
    ** Normalized Mantissa : 10100... (0xA0)
    **
    ** Shifting and aligning bits:
    **
    ** Sign:	0x80000000
    ** Exp :	0x3F800000
    ** Mant:	0x00500000
    **
    ** Resulting bit pattern:
    **
    ** bits:	0xBFD00000
    */

    lcl_f4 = -1.625;
    if ( *(i4 *)&lcl_f4 != 0xBFD00000 )
    {
	TRdisplay( "Platform configured for IEEE float.\n" );
	TRdisplay( "\tExpected bit pattern: 0x%x\n", 0xBFD00000 );
	TRdisplay( "\tActual bit pattern  : 0x%x\n", *(i4 *)&lcl_f4 );
	pass = FAIL;
    }

# else

    /*
    ** Currently, all Unix platforms support IEEE floats.
    */
    TRdisplay( "Platform does not support IEEE floats?\n" );
    pass = FAIL;

# endif /* IEEE_FLOAT */

    /*
    ** Test I1 support
    */
    if ( sizeof(i1) != CV_L_I1_SZ )
    {
	TRdisplay( "Invalid definition for CV_L_I1_SZ!\n" );
	TRdisplay( "\t sizeof(i1) = %d, CV_L_I1_SZ = %d\n",
		sizeof(i1), CV_L_I1_SZ );
        pass = FALSE;
    }

    for( i = 0; i < ARRAY_SIZE(test_i1); i++ )
    {
	status = FAIL;
	CV2L_I1_MACRO( test_i1[i].net, &lcl_i1, &status );

	if ( status != OK )
	{
	    TRdisplay( "CV2L_I1_MACRO failed: 0x%x!\n", status );
	    pass = FALSE;
	}
	else  if ( lcl_i1 != test_i1[i].lcl )
	{
	    TRdisplay( "CV2L_I1_MACRO failed!\n" );
	    TRdisplay( "\texpected %d, received %d\n", 
		test_i1[i].lcl, lcl_i1 );
	    pass = FALSE;
	}

	status = FAIL;
	CV2N_I1_MACRO( &test_i1[i].lcl, net_i1, &status );

	if ( status != OK )
	{
	    TRdisplay( "CV2N_I1_MACRO failed: 0x%x!\n", status );
	    pass = FALSE;
	}
	else  if ( MEcmp( net_i1, test_i1[i].net, CV_N_I1_SZ ) != 0 )
	{
	    TRdisplay( "CV2N_I1_MACRO failed!\n" );
	    TRdisplay( "\tExpected: " );
	    hex_display( test_i1[i].net, sizeof( test_i1[i].net ) );
	    TRdisplay( "\n" );
	    TRdisplay( "\tReceived: " );
	    hex_display( net_i1, sizeof( net_i1 ) );
	    TRdisplay( "\n" );
	    pass = FALSE;
	}
    }

    /*
    ** Test I2 support
    */
    if ( sizeof(i2) != CV_L_I2_SZ )
    {
	TRdisplay( "Invalid definition for CV_L_I2_SZ!\n" );
	TRdisplay( "\t sizeof(i2) = %d, CV_L_I2_SZ = %d\n",
		sizeof(i2), CV_L_I2_SZ );
        pass = FALSE;
    }

    for( i = 0; i < ARRAY_SIZE(test_i2); i++ )
    {
	status = FAIL;
	CV2L_I2_MACRO( test_i2[i].net, &lcl_i2, &status );

	if ( status != OK )
	{
	    TRdisplay( "CV2L_I2_MACRO failed: 0x%x!\n", status );
	    pass = FALSE;
	}
	else  if ( lcl_i2 != test_i2[i].lcl )
	{
	    TRdisplay( "CV2L_I2_MACRO failed!\n" );
	    TRdisplay( "\texpected %d, received %d\n", 
		test_i2[i].lcl, lcl_i2 );
	    pass = FALSE;
	}

	status = FAIL;
	CV2N_I2_MACRO( &test_i2[i].lcl, net_i2, &status );

	if ( status != OK )
	{
	    TRdisplay( "CV2N_I2_MACRO failed: 0x%x!\n", status );
	    pass = FALSE;
	}
	else  if ( MEcmp( net_i2, test_i2[i].net, CV_N_I2_SZ ) != 0 )
	{
	    TRdisplay( "CV2N_I2_MACRO failed!\n" );
	    TRdisplay( "\tExpected: " );
	    hex_display( test_i2[i].net, sizeof( test_i2[i].net ) );
	    TRdisplay( "\n" );
	    TRdisplay( "\tReceived: " );
	    hex_display( net_i2, sizeof( net_i2 ) );
	    TRdisplay( "\n" );
	    pass = FALSE;
	}
    }

    /* 
    ** Test I4 support
    */
    if ( sizeof(i4) != CV_L_I4_SZ )
    {
	TRdisplay( "Invalid definition for CV_L_I4_SZ!\n" );
	TRdisplay( "\t sizeof(i4) = %d, CV_L_I4_SZ = %d\n",
		sizeof(i4), CV_L_I4_SZ );
        pass = FALSE;
    }

    for( i = 0; i < ARRAY_SIZE(test_i4); i++ )
    {
	status = FAIL;
	CV2L_I4_MACRO( test_i4[i].net, &lcl_i4, &status );

	if ( status != OK )
	{
	    TRdisplay( "CV2L_I4_MACRO failed: 0x%x!\n", status );
	    pass = FALSE;
	}
	else  if ( lcl_i4 != test_i4[i].lcl )
	{
	    TRdisplay( "CV2L_I4_MACRO failed!\n" );
	    TRdisplay( "\texpected %d, received %d\n", 
		test_i4[i].lcl, lcl_i4 );
	    pass = FALSE;
	}

	status = FAIL;
	CV2N_I4_MACRO( &test_i4[i].lcl, net_i4, &status );

	if ( status != OK )
	{
	    TRdisplay( "CV2N_I4_MACRO failed: 0x%x!\n", status );
	    pass = FALSE;
	}
	else  if ( MEcmp( net_i4, test_i4[i].net, CV_N_I4_SZ ) != 0 )
	{
	    TRdisplay( "CV2N_I4_MACRO failed!\n" );
	    TRdisplay( "\tExpected: " );
	    hex_display( test_i4[i].net, sizeof( test_i4[i].net ) );
	    TRdisplay( "\n" );
	    TRdisplay( "\tReceived: " );
	    hex_display( net_i4, sizeof( net_i4 ) );
	    TRdisplay( "\n" );
	    pass = FALSE;
	}
    }

    /*
    ** Test I8 support
    */
    if ( sizeof(i8) != CV_L_I8_SZ )
    {
	TRdisplay( "Invalid definition for CV_L_I8_SZ!\n" );
	TRdisplay( "\t sizeof(i8) = %d, CV_L_I8_SZ = %d\n",
		sizeof(i8), CV_L_I8_SZ );
        pass = FALSE;
    }

    for( i = 0; i < ARRAY_SIZE(test_i8); i++ )
    {
	status = FAIL;
	CV2L_I8_MACRO( test_i8[i].net, &lcl_i8, &status );

	if ( status != OK )
	{
	    TRdisplay( "CV2L_I8_MACRO failed: 0x%x!\n", status );
	    pass = FALSE;
	}
	else  if ( lcl_i8 != test_i8[i].lcl )
	{
	    TRdisplay( "CV2L_I8_MACRO failed!\n" );
	    TRdisplay( "\texpected %d, received %d\n", 
		test_i8[i].lcl, lcl_i8 );
	    pass = FALSE;
	}

	status = FAIL;
	CV2N_I8_MACRO( &test_i8[i].lcl, net_i8, &status );

	if ( status != OK )
	{
	    TRdisplay( "CV2N_I8_MACRO failed: 0x%x!\n", status );
	    pass = FALSE;
	}
	else  if ( MEcmp( net_i8, test_i8[i].net, CV_N_I8_SZ ) != 0 )
	{
	    TRdisplay( "CV2N_I8_MACRO failed!\n" );
	    TRdisplay( "\tExpected: " );
	    hex_display( test_i8[i].net, sizeof( test_i8[i].net ) );
	    TRdisplay( "\n" );
	    TRdisplay( "\tReceived: " );
	    hex_display( net_i8, sizeof( net_i8 ) );
	    TRdisplay( "\n" );
	    pass = FALSE;
	}
    }

    /*
    ** Test F4 support
    */
    if ( sizeof(f4) != CV_L_F4_SZ )
    {
	TRdisplay( "Invalid definition for CV_L_F4_SZ!\n" );
	TRdisplay( "\t sizeof(f4) = %d, CV_L_F4_SZ = %d\n",
		sizeof(f4), CV_L_F4_SZ );
        pass = FALSE;
    }

    for( i = 0; i < ARRAY_SIZE(test_f4); i++ )
    {
	status = FAIL;
	CV2L_F4_MACRO( test_f4[i].net, &lcl_f4, &status );

	if ( status != OK )
	{
	    TRdisplay( "CV2L_F4_MACRO failed: 0x%x!\n", status );
	    pass = FALSE;
	}
	else  if ( lcl_f4 != test_f4[i].lcl )
	{
	    TRdisplay( "CV2L_F4_MACRO failed!\n" );
	    TRdisplay( "\texpected %f, received %f\n", 
		test_f4[i].lcl, lcl_f4 );
	    pass = FALSE;
	}

	status = FAIL;
	CV2N_F4_MACRO( &test_f4[i].lcl, net_f4, &status );

	if ( status != OK )
	{
	    TRdisplay( "CV2N_F4_MACRO failed: 0x%x!\n", status );
	    pass = FALSE;
	}
	else  if ( MEcmp( net_f4, test_f4[i].net, CV_N_F4_SZ ) != 0 )
	{
	    TRdisplay( "CV2N_F4_MACRO failed!\n" );
	    TRdisplay( "\tExpected: " );
	    hex_display( test_f4[i].net, sizeof( test_f4[i].net ) );
	    TRdisplay( "\n" );
	    TRdisplay( "\tReceived: " );
	    hex_display( net_f4, sizeof( net_f4 ) );
	    TRdisplay( "\n" );
	    pass = FALSE;
	}
    }

    /*
    ** Test F8 support
    */
    if ( sizeof(f8) != CV_L_F8_SZ )
    {
	TRdisplay( "Invalid definition for CV_L_F8_SZ!\n" );
	TRdisplay( "\t sizeof(f8) = %d, CV_L_F8_SZ = %d\n",
		sizeof(f8), CV_L_F8_SZ );
        pass = FALSE;
    }

    for( i = 0; i < ARRAY_SIZE(test_f8); i++ )
    {
	status = FAIL;
	CV2L_F8_MACRO( test_f8[i].net, &lcl_f8, &status );

	if ( status != OK )
	{
	    TRdisplay( "CV2L_F8_MACRO failed: 0x%x!\n", status );
	    pass = FALSE;
	}
	else  if ( lcl_f8 != test_f8[i].lcl )
	{
	    TRdisplay( "CV2L_F8_MACRO failed!\n" );
	    TRdisplay( "\texpected %f, received %f\n", 
		test_f8[i].lcl, lcl_f8 );
	    pass = FALSE;
	}

	status = FAIL;
	CV2N_F8_MACRO( &test_f8[i].lcl, net_f8, &status );

	if ( status != OK )
	{
	    TRdisplay( "CV2N_F8_MACRO failed: 0x%x!\n", status );
	    pass = FALSE;
	}
	else  if ( MEcmp( net_f8, test_f8[i].net, CV_N_F8_SZ ) != 0 )
	{
	    TRdisplay( "CV2N_F8_MACRO failed!\n" );
	    TRdisplay( "\tExpected: " );
	    hex_display( test_f8[i].net, sizeof( test_f8[i].net ) );
	    TRdisplay( "\n" );
	    TRdisplay( "\tReceived: " );
	    hex_display( net_f8, sizeof( net_f8 ) );
	    TRdisplay( "\n" );
	    pass = FALSE;
	}
    }

    /*
    ** Done!
    */
    TRdisplay( pass ? "PASS!\n" : "FAILED!\n" );
    PCexit( OK );
}


/*
** Name: hex_display
**
** Description:
**	Uses TRdisplay() to output the hex value of each byte
**	in the input array.  Does not send newline to output.
**
** Input:
**	val	Input array to display.
**	len	Length of array.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	20-Apr-04 (gordy)
**	    Created.
*/

static void
hex_display( u_i1 *val, i4 len )
{
    char	hex[17] = "0123456789ABCDEF";
    char	str[3] = "  ";
    i4		i;

    for( i = 0; i < len; i++ )
    {
	str[0] = hex[ (val[i] >> 4) & 0x0F ];
	str[1] = hex[ val[i] & 0x0F ];
	TRdisplay( str );
    }
    return;
}
