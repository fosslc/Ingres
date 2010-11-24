/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/
 
#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
/* beginning of optimizer header files */
#include    <opglobal.h>
#include    <opdstrib.h>
#include    <opfcb.h>
#include    <opgcb.h>
#include    <opscb.h>
#include    <ophisto.h>
#include    <opboolfact.h>
#include    <oplouter.h>
#include    <opeqclass.h>
#include    <opcotree.h>
#include    <opvariable.h>
#include    <opattr.h>
#include    <openum.h>
#include    <opagg.h>
#include    <opmflat.h>
#include    <opcsubqry.h>
#include    <opsubquery.h>
#include    <opcstate.h>
#include    <opstate.h>
 
/* external routine declarations definitions */
#define        OPN_EPRIME      TRUE
#include    <mh.h>
#include    <opxlint.h>
 
 
/**
**
**  Name: OPNEPRIME.C - calculate Yao eprime function
**
**  Description:
**	Routines to calculate the Yao eprime function
**
**  History:    
**      15-jun-86 (seputis)    
**          initial creation from eprime.c
**      16-aug-91 (seputis)
**          add CL include files for PC group
**      15-dec-91 (seputis)
**          rework fix for MHexp problem to reduce floating point computations
**      22-Mar-93 (swm)
**          Removed conditional <math.h> include. The later <mh.h> include
**	    causes <math.h> to be included anyway; removal of the (illegal)
**	    <math.h> include here eliminates over 100 warning messages from
**	    the axp_osf C compiler, the messages resulting from (indirect)
**	    inclusion of <sys/types.h> before <systypes.h>.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	28-jul-98 (hayke02 for cbradley)
**	    Added an #else to the #ifdef to catch parameters of non-finite value
**	    which were causing the loop to be executed a large number of times
**	    on the Sparc Ultra.
**	22-jun-04 srisu02
**          Added _isnan() for Windows as a 100% CPU error was being obtained
**          in VDBA when the properties of an index where being queried.
** 	    isnan() works for Non-windows platforms, but for windows platforms 
**          _isnan() is needed.
**	11-aug-04 srisu02
**          Changed !MHisnan() to MHisnan() and also changed the logic 
**          such that ncells/2 is returned when the value of ncells is not a 
**          NAN and a value of 1 is returned when the value of rballs and 
**          (nmallspercell of ncells) is a NaN
**          And these tests would also be applied to Solaris as 
**          MHisnan -> _isnan() for Windows and isnan() for Solaris,
**	    (As defined in cl/hdr/hdr/mhcl.h)
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
OPH_DOMAIN opn_eprime(
	OPO_TUPLES rballs,
	OPO_TUPLES mballspercell,
	OPH_DOMAIN ncells,
	f8 underflow);

/*
**  Defines of other constants.
*/
 
/*
**      Magic number for YAO eprime function
*/
 
#define                 OPN_NRM         50.0
/* if r greater than OPN_NRM then normalize problem to OPN_NRM.  OPN_NRM must be about
** 15 or greater so that n cannot be normalized to zero 
** (see test for r > n * 10.0) 
*/

/*{
** Name: opn_eprime	- calculate Yao eprime function
**
** Description:
**	Number of cells touched when choosing r balls without replacement
**	from n cells holding m balls each.
** 
**	notation:
**	here - (r,m,n,t,d)
**	Yao - (k,n/m,m,n,d)
**	description - (# balls selected without replacement, # balls per
**		cell, # cells, total # balls, constant used by Yao)
**
**	see Yau, April 77, CACM
**
**      One application of this function is to calculate the number of
**      unique values in the active domain; i.e. eprime(r,m,n) can be
**	thought of as the number of non-empty cells when r balls are
**	placed randomlly in n cells where each cell can have a max of
**	m balls. the n cells are the original number of unique values.
**	m is the number of tuples with a given unique value (i.e. the
**	original repetition factor). m*n is the original number of 
**	tuples and r is the new number of tuples. eprime(r,n,m) gives
**	the new number of unique values.
**
**      Another application is to calculate the number of blocks of the 
**      inner relation of a join that will be touched.  This works whether 
**      the outer has duplicates or not.  Let each unique value of the outer 
**      be the balls and each block of the inner be the cells.  Then each 
**      cell holds itpb balls, so if we select the balls from the cells 
**      randomly without replacement we have our equation.  
** Inputs:
**      rballs                          number of "balls" selected without
**                                      replacement
**      mballspercell			max number of balls per cell
**      ncells			        original number of cells
**
** Outputs:
**	Returns:
**	    number of cells which were touched
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-jun-86 (seputis)
**          initial creation from eprime
**	24-sep-91 (seputis)
**	    precompute constant to check for underflow
[@history_line@]...
*/
OPH_DOMAIN
opn_eprime(
	OPO_TUPLES         rballs,
	OPO_TUPLES	   mballspercell,
	OPH_DOMAIN	   ncells,
	f8		   underflow)
{
    OPH_DOMAIN          totalballs;		/* total number of balls */
 
    /* 
    ** handle special cases
    */
#ifdef	SUN_FINITE
    if (!finite(rballs) || !finite(mballspercell) || !finite(ncells))
	return (1.0);
#else
    if (!MHffinite(rballs) || !MHffinite(mballspercell) || !MHffinite(ncells))
	return (1.0);
#endif

    if (MHisnan(ncells))
	return (1.0);

    if (MHisnan(rballs)|| MHisnan(mballspercell))
	return (ncells/2.0);


    if (ncells <= 1.0)
	return (1.0);
    if (mballspercell <= 1.0)
    {	/* less than one ball per cell (on the average) */
	OPH_DOMAIN             cellstouched;
	if (rballs < ncells)
	    cellstouched = rballs;
	else
	    cellstouched = ncells;
	if (cellstouched < 1.0)
	    return(1.0);
	else
	    return(cellstouched);
    }
 
    totalballs = mballspercell * ncells;	/* total number of balls */
    if (rballs > totalballs - mballspercell)
	return (ncells);			/* all cells must be hit */
    if (rballs <= 1.0)
	return (1.0);				/* only one ball chosen */
    if (rballs < (ncells / 10.0))
	return (rballs);			/* selecting from less than 10%
						** of the cells so probably all
						** touched cells will be
						** distinct */
    if (rballs / ncells < mballspercell / 4.0)
    {
	f8	    nat_log;			/* make partial calculation to determine
						** underflow possibility */
	/*	Number of cells touched when choosing r balls with replacement from 
	**      n cells            (n * ( 1 - (1-1/n)**r ))
	*/
	nat_log = MHln((f8)(1.0 - 1.0/ncells)) * (f8)rballs;
	if (nat_log <= underflow)
	    return(ncells);			/* avoid expected underflow */
	return (ncells * (1.0 - MHexp(nat_log)));
						/* retrieving, on the average,
						** less than one quarter of the
						** balls in a cell so probably
						** with or without replacement
						** will not make a difference
                                                */
#if 0
	return (ncells * (1.0 - MHpow (1.0 - 1.0 / ncells, rballs)));
#endif
    }
    if (rballs > ncells * 10.0)
	return (ncells);			/* will probably hit all the
						** cells */
    {
	OPN_PERCENT            normfactor;	/* normalization factor */
	if (rballs > OPN_NRM)			/* to keep computation less
						** time consuming, normalize to
						** a problem of selecting 
                                                ** OPN_NRM balls from n cells 
                                                ** holding m balls each where
                                                ** m remains the same but 
                                                ** n is proportionally reduced
                                                ** (this certainly introduces 
                                                ** errors) 
						*/
	{
	    normfactor = OPN_NRM / rballs;
	    rballs = OPN_NRM;
	    ncells *= normfactor;
	    totalballs *= normfactor;
	}
	else
	{
	    normfactor = 1.0;
	}
 
	{   /* calculate as per Yao */
	    OPO_TUPLES         td;		/* number of balls in n-1 cells
                                                ** on the average */
	    i4	       ballsremaining;  /* balls remaining to be
                                                ** selected */
	    OPN_PERCENT        prod;		/* percent of cells not touched
                                                */
	    td = totalballs * (1.0 - 1.0/ ncells);
	    prod = OPN_100PC;                   /* assume all cells touched */
	    for (ballsremaining = rballs; ballsremaining--;)
		prod *= ((td - ballsremaining) / (totalballs - ballsremaining));
	    return (ncells * (1.0 - prod) / normfactor);
	}
    }
}
