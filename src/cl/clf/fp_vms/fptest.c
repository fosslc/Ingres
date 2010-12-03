#include <compat.h>
#include <fp.h>
#include <si.h>

/*
** Name: FPTEST.C - test harness for FP functions.
**
** Description:
**
**      Stand alone test process to allow the debugging of the 
**      various FP routines.
**
** History:
**      16-apr-1992 (rog)
**          Created.
**      19-jul-1992 (smc)
**          Added tests for new fpmath routines, ifdefed
**          axp_osf for generation of INF and moved finite tests
**          to the head of the list so they can fail in an obvious
**          manner if they are going to.
**      29-Nov-2010 (frima01) SIR 124685
**          Added static declaration to local functions.
*/

#define abs(x)	((x) > 0 ? (x) : -(x))

double Infinity;
double Zero = 0.0;

static void
TestFPdfinite( void )
{
    double i;

    for ( i = -FMAX ; i <= FMAX ; i += FMAX * 0.0001 )
	if (!FPdfinite(&i))
	    SIprintf("%.16f is not finite.\n", i, '.');
    
    if (FPdfinite(&Infinity))
	SIprintf("%.16f should not have been finite.\n", Infinity, '.');
}

static void
TestFPffinite( void )
{
    float i;

    for ( i = -FMAX ; i < FMAX ; i += FMAX * 0.0001 )
	if (!FPdfinite(&i))
	    SIprintf("%.16f is not finite.\n", i, '.');
    
    if (FPffinite((float *) &Infinity))
	SIprintf("%.16f should not have been finite.\n", Infinity, '.');
}

static void
TestTrig( void )
{
    double	i, result;
    STATUS	status;

    status = FPatan(&Infinity, &result);
    if (status != FP_NOT_FINITE_ARGUMENT)
	SIprintf("FPatan(): %.16f should not have been finite.\n", i, '.');
    status = FPsin(&Infinity, &result);
    if (status != FP_NOT_FINITE_ARGUMENT)
	SIprintf("FPsin(): %.16f should not have been finite.\n", i, '.');
    status = FPcos(&Infinity, &result);
    if (status != FP_NOT_FINITE_ARGUMENT)
	SIprintf("FPcos(): %.16f should not have been finite.\n", i, '.');

    for ( i = -1.0 ; i <= 1.0 ; i += 0.0000001 )
    {
	double atan, cos, sin, temp_result;

	if (FPsin(&i, &sin) != FP_OK)
	    SIprintf("FPsin(): Error calculating sin(%.16f).\n", i, '.');
	if (FPcos(&i, &cos) != FP_OK)
	    SIprintf("FPcos(): Error calculating cos(%.16f).\n", i, '.');

	temp_result = sin / cos;

	if (FPatan(&temp_result, &atan) != FP_OK)
	    SIprintf("FPatan(): Error calculating atan(%.16f).\n", sin/cos, '.');
	
	if (abs(i - atan) > 1E-7)
	    SIprintf("TrigTest doesn't match within 1E-7: %.16f, %.16f.\n", i,
		     '.', atan, '.');
    }
}

static void
TestFPceil()
{
    double	i, result;
    STATUS	status;

    status = FPceil(&Infinity, &result);
    if (status != FP_NOT_FINITE_ARGUMENT)
	SIprintf("FPceil(): %.16f should not have been finite.\n", i, '.');

    for ( i = -FMAX + FMIN ; i < FMAX ; i += FMAX * 0.0001 )
    {
	status = FPceil(&i, &result);
	if (status != FP_OK)
	    SIprintf("FPceil() returned status %d.\n", status, '.');
	else if (i != result)
	    SIprintf("FPceil() could not compute ceil(%.16f) != %.16f.\n", i,
		     '.', result, '.');
    }
}

static void
TestFPfdint()
{
    double	i, result;

    for ( i = -FMAX + FMIN ; i < FMAX ; i += FMAX * 0.0001 )
    {
	result = FPfdint(&i);
	if (i != result)
	    SIprintf("FPfdint() could not compute fdint(%.16f) != %.16f.\n", i,
		     '.', result, '.');
    }
}

static void
TestRand()
{
    int i;

    SIprintf("Seed random number generator with: 734316137.\n");
    FPsrand(734316137);
    SIprintf("Ten random numbers:\n");
    for (i = 1; i <= 10; i++)
	SIprintf("\t%.16f\n", FPrand(), '.');
}

static void
TestFPsqrt()
{
    double	i, result;
    STATUS	status;

    status = FPsqrt(&Infinity, &result);
    if (status != FP_NOT_FINITE_ARGUMENT)
	SIprintf("FPsqrt(): %.16f should not have been finite.\n", i, '.');

    i = -1.0;
    status = FPsqrt(&i, &result);
    if (status != FP_SQRT_DOMAIN_ERROR)
	SIprintf("FPsqrt(): Got wrong status while computing sqrt(-1.0).\n");

    i = 321979074624.0;
    status = FPsqrt(&i, &result);
    if (status != FP_OK || result != 567432)
	SIprintf("FPsqrt(): Could not compute sqrt(321979074624.0).\n");
}

static void
TestEfuncs()
{
    double	i, result;
    STATUS	status;

    status = FPexp(&Infinity, &result);
    if (status != FP_NOT_FINITE_ARGUMENT)
	SIprintf("FPexp(): %.16f should not have been finite.\n", i, '.');
    
    status = FPln(&Infinity, &result);
    if (status != FP_NOT_FINITE_ARGUMENT)
	SIprintf("FPln(): %.16f should not have been finite.\n", i, '.');
    
    i = FP_EXP_MIN - 1.0;
    status = FPexp(&i, &result);
    if (status != FP_UNDERFLOW)
	SIprintf("FPexp(): %.16f should have underflowed.\n", i, '.');
    
    i = FP_EXP_MAX + 1.0;
    status = FPexp(&i, &result);
    if (status != FP_OVERFLOW)
	SIprintf("FPexp(): %.16f should have overflowed.\n", i, '.');
    
    i = -1243.2;
    status = FPln(&i, &result);
    if (status != FP_LN_DOMAIN_ERROR)
	SIprintf("FPln(): %.16f should have caused a domain error.\n", i, '.');
    
    i = 0.0;
    status = FPln(&i, &result);
    if (status != FP_LN_DOMAIN_ERROR)
	SIprintf("FPln(): %.16f should have caused a domain error.\n", i, '.');
    
    for ( i = FP_EXP_MIN + 0.001 ; i < FP_EXP_MAX ; i += 0.01 )
    {
	double	temp;

	if (FPexp(&i, &result) != FP_OK)
	{
	    SIprintf("FPexp(): Error calculating exp(%.16f).\n", i, '.');
	    continue;
	}
	temp = result;
	if (FPln(&temp, &result) != FP_OK)
	{
	    SIprintf("FPln(): Error calculating ln(%.16f).\n", temp, '.');
	    continue;
	}
	if (abs(i - result) > 1.0E-16)
	{
	    SIprintf("exp(%.18f) = %.18f, ln(%.18f) = %.18f.\n", i, '.', temp,
		     '.', temp, '.', result, '.');
	}
    }

}

static void
TestFPipow()
{
    double	x, result;
    int		i;
    STATUS	status;

    status = FPipow(&Infinity, 10, &result);
    if (status != FP_NOT_FINITE_ARGUMENT)
	SIprintf("FPipow(): %.16f should not have been finite.\n", x, '.');
    
    x = 0.0;
    i = -1;
    status = FPipow(&x, i, &result);
    if (status != FP_IPOW_DOMAIN_ERROR)
	SIprintf("FPipow(): %.16f should have produced a domain error.\n", x,
		 '.');

    x = 5.5E+36;
    i = MAXI4;
    status = FPipow(&x, i, &result);
    if (status != FP_OVERFLOW)
	SIprintf("FPipow(): %.16f, %d should have overflowed.\n", x, '.', i);

    x = 2.5E-10;
    i = MAXI4;
    status = FPipow(&x, i, &result);
    if (status != FP_UNDERFLOW)
	SIprintf("FPipow(): %.16f, %d should have underflowed.\n", x, '.', i);
}

static void
TestFPpow()
{
    double	x, y, result;
    STATUS	status;

    x = 32768.0;
    status = FPpow(&Infinity, &x, &result);
    if (status != FP_NOT_FINITE_ARGUMENT)
	SIprintf("FPpow(): %.16f should not have been finite.\n", x, '.');
    
    status = FPpow(&x, &Infinity, &result);
    if (status != FP_NOT_FINITE_ARGUMENT)
	SIprintf("FPpow(): %.16f should not have been finite.\n", y, '.');
    
    x = 0.0;
    y = -1.0;
    status = FPpow(&x, &y, &result);
    if (status != FP_POW_DOMAIN_ERROR)
	SIprintf("FPpow(): %.16f should have produced a domain error.\n", x,
		 '.');

    x = 0.0;
    y = 0.0;
    status = FPpow(&x, &y, &result);
    if (status != FP_POW_DOMAIN_ERROR)
	SIprintf("FPpow(): %.16f should have produced a domain error.\n", x,
		 '.');

    x = -25.34;
    y = 12.56;
    status = FPpow(&x, &y, &result);
    if (status != FP_POW_DOMAIN_ERROR)
	SIprintf("FPpow(): %.16f should have produced a domain error.\n", x,
		 '.');

    x = 5.5E+36;
    y = FMAX * 0.25;
    status = FPpow(&x, &y, &result);
    if (status != FP_OVERFLOW)
	SIprintf("FPpow(): %.16f, %.16f should have overflowed.\n", x, '.', y,
		 '.');

    x = 2.5E-10;
    y = 1E+10;
    status = FPpow(&x, &y, &result);
    if (status != FP_UNDERFLOW)
	SIprintf("FPpow(): %.16f, %.16f should have underflowed.\n", x, '.', y,
		 '.');
}

static void
TestFPdadd()
{
    double	i, j, result;
    STATUS      status;

    i = j = FMAX;

    status = FPdadd(&Infinity, &j, &result);
    if (status != FP_NOT_FINITE_ARGUMENT)
        SIprintf("FPdadd(): %.16f should not have been finite.\n", Infinity,
                 '.');

    status = FPdadd(&i, &Infinity, &result);
    if (status != FP_NOT_FINITE_ARGUMENT)
        SIprintf("FPdadd(): %.16f should not have been finite.\n", Infinity,
                 '.');

    status = FPdadd(&i, &j, &result);
    if (status != FP_OVERFLOW)
        SIprintf("FPdadd(): %.16f should have overflowed.\n", result);

    i = j = FMIN;
    status = FPdadd(&i, &j, &result);
    if (status != FP_UNDERFLOW)
        SIprintf("FPdadd(): %.16f should have underflowed.\n", result);
}

static void
TestFPdsub()
{
    double	i, j, result;
    STATUS      status;

    i = FMAX;
    j = -FMAX;

    status = FPdsub(&Infinity, &j, &result);
    if (status != FP_NOT_FINITE_ARGUMENT)
        SIprintf("FPdsub(): %.16f should not have been finite.\n", Infinity,
                 '.');

    status = FPdsub(&i, &Infinity, &result);
    if (status != FP_NOT_FINITE_ARGUMENT)
        SIprintf("FPdsub(): %.16f should not have been finite.\n", Infinity,
                 '.');

    status = FPdsub(&i, &j, &result);
    if (status != FP_OVERFLOW)
        SIprintf("FPdsub(): %.16f should have overflowed.\n", result);

    i = FMIN;
    j = FMAX;
    status = FPdsub(&i, &j, &result);
    if (status != FP_UNDERFLOW)
        SIprintf("FPdsub(): %.16f should have underflowed.\n", result);
}

static void
TestFPdmul()
{
    double	i, j, result;
    STATUS      status;

    i = FMAX;
    j = 2.0;

    status = FPdmul(&Infinity, &j, &result);
    if (status != FP_NOT_FINITE_ARGUMENT)
        SIprintf("FPdmul(): %.16f should not have been finite.\n", Infinity,
                 '.');

    status = FPdmul(&i, &Infinity, &result);
    if (status != FP_NOT_FINITE_ARGUMENT)
        SIprintf("FPdmul(): %.16f should not have been finite.\n", Infinity,
                 '.');

    status = FPdmul(&i, &j, &result);
    if (status != FP_OVERFLOW)
        SIprintf("FPdmul(): %.16f should have overflowed.\n", result);

    i = FMIN;
    j = 0.5;

    status = FPdmul(&i, &j, &result);
    if ((status != FP_UNDERFLOW) && (result != 0.0))
        SIprintf("FPdmul(): %.16f should have underflowed or been 0.0.\n",
		 result);
}

static void
TestFPddiv()
{
    double	i, j, result;
    STATUS      status;

    i = FMAX;
    j = 0.5;

    status = FPddiv(&Infinity, &j, &result);
    if (status != FP_NOT_FINITE_ARGUMENT)
        SIprintf("FPddiv(): %.16f should not have been finite.\n", Infinity,
                 '.');

    status = FPddiv(&i, &Infinity, &result);
    if (status != FP_NOT_FINITE_ARGUMENT)
        SIprintf("FPddiv(): %.16f should not have been finite.\n", Infinity,
                 '.');

    status = FPddiv(&i, &Zero, &result);
    if (status != FP_DIVIDE_BY_ZERO)
        SIprintf("FPddiv(): should have been gotten divide-by-zero error.\n");

    status = FPddiv(&i, &j, &result);
    if (status != FP_OVERFLOW)
        SIprintf("FPddiv(): %.16f should have overflowed.\n", result);

    i = FMIN;
    j = 2.0;

    status = FPddiv(&i, &j, &result);
    if ((status != FP_UNDERFLOW) && (result != 0.0))
        SIprintf("FPddiv(): %.16f should have underflowed or been 0.0.\n",
		 result);
}
#undef main

main(int argc, char *argv[])
{
    double *p;
    int *r;

    /* Create an illegal bit pattern */
    p = &Infinity;
    Infinity = 32768.0;
    r = p;
    *r = 32768;

    SIeqinit();
    TestFPdfinite();
    TestFPffinite();
    TestTrig();
    TestRand();
    TestFPsqrt();
    TestFPfdint();
    TestFPceil();
    TestEfuncs();
    TestFPipow();
    TestFPpow();
    TestFPdadd();
    TestFPdsub();
    TestFPdmul();
    TestFPddiv();
/*    
    TestFPpkadd();
    TestFPpksub();
    TestFPpkmul();
    TestFPpkdiv();
    TestFPpkabs();
    TestFPpkceil();
    TestFPpkcmp();
    TestFPpkint();
    TestFPpkneg();
*/
    PCexit(OK);
}
