//#define NOUSEICEAPI
/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**   Project : Ingres Visual DBA
**
**   Source : icedata.c
**   low level for getting ICE objects properties information
**
**   History:
**    07-Jul-99 (noifr01)
**     bug 97822: alter business unit page was not working
**     (some of the passed input parameters to the C-API function
**     call were wrong)
**    26-Jul-99 (noifr01)
**     bug 98051: drop a business unit page or facet was failing
**     because of a wrong parameter passed to the C-API function
**    24-Dec-99 (noifr01)
**     bug 97102: fill ICE_C_PARAMS arrays through a function, since
**     non-constant initializers are not supported with some UNIX
**     compilers
**    05-Jan-2000 (noifr01)
**     bug 99866: (dropping an ICE variable doesn't work). 
**     use the getSvrVariables method rather than getVariables for dropping
**     the ICE Server Variable. (this fix is just for preparing the work
**     for when the functionality will be available. Until then, the
**     functionality will be disabled in VDBA)
**    28-Feb-2000(schph01)
**     Sir #100559: Add fonction that verify if the username and password are
**     valid for the connection.
**    01-Mar-2000 (noifr01)
**     bug 100653: add the ICE error to the history of errors when getting the
**     detail on ICE objects, where it was missing, and at a couple of other 
**     places
**    02-Mar-2000 (noifr01)
**     bug 100691: when adding or altering a business unit security user,
**     the wrong elements in the structure were passed to the ICE API function
**    03-Aug-2000 (schph01)
**     (bug 99866) updated the "action" input string passed to the ICE API 
**     for add and drop ICE servier variable, according to change 446504 for
**     bug  102229
**    27-Jul-2010 (drivi01)
**     Add #ifndef NOUSEICEAPI to remove ice code that can be enabled
**     at any time by enable NOUSEICEAPI from a solution.
********************************************************************/

#include "dba.h" 
#include "error.h"
#include "ice.h"
#include "domdata.h"
#include "dbaset.h"
#include "winutils.h"
#include "dll.h"
#include "resource.h"

static BOOL FillIceParamsArray( ICE_C_PARAMS * pDest, ...)
{
   va_list marker;
   char * pc;
   int i;
   va_start( marker, pDest );
   while( TRUE )   { 
     i  = va_arg( marker,  int);   
	 if (i==0) // end of array includes last element with zeroes
		 break;
	 pDest->type = i;
     pc = va_arg( marker, char *);   
	 pDest->name = pc;
     pc = va_arg( marker, char *);   
	 pDest->value = pc;
	 pDest++;
   }
   pDest->type = 0;
   pDest->name = NULL;
   pDest->value = NULL;
   va_end( marker );
   return TRUE;
}
void ManageIceError(ICE_C_CLIENT * pICEclient, ICE_STATUS * pstatus)
{
	if (*pstatus!=OK) {
#ifndef NOUSEICEAPI
		char * p1=ICE_C_LastError(*pICEclient);
		if (!p1) 
			p1="Error while fetching ICE error information";
		AddSqlError("ICE error", p1,-1);
#endif
	}
}
static int GetICEInfoFromID(LPUCHAR lpVirtNode,int iobjecttype, void *lpparameters)
{
#ifndef NOUSEICEAPI
	TCHAR			bufuser[100],bufpw[100];
	ICE_STATUS		status = 0;
	ICE_C_CLIENT	ICEclient = NULL;
	LPUCHAR pret[20];
	int iresult = RES_SUCCESS;
	int end,i;
	if (!lpparameters) {
		myerror(ERR_INVPARAMS);
		return RES_ERR;
	}

	// first step: when there is a parenthood, get ID of the parent object by completing 
	// the parent struture
	switch (iobjecttype) {
		case OT_ICE_BUNIT_SEC_ROLE:
		{
			LPICEBUSUNITROLEDATA lpIceData = (LPICEBUSUNITROLEDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_BUNIT, &(lpIceData->icebusunit));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_ROLE, &(lpIceData->icerole));
			break;
		}
		case OT_ICE_BUNIT_SEC_USER:
		{
			LPICEBUSUNITWEBUSERDATA lpIceData = (LPICEBUSUNITWEBUSERDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_BUNIT, &(lpIceData->icebusunit));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_WEBUSER, &(lpIceData->icewevuser));
			break;
		}
		case OT_ICE_BUNIT_FACET:
		case OT_ICE_BUNIT_PAGE:
		{
			LPICEBUSUNITDOCDATA lpIceData = (LPICEBUSUNITDOCDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_BUNIT, &(lpIceData->icebusunit));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_SERVER_LOCATION, &(lpIceData->ext_loc));
			break;
		}
		case OT_ICE_BUNIT_LOCATION:
		{
			LPICEBUSUNITLOCATIONDATA lpIceData = (LPICEBUSUNITLOCATIONDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_BUNIT, &(lpIceData->icebusunit));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_SERVER_LOCATION, &(lpIceData->icelocation));
			break;
		}
	}
	if (iresult !=RES_SUCCESS)
		return iresult;

	if (GetICeUserAndPassWord(GetVirtNodeHandle(lpVirtNode),bufuser,bufpw)!=RES_SUCCESS)
		return RES_ERR;
    if ((status =ICE_C_Initialize ()) != OK) {
		ManageIceError(&ICEclient, &status);
		return RES_ERR;
	}
    if ((status = ICE_C_Connect (CAPINodeName(lpVirtNode), bufuser,bufpw, &ICEclient)) != OK) {
		ManageIceError(&ICEclient, &status);
		return RES_ERR;
	}
	ShowHourGlass();
	switch (iobjecttype) {
		case OT_ICE_DBUSER:
		{
			LPICEDBUSERDATA lpIceData = (LPICEDBUSERDATA)lpparameters;
			ICE_C_PARAMS select[] =
			{
				{ICE_IN , "action"        ,  "select" },
				{ICE_OUT, "dbuser_id"     ,  NULL     },
				{ICE_OUT, "dbuser_alias"  ,  NULL     },
				{ICE_OUT, "dbuser_name"   ,  NULL     },
				{ICE_OUT, "dbuser_comment",  NULL     },
				{0      , NULL            ,  NULL     }
			};
			if (!x_strlen(lpIceData->ID)) {
				iresult=RES_ERR;
				break;
			}
			if ((status = ICE_C_Execute (ICEclient, "dbuser", select)) != OK) {
				ManageIceError(&ICEclient, &status);
 				iresult=RES_ERR;
				break;
			}
			do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					for (i=0;i<4;i++) {
						pret[i]=ICE_C_GetAttribute (ICEclient, i+1);
						if (!pret[i])
							pret[i]="";
					}
					if (!x_stricmp(pret[0], lpIceData->ID)) {
						fstrncpy(lpIceData->UserAlias,pret[1],sizeof(lpIceData->UserAlias));
						fstrncpy(lpIceData->User_Name ,pret[2],sizeof(lpIceData->User_Name));
						fstrncpy(lpIceData->Comment  ,pret[3],sizeof(lpIceData->Comment));
						break;
					}
                }
            }
            while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_DBCONNECTION:
		{
			LPDBCONNECTIONDATA lpDBConnectionData = (LPDBCONNECTIONDATA) lpparameters;
			ICE_C_PARAMS select[] =
			{
				{ICE_IN , "action"    ,  "select" },
				{ICE_OUT, "db_id"     ,  NULL     },
				{ICE_OUT, "db_name"   ,  NULL     },
				{ICE_OUT, "db_dbname" ,  NULL     },
				{ICE_OUT, "db_dbuser" ,  NULL     },
				{ICE_OUT, "db_comment",  NULL     },
				{0      , NULL        ,  NULL     }
			};
			if (!x_strlen(lpDBConnectionData->ID)) {
				iresult=RES_ERR;
				break;
			}
			if ((status = ICE_C_Execute (ICEclient, "database", select)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					if (!x_stricmp(ICE_C_GetAttribute (ICEclient, 1), lpDBConnectionData->ID)) {
//						fstrncpy(lpDBConnectionData->ID       ,ICE_C_GetAttribute (ICEclient, 1),sizeof(lpDBConnectionData->ID));
						fstrncpy(lpDBConnectionData->ConnectionName ,ICE_C_GetAttribute (ICEclient, 2),sizeof(lpDBConnectionData->ConnectionName));
						fstrncpy(lpDBConnectionData->DBName   ,ICE_C_GetAttribute (ICEclient, 3),sizeof(lpDBConnectionData->DBName));
						fstrncpy(lpDBConnectionData->DBUsr.ID ,ICE_C_GetAttribute (ICEclient, 4),sizeof(lpDBConnectionData->DBUsr.ID));
						fstrncpy(lpDBConnectionData->Comment  ,ICE_C_GetAttribute (ICEclient, 5),sizeof(lpDBConnectionData->Comment));
						break;
					}
                }
            }
            while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_WEBUSER:
		{
			LPICEWEBUSERDATA lpWebUserData = (LPICEWEBUSERDATA) lpparameters;
			ICE_C_PARAMS select[] =
			{
				{ICE_IN , "action"             ,  "select" },
				{ICE_OUT, "user_id"            ,  NULL     },
				{ICE_OUT, "user_name"          ,  NULL     }, //1
				{ICE_OUT, "user_dbuser"        ,  NULL     }, //2
				{ICE_OUT, "user_comment"       ,  NULL     },
				{ICE_OUT, "user_administration",  NULL     },
				{ICE_OUT, "user_security"      ,  NULL     },
				{ICE_OUT, "user_unit"          ,  NULL     },
				{ICE_OUT, "user_monitor"       ,  NULL     },
				{ICE_OUT, "user_timeout"       ,  NULL     },
				{ICE_OUT, "user_authtype"      ,  NULL     },
				{0      , NULL                 ,  NULL     }
			};
			if (!x_strlen(lpWebUserData->UserName)) {
				iresult=RES_ERR;
				break;
			}
			if ((status = ICE_C_Execute (ICEclient, "user", select)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
            {
               if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					if (!x_stricmp(ICE_C_GetAttribute (ICEclient, 2), lpWebUserData->UserName)) {
						fstrncpy(lpWebUserData->ID       ,ICE_C_GetAttribute (ICEclient,1),sizeof(lpWebUserData->ID));
//						fstrncpy(lpWebUserData->DefDBUsr.UserName,ICE_C_GetAttribute (ICEclient, 3),sizeof(lpWebUserData->DefDBUsr.UserName));
						fstrncpy(lpWebUserData->Comment  ,ICE_C_GetAttribute (ICEclient, 4),sizeof(lpWebUserData->Comment));
						lpWebUserData->bAdminPriv    = !x_stricmp(ICE_C_GetAttribute (ICEclient, 5),"checked")?TRUE:FALSE;
						lpWebUserData->bSecurityPriv = !x_stricmp(ICE_C_GetAttribute (ICEclient, 6),"checked")?TRUE:FALSE;
						lpWebUserData->bUnitMgrPriv  = !x_stricmp(ICE_C_GetAttribute (ICEclient, 7),"checked")?TRUE:FALSE;
						lpWebUserData->bMonitorPriv  = !x_stricmp(ICE_C_GetAttribute (ICEclient, 8),"checked")?TRUE:FALSE;
						lpWebUserData->ltimeoutms    = atol(ICE_C_GetAttribute (ICEclient, 9));
						if (!x_stricmp(ICE_C_GetAttribute (ICEclient, 10),"ICE"))
							lpWebUserData->bICEpwd = TRUE;
						else
							lpWebUserData->bICEpwd = FALSE;

						break;
					}
                }
            }
            while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_PROFILE:
		{
			LPICEPROFILEDATA lpIceProfileData = (LPICEPROFILEDATA) lpparameters;
			ICE_C_PARAMS select[] =
			{
				{ICE_IN , "action"                 ,  "select" },
				{ICE_OUT, "profile_id"             ,  NULL     },
				{ICE_OUT, "profile_name"           ,  NULL     },
				{ICE_OUT, "profile_dbuser"         ,  NULL     },
//				{ICE_OUT, "profile_comment"        ,  NULL     },
				{ICE_OUT, "profile_administration" ,  NULL     },
				{ICE_OUT, "profile_security"       ,  NULL     },
				{ICE_OUT, "profile_unit"           ,  NULL     },
				{ICE_OUT, "profile_monitor"        ,  NULL     },
				{ICE_OUT, "profile_timeout"        ,  NULL     },
				{0      , NULL                     ,  NULL     }
			};
			LPUCHAR pret[20];
			if (!x_strlen(lpIceProfileData->ProfileName)) {
				iresult=RES_ERR;
				break;
			}
			if ((status = ICE_C_Execute (ICEclient, "profile", select)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					for (i=0;i<8;i++) {
						pret[i]=ICE_C_GetAttribute (ICEclient, i+1);
						if (!pret[i])
							pret[i]="";
					}
					if (!x_stricmp(pret[1], lpIceProfileData->ProfileName)) {
						fstrncpy(lpIceProfileData->ID        ,pret[0],sizeof(lpIceProfileData->ID));
//						fstrncpy(lpIceProfileData->DefDBUsr.UserName ,pret[2],sizeof(lpIceProfileData->DefDBUsr.UserName));
						fstrncpy(lpIceProfileData->Comment,   "",sizeof(lpIceProfileData->Comment));
						lpIceProfileData->bAdminPriv    = !x_stricmp(pret[3],"checked")?TRUE:FALSE;
						lpIceProfileData->bSecurityPriv = !x_stricmp(pret[4],"checked")?TRUE:FALSE;
						lpIceProfileData->bUnitMgrPriv  = !x_stricmp(pret[5],"checked")?TRUE:FALSE;
						lpIceProfileData->bMonitorPriv  = !x_stricmp(pret[6],"checked")?TRUE:FALSE;
						lpIceProfileData->ltimeoutms    = atol(pret[7]);
						break;
					}
                }
            }
            while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_ROLE:
		{
			LPICEROLEDATA lpIceRoleData = (LPICEROLEDATA) lpparameters;
			ICE_C_PARAMS select[] =
			{
				{ICE_IN , "action"       ,  "select" },
				{ICE_OUT, "role_id"      ,  NULL     },
				{ICE_OUT, "role_name"    ,  NULL     },
				{ICE_OUT, "role_comment" ,  NULL     },
				{0      , NULL           ,  NULL     }
			};
			if (!x_strlen(lpIceRoleData->RoleName)) {
				iresult=RES_ERR;
				break;
			}
			if ((status = ICE_C_Execute (ICEclient, "role", select)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
			{
				if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					if (!x_stricmp(ICE_C_GetAttribute (ICEclient, 2), lpIceRoleData->RoleName)) {
						fstrncpy(lpIceRoleData->ID,ICE_C_GetAttribute (ICEclient, 1),sizeof(lpIceRoleData->ID));
						fstrncpy(lpIceRoleData->Comment,ICE_C_GetAttribute (ICEclient, 3),sizeof(lpIceRoleData->Comment));
						break;
					}
				}
			}
			while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_BUNIT:
		{
			LPICEBUSUNITDATA lpBusUnitData = (LPICEBUSUNITDATA) lpparameters;
			ICE_C_PARAMS select[] =
			{
				{ICE_IN , "action"       ,  "select" },
				{ICE_OUT, "unit_id"      ,  NULL     },
				{ICE_OUT, "unit_name"    ,  NULL     },
				{0      , NULL           ,  NULL     }
			};
			if (!x_strlen(lpBusUnitData->Name)) {
				iresult=RES_ERR;
				break;
			}
			if ((status = ICE_C_Execute (ICEclient, "unit", select)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
			{
			if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					if (!x_stricmp(ICE_C_GetAttribute (ICEclient, 2), lpBusUnitData->Name)) {
						fstrncpy(lpBusUnitData->ID,ICE_C_GetAttribute (ICEclient, 1),sizeof(lpBusUnitData->ID));
						break;
					}
				}
			}
			while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_SERVER_VARIABLE:
		{
			LPICESERVERVARDATA lpIceData = (LPICESERVERVARDATA)lpparameters;
			ICE_C_PARAMS select[] =
			{
				{ICE_OUT, "page"     ,  NULL     },
				{ICE_OUT, "session"  ,  NULL     },
				{ICE_OUT, "server"   ,  NULL     },
				{ICE_OUT, "name"     ,  NULL     },
				{ICE_OUT, "value"    ,  NULL     },
				{0      , NULL       ,  NULL     }
			};
			if (!x_strlen(lpIceData->VariableName)) {
				iresult=RES_ERR;
				break;
			}
			if ((status = ICE_C_Execute (ICEclient, "getVariables", select)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
			{
				if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					for (i=0;i<5;i++) {
						pret[i]=ICE_C_GetAttribute (ICEclient, i+1);
						if (!pret[i])
							pret[i]="";
					}
					if (!x_stricmp(pret[3], lpIceData->VariableName)) {
						fstrncpy(lpIceData->VariableValue  ,pret[4],sizeof(lpIceData->VariableValue));
						break;
					}
				}
			}
			while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		case OT_ICE_SERVER_APPLICATION:
		{
			LPICEAPPLICATIONDATA lpAppData = (LPICEAPPLICATIONDATA) lpparameters;
			ICE_C_PARAMS select[] =
			{
				{ICE_IN , "action"       ,  "select" },
				{ICE_OUT, "sess_name"    ,   NULL    },
//				{ICE_OUT, "sess_id"      ,  NULL     },
				{0      , NULL           ,  NULL     }
			};
			if (!x_strlen(lpAppData->Name)) {
				iresult=RES_ERR;
				break;
			}
			if ((status = ICE_C_Execute (ICEclient, "session_grp", select)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
			{
			if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					if (!x_stricmp(ICE_C_GetAttribute (ICEclient, 1), lpAppData->Name)) {
//						fstrncpy(lpAppData->ID,ICE_C_GetAttribute (ICEclient, 2),sizeof(lpAppData->ID));
						break;
					}
				}
			}
			while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_SERVER_LOCATION:
		{
			LPICELOCATIONDATA lpLocData = (LPICELOCATIONDATA) lpparameters;
			ICE_C_PARAMS select[] =
			{
				{ICE_IN , "action"        ,  "select" },
				{ICE_OUT, "loc_id"        ,  NULL     },
				{ICE_OUT, "loc_name"      ,  NULL     },
				{ICE_OUT, "loc_path"      ,  NULL     },
				{ICE_OUT, "loc_extensions",  NULL     },
				{ICE_OUT, "loc_http"      ,  NULL     },
				{ICE_OUT, "loc_ice"       ,  NULL     },
				{ICE_OUT, "loc_public"    ,  NULL     },
				{0      ,  NULL           ,  NULL     }
			};
			if (!x_strlen(lpLocData->ID)) {
				iresult=RES_ERR;
				break;
			}
			if ((status = ICE_C_Execute (ICEclient, "ice_locations", select)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
			{
			if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					for (i=0;i<7;i++) {
						pret[i]=ICE_C_GetAttribute (ICEclient, i+1);
						if (!pret[i])
							pret[i]="";
					}
					if (!x_stricmp(pret[0], lpLocData->ID)) {
//						fstrncpy(lpLocData->ID          ,pret[0],sizeof(lpLocData->ID));
						fstrncpy(lpLocData->LocationName,pret[1],sizeof(lpLocData->LocationName));
						fstrncpy(lpLocData->path        ,pret[2],sizeof(lpLocData->path));
						fstrncpy(lpLocData->extensions  ,pret[3],sizeof(lpLocData->extensions));
						lpLocData->bIce    = !x_stricmp(pret[4],"checked")?FALSE:TRUE;
						lpLocData->bPublic = !x_stricmp(pret[6],"checked")?TRUE:FALSE;
						break;
					}
				}
			}
			while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_BUNIT_SEC_ROLE:
		{
			LPICEBUSUNITROLEDATA lpBusUnitRole = (LPICEBUSUNITROLEDATA) lpparameters;
			ICE_C_PARAMS select[10];
			FillIceParamsArray( select,
				ICE_IN , "action"       ,  "retrieve" ,
				ICE_IN,  "ur_unit_id"   ,  lpBusUnitRole->icebusunit.ID     ,
				ICE_OUT, "ur_role_id"   ,  NULL     ,
				ICE_OUT, "ur_role_name" ,  NULL     ,
				ICE_OUT, "ur_execute"   ,  NULL     ,
				ICE_OUT, "ur_read"      ,  NULL     ,
				ICE_OUT, "ur_insert"    ,  NULL     ,
				0      ,  NULL          ,  NULL     
			);
			if ((status = ICE_C_Execute (ICEclient, "unit_role", select)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
			{
			if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					for (i=0;i<6;i++) {
						pret[i]=ICE_C_GetAttribute (ICEclient, i+1);
						if (!pret[i])
							pret[i]="";
					}
					if (!x_stricmp(pret[1],lpBusUnitRole->icerole.ID)) {
						fstrncpy(lpBusUnitRole->icerole.RoleName,pret[2],sizeof(lpBusUnitRole->icerole.RoleName));
						lpBusUnitRole->bExecDoc   = !x_stricmp(pret[3],"checked")?FALSE:TRUE;
						lpBusUnitRole->bReadDoc   = !x_stricmp(pret[4],"checked")?FALSE:TRUE;
						lpBusUnitRole->bCreateDoc = !x_stricmp(pret[5],"checked")?FALSE:TRUE;
						break;
					}
				}
			}
			while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_BUNIT_SEC_USER:
		{
			LPICEBUSUNITWEBUSERDATA lpBusUnitUser = (LPICEBUSUNITWEBUSERDATA) lpparameters;
			ICE_C_PARAMS select[10];
			FillIceParamsArray( select,
				ICE_IN , "action"       ,  "retrieve" ,
				ICE_IN,  "uu_unit_id"   ,  lpBusUnitUser->icebusunit.ID  ,
				ICE_OUT, "uu_user_id"   ,  NULL     ,
				ICE_OUT, "uu_user_name" ,  NULL     ,
				ICE_OUT, "uu_execute"   ,  NULL     ,
				ICE_OUT, "uu_read"      ,  NULL     ,
				ICE_OUT, "uu_insert"    ,  NULL     ,
				0      ,  NULL          ,  NULL     
			);

			if ((status = ICE_C_Execute (ICEclient, "unit_user", select)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
			{
			if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					for (i=0;i<5;i++) {
						pret[i]=ICE_C_GetAttribute (ICEclient, i+1);
						if (!pret[i])
							pret[i]="";
					}
					if (!x_stricmp(pret[1],lpBusUnitUser->icewevuser.ID)) {
						fstrncpy(lpBusUnitUser->icewevuser.UserName,pret[2],sizeof(lpBusUnitUser->icewevuser.UserName));
						lpBusUnitUser->bUnitRead   = !x_stricmp(pret[3],"checked")?FALSE:TRUE;
						lpBusUnitUser->bReadDoc   = !x_stricmp(pret[4],"checked")?FALSE:TRUE;
						lpBusUnitUser->bCreateDoc = !x_stricmp(pret[5],"checked")?FALSE:TRUE;
						break;
					}
				}
			}
			while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_BUNIT_FACET:
		case OT_ICE_BUNIT_PAGE:
		{
			LPICEBUSUNITDOCDATA lpBusUnitDoc = (LPICEBUSUNITDOCDATA) lpparameters;
			ICE_C_PARAMS select[] =
			{
				{ICE_IN , "action"            ,  "select" },
				{ICE_OUT,  "doc_id"           ,  NULL     }, //0
				{ICE_OUT, "doc_type"          ,  NULL     }, //1
				{ICE_OUT, "doc_unit_id"       ,  NULL     }, //2
				{ICE_OUT, "doc_unit_name"     ,  NULL     }, //3
				{ICE_OUT, "doc_name"          ,  NULL     }, //4
				{ICE_OUT, "doc_suffix"        ,  NULL     }, //5
				{ICE_OUT, "doc_public"        ,  NULL     }, //6
				{ICE_OUT, "doc_pre_cache"     ,  NULL     }, //7
				{ICE_OUT, "doc_perm_cache"    ,  NULL     }, //8
				{ICE_OUT, "doc_session_cache" ,  NULL     }, //9
				{ICE_OUT, "doc_file"          ,  NULL     }, //10
				{ICE_OUT, "doc_ext_loc"       ,  NULL     }, //11
				{ICE_OUT, "doc_ext_file"      ,  NULL     }, //12
				{ICE_OUT, "doc_ext_suffix"    ,  NULL     }, //13
				{ICE_OUT, "doc_owner"         ,  NULL     }, //14
				{ICE_OUT, "doc_transfer"      ,  NULL     }, //15
				{0      ,  NULL               ,  NULL     } 
			};
			if ((status = ICE_C_Execute (ICEclient, "unit_user", select)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
			{
			if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					for (i=0;i<16;i++) {
						pret[i]=ICE_C_GetAttribute (ICEclient, i+1);
						if (!pret[i])
							pret[i]="";
					}
					if (!x_stricmp(pret[2],lpBusUnitDoc->icebusunit.ID) &&
						!x_stricmp(pret[4],lpBusUnitDoc->name) &&
						!x_stricmp(pret[5],lpBusUnitDoc->suffix) &&
						(  (!x_stricmp(pret[1],"facet") &&iobjecttype==OT_ICE_BUNIT_FACET)
						 ||(!x_stricmp(pret[1],"page")  &&iobjecttype==OT_ICE_BUNIT_PAGE)
						)
					   ) {
						fstrncpy(lpBusUnitDoc->doc_ID,pret[0],sizeof(lpBusUnitDoc->doc_ID));
						lpBusUnitDoc->bPublic        = !x_stricmp(pret[6],"checked")?FALSE:TRUE;
						lpBusUnitDoc->bpre_cache     = !x_stricmp(pret[7],"checked")?FALSE:TRUE;
						lpBusUnitDoc->bperm_cache    = !x_stricmp(pret[8],"checked")?FALSE:TRUE;
						lpBusUnitDoc->bsession_cache = !x_stricmp(pret[9],"checked")?FALSE:TRUE;
						fstrncpy(lpBusUnitDoc->doc_file  ,pret[10],sizeof(lpBusUnitDoc->doc_file));
						fstrncpy(lpBusUnitDoc->ext_loc.ID,pret[11],sizeof(lpBusUnitDoc->ext_loc.ID));
						fstrncpy(lpBusUnitDoc->ext_file  ,pret[12],sizeof(lpBusUnitDoc->ext_file));
						fstrncpy(lpBusUnitDoc->ext_suffix,pret[13],sizeof(lpBusUnitDoc->ext_suffix));
						fstrncpy(lpBusUnitDoc->doc_owner ,pret[14],sizeof(lpBusUnitDoc->doc_owner));
						break;
					}
				}
			}
			while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_BUNIT_LOCATION:
			// the 2 substructures are already filled (begin of this function)
			//. Nothning else to do.
		break;
		default:
			//"unknown ICE type"
			TS_MessageBox(TS_GetFocus(),ResourceString(IDS_ERR_UNKNOWN_ICE_TYPE),
				NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
			iresult = RES_ERR;

	}
	if ((status =ICE_C_Disconnect (&ICEclient))!=OK) {
		ManageIceError(&ICEclient, &status);
		iresult=RES_ERR;
	}
	RemoveHourGlass();
	if (iresult!=RES_SUCCESS)
		return iresult;

	switch (iobjecttype) {
		case OT_ICE_DBCONNECTION:
		{
			LPDBCONNECTIONDATA lpDBConnectionData = (LPDBCONNECTIONDATA) lpparameters;
			iresult = GetICEInfoFromID(lpVirtNode,OT_ICE_DBUSER, &(lpDBConnectionData->DBUsr));
			break;
		}
		case OT_ICE_WEBUSER:
		{
			LPICEWEBUSERDATA lpWebUserData = (LPICEWEBUSERDATA) lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_DBCONNECTION, &(lpWebUserData->DefDBUsr));
			break;
		}
		case OT_ICE_PROFILE:
		{
			LPICEPROFILEDATA lpIceProfileData = (LPICEPROFILEDATA) lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_DBUSER, &(lpIceProfileData->DefDBUsr));
			break;
		}
		case OT_ICE_BUNIT_FACET:
		case OT_ICE_BUNIT_PAGE:
		{
			LPICEBUSUNITDOCDATA lpIceData = (LPICEBUSUNITDOCDATA)lpparameters;
			if (x_strlen(lpIceData->ext_loc.ID)>0) {
				iresult = GetICEInfo(lpVirtNode,OT_ICE_SERVER_LOCATION, &(lpIceData->ext_loc));
				if (iresult !=RES_SUCCESS)
					return iresult;
			}
			else 
				x_strcpy(lpIceData->ext_loc.LocationName,"");
			break;
		}


	}

	return iresult;
#else
	return RES_SUCCESS;
#endif
}


int GetICEInfo(LPUCHAR lpVirtNode,int iobjecttype, void *lpparameters)
{
#ifndef NOUSEICEAPI
	TCHAR			bufuser[100],bufpw[100];
	ICE_STATUS		status = 0;
	ICE_C_CLIENT	ICEclient = NULL;
	LPUCHAR pret[20];
	int iresult = RES_SUCCESS;
	int end,i;
	if (!lpparameters) {
		myerror(ERR_INVPARAMS);
		return RES_ERR;
	}

	// first step: when there is a parenthood, get ID of the parent object by completing 
	// the parent struture
	switch (iobjecttype) {
		case OT_ICE_BUNIT_FACET_ROLE:
		{
			LPICEBUSUNITDOCROLEDATA lpIceData = (LPICEBUSUNITDOCROLEDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_BUNIT_FACET, &(lpIceData->icebusunitdoc));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_ROLE, &(lpIceData->icebusunitdocrole));
			break;
		}
		case OT_ICE_BUNIT_PAGE_ROLE:
		{
			LPICEBUSUNITDOCROLEDATA lpIceData = (LPICEBUSUNITDOCROLEDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_BUNIT_PAGE, &(lpIceData->icebusunitdoc));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_ROLE, &(lpIceData->icebusunitdocrole));
			break;
		}
		case OT_ICE_BUNIT_FACET_USER:
		{
			LPICEBUSUNITDOCUSERDATA lpIceData = (LPICEBUSUNITDOCUSERDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_BUNIT_FACET, &(lpIceData->icebusunitdoc));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_WEBUSER, &(lpIceData->user));
			break;
		}
		case OT_ICE_BUNIT_PAGE_USER:
		{
			LPICEBUSUNITDOCUSERDATA lpIceData = (LPICEBUSUNITDOCUSERDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_BUNIT_PAGE, &(lpIceData->icebusunitdoc));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_WEBUSER, &(lpIceData->user));
			break;
		}
		case OT_ICE_BUNIT_SEC_ROLE:
		{
			LPICEBUSUNITROLEDATA lpIceData = (LPICEBUSUNITROLEDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_BUNIT, &(lpIceData->icebusunit));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_ROLE, &(lpIceData->icerole));
			break;
		}
		case OT_ICE_BUNIT_SEC_USER:
		{
			LPICEBUSUNITWEBUSERDATA lpIceData = (LPICEBUSUNITWEBUSERDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_BUNIT, &(lpIceData->icebusunit));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_WEBUSER, &(lpIceData->icewevuser));
			break;
		}
		case OT_ICE_BUNIT_FACET:
		case OT_ICE_BUNIT_PAGE:
		{
			LPICEBUSUNITDOCDATA lpIceData = (LPICEBUSUNITDOCDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_BUNIT, &(lpIceData->icebusunit));
			break;
		}
		case OT_ICE_BUNIT_LOCATION:
		{
			LPICEBUSUNITLOCATIONDATA lpIceData = (LPICEBUSUNITLOCATIONDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_BUNIT, &(lpIceData->icebusunit));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_SERVER_LOCATION, &(lpIceData->icelocation));
			break;
		}
		case OT_ICE_WEBUSER_ROLE:
		{
			LPICEWEBUSERROLEDATA lpIceData = (LPICEWEBUSERROLEDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_WEBUSER, &(lpIceData->icewebuser));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_ROLE, &(lpIceData->icerole));
			return iresult;  // used only to get the IDs from the obj names
			break;
		}
		case OT_ICE_WEBUSER_CONNECTION:
		{
			LPICEWEBUSERCONNECTIONDATA lpIceData = (LPICEWEBUSERCONNECTIONDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_WEBUSER, &(lpIceData->icewebuser));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_DBCONNECTION, &(lpIceData->icedbconnection));
			return iresult;  // used only to get the IDs from the obj names
			break;
		}
		case OT_ICE_PROFILE_ROLE:
		{
			LPICEPROFILEROLEDATA lpIceData = (LPICEPROFILEROLEDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_PROFILE, &(lpIceData->iceprofile));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_ROLE, &(lpIceData->icerole));
			return iresult;  // used only to get the IDs from the obj names
			break;
		}
		case OT_ICE_PROFILE_CONNECTION:
		{
			LPICEPROFILECONNECTIONDATA lpIceData = (LPICEPROFILECONNECTIONDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_PROFILE, &(lpIceData->iceprofile));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_DBCONNECTION, &(lpIceData->icedbconnection));
			return iresult;  // used only to get the IDs from the obj names
			break;
		}
	}
	if (iresult !=RES_SUCCESS)
		return iresult;

	if (GetICeUserAndPassWord(GetVirtNodeHandle(lpVirtNode),bufuser,bufpw)!=RES_SUCCESS)
		return RES_ERR;
	if ((status =ICE_C_Initialize ()) != OK) {
		ManageIceError(&ICEclient, &status);
		return RES_ERR;
	}
	if ((status = ICE_C_Connect (CAPINodeName(lpVirtNode), bufuser,bufpw, &ICEclient)) != OK) {
		ManageIceError(&ICEclient, &status);
		return RES_ERR;
	}
	ShowHourGlass(); 
	switch (iobjecttype) {
		case OT_ICE_DBUSER:
		{
			LPICEDBUSERDATA lpIceData = (LPICEDBUSERDATA)lpparameters;
			ICE_C_PARAMS select[] =
			{
				{ICE_IN , "action"        ,  "select" },
				{ICE_OUT, "dbuser_id"     ,  NULL     },
				{ICE_OUT, "dbuser_alias"  ,  NULL     },
				{ICE_OUT, "dbuser_name"   ,  NULL     },
				{ICE_OUT, "dbuser_comment",  NULL     },
				{0      , NULL            ,  NULL     }
			};
			if (!x_strlen(lpIceData->UserAlias)) {
				iresult=RES_ERR;
				break;
			}
			if ((status = ICE_C_Execute (ICEclient, "dbuser", select)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
			{
				if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					for (i=0;i<4;i++) {
						pret[i]=ICE_C_GetAttribute (ICEclient, i+1);
						if (!pret[i])
							pret[i]="";
					}
					if (!x_stricmp(pret[1], lpIceData->UserAlias)) {
						fstrncpy(lpIceData->ID       ,pret[0],sizeof(lpIceData->ID));
						fstrncpy(lpIceData->User_Name,pret[2],sizeof(lpIceData->User_Name));
						fstrncpy(lpIceData->Comment  ,pret[3],sizeof(lpIceData->Comment));
						break;
					}
                }
            }
            while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_DBCONNECTION:
		{
			LPDBCONNECTIONDATA lpDBConnectionData = (LPDBCONNECTIONDATA) lpparameters;
			ICE_C_PARAMS select[] =
			{
				{ICE_IN , "action"    ,  "select" },
				{ICE_OUT, "db_id"     ,  NULL     },
				{ICE_OUT, "db_name"   ,  NULL     },
				{ICE_OUT, "db_dbname" ,  NULL     },
				{ICE_OUT, "db_dbuser" ,  NULL     },
				{ICE_OUT, "db_comment",  NULL     },
				{0      , NULL        ,  NULL     }
			};
			if (!x_strlen(lpDBConnectionData->ConnectionName)) {
				iresult=RES_ERR;
				break;
			}
			if ((status = ICE_C_Execute (ICEclient, "database", select)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
			{
				if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					if (!x_stricmp(ICE_C_GetAttribute (ICEclient, 2), lpDBConnectionData->ConnectionName)) {
						fstrncpy(lpDBConnectionData->ID       ,ICE_C_GetAttribute (ICEclient, 1),sizeof(lpDBConnectionData->ID));
						fstrncpy(lpDBConnectionData->DBName   ,ICE_C_GetAttribute (ICEclient, 3),sizeof(lpDBConnectionData->DBName));
						fstrncpy(lpDBConnectionData->DBUsr.ID ,ICE_C_GetAttribute (ICEclient, 4),sizeof(lpDBConnectionData->DBUsr.ID));
						fstrncpy(lpDBConnectionData->Comment  ,ICE_C_GetAttribute (ICEclient, 5),sizeof(lpDBConnectionData->Comment));
						break;
					}
				}
			}
			while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_WEBUSER:
		{
			LPICEWEBUSERDATA lpWebUserData = (LPICEWEBUSERDATA) lpparameters;
			ICE_C_PARAMS select[] =
			{
				{ICE_IN , "action"             ,  "select" },
				{ICE_OUT, "user_id"            ,  NULL     }, //1
				{ICE_OUT, "user_name"          ,  NULL     }, //2
				{ICE_OUT, "user_dbuser"        ,  NULL     }, //3
				{ICE_OUT, "user_comment"       ,  NULL     }, //4
				{ICE_OUT, "user_administration",  NULL     }, //5
				{ICE_OUT, "user_security"      ,  NULL     }, //6
				{ICE_OUT, "user_unit"          ,  NULL     }, //7
				{ICE_OUT, "user_monitor"       ,  NULL     }, //8
				{ICE_OUT, "user_timeout"       ,  NULL     }, //9
				{ICE_OUT, "user_authtype"      ,  NULL     }, //10
				{0      , NULL                 ,  NULL     }
			};
			if (!x_strlen(lpWebUserData->UserName)) {
				iresult=RES_ERR;
				break;
			}
			if ((status = ICE_C_Execute (ICEclient, "user", select)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
            {
               if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					if (!x_stricmp(ICE_C_GetAttribute (ICEclient, 2), lpWebUserData->UserName)) {
						fstrncpy(lpWebUserData->ID       ,ICE_C_GetAttribute (ICEclient,1),sizeof(lpWebUserData->ID));
						fstrncpy(lpWebUserData->DefDBUsr.ID,ICE_C_GetAttribute (ICEclient, 3),sizeof(lpWebUserData->DefDBUsr.ID));
						fstrncpy(lpWebUserData->Comment  ,ICE_C_GetAttribute (ICEclient, 4),sizeof(lpWebUserData->Comment));
						lpWebUserData->bAdminPriv    = !x_stricmp(ICE_C_GetAttribute (ICEclient, 5),"checked")?TRUE:FALSE;
						lpWebUserData->bSecurityPriv = !x_stricmp(ICE_C_GetAttribute (ICEclient, 6),"checked")?TRUE:FALSE;
						lpWebUserData->bUnitMgrPriv  = !x_stricmp(ICE_C_GetAttribute (ICEclient, 7),"checked")?TRUE:FALSE;
						lpWebUserData->bMonitorPriv  = !x_stricmp(ICE_C_GetAttribute (ICEclient, 8),"checked")?TRUE:FALSE;
						lpWebUserData->ltimeoutms    = atol(ICE_C_GetAttribute (ICEclient, 9));
						if (!x_stricmp(ICE_C_GetAttribute (ICEclient, 10),"ICE"))
							lpWebUserData->bICEpwd = TRUE;
						else
							lpWebUserData->bICEpwd = FALSE;
						break;
					}
				}
			}
			while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_PROFILE:
		{
			LPICEPROFILEDATA lpIceProfileData = (LPICEPROFILEDATA) lpparameters;
			ICE_C_PARAMS select[] =
			{
				{ICE_IN , "action"                 ,  "select" },
				{ICE_OUT, "profile_id"             ,  NULL     },
				{ICE_OUT, "profile_name"           ,  NULL     },
				{ICE_OUT, "profile_dbuser"         ,  NULL     },
//				{ICE_OUT, "profile_comment"        ,  NULL     },
				{ICE_OUT, "profile_administration" ,  NULL     },
				{ICE_OUT, "profile_security"       ,  NULL     },
				{ICE_OUT, "profile_unit"           ,  NULL     },
				{ICE_OUT, "profile_monitor"        ,  NULL     },
				{ICE_OUT, "profile_timeout"        ,  NULL     },
				{0      , NULL                     ,  NULL     }
			};
			LPUCHAR pret[20];
			if (!x_strlen(lpIceProfileData->ProfileName)) {
				iresult=RES_ERR;
				break;
			}
			if ((status = ICE_C_Execute (ICEclient, "profile", select)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
			{
				if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					for (i=0;i<8;i++) {
						pret[i]=ICE_C_GetAttribute (ICEclient, i+1);
						if (!pret[i])
							pret[i]="";
					}
					if (!x_stricmp(pret[1], lpIceProfileData->ProfileName)) {
						fstrncpy(lpIceProfileData->ID        ,pret[0],sizeof(lpIceProfileData->ID));
						fstrncpy(lpIceProfileData->DefDBUsr.ID ,pret[2],sizeof(lpIceProfileData->DefDBUsr.ID));
						fstrncpy(lpIceProfileData->Comment   ,"",sizeof(lpIceProfileData->Comment));
						lpIceProfileData->bAdminPriv    = !x_stricmp(pret[3],"checked")?TRUE:FALSE;
						lpIceProfileData->bSecurityPriv = !x_stricmp(pret[4],"checked")?TRUE:FALSE;
						lpIceProfileData->bUnitMgrPriv  = !x_stricmp(pret[5],"checked")?TRUE:FALSE;
						lpIceProfileData->bMonitorPriv  = !x_stricmp(pret[6],"checked")?TRUE:FALSE;
						lpIceProfileData->ltimeoutms    = atol(pret[7]);
						break;
					}
				}
			}
			while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_ROLE:
		{
			LPICEROLEDATA lpIceRoleData = (LPICEROLEDATA) lpparameters;
			ICE_C_PARAMS select[] =
			{
				{ICE_IN , "action"       ,  "select" },
				{ICE_OUT, "role_id"      ,  NULL     },
				{ICE_OUT, "role_name"    ,  NULL     },
				{ICE_OUT, "role_comment" ,  NULL     },
				{0      , NULL           ,  NULL     }
			};
			if (!x_strlen(lpIceRoleData->RoleName)) {
				iresult=RES_ERR;
				break;
			}
			if ((status = ICE_C_Execute (ICEclient, "role", select)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					if (!x_stricmp(ICE_C_GetAttribute (ICEclient, 2), lpIceRoleData->RoleName)) {
						fstrncpy(lpIceRoleData->ID,ICE_C_GetAttribute (ICEclient, 1),sizeof(lpIceRoleData->ID));
						fstrncpy(lpIceRoleData->Comment,ICE_C_GetAttribute (ICEclient, 3),sizeof(lpIceRoleData->Comment));
						break;
					}
                }
            }
            while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_BUNIT:
		{
			LPICEBUSUNITDATA lpBusUnitData = (LPICEBUSUNITDATA) lpparameters;
			ICE_C_PARAMS select[] =
			{
				{ICE_IN , "action"       ,  "select" },
				{ICE_OUT, "unit_id"      ,  NULL     },
				{ICE_OUT, "unit_name"    ,  NULL     },
				{ICE_OUT, "unit_owner"    ,  NULL     },
				{0      , NULL           ,  NULL     }
			};
			if (!x_strlen(lpBusUnitData->Name)){
				iresult=RES_ERR;
				break;
			}
			if ((status = ICE_C_Execute (ICEclient, "unit", select)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
			{
			if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					for (i=0;i<3;i++) {
						pret[i]=ICE_C_GetAttribute (ICEclient, i+1);
						if (!pret[i])
							pret[i]="";
					}
					if (!x_stricmp(pret[1], lpBusUnitData->Name)) {
						fstrncpy(lpBusUnitData->ID   ,pret[0],sizeof(lpBusUnitData->ID));
						fstrncpy(lpBusUnitData->Owner,pret[2],sizeof(lpBusUnitData->Owner));
						break;
					}
				}
			}
			while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_BUNIT_FACET_ROLE:
		case OT_ICE_BUNIT_PAGE_ROLE:
		{
			LPICEBUSUNITDOCROLEDATA lpIceData = (LPICEBUSUNITDOCROLEDATA)lpparameters;
			ICE_C_PARAMS retrieve[10];
			FillIceParamsArray(retrieve,
				ICE_IN        , "action"     , "retrieve" ,
				ICE_IN|ICE_OUT, "dr_doc_id"  , lpIceData->icebusunitdoc.doc_ID,// 0 
				ICE_OUT       , "dr_role_id" , NULL     , //1
				ICE_OUT       , "dr_execute" , NULL     ,
				ICE_OUT       , "dr_read"    , NULL     ,
				ICE_OUT       , "dr_update"  , NULL     ,
				ICE_OUT       , "dr_delete"  , NULL     ,
				0             ,  NULL        , NULL     
			);
			if ((status = ICE_C_Execute (ICEclient, "document_role", retrieve)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
			{
			if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					for (i=1;i<6;i++) {
						pret[i]=ICE_C_GetAttribute (ICEclient, i+1);
						if (!pret[i])
							pret[i]="";
					}
					if (!x_stricmp(pret[1],lpIceData->icebusunitdocrole.ID)) {
						lpIceData->bExec    = !x_stricmp(pret[2],"checked")?TRUE:FALSE;
						lpIceData->bRead    = !x_stricmp(pret[3],"checked")?TRUE:FALSE;
						lpIceData->bUpdate  = !x_stricmp(pret[4],"checked")?TRUE:FALSE;
						lpIceData->bDelete  = !x_stricmp(pret[5],"checked")?TRUE:FALSE;
						break;
					}
				}
			}
			while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_BUNIT_FACET_USER:
		case OT_ICE_BUNIT_PAGE_USER:
		{
			LPICEBUSUNITDOCUSERDATA lpIceData = (LPICEBUSUNITDOCUSERDATA)lpparameters;
			ICE_C_PARAMS retrieve[10];
			FillIceParamsArray(retrieve,
				ICE_IN        , "action"     , "retrieve" ,
				ICE_IN|ICE_OUT, "du_doc_id"  , lpIceData->icebusunitdoc.doc_ID,// 0 
				ICE_OUT       , "du_user_id" , NULL     , //1
				ICE_OUT       , "du_execute" , NULL     ,
				ICE_OUT       , "du_read"    , NULL     ,
				ICE_OUT       , "du_update"  , NULL     ,
				ICE_OUT       , "du_delete"  , NULL     ,
				0             ,  NULL        , NULL     
			);
			if ((status = ICE_C_Execute (ICEclient, "document_user", retrieve)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
			{
			if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					for (i=1;i<6;i++) {
						pret[i]=ICE_C_GetAttribute (ICEclient, i+1);
						if (!pret[i])
							pret[i]="";
					}
					if (!x_stricmp(pret[1],lpIceData->user.ID)) {
						lpIceData->bExec    = !x_stricmp(pret[2],"checked")?TRUE:FALSE;
						lpIceData->bRead    = !x_stricmp(pret[3],"checked")?TRUE:FALSE;
						lpIceData->bUpdate  = !x_stricmp(pret[4],"checked")?TRUE:FALSE;
						lpIceData->bDelete  = !x_stricmp(pret[5],"checked")?TRUE:FALSE;
						break;
					}
				}
			}
			while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_SERVER_VARIABLE:
		{
			LPICESERVERVARDATA lpIceData = (LPICESERVERVARDATA)lpparameters;
			ICE_C_PARAMS select[] =
			{
				{ICE_OUT|ICE_IN, "page"     ,  ""        },
				{ICE_OUT|ICE_IN, "session"  ,  ""        },
				{ICE_OUT|ICE_IN, "server"   ,  "checked" },
				{ICE_OUT, "name"            ,  NULL      },
				{ICE_OUT, "value"           ,  NULL      },
				{0      , NULL              ,  NULL      }
			};
			if (!x_strlen(lpIceData->VariableName)) {
				iresult=RES_ERR;
				break;
			}
			if ((status = ICE_C_Execute (ICEclient, "getVariables", select)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
			{
				if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					for (i=0;i<4;i++) {
						pret[i]=ICE_C_GetAttribute (ICEclient, i+1);
						if (!pret[i])
							pret[i]="";
					}
					if (!x_stricmp(pret[2], lpIceData->VariableName)) {
						fstrncpy(lpIceData->VariableValue  ,pret[3],sizeof(lpIceData->VariableValue));
						break;
					}
				}
			}
			while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_SERVER_APPLICATION:
		{
			LPICEAPPLICATIONDATA lpAppData = (LPICEAPPLICATIONDATA) lpparameters;
			ICE_C_PARAMS select[] =
			{
				{ICE_IN , "action"       ,  "select" },
				{ICE_OUT, "sess_name"    ,  NULL     },
				{ICE_OUT, "sess_id"      ,  NULL     },
				{0      , NULL           ,  NULL     }
			};
			if (!x_strlen(lpAppData->Name)) {
				iresult=RES_ERR;
				break;
			} 
			if ((status = ICE_C_Execute (ICEclient, "session_grp", select)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
			{
			if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					if (!x_stricmp(ICE_C_GetAttribute (ICEclient, 1), lpAppData->Name)) {
						fstrncpy(lpAppData->ID,ICE_C_GetAttribute (ICEclient, 2),sizeof(lpAppData->ID));
						break;
					}
				}
			}
			while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_SERVER_LOCATION:
		{
			LPICELOCATIONDATA lpLocData = (LPICELOCATIONDATA) lpparameters;
			ICE_C_PARAMS select[] =
			{
				{ICE_IN , "action"        ,  "select" },
				{ICE_OUT, "loc_id"        ,  NULL     },
				{ICE_OUT, "loc_name"      ,  NULL     },
				{ICE_OUT, "loc_path"      ,  NULL     },
				{ICE_OUT, "loc_extensions",  NULL     },
				{ICE_OUT, "loc_http"      ,  NULL     },
				{ICE_OUT, "loc_ice"       ,  NULL     },
				{ICE_OUT, "loc_public"    ,  NULL     },
				{0      ,  NULL           ,  NULL     }
			};
			if (!x_strlen(lpLocData->LocationName)) {
				iresult=RES_ERR;
				break;
			}
			if ((status = ICE_C_Execute (ICEclient, "ice_locations", select)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
			{
			if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					for (i=0;i<7;i++) {
						pret[i]=ICE_C_GetAttribute (ICEclient, i+1);
						if (!pret[i])
							pret[i]="";
					}
					if (!x_stricmp(pret[1], lpLocData->LocationName)) {
						fstrncpy(lpLocData->ID        ,pret[0],sizeof(lpLocData->ID));
						fstrncpy(lpLocData->path      ,pret[2],sizeof(lpLocData->path));
						fstrncpy(lpLocData->extensions,pret[3],sizeof(lpLocData->extensions));
						lpLocData->bIce    = !x_stricmp(pret[4],"checked")?FALSE:TRUE;
						lpLocData->bPublic = !x_stricmp(pret[6],"checked")?TRUE:FALSE;
						break;
					}
				}
			}
			while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_BUNIT_SEC_ROLE:
		{
			LPICEBUSUNITROLEDATA lpBusUnitRole = (LPICEBUSUNITROLEDATA) lpparameters;
			ICE_C_PARAMS select[10];
			FillIceParamsArray( select,
				ICE_IN , "action"       ,  "retrieve" ,
				ICE_IN|ICE_OUT,  "ur_unit_id"   ,  lpBusUnitRole->icebusunit.ID     ,
				ICE_OUT, "ur_role_id"   ,  NULL     ,
				ICE_OUT, "ur_role_name" ,  NULL     ,
				ICE_OUT, "ur_execute"   ,  NULL     ,
				ICE_OUT, "ur_read"      ,  NULL     ,
				ICE_OUT, "ur_insert"    ,  NULL     ,
				0      ,  NULL          ,  NULL     
			);
			if ((status = ICE_C_Execute (ICEclient, "unit_role", select)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
			{
			if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					for (i=1;i<6;i++) {
						pret[i]=ICE_C_GetAttribute (ICEclient, i+1);
						if (!pret[i])
							pret[i]="";
					}
					if (!x_stricmp(pret[1],lpBusUnitRole->icerole.ID)) {
						fstrncpy(lpBusUnitRole->icerole.RoleName,pret[2],sizeof(lpBusUnitRole->icerole.RoleName));
						lpBusUnitRole->bExecDoc   = !x_stricmp(pret[3],"checked")?TRUE:FALSE;
						lpBusUnitRole->bReadDoc   = !x_stricmp(pret[4],"checked")?TRUE:FALSE;
						lpBusUnitRole->bCreateDoc = !x_stricmp(pret[5],"checked")?TRUE:FALSE;
						break;
					}
				}
			}
			while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_BUNIT_SEC_USER:
		{
			LPICEBUSUNITWEBUSERDATA lpBusUnitUser = (LPICEBUSUNITWEBUSERDATA) lpparameters;
			ICE_C_PARAMS select[10];
			FillIceParamsArray( select,
				ICE_IN , "action"       ,  "retrieve" ,
				ICE_IN|ICE_OUT,  "uu_unit_id"   ,  lpBusUnitUser->icebusunit.ID     ,
				ICE_OUT, "uu_user_id"   ,  NULL     ,
				ICE_OUT, "uu_user_name" ,  NULL     ,
				ICE_OUT, "uu_execute"   ,  NULL     ,
				ICE_OUT, "uu_read"      ,  NULL     ,
				ICE_OUT, "uu_insert"    ,  NULL     ,
				0      ,  NULL          ,  NULL     
			);

			if ((status = ICE_C_Execute (ICEclient, "unit_user", select)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
			{
			if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					for (i=1;i<6;i++) {
						pret[i]=ICE_C_GetAttribute (ICEclient, i+1);
						if (!pret[i])
							pret[i]="";
					}
					if (!x_stricmp(pret[1],lpBusUnitUser->icewevuser.ID)) {
						fstrncpy(lpBusUnitUser->icewevuser.UserName,pret[2],sizeof(lpBusUnitUser->icewevuser.UserName));
						lpBusUnitUser->bUnitRead  = !x_stricmp(pret[3],"checked")?TRUE:FALSE;
						lpBusUnitUser->bReadDoc   = !x_stricmp(pret[4],"checked") ?TRUE:FALSE;
						lpBusUnitUser->bCreateDoc = !x_stricmp(pret[5],"checked") ?TRUE:FALSE;
						break;
					}
				}
			}
			while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_BUNIT_FACET:
		case OT_ICE_BUNIT_PAGE:
		{
			LPICEBUSUNITDOCDATA lpBusUnitDoc = (LPICEBUSUNITDOCDATA) lpparameters;
			ICE_C_PARAMS select[] =
			{
				{ICE_IN , "action"            ,  "select" },
				{ICE_OUT,  "doc_id"           ,  NULL     }, //0
				{ICE_OUT, "doc_type"          ,  NULL     }, //1
				{ICE_OUT, "doc_unit_id"       ,  NULL     }, //2
				{ICE_OUT, "doc_unit_name"     ,  NULL     }, //3
				{ICE_OUT, "doc_name"          ,  NULL     }, //4
				{ICE_OUT, "doc_suffix"        ,  NULL     }, //5
				{ICE_OUT, "doc_public"        ,  NULL     }, //6
				{ICE_OUT, "doc_pre_cache"     ,  NULL     }, //7
				{ICE_OUT, "doc_perm_cache"    ,  NULL     }, //8
				{ICE_OUT, "doc_session_cache" ,  NULL     }, //9
				{ICE_OUT, "doc_file"          ,  NULL     }, //10
				{ICE_OUT, "doc_ext_loc"       ,  NULL     }, //11
				{ICE_OUT, "doc_ext_file"      ,  NULL     }, //12
				{ICE_OUT, "doc_ext_suffix"    ,  NULL     }, //13
				{ICE_OUT, "doc_owner"         ,  NULL     }, //14
//				{ICE_OUT, "doc_transfer"      ,  NULL     }, //15
				{0      ,  NULL               ,  NULL     } 
			};
			if ((status = ICE_C_Execute (ICEclient, "document", select)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
				break;
			}
			do
			{
			if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end)) {
					char name1[100];
					char suffix1[100];
					char * pc1;
					x_strcpy(name1,lpBusUnitDoc->name);
					pc1 = x_strchr(name1,'.');
					if (pc1!=NULL && x_strlen(lpBusUnitDoc->suffix)==0) {
						x_strcpy(suffix1, pc1+1);
						*pc1='\0';
					}
					else {
						x_strcpy(suffix1,lpBusUnitDoc->suffix);
					}

					for (i=0;i<15;i++) {
						pret[i]=ICE_C_GetAttribute (ICEclient, i+1);
						if (!pret[i] || !x_stricmp(pret[i],"<NULL>"))
							pret[i]="";
					}
					if (!x_stricmp(pret[2],lpBusUnitDoc->icebusunit.ID) &&
						!x_stricmp(pret[4],name1) &&
						!x_stricmp(pret[5],suffix1) &&
						(  (!x_stricmp(pret[1],"facet") &&iobjecttype==OT_ICE_BUNIT_FACET)
						 ||(!x_stricmp(pret[1],"page")  &&iobjecttype==OT_ICE_BUNIT_PAGE)
						)
					   ) {
						fstrncpy(lpBusUnitDoc->doc_ID,pret[0],sizeof(lpBusUnitDoc->doc_ID));
						lpBusUnitDoc->bPublic        = !x_stricmp(pret[6],"checked")?TRUE:FALSE;
						lpBusUnitDoc->bpre_cache     = !x_stricmp(pret[7],"checked")?TRUE:FALSE;
						lpBusUnitDoc->bperm_cache    = !x_stricmp(pret[8],"checked")?TRUE:FALSE;
						lpBusUnitDoc->bsession_cache = !x_stricmp(pret[9],"checked")?TRUE:FALSE;
						fstrncpy(lpBusUnitDoc->doc_file  ,pret[10],sizeof(lpBusUnitDoc->doc_file));
						fstrncpy(lpBusUnitDoc->ext_loc.ID,pret[11],sizeof(lpBusUnitDoc->ext_loc.ID));
						fstrncpy(lpBusUnitDoc->ext_file  ,pret[12],sizeof(lpBusUnitDoc->ext_file));
						fstrncpy(lpBusUnitDoc->ext_suffix,pret[13],sizeof(lpBusUnitDoc->ext_suffix));
						fstrncpy(lpBusUnitDoc->doc_owner ,pret[14],sizeof(lpBusUnitDoc->doc_owner));
						break;
					}
				}
			}
			while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			if (status!=OK || end)
				iresult = RES_ERR;
		}
		break;
		case OT_ICE_BUNIT_LOCATION:
			// the 2 substructures are already filled (begin of this function)
			//. Nothning else to do.
		break;
		default:
			//"unknown ICE type"
			TS_MessageBox(TS_GetFocus(),
				ResourceString(IDS_ERR_UNKNOWN_ICE_TYPE),
				NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
			iresult = RES_ERR;
	}
	if ((status =ICE_C_Disconnect (&ICEclient))!=OK) {
		ManageIceError(&ICEclient, &status);
		iresult=RES_ERR;
	}
	RemoveHourGlass();
	if (iresult!=RES_SUCCESS)
		return iresult;

	switch (iobjecttype) {
		case OT_ICE_DBCONNECTION:
		{
			LPDBCONNECTIONDATA lpDBConnectionData = (LPDBCONNECTIONDATA) lpparameters;
			iresult = GetICEInfoFromID(lpVirtNode,OT_ICE_DBUSER, &(lpDBConnectionData->DBUsr));
			break;
		}
		case OT_ICE_WEBUSER:
		{
			LPICEWEBUSERDATA lpWebUserData = (LPICEWEBUSERDATA) lpparameters;
			iresult = GetICEInfoFromID(lpVirtNode,OT_ICE_DBUSER, &(lpWebUserData->DefDBUsr));
			break;
		}
		case OT_ICE_PROFILE:
		{
			LPICEPROFILEDATA lpIceProfileData = (LPICEPROFILEDATA) lpparameters;
			iresult = GetICEInfoFromID(lpVirtNode,OT_ICE_DBUSER, &(lpIceProfileData->DefDBUsr));
			break;
		}
		case OT_ICE_BUNIT_FACET:
		case OT_ICE_BUNIT_PAGE:
		{
			LPICEBUSUNITDOCDATA lpIceData = (LPICEBUSUNITDOCDATA)lpparameters;
			if (x_strlen(lpIceData->ext_loc.ID)>0) {
				iresult = GetICEInfoFromID(lpVirtNode,OT_ICE_SERVER_LOCATION, &(lpIceData->ext_loc));
				if (iresult !=RES_SUCCESS)
					return iresult;
			}
			else 
				x_strcpy(lpIceData->ext_loc.LocationName,"");
			break;
		}


	}

	return iresult;
#else
	return RES_SUCCESS;
#endif
}

int ICEAddobject(LPUCHAR lpVirtNode,int iobjecttype, void *lpparameters)
{
#ifndef NOUSEICEAPI
	TCHAR			bufuser[100],bufpw[100];
	ICE_STATUS		status = 0;
	ICE_C_CLIENT	ICEclient = NULL;
	int iresult = RES_SUCCESS;
	if (!lpparameters || !lpVirtNode) {
		myerror(ERR_INVPARAMS);
		return RES_ERR;
	}

	// first step: when there is a parenthood, get ID of the parent object by completing 
	// the parent struture
	switch (iobjecttype) {
		case OT_ICE_BUNIT_FACET_ROLE:
		{
			LPICEBUSUNITDOCROLEDATA lpIceData = (LPICEBUSUNITDOCROLEDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_BUNIT_FACET, &(lpIceData->icebusunitdoc));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_ROLE, &(lpIceData->icebusunitdocrole));
			break;
		}
		case OT_ICE_BUNIT_PAGE_ROLE:
		{
			LPICEBUSUNITDOCROLEDATA lpIceData = (LPICEBUSUNITDOCROLEDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_BUNIT_PAGE, &(lpIceData->icebusunitdoc));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_ROLE, &(lpIceData->icebusunitdocrole));
			break;
		}
		case OT_ICE_BUNIT_FACET_USER:
		{
			LPICEBUSUNITDOCUSERDATA lpIceData = (LPICEBUSUNITDOCUSERDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_BUNIT_FACET, &(lpIceData->icebusunitdoc));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_WEBUSER, &(lpIceData->user));
			break;
		}
		case OT_ICE_BUNIT_PAGE_USER:
		{
			LPICEBUSUNITDOCUSERDATA lpIceData = (LPICEBUSUNITDOCUSERDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_BUNIT_PAGE, &(lpIceData->icebusunitdoc));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_WEBUSER, &(lpIceData->user));
			break;
		}
		case OT_ICE_WEBUSER:
		{
			LPICEWEBUSERDATA lpWebUserData = (LPICEWEBUSERDATA) lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_DBUSER, &(lpWebUserData->DefDBUsr));
			break;
		}
		case OT_ICE_PROFILE:
		{
			LPICEPROFILEDATA lpIceProfileData = (LPICEPROFILEDATA) lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_DBUSER, &(lpIceProfileData->DefDBUsr));
			break;
		}
		case OT_ICE_DBCONNECTION:
		{
			LPDBCONNECTIONDATA lpDBConnectionData = (LPDBCONNECTIONDATA) lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_DBUSER, &(lpDBConnectionData->DBUsr));
			break;
		}
		case OT_ICE_WEBUSER_ROLE:
		{
			LPICEWEBUSERROLEDATA lpIceData = (LPICEWEBUSERROLEDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_WEBUSER, &(lpIceData->icewebuser));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_ROLE, &(lpIceData->icerole));
			break;
		}
		case OT_ICE_WEBUSER_CONNECTION:
		{
			LPICEWEBUSERCONNECTIONDATA lpIceData = (LPICEWEBUSERCONNECTIONDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_WEBUSER, &(lpIceData->icewebuser));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_DBCONNECTION, &(lpIceData->icedbconnection));
			break;
		}
		case OT_ICE_PROFILE_ROLE:
		{
			LPICEPROFILEROLEDATA lpIceData = (LPICEPROFILEROLEDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_PROFILE, &(lpIceData->iceprofile));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_ROLE, &(lpIceData->icerole));
			break;
		}
		case OT_ICE_PROFILE_CONNECTION:
		{
			LPICEPROFILECONNECTIONDATA lpIceData = (LPICEPROFILECONNECTIONDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_PROFILE, &(lpIceData->iceprofile));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_DBCONNECTION, &(lpIceData->icedbconnection));
			break;
		}
		case OT_ICE_BUNIT_SEC_ROLE:
		{
			LPICEBUSUNITROLEDATA lpIceData = (LPICEBUSUNITROLEDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_BUNIT, &(lpIceData->icebusunit));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_ROLE, &(lpIceData->icerole));
			break;
		}
		case OT_ICE_BUNIT_SEC_USER:
		{
			LPICEBUSUNITWEBUSERDATA lpIceData = (LPICEBUSUNITWEBUSERDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_BUNIT, &(lpIceData->icebusunit));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_WEBUSER, &(lpIceData->icewevuser));
			break;
		}
		case OT_ICE_BUNIT_FACET:
		case OT_ICE_BUNIT_PAGE:
		{
			LPICEBUSUNITDOCDATA lpIceData = (LPICEBUSUNITDOCDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_BUNIT, &(lpIceData->icebusunit));
			if (iresult !=RES_SUCCESS)
				return iresult;
			if (lpIceData->ext_loc.LocationName[0])
				iresult = GetICEInfo(lpVirtNode,OT_ICE_SERVER_LOCATION, &(lpIceData->ext_loc));
			break;
		}
		case OT_ICE_BUNIT_LOCATION:
		{
			LPICEBUSUNITLOCATIONDATA lpIceData = (LPICEBUSUNITLOCATIONDATA)lpparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_BUNIT, &(lpIceData->icebusunit));
			if (iresult !=RES_SUCCESS)
				return iresult;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_SERVER_LOCATION, &(lpIceData->icelocation));
			break;
		}
	}
	if (iresult !=RES_SUCCESS)
		return iresult;

	iresult = RES_SUCCESS;

	if (GetICeUserAndPassWord(GetVirtNodeHandle(lpVirtNode),bufuser,bufpw)!=RES_SUCCESS)
		return RES_ERR;
    if ((status =ICE_C_Initialize ()) != OK) {
		ManageIceError(&ICEclient, &status);
		return RES_ERR;
	}
    if ((status = ICE_C_Connect (CAPINodeName(lpVirtNode), bufuser,bufpw, &ICEclient)) != OK) {
		ManageIceError(&ICEclient, &status);
		return RES_ERR;
	}
	ShowHourGlass();
	switch (iobjecttype) {
		case OT_ICE_DBUSER:
		{
			LPICEDBUSERDATA lpIceData = (LPICEDBUSERDATA)lpparameters;
			ICE_C_PARAMS insert[10];
			FillIceParamsArray( insert,
				ICE_IN , "action"         ,  "insert" ,
//				ICE_IN, "dbuser_id"       ,  lpIceData->     ,
				ICE_IN, "dbuser_alias"    ,  lpIceData->UserAlias ,
				ICE_IN, "dbuser_name"     ,  lpIceData->User_Name ,
				ICE_IN, "dbuser_comment"  ,  lpIceData->Comment  ,
				ICE_IN, "dbuser_password1",  lpIceData->Password ,
				ICE_IN, "dbuser_password2",  lpIceData->Password ,
				0     , NULL              ,  NULL
			);
			if ((status = ICE_C_Execute (ICEclient, "dbuser", insert)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_WEBUSER_CONNECTION:
		{
			LPICEWEBUSERCONNECTIONDATA lpIceData = (LPICEWEBUSERCONNECTIONDATA)lpparameters;
			ICE_C_PARAMS insert[8];
			FillIceParamsArray( insert,
				ICE_IN , "action"   , "insert" ,
				ICE_IN, "ud_user_id" , lpIceData->icewebuser.ID      ,
				ICE_IN, "ud_db_id"   , lpIceData->icedbconnection.ID ,
				0      , NULL       ,  NULL 
			);
			if ((status = ICE_C_Execute (ICEclient, "user_database", insert)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_PROFILE_CONNECTION:
		{
			LPICEPROFILECONNECTIONDATA lpIceData = (LPICEPROFILECONNECTIONDATA)lpparameters;
			ICE_C_PARAMS insert[6];
			FillIceParamsArray( insert,
				ICE_IN , "action"   , "insert" ,
				ICE_IN, "pd_profile_id" , lpIceData->iceprofile.ID   ,
				ICE_IN, "pd_db_id"   , lpIceData->icedbconnection.ID ,
				0      , NULL       ,  NULL 
			);
			if ((status = ICE_C_Execute (ICEclient, "profile_database", insert)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_DBCONNECTION:
		{
			LPDBCONNECTIONDATA lpDBConnectionData = (LPDBCONNECTIONDATA) lpparameters;
			ICE_C_PARAMS insert[10];
			FillIceParamsArray( insert,
				ICE_IN , "action"   , "insert" ,
//				ICE_IN, "db_id"     , lpDBConnectionData->ID     ,
				ICE_IN, "db_name"   , lpDBConnectionData->ConnectionName     ,
				ICE_IN, "db_dbname" , lpDBConnectionData->DBName     ,
				ICE_IN, "db_dbuser" , lpDBConnectionData->DBUsr.ID     ,
				ICE_IN, "db_comment", lpDBConnectionData->Comment     ,
				0      , NULL       ,  NULL
			);
			if ((status = ICE_C_Execute (ICEclient, "database", insert)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_WEBUSER:
		{
			LPICEWEBUSERDATA lpWebUserData = (LPICEWEBUSERDATA) lpparameters;
			TCHAR buftimeout[100];
			TCHAR bufICEorOS[20];
			ICE_C_PARAMS insert[16];
			FillIceParamsArray( insert,
				ICE_IN , "action"   , "insert" ,
//				ICE_OUT, "user_id"            ,  NULL     ,
				ICE_IN, "user_name"          ,  lpWebUserData->UserName   ,
				ICE_IN, "user_password1"     ,  lpWebUserData->Password,
				ICE_IN, "user_password2"     ,  lpWebUserData->Password,
				ICE_IN, "user_dbuser"        ,  lpWebUserData->DefDBUsr.ID  ,
				ICE_IN, "user_comment"       ,  lpWebUserData->Comment    ,
				ICE_IN, "user_administration",  lpWebUserData->bAdminPriv   ? "checked":""  ,
				ICE_IN, "user_security"      ,  lpWebUserData->bSecurityPriv? "checked":""  ,
				ICE_IN, "user_unit"          ,  lpWebUserData->bUnitMgrPriv ? "checked":""  ,
				ICE_IN, "user_monitor"       ,  lpWebUserData->bMonitorPriv ? "checked":""  ,
				ICE_IN, "user_timeout"       ,  buftimeout     ,
				ICE_IN, "user_authtype"      ,  bufICEorOS     ,
				0      , NULL       ,  NULL
			);
			wsprintf(buftimeout,"%ld",lpWebUserData->ltimeoutms);
			if (lpWebUserData->bICEpwd) {
				x_strcpy(bufICEorOS,"ICE");
			}
			else {
				x_strcpy(lpWebUserData->Password,"");
				x_strcpy(bufICEorOS,"OS");
			}
			status = ICE_C_Execute (ICEclient, "user", insert);
			if (status != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_PROFILE:
		{
			LPICEPROFILEDATA lpIceProfileData = (LPICEPROFILEDATA) lpparameters;
			TCHAR buftimeout[100];
			ICE_C_PARAMS insert[12];
			FillIceParamsArray( insert,
				ICE_IN , "action"   , "insert" ,
//				ICE_OUT, "profile_id"            ,  NULL     ,
				ICE_IN, "profile_name"          ,  lpIceProfileData->ProfileName   ,
				ICE_IN, "profile_dbuser"        ,  lpIceProfileData->DefDBUsr.ID  ,
//				ICE_IN, "profile_comment"       ,  lpIceProfileData->Comment    ,
				ICE_IN, "profile_administration",  lpIceProfileData->bAdminPriv   ? "checked":""  ,
				ICE_IN, "profile_security"      ,  lpIceProfileData->bSecurityPriv? "checked":""  ,
				ICE_IN, "profile_unit"          ,  lpIceProfileData->bUnitMgrPriv ? "checked":""  ,
				ICE_IN, "profile_monitor"       ,  lpIceProfileData->bMonitorPriv ? "checked":""  ,
				ICE_IN, "profile_timeout"       ,  buftimeout     ,
				0      , NULL       ,  NULL 
			);
			wsprintf(buftimeout,"%ld",lpIceProfileData->ltimeoutms);
			if ((status = ICE_C_Execute (ICEclient, "profile", insert)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_WEBUSER_ROLE:
		{
			LPICEWEBUSERROLEDATA lpIceData = (LPICEWEBUSERROLEDATA)lpparameters;
			ICE_C_PARAMS insert[6];
			FillIceParamsArray( insert,
				ICE_IN , "action"   , "insert" ,
				ICE_IN, "ur_user_id" , lpIceData->icewebuser.ID ,
				ICE_IN, "ur_role_id"   , lpIceData->icerole.ID  ,
				0      , NULL       ,  NULL
			);
			if ((status = ICE_C_Execute (ICEclient, "user_role", insert)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_PROFILE_ROLE:
		{
			LPICEPROFILEROLEDATA lpIceData = (LPICEPROFILEROLEDATA)lpparameters;
			ICE_C_PARAMS insert[6];
			FillIceParamsArray( insert,
				ICE_IN , "action"   , "insert" ,
				ICE_IN, "pr_profile_id" , lpIceData->iceprofile.ID ,
				ICE_IN, "pr_role_id"   , lpIceData->icerole.ID ,
				0      , NULL       ,  NULL
			);
			if ((status = ICE_C_Execute (ICEclient, "profile_role", insert)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_ROLE:
		{
			LPICEROLEDATA lpIceRoleData = (LPICEROLEDATA) lpparameters;
			ICE_C_PARAMS insert[6];
			FillIceParamsArray( insert,
				ICE_IN , "action"       ,  "insert" ,
//				ICE_IN, "role_id"      ,  NULL     ,
				ICE_IN, "role_name"    , lpIceRoleData->RoleName ,
				ICE_IN, "role_comment" , lpIceRoleData->Comment  ,
				0      , NULL           ,  NULL
			);
			if ((status = ICE_C_Execute (ICEclient, "role", insert)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_BUNIT:
		{
			LPICEBUSUNITDATA lpBusUnitData = (LPICEBUSUNITDATA) lpparameters;
			ICE_C_PARAMS insert[6];
			FillIceParamsArray( insert,
				ICE_IN , "action"     , "insert" ,
//				{ICE_IN , "unit_id"    , lpBusUnitData->ID     ,
				ICE_IN , "unit_name"  , lpBusUnitData->Name ,
//				{ICE_IN , "unit_owner" , lpBusUnitData->Owner  ,
				0      , NULL         ,  NULL 
			);
			if ((status = ICE_C_Execute (ICEclient, "unit", insert)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_SERVER_VARIABLE:
		{
			LPICESERVERVARDATA lpvardata = (LPICESERVERVARDATA) lpparameters;
			ICE_C_PARAMS insert[6];
			FillIceParamsArray( insert,
				ICE_IN , "action"  ,  "insert" ,
				ICE_IN, "name"     ,  lpvardata->VariableName     ,
				ICE_IN, "value"    ,  lpvardata->VariableValue    ,
				0     , NULL       ,  NULL
			);
			if ((status = ICE_C_Execute (ICEclient, "getSvrVariables", insert)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_SERVER_APPLICATION:
		{
			LPICEAPPLICATIONDATA lpAppData = (LPICEAPPLICATIONDATA) lpparameters;
			ICE_C_PARAMS insert[6];
			FillIceParamsArray( insert,
				ICE_IN , "action"     , "insert"        ,
//				ICE_IN , "sess_id"    , lpAppData->ID   ,
				ICE_IN , "sess_name"  , lpAppData->Name ,
				0      , NULL         ,  NULL 
			);
			if ((status = ICE_C_Execute (ICEclient, "session_grp", insert)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_SERVER_LOCATION:
		{
			LPICELOCATIONDATA lpLocData = (LPICELOCATIONDATA) lpparameters;
			ICE_C_PARAMS insert[12];
			FillIceParamsArray( insert,
				ICE_IN , "action"     , "insert" ,
//				ICE_IN, "loc_id"        ,  lpLocData->ID                    ,
				ICE_IN, "loc_name"      ,  lpLocData->LocationName          ,
				ICE_IN, "loc_path"      ,  lpLocData->path                  ,
				ICE_IN, "loc_extensions",  lpLocData->extensions            ,
				ICE_IN, "loc_http"      ,  !lpLocData->bIce?    "checked":"" ,
				ICE_IN, "loc_ice"       ,  lpLocData->bIce?    "checked":"" ,
				ICE_IN, "loc_public"    ,  lpLocData->bPublic? "checked":"" ,
				0      , NULL         ,  NULL
			);
			if ((status = ICE_C_Execute (ICEclient, "ice_locations", insert)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_BUNIT_SEC_ROLE:
		{
			LPICEBUSUNITROLEDATA lpIceData = (LPICEBUSUNITROLEDATA)lpparameters;
			ICE_C_PARAMS insert[10];
			FillIceParamsArray( insert,
				ICE_IN , "action"    , "update" ,
				ICE_IN, "ur_unit_id" , lpIceData->icebusunit.ID ,
				ICE_IN, "ur_role_id" , lpIceData->icerole.ID  ,
				ICE_IN, "ur_execute" , lpIceData->bExecDoc  ?"checked" :"",
				ICE_IN, "ur_insert"  , lpIceData->bCreateDoc?"checked" :"",
				ICE_IN, "ur_read"    , lpIceData->bReadDoc  ?"checked" :"",
				0      , NULL        ,  NULL
			);
			if ((status = ICE_C_Execute (ICEclient, "unit_role", insert)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_BUNIT_SEC_USER:
		{
			LPICEBUSUNITWEBUSERDATA lpIceData = (LPICEBUSUNITWEBUSERDATA)lpparameters;
			ICE_C_PARAMS insert[10];
			FillIceParamsArray( insert,
				ICE_IN , "action"    , "update" ,
				ICE_IN, "uu_unit_id" , lpIceData->icebusunit.ID ,
				ICE_IN, "uu_user_id" , lpIceData->icewevuser.ID  ,
				ICE_IN, "uu_execute" , lpIceData->bUnitRead   ?"checked" :"",
				ICE_IN, "uu_read"    , lpIceData->bReadDoc    ?"checked" :"",
				ICE_IN, "uu_insert"  , lpIceData->bCreateDoc  ?"checked" :"",
				0      , NULL        ,  NULL
			);
			if ((status = ICE_C_Execute (ICEclient, "unit_user", insert)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_BUNIT_FACET:
		case OT_ICE_BUNIT_PAGE:
		{
			LPICEBUSUNITDOCDATA lpIceData = (LPICEBUSUNITDOCDATA)lpparameters;
//			ICE_C_PARAMS *insert;
			char buffer[50];
			ICE_C_PARAMS insert1[24];
			ICE_C_PARAMS insert2[24];
			FillIceParamsArray( insert1,
					ICE_IN , "action"          , "insert" ,
	//				ICE_IN, "doc_id"           , lpIceData->icebusunit.ID ,
	//				ICE_IN, "doc_type"         , lpIceData->icewevuser.ID  ,
					ICE_IN, "doc_unit_id"      , lpIceData->icebusunit.ID  ,
	//				ICE_IN, "doc_unit_name"    , lpIceData->icewevuser.ID  ,
					ICE_IN, "doc_name"         , lpIceData->name           ,
					ICE_IN, "doc_suffix"       , lpIceData->suffix         ,
					ICE_IN, "doc_public"       , lpIceData->bPublic       ? "checked" :"",
					ICE_IN, "doc_pre_cache"    , lpIceData->bpre_cache    ? "checked" :"",
					ICE_IN, "doc_perm_cache"   , lpIceData->bperm_cache   ? "checked" :"",
					ICE_IN, "doc_session_cache", lpIceData->bsession_cache? "checked" :"",
					ICE_IN|ICE_BLOB, "doc_file", lpIceData->doc_file       ,
				//	ICE_IN, "doc_ext_loc"      , lpIceData->ext_loc.ID     ,
				//	ICE_IN, "doc_ext_file"     , lpIceData->ext_file       ,
				//	ICE_IN, "doc_ext_suffix"   , lpIceData->ext_suffix     ,
	//				ICE_IN, "doc_owner"        , lpIceData->??,
	//				ICE_IN, "doc_transfer"     , lpIceData->??,
					ICE_IN, "doc_type"         , buffer       ,
					0     , NULL               ,  NULL  
				);
			FillIceParamsArray( insert2,
					ICE_IN , "action"          , "insert" ,
	//				ICE_IN, "doc_id"           , lpIceData->icebusunit.ID ,
	//				ICE_IN, "doc_type"         , lpIceData->icewevuser.ID  ,
					ICE_IN, "doc_unit_id"      , lpIceData->icebusunit.ID  ,
	//				ICE_IN, "doc_unit_name"    , lpIceData->icewevuser.ID  ,
					ICE_IN, "doc_name"         , lpIceData->name           ,
					ICE_IN, "doc_suffix"       , lpIceData->suffix         ,
					ICE_IN, "doc_public"       , lpIceData->bPublic       ? "checked" :"",
				//	ICE_IN, "doc_pre_cache"    , lpIceData->bpre_cache    ? "checked" :"",
				//	ICE_IN, "doc_perm_cache"   , lpIceData->bperm_cache   ? "checked" :"",
				//	ICE_IN, "doc_session_cache", lpIceData->bsession_cache? "checked" :"",
				//	ICE_IN, "doc_file"         , lpIceData->doc_file       ,
					ICE_IN, "doc_ext_loc"      , lpIceData->ext_loc.ID     ,
					ICE_IN, "doc_ext_file"     , lpIceData->ext_file       ,
					ICE_IN, "doc_ext_suffix"   , lpIceData->ext_suffix     ,
	//				ICE_IN, "doc_owner"        , lpIceData->??,
	//				ICE_IN, "doc_transfer"     , lpIceData->??,
					ICE_IN, "doc_type"         , buffer       ,
					0     , NULL               ,  NULL 
				);
			if (iobjecttype==OT_ICE_BUNIT_FACET)
				x_strcpy(buffer,"facet");
			else
				x_strcpy(buffer,"page");

			if (!x_strlen(lpIceData->ext_loc.LocationName)) 
				status = ICE_C_Execute (ICEclient, "document", insert1);
			else 
				status = ICE_C_Execute (ICEclient, "document", insert2);
			
			if (status != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_BUNIT_FACET_ROLE:
		case OT_ICE_BUNIT_PAGE_ROLE:
		{
			LPICEBUSUNITDOCROLEDATA lpIceData = (LPICEBUSUNITDOCROLEDATA)lpparameters;
			ICE_C_PARAMS insert[10];
			FillIceParamsArray( insert,
				ICE_IN , "action"     , "update" ,
				ICE_IN , "dr_doc_id"  , lpIceData->icebusunitdoc.doc_ID,
				ICE_IN , "dr_role_id" , lpIceData->icebusunitdocrole.ID, 
				ICE_IN , "dr_execute" , lpIceData->bExec   ? "checked" :"",
				ICE_IN , "dr_read"    , lpIceData->bRead   ? "checked" :"",
				ICE_IN , "dr_update"  , lpIceData->bUpdate ? "checked" :"",
				ICE_IN , "dr_delete"  , lpIceData->bDelete ? "checked" :"",
				0      ,  NULL        , NULL
			);
			if ((status = ICE_C_Execute (ICEclient, "document_role", insert)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_BUNIT_FACET_USER:
		case OT_ICE_BUNIT_PAGE_USER:
		{
			LPICEBUSUNITDOCUSERDATA lpIceData = (LPICEBUSUNITDOCUSERDATA)lpparameters;
			ICE_C_PARAMS insert[10];
			FillIceParamsArray( insert,
				ICE_IN , "action"     , "update" ,
				ICE_IN , "du_doc_id"  , lpIceData->icebusunitdoc.doc_ID,
				ICE_IN , "du_user_id" , lpIceData->user.ID  , 
				ICE_IN , "du_execute" , lpIceData->bExec   ? "checked" :"",
				ICE_IN , "du_read"    , lpIceData->bRead   ? "checked" :"",
				ICE_IN , "du_update"  , lpIceData->bUpdate ? "checked" :"",
				ICE_IN , "du_delete"  , lpIceData->bDelete ? "checked" :"",
				0      , NULL         , NULL
			);
			if ((status = ICE_C_Execute (ICEclient, "document_user", insert)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_BUNIT_LOCATION:
		{
			LPICEBUSUNITLOCATIONDATA lpIceData = (LPICEBUSUNITLOCATIONDATA)lpparameters;
			ICE_C_PARAMS insert[6];
			FillIceParamsArray( insert,
				ICE_IN , "action"    , "insert" ,
				ICE_IN, "ul_unit_id"       , lpIceData->icebusunit.ID   ,
				ICE_IN, "ul_location_id"   , lpIceData->icelocation.ID  ,
//				ICE_IN, "ul_location_name" , lpIceData->??,
				0      , NULL        ,  NULL
			);
			if ((status = ICE_C_Execute (ICEclient, "unit_location", insert)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		default:
			TS_MessageBox(TS_GetFocus(),
						ResourceString(IDS_ERR_UNKNOWN_ICE_TYPE),
						NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
			iresult=RES_ERR;
	}

	if ((status =ICE_C_Disconnect (&ICEclient))!=OK) {
		ManageIceError(&ICEclient, &status);
		iresult=RES_ERR;
	}
	RemoveHourGlass();
	return iresult;
#else
	return RES_SUCCESS;
#endif
}

int ICEAlterobject(LPUCHAR lpVirtNode,int iobjecttype, void *lpoldparameters,void *lpnewparameters)
{
#ifndef NOUSEICEAPI
	TCHAR			bufuser[100],bufpw[100];
	ICE_STATUS		status = 0;
	ICE_C_CLIENT	ICEclient = NULL;
	int iresult = RES_SUCCESS;

	if (!lpoldparameters || !lpoldparameters ||!lpVirtNode) {
		myerror(ERR_INVPARAMS);
		return RES_ERR;
	}

	// first step: when there is a parenthood, get ID of the parent object by completing 
	// the parent struture
	switch (iobjecttype) {
		case OT_ICE_DBCONNECTION:
		{
			LPDBCONNECTIONDATA lpIceDataOld = (LPDBCONNECTIONDATA) lpoldparameters;
			LPDBCONNECTIONDATA lpIceDataNew = (LPDBCONNECTIONDATA) lpnewparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_DBUSER, &(lpIceDataNew->DBUsr));
		}
		break;
		case OT_ICE_WEBUSER:
		{
			LPICEWEBUSERDATA lpIceDataOld = (LPICEWEBUSERDATA) lpoldparameters;
			LPICEWEBUSERDATA lpIceDataNew = (LPICEWEBUSERDATA) lpnewparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_DBUSER, &(lpIceDataNew->DefDBUsr));
		}
		break;
		case OT_ICE_PROFILE:
		{
			LPICEPROFILEDATA lpIceDataOld = (LPICEPROFILEDATA) lpoldparameters;
			LPICEPROFILEDATA lpIceDataNew = (LPICEPROFILEDATA) lpnewparameters;
			iresult = GetICEInfo(lpVirtNode,OT_ICE_DBUSER, &(lpIceDataNew->DefDBUsr));
		}
		break;
		case OT_ICE_BUNIT_SEC_ROLE:
		case OT_ICE_BUNIT_SEC_USER:
		case OT_ICE_BUNIT_FACET:
		case OT_ICE_BUNIT_PAGE:
		case OT_ICE_BUNIT_FACET_ROLE:
		case OT_ICE_BUNIT_PAGE_ROLE:
		case OT_ICE_BUNIT_FACET_USER:
		case OT_ICE_BUNIT_PAGE_USER:
		{
			iresult = GetICEInfo(lpVirtNode,iobjecttype, lpoldparameters);
			if (iresult !=RES_SUCCESS)
				return iresult;
			switch (iobjecttype) {
				case OT_ICE_BUNIT_FACET:
				case OT_ICE_BUNIT_PAGE:
					{
						LPICEBUSUNITDOCDATA lpIceData = (LPICEBUSUNITDOCDATA)lpnewparameters;
						if (lpIceData->ext_loc.LocationName[0])
							iresult = GetICEInfo(lpVirtNode,OT_ICE_SERVER_LOCATION, &(lpIceData->ext_loc));
						break;
					}
				default:
					break;
			}
		}
		break;
	}
	if (iresult !=RES_SUCCESS)
		return iresult;

	if (GetICeUserAndPassWord(GetVirtNodeHandle(lpVirtNode),bufuser,bufpw)!=RES_SUCCESS)
		return RES_ERR;
    if ((status =ICE_C_Initialize ()) != OK) {
		ManageIceError(&ICEclient, &status);
		return RES_ERR;
	}
    if ((status = ICE_C_Connect (CAPINodeName(lpVirtNode), bufuser,bufpw, &ICEclient)) != OK) {
		ManageIceError(&ICEclient, &status);
		return RES_ERR;
	}
	ShowHourGlass();
	switch (iobjecttype) {
		case OT_ICE_DBUSER:
		{
			LPICEDBUSERDATA lpIceDataOld = (LPICEDBUSERDATA) lpoldparameters;
			LPICEDBUSERDATA lpIceDataNew = (LPICEDBUSERDATA) lpnewparameters;
			ICE_C_PARAMS update[10];
			FillIceParamsArray( update,
				ICE_IN , "action"       ,  "update" ,
				ICE_IN, "dbuser_id"       ,  lpIceDataOld->ID    ,
				ICE_IN, "dbuser_alias"    ,  lpIceDataNew->UserAlias ,
				ICE_IN, "dbuser_name"     ,  lpIceDataNew->User_Name ,
				ICE_IN, "dbuser_password1",  lpIceDataNew->Password  ,
				ICE_IN, "dbuser_password2",  lpIceDataNew->Password ,
				ICE_IN, "dbuser_comment"  ,  lpIceDataNew->Comment  ,
				0      , NULL           ,  NULL 
			);
			if ((status = ICE_C_Execute (ICEclient, "dbuser", update)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_DBCONNECTION:
		{
			LPDBCONNECTIONDATA lpIceDataOld = (LPDBCONNECTIONDATA) lpoldparameters;
			LPDBCONNECTIONDATA lpIceDataNew = (LPDBCONNECTIONDATA) lpnewparameters;
			ICE_C_PARAMS update[10];
			FillIceParamsArray( update,
				ICE_IN , "action"       ,  "update" ,
				ICE_IN, "db_id"     , lpIceDataOld->ID     ,
				ICE_IN, "db_name"   , lpIceDataNew->ConnectionName     ,
				ICE_IN, "db_dbname" , lpIceDataNew->DBName     ,
				ICE_IN, "db_dbuser" , lpIceDataNew->DBUsr.ID     ,
				ICE_IN, "db_comment", lpIceDataNew->Comment     ,
				0      , NULL           ,  NULL
			);
			if ((status = ICE_C_Execute (ICEclient, "database", update)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_WEBUSER:
		{
			LPICEWEBUSERDATA lpIceDataOld = (LPICEWEBUSERDATA) lpoldparameters;
			LPICEWEBUSERDATA lpIceDataNew = (LPICEWEBUSERDATA) lpnewparameters;
			TCHAR buftimeout[100];
			TCHAR bufICEorOS[20];
			ICE_C_PARAMS update[20];
			FillIceParamsArray( update,
				ICE_IN , "action"       ,  "update" ,
				ICE_IN, "user_id"            ,  lpIceDataOld->ID     ,
				ICE_IN, "user_name"          ,  lpIceDataNew->UserName ,
				ICE_IN, "user_password1"     ,  lpIceDataNew->Password,
				ICE_IN, "user_password2"     ,  lpIceDataNew->Password,
				ICE_IN, "user_dbuser"        ,  lpIceDataNew->DefDBUsr.ID,
				ICE_IN, "user_comment"       ,  lpIceDataNew->Comment    ,
				ICE_IN, "user_administration",  lpIceDataNew->bAdminPriv   ? "checked":""  ,
				ICE_IN, "user_security"      ,  lpIceDataNew->bSecurityPriv? "checked":""  ,
				ICE_IN, "user_unit"          ,  lpIceDataNew->bUnitMgrPriv ? "checked":""  ,
				ICE_IN, "user_monitor"       ,  lpIceDataNew->bMonitorPriv ? "checked":""  ,
				ICE_IN, "user_timeout"       ,  buftimeout ,
				ICE_IN, "user_authtype"     ,  bufICEorOS  ,
				0      , NULL           ,  NULL 
			);
			wsprintf(buftimeout,"%ld",lpIceDataNew->ltimeoutms);
			if (lpIceDataNew->bICEpwd)
				x_strcpy(bufICEorOS,"ICE");
			else {
				x_strcpy(bufICEorOS,"OS");
				x_strcpy(lpIceDataNew->Password,"");
			}
			if ((status = ICE_C_Execute (ICEclient, "user", update)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_PROFILE:
		{
			LPICEPROFILEDATA lpIceDataOld = (LPICEPROFILEDATA) lpoldparameters;
			LPICEPROFILEDATA lpIceDataNew = (LPICEPROFILEDATA) lpnewparameters;
			TCHAR buftimeout[100];
			ICE_C_PARAMS update[16];
			FillIceParamsArray( update,
				ICE_IN , "action"       ,  "update" ,
				ICE_IN, "profile_id"            ,  lpIceDataOld->ID     ,
				ICE_IN, "profile_name"          ,  lpIceDataNew->ProfileName   ,
				ICE_IN, "profile_dbuser"        ,  lpIceDataNew->DefDBUsr.ID  ,
//				ICE_IN, "profile_comment"       ,  lpIceDataNew->Comment    ,
				ICE_IN, "profile_administration",  lpIceDataNew->bAdminPriv   ? "checked":""  ,
				ICE_IN, "profile_security"      ,  lpIceDataNew->bSecurityPriv? "checked":""  ,
				ICE_IN, "profile_unit"          ,  lpIceDataNew->bUnitMgrPriv ? "checked":""  ,
				ICE_IN, "profile_monitor"       ,  lpIceDataNew->bMonitorPriv ? "checked":""  ,
				ICE_IN, "profile_timeout"       ,  buftimeout     ,
				0      , NULL           ,  NULL 
			);
			wsprintf(buftimeout,"%ld",lpIceDataNew->ltimeoutms);
			if ((status = ICE_C_Execute (ICEclient, "profile", update)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_ROLE:
		{
			LPICEROLEDATA lpIceRoleDataOld = (LPICEROLEDATA) lpoldparameters;
			LPICEROLEDATA lpIceRoleDataNew = (LPICEROLEDATA) lpnewparameters;
			ICE_C_PARAMS update[10];
			FillIceParamsArray( update,
				ICE_IN , "action"       ,  "update" ,
				ICE_IN,   "role_id"      , lpIceRoleDataOld->ID  ,
				ICE_IN, "role_name"    , lpIceRoleDataNew->RoleName ,
				ICE_IN, "role_comment" , lpIceRoleDataNew->Comment  ,
				0      , NULL           ,  NULL     
			);
			if ((status = ICE_C_Execute (ICEclient, "role", update)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_BUNIT:
		{
			LPICEBUSUNITDATA lpBusUnitData = (LPICEBUSUNITDATA) lpoldparameters;
			if (!x_strlen(lpBusUnitData->Name))
				iresult=RES_ERR;
//			lpBusUnitData->Name
//			lpBusUnitData->Owner
		}
		break;

		case OT_ICE_SERVER_VARIABLE:
		{
			LPICESERVERVARDATA lpvardata = (LPICESERVERVARDATA) lpnewparameters;
			ICE_C_PARAMS update[6];
			FillIceParamsArray( update,
				ICE_IN , "action"  ,  "update" ,
				ICE_IN, "name"     ,  lpvardata->VariableName     ,
				ICE_IN, "value"    ,  lpvardata->VariableValue    ,
				0     , NULL       ,  NULL     
			);
			if ((status = ICE_C_Execute (ICEclient, "getSvrVariables", update)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_SERVER_LOCATION:
		{
			LPICELOCATIONDATA lpIceLocDataOld = (LPICELOCATIONDATA) lpoldparameters;
			LPICELOCATIONDATA lpIceLocDataNew = (LPICELOCATIONDATA) lpnewparameters;
			ICE_C_PARAMS update[12];
			FillIceParamsArray( update,
				ICE_IN , "action"       ,  "update" ,
				ICE_IN, "loc_id"        ,  lpIceLocDataOld->ID                    ,
				ICE_IN, "loc_name"      ,  lpIceLocDataNew->LocationName          ,
				ICE_IN, "loc_path"      ,  lpIceLocDataNew->path                  ,
				ICE_IN, "loc_extensions",  lpIceLocDataNew->extensions            ,
				ICE_IN, "loc_http"      ,  lpIceLocDataNew->bIce?    "":"checked" ,
				ICE_IN, "loc_ice"       ,  lpIceLocDataNew->bIce?    "checked":"" ,
				ICE_IN, "loc_public"    ,  lpIceLocDataNew->bPublic? "checked":"" ,
				0      , NULL           ,                                NULL     
			);
			if ((status = ICE_C_Execute (ICEclient, "ice_locations", update)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_BUNIT_SEC_ROLE:
		{
			LPICEBUSUNITROLEDATA lpIceDataOld = (LPICEBUSUNITROLEDATA)lpoldparameters;
			LPICEBUSUNITROLEDATA lpIceDataNew = (LPICEBUSUNITROLEDATA)lpnewparameters;
			ICE_C_PARAMS update[10];
			FillIceParamsArray( update,
				ICE_IN , "action"    , "update" ,
				ICE_IN, "ur_unit_id" , lpIceDataOld->icebusunit.ID ,
				ICE_IN, "ur_role_id" , lpIceDataOld->icerole.ID  ,
				ICE_IN, "ur_execute" , lpIceDataNew->bExecDoc  ?"checked" :"",
				ICE_IN, "ur_insert"  , lpIceDataNew->bCreateDoc?"checked" :"",
				ICE_IN, "ur_read"    , lpIceDataNew->bReadDoc  ?"checked" :"",
				0      , NULL        ,  NULL     
			);
			if ((status = ICE_C_Execute (ICEclient, "unit_role", update)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_BUNIT_SEC_USER:
		{
			LPICEBUSUNITWEBUSERDATA lpIceDataOld = (LPICEBUSUNITWEBUSERDATA)lpoldparameters;
			LPICEBUSUNITWEBUSERDATA lpIceDataNew = (LPICEBUSUNITWEBUSERDATA)lpnewparameters;
			ICE_C_PARAMS update[10];
			FillIceParamsArray( update,
				ICE_IN , "action"    , "update" ,
				ICE_IN, "uu_unit_id" , lpIceDataOld->icebusunit.ID ,
				ICE_IN, "uu_user_id" , lpIceDataOld->icewevuser.ID  ,
				ICE_IN, "uu_execute" , lpIceDataNew->bUnitRead  ?"checked" :"",
				ICE_IN, "uu_read"    , lpIceDataNew->bReadDoc   ?"checked" :"",
				ICE_IN, "uu_insert"  , lpIceDataNew->bCreateDoc ?"checked" :"",
				0      , NULL        ,  NULL    
			);
			if ((status = ICE_C_Execute (ICEclient, "unit_user", update)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_BUNIT_FACET:
		case OT_ICE_BUNIT_PAGE:
		{
			LPICEBUSUNITDOCDATA lpIceDataOld = (LPICEBUSUNITDOCDATA)lpoldparameters;
			LPICEBUSUNITDOCDATA lpIceDataNew = (LPICEBUSUNITDOCDATA)lpnewparameters;
			ICE_C_PARAMS update1[24];
			ICE_C_PARAMS update2[24];
			ICE_C_PARAMS update3[24];
			FillIceParamsArray( update1,
				ICE_IN , "action"          , "update" ,
				ICE_IN, "doc_id"           , lpIceDataOld->doc_ID,
//				ICE_IN, "doc_type"         , lpIceData->icewevuser.ID  ,
				ICE_IN, "doc_unit_id"      , lpIceDataOld->icebusunit.ID  ,
//				ICE_IN, "doc_unit_name"    , lpIceData->icewevuser.ID  ,
				ICE_IN, "doc_name"         , lpIceDataNew->name           ,
				ICE_IN, "doc_suffix"       , lpIceDataNew->suffix         ,
				ICE_IN, "doc_public"       , lpIceDataNew->bPublic       ? "checked" :"",
				ICE_IN, "doc_pre_cache"    , lpIceDataNew->bpre_cache    ? "checked" :"",
				ICE_IN, "doc_perm_cache"   , lpIceDataNew->bperm_cache   ? "checked" :"",
				ICE_IN, "doc_session_cache", lpIceDataNew->bsession_cache? "checked" :"",
				ICE_IN|ICE_BLOB, "doc_file", lpIceDataNew->doc_file       ,
//				ICE_IN, "doc_ext_loc"      , lpIceDataNew->ext_loc.ID     ,
//				ICE_IN, "doc_ext_file"     , lpIceDataNew->ext_file       ,
//				ICE_IN, "doc_ext_suffix"   , lpIceDataNew->ext_suffix     ,
//				ICE_IN, "doc_owner"        , lpIceDataNew->??,
//				ICE_IN, "doc_transfer"     , lpIceDataNew->??,
//				ICE_IN, "doc_type"         , buffer       ,
				0     , NULL               ,  NULL     
			);
			FillIceParamsArray( update2,
				ICE_IN , "action"          , "update" ,
				ICE_IN, "doc_id"           , lpIceDataOld->doc_ID ,
//				ICE_IN, "doc_type"         , lpIceData->icewevuser.ID  ,
				ICE_IN, "doc_unit_id"      , lpIceDataOld->icebusunit.ID  ,
//				ICE_IN, "doc_unit_name"    , lpIceData->icewevuser.ID  ,
				ICE_IN, "doc_name"         , lpIceDataNew->name           ,
				ICE_IN, "doc_suffix"       , lpIceDataNew->suffix         ,
				ICE_IN, "doc_public"       , lpIceDataNew->bPublic       ? "checked" :"",
				ICE_IN, "doc_pre_cache"    , lpIceDataNew->bpre_cache    ? "checked" :"",
				ICE_IN, "doc_perm_cache"   , lpIceDataNew->bperm_cache   ? "checked" :"",
				ICE_IN, "doc_session_cache", lpIceDataNew->bsession_cache? "checked" :"",
//				ICE_IN|ICE_BLOB, "doc_file", lpIceDataNew->doc_file       ,
//				ICE_IN, "doc_ext_loc"      , lpIceDataNew->ext_loc.ID     ,
//				ICE_IN, "doc_ext_file"     , lpIceDataNew->ext_file       ,
//				ICE_IN, "doc_ext_suffix"   , lpIceDataNew->ext_suffix     ,
//				ICE_IN, "doc_owner"        , lpIceDataNew->??,
//				ICE_IN, "doc_transfer"     , lpIceDataNew->??,
//				ICE_IN, "doc_type"         , buffer       ,
				0     , NULL               ,  NULL 
			);
			FillIceParamsArray( update3,
				ICE_IN , "action"          , "update" ,
				ICE_IN, "doc_id"           , lpIceDataOld->doc_ID ,
//				ICE_IN, "doc_type"         , lpIceData->icewevuser.ID  ,
				ICE_IN, "doc_unit_id"      , lpIceDataOld->icebusunit.ID  ,
//				ICE_IN, "doc_unit_name"    , lpIceData->icewevuser.ID  ,
				ICE_IN, "doc_name"         , lpIceDataNew->name           ,
				ICE_IN, "doc_suffix"       , lpIceDataNew->suffix         ,
				ICE_IN, "doc_public"       , lpIceDataNew->bPublic       ? "checked" :"",
//				ICE_IN, "doc_pre_cache"    , lpIceDataNew->bpre_cache    ? "checked" :"",
//				ICE_IN, "doc_perm_cache"   , lpIceDataNew->bperm_cache   ? "checked" :"",
//				ICE_IN, "doc_session_cache", lpIceDataNew->bsession_cache? "checked" :"",
//				ICE_IN, "doc_file"         , lpIceDataNew->doc_file       ,
				ICE_IN, "doc_ext_loc"      , lpIceDataNew->ext_loc.ID     ,
				ICE_IN, "doc_ext_file"     , lpIceDataNew->ext_file       ,
				ICE_IN, "doc_ext_suffix"   , lpIceDataNew->ext_suffix     ,
//				ICE_IN, "doc_owner"        , lpIceDataNew->??,
//				ICE_IN, "doc_transfer"     , lpIceDataNew->??,
//				ICE_IN, "doc_type"         , buffer       ,
				0     , NULL               ,  NULL 
			);
			if (!x_strlen(lpIceDataNew->ext_loc.LocationName))  { // both old and new are "in repository"
				if (x_strlen(lpIceDataNew->doc_file) !=0) // new download
					status = ICE_C_Execute (ICEclient, "document", update1);
				else
					status = ICE_C_Execute (ICEclient, "document", update2);
			}
			else {
				status = ICE_C_Execute (ICEclient, "document", update3);
			}
			if (status  != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_BUNIT_FACET_ROLE:
		case OT_ICE_BUNIT_PAGE_ROLE:
		{
			LPICEBUSUNITDOCROLEDATA lpIceDataOld = (LPICEBUSUNITDOCROLEDATA)lpoldparameters;
			LPICEBUSUNITDOCROLEDATA lpIceDataNew = (LPICEBUSUNITDOCROLEDATA)lpnewparameters;
			ICE_C_PARAMS update[10];
			FillIceParamsArray( update,
				ICE_IN , "action"     , "update" ,
				ICE_IN , "dr_doc_id"  , lpIceDataOld->icebusunitdoc.doc_ID,
				ICE_IN , "dr_role_id" , lpIceDataOld->icebusunitdocrole.ID, 
				ICE_IN , "dr_execute" , lpIceDataNew->bExec   ? "checked" :"",
				ICE_IN , "dr_read"    , lpIceDataNew->bRead   ? "checked" :"",
				ICE_IN , "dr_update"  , lpIceDataNew->bUpdate ? "checked" :"",
				ICE_IN , "dr_delete"  , lpIceDataNew->bDelete ? "checked" :"",
				0      ,  NULL        , NULL 
			);
			if ((status = ICE_C_Execute (ICEclient, "document_role", update)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_BUNIT_FACET_USER:
		case OT_ICE_BUNIT_PAGE_USER:
		{
			LPICEBUSUNITDOCUSERDATA lpIceDataOld = (LPICEBUSUNITDOCUSERDATA)lpoldparameters;
			LPICEBUSUNITDOCUSERDATA lpIceDataNew = (LPICEBUSUNITDOCUSERDATA)lpnewparameters;
			ICE_C_PARAMS update[10];
			FillIceParamsArray( update,
				ICE_IN , "action"     , "update" ,
				ICE_IN , "du_doc_id"  , lpIceDataOld->icebusunitdoc.doc_ID,
				ICE_IN , "du_user_id" , lpIceDataOld->user.ID  , 
				ICE_IN , "du_execute" , lpIceDataNew->bExec   ? "checked" :"",
				ICE_IN , "du_read"    , lpIceDataNew->bRead   ? "checked" :"",
				ICE_IN , "du_update"  , lpIceDataNew->bUpdate ? "checked" :"",
				ICE_IN , "du_delete"  , lpIceDataNew->bDelete ? "checked" :"",
				0      , NULL         , NULL 
			);
			if ((status = ICE_C_Execute (ICEclient, "document_user", update)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_BUNIT_LOCATION:
		case OT_ICE_SERVER_APPLICATION:
			iresult=RES_ERR; // no alter for these types
		default:
			TS_MessageBox(TS_GetFocus(),
				ResourceString(IDS_ERR_UNKNOWN_ICE_TYPE),
				NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
			iresult=RES_ERR;
	}
	if ((status =ICE_C_Disconnect (&ICEclient))!=OK) {
		ManageIceError(&ICEclient, &status);
		iresult=RES_ERR;
	}
	RemoveHourGlass();
	return iresult;
#else
	return RES_SUCCESS;
#endif
}


int ICEDropobject(LPUCHAR lpVirtNode,int iobjecttype, void *lpparameters)
{
#ifndef NOUSEICEAPI
	TCHAR			bufuser[100],bufpw[100];
	ICE_STATUS		status = 0;
	ICE_C_CLIENT	ICEclient = NULL;
	int iresult = RES_SUCCESS;
	if (!lpparameters || !lpVirtNode) {
		myerror(ERR_INVPARAMS);
		return RES_ERR;
	}
	iresult =  GetICEInfo(lpVirtNode,iobjecttype,lpparameters);
	if (iresult!=RES_SUCCESS)
		return iresult;
	if (GetICeUserAndPassWord(GetVirtNodeHandle(lpVirtNode),bufuser,bufpw)!=RES_SUCCESS)
		return RES_ERR;
	if ((status =ICE_C_Initialize ()) != OK) {
		ManageIceError(&ICEclient, &status);
		return RES_ERR;
	}
	if ((status = ICE_C_Connect (CAPINodeName(lpVirtNode), bufuser,bufpw, &ICEclient)) != OK) {
		ManageIceError(&ICEclient, &status);
		return RES_ERR;
	}

	ShowHourGlass();
	switch (iobjecttype) {
		case OT_ICE_DBUSER:
		{
			LPICEDBUSERDATA lpIceData = (LPICEDBUSERDATA)lpparameters;
			ICE_C_PARAMS del[4];
			FillIceParamsArray( del,
				ICE_IN , "action"         ,  "delete" ,
				ICE_IN, "dbuser_id"       ,  lpIceData->ID     ,
				0     , NULL              ,  NULL
			);
			if ((status = ICE_C_Execute (ICEclient, "dbuser", del)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			break;
		}
		case OT_ICE_WEBUSER_CONNECTION:
		{
			LPICEWEBUSERCONNECTIONDATA lpIceData = (LPICEWEBUSERCONNECTIONDATA)lpparameters;
			ICE_C_PARAMS del[6];
			FillIceParamsArray( del,
				ICE_IN , "action"   , "delete" ,
				ICE_IN, "ud_user_id" , lpIceData->icewebuser.ID      ,
				ICE_IN, "ud_db_id"   , lpIceData->icedbconnection.ID ,
				0      , NULL       ,  NULL
			);
			if ((status = ICE_C_Execute (ICEclient, "user_database", del)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			break;

		}
		case OT_ICE_PROFILE_CONNECTION:
		{
			LPICEPROFILECONNECTIONDATA lpIceData = (LPICEPROFILECONNECTIONDATA)lpparameters;
			ICE_C_PARAMS del[6];
			FillIceParamsArray( del,
				ICE_IN , "action"   , "delete" ,
				ICE_IN, "pd_profile_id" , lpIceData->iceprofile.ID   ,
				ICE_IN, "pd_db_id"   , lpIceData->icedbconnection.ID ,
				0      , NULL       ,  NULL 
			);
			if ((status = ICE_C_Execute (ICEclient, "profile_database", del)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			break;

		}
		case OT_ICE_DBCONNECTION:
		{
			LPDBCONNECTIONDATA lpDBConnectionData = (LPDBCONNECTIONDATA) lpparameters;
			ICE_C_PARAMS del[6];
			FillIceParamsArray( del,
				ICE_IN , "action"   , "delete" ,
				ICE_IN, "db_id"     , lpDBConnectionData->ID     ,
				0      , NULL       ,  NULL
			);
			if ((status = ICE_C_Execute (ICEclient, "database", del)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			break;
		}
		case OT_ICE_WEBUSER:
		{
			LPICEWEBUSERDATA lpWebUserData = (LPICEWEBUSERDATA) lpparameters;
			TCHAR buftimeout[100];
			ICE_C_PARAMS del[6];
			FillIceParamsArray( del,
				ICE_IN , "action"   , "delete"          ,
				ICE_IN, "user_id"  ,  lpWebUserData->ID ,
				0      , NULL       ,  NULL
			);
			wsprintf(buftimeout,"%ld",lpWebUserData->ltimeoutms);
			if ((status = ICE_C_Execute (ICEclient, "user", del)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			break;
		}
		case OT_ICE_PROFILE:
		{
			LPICEPROFILEDATA lpIceProfileData = (LPICEPROFILEDATA) lpparameters;
			TCHAR buftimeout[100];
			ICE_C_PARAMS del[6];
			FillIceParamsArray( del,
				ICE_IN , "action"      , "delete"             ,
				ICE_IN, "profile_id"  ,  lpIceProfileData->ID ,
				0      , NULL          ,   NULL 
			);
			wsprintf(buftimeout,"%ld",lpIceProfileData->ltimeoutms);
			if ((status = ICE_C_Execute (ICEclient, "profile", del)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			break;
		}
		case OT_ICE_WEBUSER_ROLE:
		{
			LPICEWEBUSERROLEDATA lpIceData = (LPICEWEBUSERROLEDATA)lpparameters;
			ICE_C_PARAMS del[6];
			FillIceParamsArray( del,
				ICE_IN , "action"   , "delete" ,
				ICE_IN, "ur_user_id" , lpIceData->icewebuser.ID ,
				ICE_IN, "ur_role_id"   , lpIceData->icerole.ID  ,
				0      , NULL       ,  NULL 
			);
			if ((status = ICE_C_Execute (ICEclient, "user_role", del)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			break;
		}
		case OT_ICE_PROFILE_ROLE:
		{
			LPICEPROFILEROLEDATA lpIceData = (LPICEPROFILEROLEDATA)lpparameters;
			ICE_C_PARAMS del[6];
			FillIceParamsArray( del,
				ICE_IN , "action"   , "delete" ,
				ICE_IN, "pr_profile_id" , lpIceData->iceprofile.ID ,
				ICE_IN, "pr_role_id"   , lpIceData->icerole.ID,
				0      , NULL       ,  NULL
			);
			if ((status = ICE_C_Execute (ICEclient, "profile_role", del)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			break;
		}
		case OT_ICE_ROLE:
		{
			LPICEROLEDATA lpIceRoleData = (LPICEROLEDATA) lpparameters;
			ICE_C_PARAMS del[6];
			FillIceParamsArray( del,
				ICE_IN , "action"       ,  "delete" ,
				ICE_IN, "role_id"      ,  lpIceRoleData->ID,
				0      , NULL           ,  NULL
			);
			if ((status = ICE_C_Execute (ICEclient, "role", del)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			break;
		}
		case OT_ICE_BUNIT:
		{
			LPICEBUSUNITDATA lpBusUnitData = (LPICEBUSUNITDATA) lpparameters;
			ICE_C_PARAMS del[6];
			FillIceParamsArray( del,
				ICE_IN , "action"     , "delete" ,
				ICE_IN , "unit_id"    , lpBusUnitData->ID ,
				0      , NULL         ,  NULL 
			);
			if ((status = ICE_C_Execute (ICEclient, "unit", del)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			break;
		}
		case OT_ICE_SERVER_VARIABLE:
		{
			LPICESERVERVARDATA lpvardata = (LPICESERVERVARDATA) lpparameters;
			ICE_C_PARAMS del[10];
			FillIceParamsArray( del,
				ICE_IN ,"action"  ,  "delete" ,
				ICE_IN, "name"     ,  lpvardata->VariableName ,
				ICE_IN, "value"    ,  ""    ,
				0     , NULL       ,  NULL
			);
			if ((status = ICE_C_Execute (ICEclient, "getSvrVariables", del)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			break;
		}
		case OT_ICE_SERVER_APPLICATION:
		{
			LPICEAPPLICATIONDATA lpAppData = (LPICEAPPLICATIONDATA) lpparameters;
			ICE_C_PARAMS del[6];
			FillIceParamsArray( del,
				ICE_IN , "action"     , "delete"      ,
				ICE_IN , "sess_id"    , lpAppData->ID ,
				0      , NULL         ,  NULL
			);
			if ((status = ICE_C_Execute (ICEclient, "session_grp", del)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			break;
		}
		case OT_ICE_SERVER_LOCATION:
		{
			LPICELOCATIONDATA lpLocData = (LPICELOCATIONDATA) lpparameters;
			ICE_C_PARAMS del[6];
			FillIceParamsArray( del,
				ICE_IN , "action"   , "delete"       ,
				ICE_IN, "loc_id"    , lpLocData->ID  ,
				0      , NULL       ,  NULL 
			);
			if ((status = ICE_C_Execute (ICEclient, "ice_locations", del)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			break;
		}
		case OT_ICE_BUNIT_SEC_ROLE:
		{
			LPICEBUSUNITROLEDATA lpIceData = (LPICEBUSUNITROLEDATA)lpparameters;
			ICE_C_PARAMS del[10];
			FillIceParamsArray( del,
				ICE_IN , "action"     , "update"                 ,
				ICE_IN , "ur_unit_id" , lpIceData->icebusunit.ID ,
				ICE_IN , "ur_role_id" , lpIceData->icerole.ID    ,
				ICE_IN , "ur_execute" , "",
				ICE_IN , "ur_insert"  , "",
				ICE_IN , "ur_read"    , "",
				0      , NULL         ,  NULL 
			);
			if ((status = ICE_C_Execute (ICEclient, "unit_role", del)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			break;
		}
		case OT_ICE_BUNIT_SEC_USER:
		{
			LPICEBUSUNITWEBUSERDATA lpIceData = (LPICEBUSUNITWEBUSERDATA)lpparameters;
			ICE_C_PARAMS del[10];
			FillIceParamsArray( del,
				ICE_IN , "action"     , "update" ,
				ICE_IN , "uu_unit_id" , lpIceData->icebusunit.ID ,
				ICE_IN , "uu_user_id" , lpIceData->icewevuser.ID ,
				ICE_IN , "uu_execute" , "",
				ICE_IN , "uu_read"    , "",
				ICE_IN , "uu_insert"  , "",
				0      , NULL         ,  NULL
			);
			if ((status = ICE_C_Execute (ICEclient, "unit_user", del)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			break;
		}
		case OT_ICE_BUNIT_FACET:
		case OT_ICE_BUNIT_PAGE:
		{
			LPICEBUSUNITDOCDATA lpIceData = (LPICEBUSUNITDOCDATA)lpparameters;
			ICE_C_PARAMS del[8];
			FillIceParamsArray( del,
				ICE_IN , "action"          , "delete" ,
				ICE_IN, "doc_id"           , lpIceData->doc_ID         ,
//				ICE_IN, "doc_type"         , lpIceData->icewevuser.ID  ,
				ICE_IN, "doc_unit_id"      , lpIceData->icebusunit.ID  ,
				0     , NULL               ,  NULL 
			);
			if ((status = ICE_C_Execute (ICEclient, "document", del)) != OK) {
				ManageIceError(&ICEclient, &status);
				iresult=RES_ERR;
			}
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			break;
		}
		case OT_ICE_BUNIT_FACET_ROLE:
		case OT_ICE_BUNIT_PAGE_ROLE:
		{
			LPICEBUSUNITDOCROLEDATA lpIceData = (LPICEBUSUNITDOCROLEDATA)lpparameters;
			ICE_C_PARAMS del[10];
			FillIceParamsArray( del,
				ICE_IN , "action"     , "update" ,
				ICE_IN , "dr_doc_id"  , lpIceData->icebusunitdoc.doc_ID,
				ICE_IN , "dr_role_id" , lpIceData->icebusunitdocrole.ID, 
				ICE_IN , "dr_execute" , ""   ,
				ICE_IN , "dr_read"    , ""   ,
				ICE_IN , "dr_update"  , ""   ,
				ICE_IN , "dr_delete"  , ""   ,
				0      ,  NULL        , NULL 
			);
			if ((status = ICE_C_Execute (ICEclient, "document_role", del)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_BUNIT_FACET_USER:
		case OT_ICE_BUNIT_PAGE_USER:
		{
			LPICEBUSUNITDOCUSERDATA lpIceData = (LPICEBUSUNITDOCUSERDATA)lpparameters;
			ICE_C_PARAMS del[10];
			FillIceParamsArray( del,
				ICE_IN , "action"     , "update",
				ICE_IN , "du_doc_id"  , lpIceData->icebusunitdoc.doc_ID,
				ICE_IN , "du_user_id" , lpIceData->user.ID, 
				ICE_IN , "du_execute" , "" ,
				ICE_IN , "du_read"    , "" ,
				ICE_IN , "du_update"  , "" ,
				ICE_IN , "du_delete"  , "" ,
				0      , NULL         , NULL 
			);
			if ((status = ICE_C_Execute (ICEclient, "document_user", del)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
		}
		break;

		case OT_ICE_BUNIT_LOCATION:
		{
			LPICEBUSUNITLOCATIONDATA lpIceData = (LPICEBUSUNITLOCATIONDATA)lpparameters;
			ICE_C_PARAMS del[6];
			FillIceParamsArray( del,
				ICE_IN , "action"    , "delete" ,
				ICE_IN, "ul_unit_id"       , lpIceData->icebusunit.ID   ,
				ICE_IN, "ul_location_id"   , lpIceData->icelocation.ID  ,
//				ICE_IN, "ul_location_name" , lpIceData->??,
				0      , NULL        ,  NULL 
			);
			if ((status = ICE_C_Execute (ICEclient, "unit_location", del)) != OK) 
				iresult=RES_ERR;
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			break;
		}
		default:
			TS_MessageBox(TS_GetFocus(),
				ResourceString(IDS_ERR_UNKNOWN_ICE_TYPE),
				NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
			iresult=RES_ERR;
	}
	if ((status =ICE_C_Disconnect (&ICEclient))!=OK) {
		ManageIceError(&ICEclient, &status);
		iresult=RES_ERR;
	}
	RemoveHourGlass();
	return iresult;
#else
	return RES_SUCCESS;
#endif
}



int ICE_FillTempFileFromDoc(LPUCHAR lpVirtNode,ICEBUSUNITDOCDATA * lpdocdata,LPUCHAR lpbufresult)
{
#ifndef NOUSEICEAPI
	TCHAR			bufuser[100],bufpw[100];
	ICE_STATUS		status = 0;
	ICE_C_CLIENT	ICEclient = NULL;
	int iresult = RES_SUCCESS;

	x_strcpy(lpbufresult,"");

	if (!lpdocdata || !lpVirtNode) {
		myerror(ERR_INVPARAMS);
		return RES_ERR;
	}
	if (GetICeUserAndPassWord(GetVirtNodeHandle(lpVirtNode),bufuser,bufpw)!=RES_SUCCESS)
		return RES_ERR;
	if ((status =ICE_C_Initialize ()) != OK) {
		ManageIceError(&ICEclient, &status);
		return RES_ERR;
	}
	if ((status = ICE_C_Connect (CAPINodeName(lpVirtNode), bufuser,bufpw, &ICEclient)) != OK) {
		ManageIceError(&ICEclient, &status);
		return RES_ERR;
	}

	ShowHourGlass();

	{
		char TmpFileName[_MAX_PATH];
		char FileName[100];
		ICE_C_PARAMS docdownload[8];
			FillIceParamsArray( docdownload,
			ICE_IN	,	"unit"		,	lpdocdata->icebusunit.Name	,
			ICE_IN	,	"document"	,	FileName					,
			ICE_IN	,	"target"	,	TmpFileName					,
			0		,	NULL		,	NULL						
		);
		if (x_strcmp(lpdocdata->suffix,"")!=0)
			wsprintf(FileName,"%s.%s", lpdocdata->name,lpdocdata->suffix);
		else
			x_strcpy(FileName,lpdocdata->name);

		if (ESL_GetTempFileName(TmpFileName, sizeof(TmpFileName))!=RES_SUCCESS) 
			iresult =RES_ERR;
		else {
			if ((status = ICE_C_Execute (ICEclient, "download", docdownload)) != OK) 
				iresult=RES_ERR;
			else 
				x_strcpy(lpbufresult,TmpFileName);
		}
		ManageIceError(&ICEclient, &status);
		ICE_C_Close (ICEclient);
	}

	if ((status=ICE_C_Disconnect (&ICEclient))!=OK) {
		ManageIceError(&ICEclient, &status);
		if (x_strcmp(lpbufresult,"")!=0) {
			ESL_RemoveFile(lpbufresult);
			x_strcpy(lpbufresult,"");
		}
		iresult=RES_ERR;
	}

	RemoveHourGlass();
	return iresult;
#else
	return RES_SUCCESS;
#endif
}

BOOL IsIceLoginPasswordValid(TCHAR *lpVirtNode, TCHAR *lpUserName,
							 TCHAR *lpPasswd, TCHAR *tcErrBuf, UINT iBufLen)
{
#ifndef NOUSEICEAPI
	ICE_STATUS   status = 0;
	ICE_C_CLIENT ICEclient = NULL;
	
	x_strcpy(tcErrBuf,"");

	if ((status =ICE_C_Initialize ()) != OK)
	{
		char * p1 = ICE_C_LastError(*(&ICEclient));
		if (p1)
		{
			if (x_strlen(p1) >= iBufLen)
				fstrncpy(tcErrBuf,p1,iBufLen);
			else
				x_strcpy(tcErrBuf,p1);
		}
		return FALSE;
	}

	if ((status = ICE_C_Connect (CAPINodeName(lpVirtNode), lpUserName,lpPasswd, &ICEclient)) != OK)
	{
		char * p1 = ICE_C_LastError(*(&ICEclient));
		if (p1)
		{
			if (x_strlen(p1) >= iBufLen)
				fstrncpy(tcErrBuf,p1,iBufLen);
			else
				x_strcpy(tcErrBuf,p1);
		}
		return FALSE;
	}

	if ((status =ICE_C_Disconnect (&ICEclient))!=OK) 
		ManageIceError(&ICEclient, &status);

	return TRUE;
#else
	return FALSE;
#endif
}
