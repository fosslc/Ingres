/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project : Ingres Visual DBA
//
//    Source : ice.h
//    ice 2.0 get/add/alter/drop ICE objects
//
********************************************************************/

#ifndef VDBA_ICE_HEADER
#define VDBA_ICE_HEADER

#include "dba.h" // required for LPUCHAR definition
#include "ice_c_api.h"

typedef struct IceLocationdata { 

	UCHAR ID [100];

	UCHAR LocationName		[32+1];	// input for GetICEInfo()
	BOOL bIce;						// FALSE => HTTP location
	BOOL bPublic;

	UCHAR path				[256];
	UCHAR extensions		[256];

	UCHAR Comment			[100];
	BOOL bAlter;			// TRUE for Alter

} ICELOCATIONDATA, FAR * LPICELOCATIONDATA;

typedef struct IceRoledata { 

	UCHAR ID [100];

	UCHAR RoleName		[32+1];// input for GetICEInfo()

	UCHAR Comment		[100];

	BOOL bAlter;				// TRUE for Alter

} ICEROLEDATA, FAR * LPICEROLEDATA;

typedef struct ICEDBuserdata { 

	UCHAR ID [100];

	UCHAR User_Name		[32+1];
	UCHAR UserAlias		[32+1]; // displayed information in the tree

	UCHAR Password		[32+1];

	UCHAR Comment		[100];

	BOOL bAlter;				// TRUE for Alter

} ICEDBUSERDATA, FAR * LPICEDBUSERDATA;

typedef struct DBConnection { 

	UCHAR ID [100];

	UCHAR ConnectionName	[32+1];// input for GetICEInfo()
	UCHAR DBName			[32+1];

	ICEDBUSERDATA DBUsr;

	UCHAR Comment		[100];

	BOOL bAlter;				// TRUE for Alter

} DBCONNECTIONDATA, FAR * LPDBCONNECTIONDATA;

typedef struct ICEWebuserdata { 

	UCHAR ID [100];

	UCHAR UserName		[32+1];// input for GetICEInfo()

	ICEDBUSERDATA DefDBUsr;

	BOOL  bICEpwd;   // FALSE: OS password. TRUE: ICE (repository) password

	UCHAR Password		[32+1];

	UCHAR Comment		[100];

	BOOL bAdminPriv;
	BOOL bSecurityPriv;
	BOOL bUnitMgrPriv;
	BOOL bMonitorPriv;

	long ltimeoutms;
	BOOL bAlter;				// TRUE for Alter

} ICEWEBUSERDATA, FAR * LPICEWEBUSERDATA;


typedef struct ICEWebuserRoledata { 
	ICEWEBUSERDATA icewebuser;
	ICEROLEDATA    icerole;
	BOOL bAlter;				// TRUE for Alter
} ICEWEBUSERROLEDATA, FAR * LPICEWEBUSERROLEDATA;

typedef struct ICEWebuserConnectiondata { 
	ICEWEBUSERDATA icewebuser;
	DBCONNECTIONDATA icedbconnection;
	BOOL bAlter;				// TRUE for Alter
} ICEWEBUSERCONNECTIONDATA, FAR * LPICEWEBUSERCONNECTIONDATA;


typedef struct IceProfiledata { 

	UCHAR ID [100];

	UCHAR ProfileName		[32+1];// input for GetICEInfo()

	ICEDBUSERDATA DefDBUsr;

	UCHAR Comment[100];

	BOOL bAdminPriv;
	BOOL bSecurityPriv;
	BOOL bUnitMgrPriv;
	BOOL bMonitorPriv;

	long ltimeoutms;
	BOOL bAlter;				// TRUE for Alter 

} ICEPROFILEDATA, FAR * LPICEPROFILEDATA;

typedef struct ICEProfileRoledata { 
	ICEPROFILEDATA iceprofile;
	ICEROLEDATA    icerole;
	BOOL bAlter;				// TRUE for Alter
} ICEPROFILEROLEDATA, FAR * LPICEPROFILEROLEDATA;

typedef struct ICEProfileConnectiondata { 
	ICEPROFILEDATA iceprofile;
	DBCONNECTIONDATA icedbconnection;
	BOOL bAlter;				// TRUE for Alter
} ICEPROFILECONNECTIONDATA, FAR * LPICEPROFILECONNECTIONDATA;

typedef struct IceBusUnitdata { 

	UCHAR ID [100];

	UCHAR Name		[32+1];// input for GetICEInfo()

	UCHAR Owner		[100];

} ICESBUSUNITDATA, FAR * LPICEBUSUNITDATA;

typedef struct ICEBusUnitRoledata { 
	ICESBUSUNITDATA icebusunit;
	ICEROLEDATA    icerole;
	BOOL bExecDoc;
	BOOL bCreateDoc;
	BOOL bReadDoc;
	BOOL bAlter;				// TRUE for Alter
} ICEBUSUNITROLEDATA, FAR * LPICEBUSUNITROLEDATA;

typedef struct ICEBusUnitWebUserdata { 
	ICESBUSUNITDATA icebusunit;
	ICEWEBUSERDATA  icewevuser;
	BOOL bUnitRead;
	BOOL bCreateDoc;
	BOOL bReadDoc;
	BOOL bAlter;				// TRUE for Alter
} ICEBUSUNITWEBUSERDATA, FAR * LPICEBUSUNITWEBUSERDATA;

typedef struct ICEBusUnitDocdata { // both for pages and facets
	ICESBUSUNITDATA icebusunit;

	UCHAR doc_ID [100];

	BOOL bFacet; // otherwise "page"
	TCHAR name[32+1];
	TCHAR suffix[10+1];
	BOOL  bPublic;
	BOOL  bpre_cache;
	BOOL  bperm_cache;
	BOOL  bsession_cache;
	TCHAR doc_file[100];
	ICELOCATIONDATA ext_loc;
	TCHAR ext_file[32+1];
	TCHAR ext_suffix[10+1];
	TCHAR doc_owner[32+1];
	BOOL btransfer;

	BOOL bAlter;				// TRUE for Alter
} ICEBUSUNITDOCDATA, FAR * LPICEBUSUNITDOCDATA;

typedef struct ICEBusUnitLocationdata { // both for pages and facets
	ICESBUSUNITDATA icebusunit;
	ICELOCATIONDATA icelocation;

	BOOL bAlter;				// TRUE for Alter
} ICEBUSUNITLOCATIONDATA, FAR * LPICEBUSUNITLOCATIONDATA;

typedef struct ICEBusUnitDocRole {
	ICEBUSUNITDOCDATA icebusunitdoc;
	ICEROLEDATA    icebusunitdocrole;

	BOOL bExec;
	BOOL bRead;
	BOOL bUpdate;
	BOOL bDelete;

	BOOL bAlter;				// TRUE for Alter
} ICEBUSUNITDOCROLEDATA, FAR * LPICEBUSUNITDOCROLEDATA;

typedef struct ICEBusUnitDocUser {
	ICEBUSUNITDOCDATA icebusunitdoc;
	ICEWEBUSERDATA  user;

	BOOL bExec;
	BOOL bRead;
	BOOL bUpdate;
	BOOL bDelete;

	BOOL bAlter;				// TRUE for Alter
} ICEBUSUNITDOCUSERDATA, FAR * LPICEBUSUNITDOCUSERDATA;


typedef struct IceApplicationdata { 

	UCHAR ID [100];

	UCHAR Name		[32+1];// input for GetICEInfo()

} ICEAPPLICATIONDATA, FAR * LPICEAPPLICATIONDATA;


typedef struct IceSvrVardata { 

	UCHAR ID [100];

	UCHAR VariableName		[32+1];// input for GetICEInfo()

	UCHAR VariableValue		[100];
	BOOL bAlter;			// TRUE for Alter

} ICESERVERVARDATA, FAR * LPICESERVERVARDATA;

typedef struct IcePassWorddata { 

	TCHAR tchUserName			[32+1];
	TCHAR tchPassword			[32+1];

} ICEPASSWORDDATA, FAR * LPICEPASSWORDDATA;

int GetICEInfo  (LPUCHAR lpVirtNode,int iobjecttype, void *lpparameters);
int ICEAddobject(LPUCHAR lpVirtNode,int iobjecttype, void *lpparameters);
int ICEAlterobject(LPUCHAR lpVirtNode,int iobjecttype, void *lpoldparameters,void *lpnewparameters);
int ICEDropobject(LPUCHAR lpVirtNode,int iobjecttype, void *lpparameters);
LPUCHAR CAPINodeName(LPUCHAR lpVirtNode);
void ManageIceError(ICE_C_CLIENT * pICEclient, ICE_STATUS * pstatus);
int ICE_FillTempFileFromDoc(LPUCHAR lpVirtNode,ICEBUSUNITDOCDATA * lpdocdata,LPUCHAR lpbufresult);
BOOL IsIceLoginPasswordValid(TCHAR *lpVirtNode, TCHAR *lpUserName, TCHAR *lpPasswd, TCHAR *tcErrBuf, UINT iBufLen);

#endif  /* #define VDBA_ICE_HEADER */
