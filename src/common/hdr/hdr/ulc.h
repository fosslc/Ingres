/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: ULC.H - Utility for FE/DBMS Server Communications
**
** Description:
**      This file contains the definitions necessary for communication
**      between the frontend and the DBMS Server.  #defines are specified
**      as if they were typedef definitions so that the appropriate amount
**      of documentation can be included.
**
** History: $Log-for RCS$
**      08-Aug-86 (fred)
**          Created on Jupiter.
**      28-Aug-86 (neil)
**          Added definitions for front ends (mostly included in ulcfe.h).
**      15-Nov-1986 (fred)
**          Added ULC_NOROWCOUNT definition to response struct.
**      20-Nov-1986 (neil)
**          Added ULC_END_QRY in order to develop cursor code, and another bit
**	    mask for the global state (used by client).
**      19-Jan-1987 (rogerk)
**	    Added ULC_COPY, ULC_CPSTAT blocks, took out ULC_BCOPY, ULC_ECOPY
**	    and ULC_CCHECK.
**      19-Jan-1987 (rogerk)
**	    Added ulc modifier ULC_BOS_MASK to specify the begining block
**	    of a call sequence.
**      09-feb-1987 (fred)
**          Added sql stuff.  In particular, add ulc_qlang field to block hdr,
**          add ULC_SCOMMIT as a block type for sql sematics on a commit, and
**          add ULC_ALLDELUPD and ULC_REMNULLS to response block as warnings.
**      02-mar-1987 (neil)
**          Recommented some of the blocks and structures.
**      06-Apr-1987 (fred)
**          Added giving and taking of protection blocks for catalog update issues.
**      14-Jul-1987 (fred)
**          Radically altered for GCF addition
**      18-Oct-1987 (fred)
**          OBSOLETE
**      02-sep-1992 (rog)
**	    Resurrected to hold ulc function prototypes.
[@history_line@]...
[@history_template@]...
**/

/*
** Function prototypes
*/
FUNC_EXTERN DB_STATUS	ulc_bld_descriptor( PST_QNODE *target_list, i4  names,
					    QSF_RCB *qsf_rb,
					    GCA_TD_DATA **descrip,
					    i4  fake_big_as_small );
