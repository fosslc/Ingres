/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : libguids.h : include file for CLSID and IID,
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : QUIDS Management
**
** History:
**
** 29-Nov-2000 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
**/


#if !defined (ICASGUIDS_HEADER)
#define ICASGUIDS_HEADER
#if defined (__cplusplus)
extern "C"{
#endif 

//
// ICAS COM Server MTA Apartment:
// ************************************************************************************************
//
// ICAS COM Server MTA Apartment:
// {2185DF31-C790-11d4-872D-00C04F1F754A}
DEFINE_GUID(CLSID_ICASServer,
	0x2185DF31, 0xC790, 0x11d4, 0x87, 0x2D, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A);
//
// String in Registry format: {2185DF31-C790-11d4-872D-00C04F1F754A}
extern LPCTSTR cstrCLSID_ICASServer; 

//
// Interface IQueryObject:
// {2185DF32-C790-11d4-872D-00C04F1F754A}
DEFINE_GUID(IID_IQueryObject,
	0x2185DF32, 0xC790, 0x11d4, 0x87, 0x2D, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A);
//
// String in Registry format: {2185DF32-C790-11d4-872D-00C04F1F754A}
extern LPCTSTR cstrIID_IQueryObject;

//
// Interface IExecSQL:
// {2185DF33-C790-11d4-872D-00C04F1F754A}
DEFINE_GUID(IID_IExecSQL,
	0x2185DF33, 0xC790, 0x11d4, 0x87, 0x2D, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A);
//
// String in Registry format: {2185DF33-C790-11d4-872D-00C04F1F754A}
extern LPCTSTR cstrIID_IExecSQL;

//
// Interface IQueryDetail:
// {2185DF34-C790-11d4-872D-00C04F1F754A}
DEFINE_GUID(IID_IQueryDetail,
	0x2185DF34, 0xC790, 0x11d4, 0x87, 0x2D, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A);
//
// String in Registry format: {2185DF34-C790-11d4-872D-00C04F1F754A}
extern LPCTSTR cstrIID_IQueryDetail;


//
// Interface IClientStateNotification:
// {2185DF35-C790-11d4-872D-00C04F1F754A}
DEFINE_GUID(IID_IClientStateNotification,
	0x2185DF35, 0xC790, 0x11d4, 0x87, 0x2D, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A);
//
// String in Registry format: {2185DF35-C790-11d4-872D-00C04F1F754A}
extern LPCTSTR cstrIID_IClientStateNotification;


//
// Interface IRmcmd:
// {2185DF36-C790-11d4-872D-00C04F1F754A}
DEFINE_GUID(IID_IRmcmd,
	0x2185DF36, 0xC790, 0x11d4, 0x87, 0x2D, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A);
//
// String in Registry format: {2185DF36-C790-11d4-872D-00C04F1F754A}
extern LPCTSTR cstrIID_IRmcmd;

//
// FUTURE INTERFACES:
// {2185DF37-C790-11d4-872D-00C04F1F754A}
// {2185DF38-C790-11d4-872D-00C04F1F754A}
// {2185DF39-C790-11d4-872D-00C04F1F754A}

//
// Interface ISinkDataChange:
// {2185DF3A-C790-11d4-872D-00C04F1F754A}
DEFINE_GUID(IID_ISinkDataChange,     0x2185DF3A, 0xC790, 0x11d4, 0x87, 0x2D, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A);
//
// String in Registry format: {2185DF3A-C790-11d4-872D-00C04F1F754A}
extern LPCTSTR cstrIID_ISinkDataChange;




//
// SDCN COM Server MTA Apartment:
// {2185DF3B-C790-11d4-872D-00C04F1F754A}
DEFINE_GUID(CLSID_SDCNServer,
	0x2185DF3B, 0xC790, 0x11d4, 0x87, 0x2D, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A);
//
// String in Registry format: {2185DF3B-C790-11d4-872D-00C04F1F754A}
extern LPCTSTR cstrCLSID_SDCNServer;

//
// Interface ISourceSDCN:
// {2185DF3C-C790-11d4-872D-00C04F1F754A}
DEFINE_GUID(IID_ISourceSDCN,
	0x2185DF3C, 0xC790, 0x11d4, 0x87, 0x2D, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A);
//
// String in Registry format: {2185DF3C-C790-11d4-872D-00C04F1F754A}
extern LPCTSTR cstrIID_ISourceSDCN;

//
// FUTURE INTERFACES:
// {2185DF3D-C790-11d4-872D-00C04F1F754A}




//
// IMPORT & EXPORT ASSISTANT:
// ************************************************************************************************
//{57CE8441-CF4E-11d4-872E-00C04F1F754A}
DEFINE_GUID(CLSID_IMPxEXPxASSISTANCT,
	0x57CE8441, 0xCF4E, 0x11d4, 0x87, 0x2E, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A);
//
// String in Registry format: {57CE8441-CF4E-11d4-872E-00C04F1F754A}
extern LPCTSTR cstrCLSID_IMPxEXPxASSISTANCT;

//
// Interface IImportAssistant:
// {57CE8442-CF4E-11d4-872E-00C04F1F754A}
DEFINE_GUID(IID_IImportAssistant,
	0x57CE8442, 0xCF4E, 0x11d4, 0x87, 0x2E, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A);
//
// String in Registry format: {57CE8442-CF4E-11d4-872E-00C04F1F754A}
extern LPCTSTR cstrIID_IImportAssistant;
//
// Interface IExportAssistant:
// {57CE8443-CF4E-11d4-872E-00C04F1F754A}
DEFINE_GUID(IID_IExportAssistant,
	0x57CE8443, 0xCF4E, 0x11d4, 0x87, 0x2E, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A);
//
// String in Registry format: {57CE8443-CF4E-11d4-872E-00C04F1F754A}
extern LPCTSTR cstrIID_IExportAssistant;


//
// SQL ASSISTANT:
// ************************************************************************************************
#if defined (EDBC)
//{FC918E94-E9DC-4497-AD7E-7BF28DF7FD14}
DEFINE_GUID(CLSID_SQLxASSISTANCT,
	0xfc918e94, 0xe9dc, 0x4497, 0xad, 0x7e, 0x7b, 0xf2, 0x8d, 0xf7, 0xfd, 0x14);
#else
//{57CE8444-CF4E-11d4-872E-00C04F1F754A}
DEFINE_GUID(CLSID_SQLxASSISTANCT,
	0x57CE8444, 0xCF4E, 0x11d4, 0x87, 0x2E, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A);
#endif
//
// String in Registry format: {57CE8444-CF4E-11d4-872E-00C04F1F754A}
//                            {FC918E94-E9DC-4497-AD7E-7BF28DF7FD14} for EDBC
extern LPCTSTR cstrCLSID_SQLxASSISTANCT;

//
// Interface ISqlAssistant:
#if defined (EDBC)
// {B053C09F-D831-4f4b-9929-21903C0230C3}
DEFINE_GUID(IID_ISqlAssistant,
	0xb053c09f, 0xd831, 0x4f4b, 0x99, 0x29, 0x21, 0x90, 0x3c, 0x02, 0x30, 0xc3);
#else
// {57CE8445-CF4E-11d4-872E-00C04F1F754A}
DEFINE_GUID(IID_ISqlAssistant,
	0x57CE8445, 0xCF4E, 0x11d4, 0x87, 0x2E, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A);
#endif
//
// String in Registry format: {57CE8445-CF4E-11d4-872E-00C04F1F754A}
//                            {B053C09F-D831-4f4b-9929-21903C0230C3} for EDBC
extern LPCTSTR cstrIID_ISqlAssistant;


//
// Add/Alter/Drop Dialog (In-Process Server):
// ************************************************************************************************
//{D0338860-8264-11d5-8760-00C04F1F754A}
DEFINE_GUID(CLSID_DIALOGxADDxALTERxDROP,
	0xD0338860, 0x8264, 0x11d5, 0x87, 0x60, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A);
//
// String in Registry format: {D0338860-8264-11d5-8760-00C04F1F754A}
extern LPCTSTR cstrCLSID_DIALOGxADDxALTERxDROP;

//
// Interface IAdd:
// {D0338861-8264-11d5-8760-00C04F1F754A}
DEFINE_GUID(IID_IAdd,
	0xD0338861, 0x8264, 0x11d5, 0x87, 0x60, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A);
//
// String in Registry format: {D0338861-8264-11d5-8760-00C04F1F754A}
extern LPCTSTR cstrIID_IAdd;
//
// Interface IAlter:
// {D0338862-8264-11d5-8760-00C04F1F754A}
DEFINE_GUID(IID_IAlter,
	0xD0338862, 0x8264, 0x11d5, 0x87, 0x60, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A);
//
// String in Registry format: {D0338862-8264-11d5-8760-00C04F1F754A}
extern LPCTSTR cstrIID_IAlter;

//
// Interface IAlter:
// {D0338863-8264-11d5-8760-00C04F1F754A}
DEFINE_GUID(IID_IDrop,
	0xD0338863, 0x8264, 0x11d5, 0x87, 0x60, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A);
//
// String in Registry format: {D0338863-8264-11d5-8760-00C04F1F754A}
extern LPCTSTR cstrIID_IDrop;

/*
{D0338864-8264-11d5-8760-00C04F1F754A}
{D0338865-8264-11d5-8760-00C04F1F754A}
{D0338866-8264-11d5-8760-00C04F1F754A}
{D0338867-8264-11d5-8760-00C04F1F754A}
{D0338868-8264-11d5-8760-00C04F1F754A}
{D0338869-8264-11d5-8760-00C04F1F754A}
{D033886A-8264-11d5-8760-00C04F1F754A}
*/



//
// ************************************************************************************************
// Register Funtions:

//
// Check to see if CLSID is registered under the
// HKEY_CLASSES_ROOT\CLSID
BOOL LIBGUIDS_IsCLSIDRegistered(LPCTSTR lpszCLSID);
//
// lpszServerName: Dll name, ex: svriia.dll, ijactrl.ocx, ...
// lpszPath      : If this parameter is NULL, the function will use
//                 II_SYSTEM\ingres\vdba as path name.
// return:
//     0: successful.
//     1: fail
//     2: fail because of II_SYSTEM is not defined.
int LIBGUIDS_RegisterServer(LPCTSTR lpszServerName, LPCTSTR lpszPath);

#if defined (__cplusplus)
}
#endif 


#endif // ICASGUIDS_HEADER
