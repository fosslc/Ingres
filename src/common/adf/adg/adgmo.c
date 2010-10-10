/*
** Copyright (c) 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cv.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <tm.h>
#include    <tr.h>
#include    <erglf.h>
#include    <sp.h>
#include    <mo.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>

/*
** Name: ADGMO.C		- ADF Managed Objects
**
** Description:
**	This file contains the routines and data structures which define the
**	managed objects for ADF. Managed objects appear in the MIB, and allow
**	browsing of the ADF Datatype, Operator, and Function-Instance tables
**  through the MO interface.
**
** History:
**	22-may-1996 (shero03)
**	    created
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	16-Feb-1999 (shero03)
**	    support up to 4 operands
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Jun-2008 (kiria01) b120519
**	    Add missing entries to ADG structure access
**	02-Jul-2008 (kiria01) SIR120473
**	    Added pattern related MO access
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
**	04-Oct-2010 (kiria01) b124065
**	    Fix compiler warnings.
*/

static STATUS	ADFmo_dt_index(
		    i4	    msg,
		    PTR	    cdata,
		    i4	    linstance,
		    char    *instance,
		    PTR	    *instdata);

static STATUS	ADFmo_op_index(
		    i4	    msg,
		    PTR	    cdata,
		    i4	    linstance,
		    char    *instance,
		    PTR	    *instdata);

static STATUS	ADFmo_fi_index(
		    i4	    msg,
		    PTR	    cdata,
		    i4	    linstance,
		    char    *instance,
		    PTR	    *instdata);

/*
** Format routine for operator type.
*/
static MO_GET_METHOD ADFmo_adg_op_type_get;
static MO_GET_METHOD ADFmo_adg_op_use_get;
static MO_GET_METHOD ADFmo_adg_fi_type_get;

static char dt_index[] = "exp.adf.adg.dt_ix";

static MO_CLASS_DEF ADFmo_adg_dt_classes[] =
{
    {0, dt_index,
	0, MO_READ, dt_index,
	0, MOpvget, MOnoset, (PTR)0, ADFmo_dt_index
    },
    {0, "exp.adf.adg.dt_name",
	MO_SIZEOF_MEMBER(ADI_DATATYPE, adi_dtname), MO_READ, dt_index,
	CL_OFFSETOF(ADI_DATATYPE, adi_dtname),
	MOstrget, MOnoset, (PTR)0, ADFmo_dt_index
    },
    {0, "exp.adf.adg.dt_id",
	MO_SIZEOF_MEMBER(ADI_DATATYPE, adi_dtid), MO_READ, dt_index,
	CL_OFFSETOF(ADI_DATATYPE, adi_dtid),
	MOintget, MOnoset, (PTR)0, ADFmo_dt_index
    },
    {0, "exp.adf.adg.dt_stat",
	MO_SIZEOF_MEMBER(ADI_DATATYPE, adi_dtstat_bits), MO_READ, dt_index,
	CL_OFFSETOF(ADI_DATATYPE, adi_dtstat_bits),
	MOintget, MOnoset, (PTR)0, ADFmo_dt_index
    },
    { 0 , NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL}
};


static char op_index[] = "exp.adf.adg.op_ix";

static MO_CLASS_DEF ADFmo_adg_op_classes[] =
{
    {0, op_index,
	0, MO_READ, op_index,
	0, MOpvget, MOnoset, (PTR)0, ADFmo_op_index
    },
    {0, "exp.adf.adg.op_name",
	MO_SIZEOF_MEMBER(ADI_OPRATION, adi_opname), MO_READ, op_index,
	CL_OFFSETOF(ADI_OPRATION, adi_opname),
	MOstrget, MOnoset, (PTR)0, ADFmo_op_index
    },
    {0, "exp.adf.adg.op_id",
	MO_SIZEOF_MEMBER(ADI_OPRATION, adi_opid), MO_READ, op_index,
	CL_OFFSETOF(ADI_OPRATION, adi_opid),
	MOintget, MOnoset, (PTR)0, ADFmo_op_index
    },
    {0, "exp.adf.adg.op_type",
	0, MO_READ, op_index,
	0, ADFmo_adg_op_type_get, MOnoset, (PTR)0, ADFmo_op_index
    },
    {0, "exp.adf.adg.op_use",
	0, MO_READ, op_index,
	0, ADFmo_adg_op_use_get, MOnoset, (PTR)0, ADFmo_op_index
    },
    {0, "exp.adf.adg.op_qlangs",
	MO_SIZEOF_MEMBER(ADI_OPRATION, adi_opqlangs), MO_READ, op_index,
	CL_OFFSETOF(ADI_OPRATION, adi_opqlangs),
	MOintget, MOnoset, (PTR)0, ADFmo_op_index
    },
    {0, "exp.adf.adg.op_systems",
	MO_SIZEOF_MEMBER(ADI_OPRATION, adi_opsystems), MO_READ, op_index,
	CL_OFFSETOF(ADI_OPRATION, adi_opsystems),
	MOintget, MOnoset, (PTR)0, ADFmo_op_index
    },
    {0, "exp.adf.adg.op_cntfi",
	MO_SIZEOF_MEMBER(ADI_OPRATION, adi_opcntfi), MO_READ, op_index,
	CL_OFFSETOF(ADI_OPRATION, adi_opcntfi),
	MOintget, MOnoset, (PTR)0, ADFmo_op_index
    },
    { 0 , NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL}
};

static char fi_index[] = "exp.adf.adg.fi_ix";

static MO_CLASS_DEF ADFmo_adg_fi_classes[] =
{
    {0, fi_index,
	0, MO_READ, fi_index,
	0, MOpvget, MOnoset, (PTR)0, ADFmo_fi_index
    },
    {0, "exp.adf.adg.fi_id",
	MO_SIZEOF_MEMBER(ADI_FI_DESC, adi_finstid), MO_READ, fi_index,
	CL_OFFSETOF(ADI_FI_DESC, adi_finstid),
	MOintget, MOnoset, (PTR)0, ADFmo_fi_index
    },
    {0, "exp.adf.adg.fi_cmplmnt",
	MO_SIZEOF_MEMBER(ADI_FI_DESC, adi_cmplmnt), MO_READ, fi_index,
	CL_OFFSETOF(ADI_FI_DESC, adi_cmplmnt),
	MOintget, MOnoset, (PTR)0, ADFmo_fi_index
    },
    {0, "exp.adf.adg.fi_type",
	0, MO_READ, fi_index,
	0, ADFmo_adg_fi_type_get, MOnoset, (PTR)0, ADFmo_fi_index
    },
    {0, "exp.adf.adg.fi_flags",
	MO_SIZEOF_MEMBER(ADI_FI_DESC, adi_fiflags), MO_READ, fi_index,
	CL_OFFSETOF(ADI_FI_DESC, adi_fiflags),
	MOintget, MOnoset, (PTR)0, ADFmo_fi_index
    },
    {0, "exp.adf.adg.fi_opid",
	MO_SIZEOF_MEMBER(ADI_FI_DESC, adi_fiopid), MO_READ, fi_index,
	CL_OFFSETOF(ADI_FI_DESC, adi_fiopid),
	MOintget, MOnoset, (PTR)0, ADFmo_fi_index
    },
    {0, "exp.adf.adg.fi_numargs",
	MO_SIZEOF_MEMBER(ADI_FI_DESC, adi_numargs), MO_READ, fi_index,
	CL_OFFSETOF(ADI_FI_DESC, adi_numargs),
	MOintget, MOnoset, (PTR)0, ADFmo_fi_index
    },
    {0, "exp.adf.adg.fi_dtresult",
	MO_SIZEOF_MEMBER(ADI_FI_DESC, adi_dtresult), MO_READ, fi_index,
	CL_OFFSETOF(ADI_FI_DESC, adi_dtresult),
	MOintget, MOnoset, (PTR)0, ADFmo_fi_index
    },
    {0, "exp.adf.adg.fi_dtarg1",
	MO_SIZEOF_MEMBER(ADI_FI_DESC, adi_dt[0]), MO_READ, fi_index,
	CL_OFFSETOF(ADI_FI_DESC, adi_dt[0]),
	MOintget, MOnoset, (PTR)0, ADFmo_fi_index
    },
    {0, "exp.adf.adg.fi_dtarg2",
	MO_SIZEOF_MEMBER(ADI_FI_DESC, adi_dt[1]), MO_READ, fi_index,
	CL_OFFSETOF(ADI_FI_DESC, adi_dt[1]),
	MOintget, MOnoset, (PTR)0, ADFmo_fi_index
    },
    {0, "exp.adf.adg.fi_dtarg3",
	MO_SIZEOF_MEMBER(ADI_FI_DESC, adi_dt[2]), MO_READ, fi_index,
	CL_OFFSETOF(ADI_FI_DESC, adi_dt[2]),
	MOintget, MOnoset, (PTR)0, ADFmo_fi_index
    },
    {0, "exp.adf.adg.fi_dtarg4",
	MO_SIZEOF_MEMBER(ADI_FI_DESC, adi_dt[3]), MO_READ, fi_index,
	CL_OFFSETOF(ADI_FI_DESC, adi_dt[3]),
	MOintget, MOnoset, (PTR)0, ADFmo_fi_index
    },
    { 0 , NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL}
};


static char *type_string =
"none,COMPARISON,OPERATOR,AGGREGATE,NORMAL,PREDICATE,COERCION,COPY-COERCION";

static char *use_string =
"none,PREFIX,POSTFIX,INFIX";

									 
static STATUS
ADFmo_dt_index(
    i4	    msg,
    PTR	    cdata,
    i4	    linstance,
    char    *instance,
    PTR	    *instdata)
{
    ADI_DATATYPE *dt;
    STATUS	    status;
    i4	    ix;
    i4	    maxix = (Adf_globs->Adi_dt_size) / sizeof(ADI_DATATYPE);

    CVal(instance, &ix);

    switch(msg)
    {
	case MO_GET:
	    if (ix < 1 || ix > maxix )
	    {
		  status = MO_NO_INSTANCE;
		  break;
	    }
	    dt = &Adf_globs->Adi_datatypes[ix];
	    if ((dt == NULL) ||
		(dt->adi_dtid < 1))
	    {
		  status = MO_NO_INSTANCE;
		  break;
	    }

	    *instdata = (PTR)dt;
	    status = OK;

	    break;

	case MO_GETNEXT:
	    while (++ix <= maxix)
	    {
	      dt = &Adf_globs->Adi_datatypes[ix];
	          if ((dt == NULL) ||
		      (dt->adi_dtid < 1))
		  {
		    continue;
		  }
		  else
		  {
		    *instdata = (PTR)dt;
		    status = MOlongout( MO_INSTANCE_TRUNCATED,
					(i8)ix,
					linstance, instance);
		    break;
		  }
	    }
	    if (ix > maxix)
		status = MO_NO_INSTANCE;
		
	    break;

	default:
	    status = MO_BAD_MSG;
	    break;
    }

    return (status);
}
									 
static STATUS
ADFmo_op_index(
    i4	    msg,
    PTR	    cdata,
    i4	    linstance,
    char    *instance,
    PTR	    *instdata)
{
    ADI_OPRATION *op;
    STATUS	    status;
    i4	    ix;
    i4         maxix = (Adf_globs->Adi_op_size) / sizeof(ADI_OPRATION);

    CVal(instance, &ix);

    switch(msg)
    {
	case MO_GET:
	    if (ix < 1 || ix > maxix )
	    {
		  status = MO_NO_INSTANCE;
		  break;
	    }
	    op = &Adf_globs->Adi_operations[ix];
	    if ((op == NULL) ||
		(op->adi_opid < 1))
	    {
		  status = MO_NO_INSTANCE;
		  break;
	    }

	    *instdata = (PTR)op;
	    status = OK;

	    break;

	case MO_GETNEXT:
	    while (++ix <= maxix)
	    {
	      op = &Adf_globs->Adi_operations[ix];
	          if ((op == NULL) ||
		      (op->adi_opid < 1))
		  {
		    continue;
		  }
		  else
		  {
		    *instdata = (PTR)op;
		    status = MOlongout( MO_INSTANCE_TRUNCATED,
					(i8)ix,
					linstance, instance);
		    break;
		  }
	    }
	    if (ix > maxix)
		status = MO_NO_INSTANCE;
		
	    break;

	default:
	    status = MO_BAD_MSG;
	    break;
    }

    return (status);
}
									 
static STATUS
ADFmo_fi_index(
    i4	    msg,
    PTR	    cdata,
    i4	    linstance,
    char    *instance,
    PTR	    *instdata)
{
    ADI_FI_DESC *fi;
    STATUS	    status;
    i4	    ix;
    i4         maxix = Adf_globs->Adi_num_fis;

    CVal(instance, &ix);

    switch (msg)
    {
	case MO_GET:
	    if (ix < 1 || ix > maxix )
	    {
		  status = MO_NO_INSTANCE;
		  break;
	    }
	    fi = &Adf_globs->Adi_fis[ix];
	    if ((fi == NULL) ||
		(fi->adi_finstid < 1))
	    {
		  status = MO_NO_INSTANCE;
		  break;
	    }

	    *instdata = (PTR)fi;
	    status = OK;

	    break;

	case MO_GETNEXT:
	    while (++ix <= maxix)
	    {
	      fi = &Adf_globs->Adi_fis[ix];
	      if ((fi == NULL) ||
	          (fi->adi_finstid < 1))
	      {
	        continue;
	      }
	      else
	      {
	        *instdata = (PTR)fi;
	        status = MOlongout( MO_INSTANCE_TRUNCATED,
					(i8)ix,
					linstance, instance);
	        break;
	      }
	    }
	    if (ix > maxix)
		status = MO_NO_INSTANCE;
		
	    break;

	default:
	    status = MO_BAD_MSG;
	    break;
    }

    return (status);
}

/*
** Name: ADFmo_adg_op_type_get - MO Get function for the op_type field
**
** Description:
**	This routine is called by MO to provide a "user-friendly"
**	interpretation of the operation type.
**
** Inputs:
**	offset			    - offset into the ADI_OPRATION, ignored.
**	objsize			    - size of the type field, ignored
**	object			    - the ADI_OPRATION address
**	luserbuf		    - length of the output buffer
**
** Outputs:
**	userbuf			    - written with nice status string
**
** Returns:
**	STATUS
**
** History:
**	22-may-1996 (shero03)
**	    Created from LK_lkb_rsb_get.
*/
static STATUS
ADFmo_adg_op_type_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    ADI_OPRATION	*op = (ADI_OPRATION *)object;
    char	format_buf [30];

    MEfill(sizeof(format_buf), (u_char)0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%w",
	    type_string, op->adi_optype);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}


/*
** Name: ADFmo_adg_fi_type_get - MO Get function for the fi_type field
**
** Description:
**	This routine is called by MO to provide a "user-friendly"
**	interpretation of the operation type.
**
** Inputs:
**	offset			    - offset into the ADI_FI_DESC, ignored.
**	objsize			    - size of the type field, ignored
**	object			    - the ADI_FI_DESC address
**	luserbuf		    - length of the output buffer
**
** Outputs:
**	userbuf			    - written with nice status string
**
** Returns:
**	STATUS
**
** History:
**	22-may-1996 (shero03)
**	    Created from LK_lkb_rsb_get.
*/
static STATUS
ADFmo_adg_fi_type_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    ADI_FI_DESC	*fi = (ADI_FI_DESC *)object;
    char	format_buf [30];

    MEfill(sizeof(format_buf), (u_char)0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%w",
	    type_string, fi->adi_fitype);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: ADFmo_adg_op_use_get - MO Get function for the op_use field
**
** Description:
**	This routine is called by MO to provide a "user-friendly"
**	interpretation of the operation use.
**
** Inputs:
**	offset			    - offset into the ADI_OPRATION, ignored.
**	objsize			    - size of the use field, ignored
**	object			    - the ADI_OPRATION address
**	luserbuf		    - length of the output buffer
**
** Outputs:
**	userbuf			    - written with nice status string
**
** Returns:
**	STATUS
**
** History:
**	18-Jun-2008 (kiria01) b120519
**	    Created
*/
static STATUS
ADFmo_adg_op_use_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    ADI_OPRATION	*op = (ADI_OPRATION *)object;
    char	format_buf [30];

    MEfill(sizeof(format_buf), (u_char)0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%w",
	    use_string, op->adi_opuse);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}



extern i4 ADU_pat_debug;
extern i4 ADU_pat_legacy;
extern u_i4 ADU_pat_cmplx_lim;
extern u_i4 ADU_pat_cmplx_max;
extern u_i4 ADU_pat_seg_sz;

static MO_CLASS_DEF ADFmo_adu_pat_control[] =
{
    {0, "exp.adf.adu.pat_debug",
	sizeof(i4), MO_READ | MO_DBA_WRITE | MO_SERVER_WRITE | MO_SYSTEM_WRITE | MO_SECURITY_WRITE,
	0, 0, MOintget, MOintset, (PTR)&ADU_pat_debug, MOcdata_index
    },
    {0, "exp.adf.adu.pat_legacy",
	sizeof(i4), MO_READ | MO_DBA_WRITE | MO_SERVER_WRITE | MO_SYSTEM_WRITE | MO_SECURITY_WRITE,
	0, 0, MOintget, MOintset, (PTR)&ADU_pat_legacy, MOcdata_index
    },
    {0, "exp.adf.adu.pat_cmplx_max",
	sizeof(i4), MO_READ | MO_DBA_WRITE | MO_SERVER_WRITE | MO_SYSTEM_WRITE | MO_SECURITY_WRITE,
	0, 0, MOintget, MOintset, (PTR)&ADU_pat_cmplx_max, MOcdata_index
    },
    {0, "exp.adf.adu.pat_cmplx_lim",
	sizeof(i4), MO_READ | MO_DBA_WRITE | MO_SERVER_WRITE | MO_SYSTEM_WRITE | MO_SECURITY_WRITE,
	0, 0, MOintget, MOintset, (PTR)&ADU_pat_cmplx_lim, MOcdata_index
    },
    {0, "exp.adf.adu.pat_seg_sz",
	sizeof(i4), MO_READ | MO_DBA_WRITE | MO_SERVER_WRITE | MO_SYSTEM_WRITE | MO_SECURITY_WRITE,
	0, 0, MOintget, MOintset, (PTR)&ADU_pat_seg_sz, MOcdata_index
    },
    { 0 , NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL}
};


STATUS
ADFmo_attach_adg(void)
{
    STATUS	status;

    status = MOclassdef(MAXI2, ADFmo_adg_dt_classes);

    if (status == OK)
	status = MOclassdef(MAXI2, ADFmo_adg_op_classes);

    if (status == OK)
	status = MOclassdef(MAXI2, ADFmo_adg_fi_classes);

    if (status == OK)
	status = MOclassdef(MAXI2, ADFmo_adu_pat_control);

    return (status);
}
