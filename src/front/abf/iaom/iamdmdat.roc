/*
**	Copyright (c) 1989, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include 	"iamdmtyp.h"
# include 	"eram.h"

/**
** Name:	iamdmdat.roc - Readonly data for Dependency Manager
**
** Description:
**	These are the tables that drive the DM.  Each is described where
**	declared.
**
** History:
**	7/21/90 (Mike S) Initial version
**/

FUNC_EXTERN STATUS IIAMdx1Disallow();
FUNC_EXTERN STATUS IIAMdx2Invalidate();
FUNC_EXTERN STATUS IIAMdx4CompCheck();
FUNC_EXTERN STATUS IIAMdx5FormDateCheck();
FUNC_EXTERN STATUS IIAMdx6TableDateCheck();
FUNC_EXTERN STATUS IIAMdx7RecordName();

/* Checks to make before deleting a component */

    /* Don't delete a record which is used by a global */
    static DM_CHECK del_record_global = 
	{DM_S_TRAVERSE, IIAMdx1Disallow, E_AM0030_GlobUsesRec, FALSE};

    /* Don't delete a record which contains another record */
    static DM_CHECK del_record_record = 
	{DM_S_TRAVERSE, IIAMdx1Disallow, E_AM0031_RecUsesRec, FALSE};

    GLOBALCONSTDEF DM_CHECK *iiAMdm1delCheck[NO_RC_TYPES] =
    {
	NULL, NULL, NULL, NULL, &del_record_global, &del_record_record, 
	NULL, NULL, NULL
    };

    /* External name, for callers outside the DM */
    GLOBALCONSTDEF PTR IIAMzdcDelCheck = (PTR)&iiAMdm1delCheck[0];

/* Things to do after changing or deleting a component */

    /* Invalidate a frame whose RCALL caller was deleted */
    static DM_CHECK inval_frame_return = 
	{DM_S_TRAVERSE, IIAMdx2Invalidate, APC_RETCHANGE, FALSE};

    /* Invalidate a frame if a Global or Record definition changes */
    static DM_CHECK inval_frame_glob = 
	{DM_S_TRAVERSE, IIAMdx2Invalidate, APC_GLOBCHANGE, FALSE};

    /* Keep traversing from a changed record to a containing record or global */
    static DM_CHECK traverse = 
	{DM_S_TRAVERSE, NULL, 0, TRUE};

    GLOBALCONSTDEF DM_CHECK *iiAMdm2Change[NO_RC_TYPES] = 
    {
	NULL, NULL, &inval_frame_return, &inval_frame_glob, &traverse, 
	&traverse, &inval_frame_glob, NULL, NULL
    };

    /* External name, for callers outside the DM */
    GLOBALCONSTDEF PTR IIAMzccCompChange = (PTR)&iiAMdm2Change[0];

/* Find a frame to be checked for compilation */

    /* Check for compilation */
    static DM_CHECK comp_check = 
	{DM_P_TRAVERSE, IIAMdx4CompCheck, 0, TRUE};

    GLOBALCONSTDEF DM_CHECK *IIAMdm3CompCheck[NO_RC_TYPES] =
    { &comp_check, &comp_check, &comp_check, 
      NULL, NULL, NULL, NULL, NULL, NULL };

    /* External name, for callers outside the DM */
    GLOBALCONSTDEF PTR IIAMzctCompTree = (PTR)&IIAMdm3CompCheck[0];

/* Check a frame for recompilation */

    /* Check form date */
    static DM_CHECK form_date_check =
	{DM_P_TRAVERSE, IIAMdx5FormDateCheck, 0, FALSE};

    /* Check table date */
    static DM_CHECK table_date_check =
	{DM_P_TRAVERSE, IIAMdx6TableDateCheck, 0, FALSE};

    GLOBALCONSTDEF DM_CHECK *IIAMdm4DateCheck[NO_RC_TYPES] =
    {NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
     &form_date_check, &table_date_check};

    /* External name, for callers outside the DM */
    GLOBALCONSTDEF PTR IIAMzdcDateChange = (PTR)&IIAMdm4DateCheck[0];

/* Check for recursive use of Record types */
    static DM_CHECK record_check =
	{DM_S_TRAVERSE, IIAMdx7RecordName, 0, TRUE};

    GLOBALCONSTDEF DM_CHECK *IIAMdm5RecNameChk[NO_RC_TYPES] = 
    {NULL, NULL, NULL, NULL, NULL, &record_check, NULL, NULL, NULL};

    /* External name, for callers outside the DM */
    GLOBALCONSTDEF PTR IIAMzraRecAttCheck = (PTR)&IIAMdm5RecNameChk[0];
