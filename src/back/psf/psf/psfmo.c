/*
** Copyright (c) 2008 Ingres Corporation
*/

#include <compat.h>
#include <cs.h>
#include <iicommon.h>
#include <mo.h>
/*
** Name: PSFMO.C		- PSF Managed Objects
**
** Description:
**	This file contains the routines and data structures which define the
**	managed objects for PSF. Managed objects appear in the MIB, and allow
**	control and browsing of the PSF stats.
**
** History:
**	12-Oct-2008 (kiria01) SIR121012
**	    Added constant folding stats
**	27-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-queries - added exp.psf.pst.ssflatten
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
*/

/* TABLE OF CONTENTS */
STATUS psf_mo_init(void);


/*
** Counters for constant folding stats
*/
extern u_i4 Psf_fold;
extern u_i4 Psf_ssflatten;
#ifdef PSF_FOLD_DEBUG
extern u_i4 Psf_nops;
extern u_i4 Psf_nfolded;
extern u_i4 Psf_ncoerce;
extern u_i4 Psf_nfoldcoerce;
extern u_i4 Psf_nbexpr;
extern u_i4 Psf_nfoldbexpr;
#endif

static MO_CLASS_DEF PSFmo_psf_fold_control[] =
{
    {0, "exp.psf.pst.fold",
	sizeof(i4), MO_READ | MO_DBA_WRITE | MO_SERVER_WRITE | MO_SYSTEM_WRITE | MO_SECURITY_WRITE,
	0, 0, MOintget, MOintset, (PTR)&Psf_fold, MOcdata_index
    },
    {0, "exp.psf.pst.ssflatten",
	sizeof(i4), MO_READ | MO_DBA_WRITE | MO_SERVER_WRITE | MO_SYSTEM_WRITE | MO_SECURITY_WRITE,
	0, 0, MOintget, MOintset, (PTR)&Psf_ssflatten, MOcdata_index
    },
#   ifdef PSF_FOLD_DEBUG
    {0, "exp.psf.pst.nops",
	sizeof(i4), MO_READ | MO_DBA_WRITE | MO_SERVER_WRITE | MO_SYSTEM_WRITE | MO_SECURITY_WRITE,
	0, 0, MOintget, MOintset, (PTR)&Psf_nops, MOcdata_index
    },
    {0, "exp.psf.pst.nfolded",
	sizeof(i4), MO_READ | MO_DBA_WRITE | MO_SERVER_WRITE | MO_SYSTEM_WRITE | MO_SECURITY_WRITE,
	0, 0, MOintget, MOintset, (PTR)&Psf_nfolded, MOcdata_index
    },
    {0, "exp.psf.pst.ncoerce",
	sizeof(i4), MO_READ | MO_DBA_WRITE | MO_SERVER_WRITE | MO_SYSTEM_WRITE | MO_SECURITY_WRITE,
	0, 0, MOintget, MOintset, (PTR)&Psf_ncoerce, MOcdata_index
    },
    {0, "exp.psf.pst.nfoldcoerce",
	sizeof(i4), MO_READ | MO_DBA_WRITE | MO_SERVER_WRITE | MO_SYSTEM_WRITE | MO_SECURITY_WRITE,
	0, 0, MOintget, MOintset, (PTR)&Psf_nfoldcoerce, MOcdata_index
    },
    {0, "exp.psf.pst.nbexpr",
	sizeof(i4), MO_READ | MO_DBA_WRITE | MO_SERVER_WRITE | MO_SYSTEM_WRITE | MO_SECURITY_WRITE,
	0, 0, MOintget, MOintset, (PTR)&Psf_nbexpr, MOcdata_index
    },
    {0, "exp.psf.pst.nfoldbexpr",
	sizeof(i4), MO_READ | MO_DBA_WRITE | MO_SERVER_WRITE | MO_SYSTEM_WRITE | MO_SECURITY_WRITE,
	0, 0, MOintget, MOintset, (PTR)&Psf_nfoldbexpr, MOcdata_index
    },
#   endif
    {0, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL}
};


STATUS
psf_mo_init(void)
{
    STATUS	status;

    status = MOclassdef(MAXI2, PSFmo_psf_fold_control);

    return (status);
}
