# include	<compat.h>		/* header file for compatibility lib */
# include	<gl.h>		        /* header file for compatibility lib */
# include	<systypes.h>
# include	<math.h>		/* header file for math package */
# include	<errno.h>		/* header file for error checking */
# include	<ex.h>			/* header file for exceptions */
# include	<mh.h>
# include	<clfloat.h>

/*
* Inside the roc file we have two machine generated lookup tables 
*/
#include "mhround.roc"

/* This union allows us to read the bits within an IEEE
** double precision floating point number. The position
** of the most significant bits is Big/Little endian
** orientated.
*/ 
typedef union
{
    f8   floatValue;
    u_i8 bitValue;
#ifdef LITTLE_ENDIAN_INT /* in bzarch.h */
    struct
    {
	u_i4 botBits;
	u_i4 topBits;
    } parts;
#else
    struct
    {
	u_i4 topBits;
	u_i4 botBits;
    } parts;
#endif    
} fpEndianDataType;


#define EXPMASK        0x7FF
#define SIGNIG_MASK    0x000FFFFF
#define BIAS           0x3FF
#define LARGE          1.0e300
#define DBL_UNDERFLOW  DBL_MIN-0.5
#define DBL_OVERFLOW   DBL_MAX+0.5



/*{
** Name: mhround  - A utility routine to perform a SQL style 
**                  round of double. Not quite the same as
**                  C99 round() for we have a second parameter
**                  to specify which digit we want to round on.
**
** Input:
**          inputValue : The double to be rounded
**
**          position   : An integer index, 
**                          if +ve the position after the decimal point
**                          if -ve the position prior to the decimal point                   
** History:
**       04-Nov-2009 (coomi01) b122767
**            Created.
**       18-Dec-2009 (coomi01) b122767
**            Add clfloat.h header
*/
f8
MHround(f8 inputValue, i4 position)
{
    fpEndianDataType data;
    i4           exponent;
    i4             digCnt;
    f8           tenShift;
    u_i8              sig;
    u_i4          tmpVal1;
    u_i4          tmpVal2;
    u_i4          botBits;
    i4            topBits;
    bool         negRound;
    f8       shiftedValue;
    f8              after;
    f8             result;

#ifndef IEEE_FLOAT /* in bzarch.h */
    PROBLEM : If we are not using IEEE floats the algorithm below will fail.
#endif

    data.floatValue = inputValue;
    exponent = ((data.parts.topBits >> 20) & EXPMASK);
    digCnt = information[exponent].digits;

    if (1 == information[exponent].boundryPresent)
    {
	/* 
	** Need to be more careful 
	*/
	data.floatValue = inputValue;
	data.parts.topBits &= SIGNIG_MASK;
	data.parts.topBits |= (BIAS << 20);
	sig = data.bitValue; 

	if ( sig >= information[exponent].sig )
	    digCnt += 1;

	/* Restore data */
	data.floatValue = inputValue;
    }

    /*
    ** We may now look to see if we can optimize the request
    ** when the second 'position' parameter is attempting to 
    ** the floating point precision range.
    */
    if ( position < 1-digCnt )
    {
	return 0.0;
    } 
    else if (position > 23-digCnt)
    {
	return inputValue;
    }

    /* If we get here we must do the calculation in detail */
    /* To do this we first slide the float towards unity   */  

    /* 
    ** Flag negative position
    */
    negRound = FALSE;
    if ( position < 0 )
    {
	/* Invert */
	negRound  = TRUE;
	position *= -1;
    }
    
    /* 
    ** Calculate Ten's Shift for the slide
    */
    if (position < MAX_P10TAB)
    {
	/* 
	** Quick lookup of 10^position
	*/
	tenShift = powerTab[position];
    }
    else
    {
	/*
	** Shifting out-of-range would cause an overflow
	*/
	EXsignal(EXFLTOVF, 0);
	return 0.0;
    }

    if ( negRound )
    {
	/* 
	** Bring d-point into range
	*/
	shiftedValue = inputValue / tenShift;
	    
        /*
	** Check F8 Underflow 
	*/
	if ( abs(shiftedValue) <= DBL_UNDERFLOW )
	{
	    EXsignal(EXFLTOVF, 0);
	    return 0.0;
	}
    }
    else
    {
	/* 
	** Bring d-point into range
	** ~ Reverse procedure to stuff above. 
	*/
	shiftedValue = inputValue * tenShift;

        /*
	** Check F8 Overflow 
	*/
	if ( abs(shiftedValue) >= DBL_OVERFLOW )
	{
	    EXsignal(EXFLTOVF, 0);
	    return 0.0;
	}
    }

    /* We now have the number near unity, and we are to round
    ** away the part immediately after the decimal point
    */

    /* 
    ** Break shifted double down into two integer parts
    */
    data.floatValue = shiftedValue;
    topBits = data.parts.topBits;
    botBits = data.parts.botBits;

    /* 
    ** Now work on these parts
    */
    exponent = ((topBits >> 20) & EXPMASK) - BIAS;
    if (exponent < 20)
    {
	if (exponent < 0)
	{
	    if (LARGE + shiftedValue > 0.0)
	    {
		topBits &= 0x80000000;
		if (exponent == -1)
		    topBits |= 0x3ff00000;
		botBits = 0;
	    }
	}
	else
	{
	    tmpVal1 = 0x000fffff >> exponent;
	    if (((topBits & tmpVal1) | botBits) == 0)
	    {
		/* 
		** No adjustment required
		*/
		return inputValue;
	    }
	    if (LARGE + shiftedValue > 0.0)
	    {
		topBits += 0x00080000 >> exponent;
		topBits &= ~tmpVal1;
		botBits = 0;
	    }
	}
    }
    else if (exponent > 51)
    {
	if (exponent == 0x400)
	{
	    /* 
	    ** Inf or NaN.
	    */
	    EXsignal(EXFLTOVF, 0);
	    return 0.0;
	}
	else
	{
	    /* 
	    ** No adjustment required
	    */
	    return inputValue;
	}
    }
    else
    {
	tmpVal1 = 0xffffffff >> (exponent - 20);
	if ((botBits & tmpVal1) == 0)
	{
	    /* 
	    ** No adjustment required
	    */
	    return inputValue;
	}

	if (LARGE + shiftedValue > 0.0)
	{
	    tmpVal2 = botBits + (1 << (51 - exponent));
	    if (tmpVal2 < botBits)
		topBits += 1;
	    botBits = tmpVal2;
	}
	botBits &= ~tmpVal1;
    }

    /*
    ** Finally recombine parts into a double again
    ** ready to shift all the way back to the original
    ** location.
    */
    data.parts.topBits = topBits;
    data.parts.botBits = botBits;
    after = data.floatValue;

    if ( negRound )
    {
	if ( abs(after) <= DBL_UNDERFLOW )
	{
	    EXsignal(EXFLTOVF, 0);
	    return 0.0;
	}

	/*
	** Now shift d-point back to original position
	*/
	result = tenShift * after;
    }
    else
    {
        /*
	** Check F8 Overflow 
	*/
	if ( abs(after) >= DBL_OVERFLOW )
	{
	    EXsignal(EXFLTOVF, 0);
	    return 0.0;
	}

	/*
	** Now shift d-point back to original position
	*/
	result = after / tenShift;
    }

    /* 
    ** Done.
    */
    return result;
}

