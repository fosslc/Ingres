/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : compbase.h , Header File
**
**  Project  : Ingres II / Visual Manager
**
**  Author   : Sotheavut UK (uk$so01)
**
**  Purpose  : Manipulation Ingres Visual Manager Data
**
**  History:
** 13-Jan-1999 (uk$so01)
**    Created.
** 04-feb-2000 (somsa01)
**    Took out extra "," in  Corner icons indicator.
** 03-Mar-2000 (uk$so01)
**    SIR #100706. Provide the different context help id.
** 08-Mar-2000 (uk$so01)
**    SIR #100706. Provide the different context help id.
**    Use the specific defined help ID instead of calculating.
**    Handle the generic folder of gateways, 
** 18-Jan-2001 (uk$so01)
**    SIR #103727 (Make IVM support JDBC Server)
**    Add image indexes and the order position to display this JDBC Server
** 20-Aug-2001 (uk$so01)
**    Fix BUG #105382, Incorrectly notify an alert message when there are no 
**    events displayed in the list.
** 09-Mar-2004 (uk$so01)
**    SIR #110013 (Handle the DAS Server)
*/

#if !defined(COMPBASE_HEADER)
#define COMPBASE_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//
// Information about the query of events:
class CaQueryEventInformation
{
public:
	//
	// Constructor:
	CaQueryEventInformation(): 
		m_bLimitReached(FALSE), 
		m_lExceed(0), 
		m_lFirstEvent(-1), 
		m_lLastEvent(-1),
		m_nSequence(-1)
	{

	}
	//
	// Destructor:
	~CaQueryEventInformation(){}

	void SetLimitReach(BOOL bReach = TRUE){m_bLimitReached = bReach;}
	void SetExceed(long lExceed){m_lExceed = lExceed;}
	void SetFirstEventId(long lf){m_lFirstEvent = lf;}
	void SetLastEventId(long ll){m_lLastEvent = ll;}
	void SetBlockSequence(int nSeq){m_nSequence = nSeq;}
	long GetExceed(){return m_lExceed;}
	BOOL GetLimitReach(){return m_bLimitReached;}
	long GetFirstEventId(){return m_lFirstEvent;}
	long GetLastEventId(){return m_lLastEvent;}
	int  GetBlockSequence(){return m_nSequence;}

protected:
	BOOL m_bLimitReached; // TRUE: when the limit of events in memory has been reached
	long m_lExceed;       // 0 : not exceed, 
	                      // +n: exceed of n events
	                      // -n: exceed of n Bytes.

	long m_lFirstEvent;   // the first read of event identifier of the last query operation.
	long m_lLastEvent;    // the last read of event identifier of the last query operation.
	//
	// The Thread will query events and post/send events to the Mainframe by block.
	// If there are events in each block that need to be alerted, then do alert one time per thread
	// activity (begin read event ... end read event).
	// Thus the 'm_nSequence' will indecate the Mainframe when it receives the events
	// on the handler of message 'WMUSRMSG_UPDATE_EVENT':
	int  m_nSequence;     // 0: begin block, 1: next block, -1: end of read.
};


//
// Store the TreeItem State along with the TreeData:
class CaTreeCtrlData
{
public:
	enum ItemState {ITEM_DELETE = -1, ITEM_EXIST, ITEM_NEW};
	CaTreeCtrlData();
	CaTreeCtrlData(ItemState nFlag, HTREEITEM hTreeItem);
	~CaTreeCtrlData(){}

	ItemState GetState(){return m_nItemFlag;}
	void SetState(ItemState nFlag){m_nItemFlag = nFlag;}
	HTREEITEM GetTreeItem (){return m_hTreeItem;}
	void SetTreeItem (HTREEITEM hTreeItem){m_hTreeItem = hTreeItem;}

	BOOL IsExpanded(){return m_bExpanded;}
	void SetExpand(BOOL bExpanded){m_bExpanded = bExpanded;};

	int GetImage(){return m_nImage;}
	int GetImageExpanded(){return m_nExpandedImage;}
	void SetImage (int nImage, int nExpandedImage)
	{
		m_nImage = nImage;
		m_nExpandedImage = nExpandedImage;
	}

	BOOL IsRead() {return m_bRead;}
	void SetRead(BOOL bRead){m_bRead = bRead;}

	//
	// Important event management:
	void AlertInc(){m_nAlertCount++;}
	void AlertDec(){if (m_nAlertCount > 0) m_nAlertCount--;}
	void AlertZero(){m_nAlertCount = 0;}
	int  AlertGetCount(){return m_nAlertCount;}

	void AlertMarkUpdate (CTreeCtrl* pTree, BOOL bInstance = FALSE);


	UINT GetComponentState (){return m_nState;}
	void SetComponentState (UINT nState){m_nState = nState;}


protected:
	UINT m_nState;

private:
	//
	// When the tree item is created, it has a 'm_nItemFlag' set to ITEM_NEW.
	// When the thee item is displayed in TreeCtrl, it has a 'm_nItemFlag' set to ITEM_EXIST.
	// Before refreshin the Tree, all tree items have 'm_nItemFlag' initialized to ITEM_DELETE.
	// After requering the data: the tree items will have the following flags:
	//   ITEM_DELETE: the tree item must be delete:
	//   ITEM_EXIST : the tree item exist already (its properties changed ?)
	//   ITEM_NEW   : the tree item must be added.
	ItemState m_nItemFlag;

	HTREEITEM m_hTreeItem; // Handle of Tree Item 
	BOOL m_bExpanded;      // Expanded / Collapsed

	BOOL m_bRead;          // TRUE: Read Message. FALSE: Not Read Message
	int  m_nActivity;      // 0: Stopped. 1: Started. 2: Half Started

	int m_nImage;
	int m_nExpandedImage;
	int m_nAlertCount;       // > 0 (Alert mark is checked)

};



#define IM_WIDTH     32
#define IM_HEIGHT    16
enum 
{
	IM_FOLDER_CLOSED_R_NORMAL = 0,
	IM_FOLDER_CLOSED_R_STOPPED,
	IM_FOLDER_CLOSED_R_STARTED,
	IM_FOLDER_CLOSED_R_HALFSTARTED,
	IM_FOLDER_CLOSED_R_STARTEDMORE,
	IM_FOLDER_CLOSED_N_NORMAL,
	IM_FOLDER_CLOSED_N_STOPPED,
	IM_FOLDER_CLOSED_N_STARTED,
	IM_FOLDER_CLOSED_N_HALFSTARTED,
	IM_FOLDER_CLOSED_N_STARTEDMORE,

	IM_FOLDER_OPENED_R_NORMAL,
	IM_FOLDER_OPENED_R_STOPPED,
	IM_FOLDER_OPENED_R_STARTED,
	IM_FOLDER_OPENED_R_HALFSTARTED,
	IM_FOLDER_OPENED_R_STARTEDMORE,
	IM_FOLDER_OPENED_N_NORMAL,
	IM_FOLDER_OPENED_N_STOPPED,
	IM_FOLDER_OPENED_N_STARTED,
	IM_FOLDER_OPENED_N_HALFSTARTED,
	IM_FOLDER_OPENED_N_STARTEDMORE,

	IM_INSTALLATION_R_NORMAL,
	IM_INSTALLATION_R_STOPPED,
	IM_INSTALLATION_R_STARTED,
	IM_INSTALLATION_R_HALFSTARTED,
	IM_INSTALLATION_R_STARTEDMORE,
	IM_INSTALLATION_N_NORMAL,
	IM_INSTALLATION_N_STOPPED,
	IM_INSTALLATION_N_STARTED,
	IM_INSTALLATION_N_HALFSTARTED,
	IM_INSTALLATION_N_STARTEDMORE,

	IM_BRIDGE_R_NORMAL,
	IM_BRIDGE_R_STOPPED,
	IM_BRIDGE_R_STARTED,
	IM_BRIDGE_R_HALFSTARTED,
	IM_BRIDGE_R_STARTEDMORE,
	IM_BRIDGE_N_NORMAL,
	IM_BRIDGE_N_STOPPED,
	IM_BRIDGE_N_STARTED,
	IM_BRIDGE_N_HALFSTARTED,
	IM_BRIDGE_N_STARTEDMORE,
	IM_BRIDGE_R_INSTANCE,
	IM_BRIDGE_N_INSTANCE,

	IM_DBMS_R_NORMAL,
	IM_DBMS_R_STOPPED,
	IM_DBMS_R_STARTED,
	IM_DBMS_R_HALFSTARTED,
	IM_DBMS_R_STARTEDMORE,
	IM_DBMS_N_NORMAL,
	IM_DBMS_N_STOPPED,
	IM_DBMS_N_STARTED,
	IM_DBMS_N_HALFSTARTED,
	IM_DBMS_N_STARTEDMORE,
	IM_DBMS_R_INSTANCE,
	IM_DBMS_N_INSTANCE,

	IM_NET_R_NORMAL,
	IM_NET_R_STOPPED,
	IM_NET_R_STARTED,
	IM_NET_R_HALFSTARTED,
	IM_NET_R_STARTEDMORE,
	IM_NET_N_NORMAL,
	IM_NET_N_STOPPED,
	IM_NET_N_STARTED,
	IM_NET_N_HALFSTARTED,
	IM_NET_N_STARTEDMORE,
	IM_NET_R_INSTANCE,
	IM_NET_N_INSTANCE,

	IM_STAR_R_NORMAL,
	IM_STAR_R_STOPPED,
	IM_STAR_R_STARTED,
	IM_STAR_R_HALFSTARTED,
	IM_STAR_R_STARTEDMORE,
	IM_STAR_N_NORMAL,
	IM_STAR_N_STOPPED,
	IM_STAR_N_STARTED,
	IM_STAR_N_HALFSTARTED,
	IM_STAR_N_STARTEDMORE,
	IM_STAR_R_INSTANCE,
	IM_STAR_N_INSTANCE,

	IM_ICE_R_NORMAL,
	IM_ICE_R_STOPPED,
	IM_ICE_R_STARTED,
	IM_ICE_R_HALFSTARTED,
	IM_ICE_R_STARTEDMORE,
	IM_ICE_N_NORMAL,
	IM_ICE_N_STOPPED,
	IM_ICE_N_STARTED,
	IM_ICE_N_HALFSTARTED,
	IM_ICE_N_STARTEDMORE,
	IM_ICE_R_INSTANCE,
	IM_ICE_N_INSTANCE,

	IM_GATEWAY_R_NORMAL,
	IM_GATEWAY_R_STOPPED,
	IM_GATEWAY_R_STARTED,
	IM_GATEWAY_R_HALFSTARTED,
	IM_GATEWAY_R_STARTEDMORE,
	IM_GATEWAY_N_NORMAL,
	IM_GATEWAY_N_STOPPED,
	IM_GATEWAY_N_STARTED,
	IM_GATEWAY_N_HALFSTARTED,
	IM_GATEWAY_N_STARTEDMORE,
	IM_GATEWAY_R_INSTANCE,
	IM_GATEWAY_N_INSTANCE,

	IM_NAME_R_NORMAL,
	IM_NAME_R_STOPPED,
	IM_NAME_R_STARTED,
	IM_NAME_R_HALFSTARTED,
	IM_NAME_R_STARTEDMORE,
	IM_NAME_N_NORMAL,
	IM_NAME_N_STOPPED,
	IM_NAME_N_STARTED,
	IM_NAME_N_HALFSTARTED,
	IM_NAME_N_STARTEDMORE,
	IM_NAME_R_INSTANCE,
	IM_NAME_N_INSTANCE,

	IM_RMCMD_R_NORMAL,
	IM_RMCMD_R_STOPPED,
	IM_RMCMD_R_STARTED,
	IM_RMCMD_R_HALFSTARTED,
	IM_RMCMD_R_STARTEDMORE,
	IM_RMCMD_N_NORMAL,
	IM_RMCMD_N_STOPPED,
	IM_RMCMD_N_STARTED,
	IM_RMCMD_N_HALFSTARTED,
	IM_RMCMD_N_STARTEDMORE,
	IM_RMCMD_R_INSTANCE,
	IM_RMCMD_N_INSTANCE,
	
	IM_SECURITY_R_NORMAL,
	IM_SECURITY_R_STOPPED,
	IM_SECURITY_R_STARTED,
	IM_SECURITY_R_HALFSTARTED,
	IM_SECURITY_R_STARTEDMORE,
	IM_SECURITY_N_NORMAL,
	IM_SECURITY_N_STOPPED,
	IM_SECURITY_N_STARTED,
	IM_SECURITY_N_HALFSTARTED,
	IM_SECURITY_N_STARTEDMORE,
	IM_SECURITY_R_INSTANCE,
	IM_SECURITY_N_INSTANCE,

	IM_LOCKING_R_NORMAL,
	IM_LOCKING_R_STOPPED,
	IM_LOCKING_R_STARTED,
	IM_LOCKING_R_HALFSTARTED,
	IM_LOCKING_R_STARTEDMORE,
	IM_LOCKING_N_NORMAL,
	IM_LOCKING_N_STOPPED,
	IM_LOCKING_N_STARTED,
	IM_LOCKING_N_HALFSTARTED,
	IM_LOCKING_N_STARTEDMORE,
	IM_LOCKING_R_INSTANCE,
	IM_LOCKING_N_INSTANCE,

	IM_LOGGING_R_NORMAL,
	IM_LOGGING_R_STOPPED,
	IM_LOGGING_R_STARTED,
	IM_LOGGING_R_HALFSTARTED,
	IM_LOGGING_R_STARTEDMORE,
	IM_LOGGING_N_NORMAL,
	IM_LOGGING_N_STOPPED,
	IM_LOGGING_N_STARTED,
	IM_LOGGING_N_HALFSTARTED,
	IM_LOGGING_N_STARTEDMORE,
	IM_LOGGING_R_INSTANCE,
	IM_LOGGING_N_INSTANCE,

	IM_TRANSACTION_R_NORMAL,
	IM_TRANSACTION_R_STOPPED,
	IM_TRANSACTION_R_STARTED,
	IM_TRANSACTION_R_HALFSTARTED,
	IM_TRANSACTION_R_STARTEDMORE,
	IM_TRANSACTION_N_NORMAL,
	IM_TRANSACTION_N_STOPPED,
	IM_TRANSACTION_N_STARTED,
	IM_TRANSACTION_N_HALFSTARTED,
	IM_TRANSACTION_N_STARTEDMORE,
	IM_TRANSACTION_R_INSTANCE,
	IM_TRANSACTION_N_INSTANCE,

	IM_RECOVERY_R_NORMAL,
	IM_RECOVERY_R_STOPPED,
	IM_RECOVERY_R_STARTED,
	IM_RECOVERY_R_HALFSTARTED,
	IM_RECOVERY_R_STARTEDMORE,
	IM_RECOVERY_N_NORMAL,
	IM_RECOVERY_N_STOPPED,
	IM_RECOVERY_N_STARTED,
	IM_RECOVERY_N_HALFSTARTED,
	IM_RECOVERY_N_STARTEDMORE,
	IM_RECOVERY_R_INSTANCE,
	IM_RECOVERY_N_INSTANCE,

	IM_ARCHIVER_R_NORMAL,
	IM_ARCHIVER_R_STOPPED,
	IM_ARCHIVER_R_STARTED,
	IM_ARCHIVER_R_HALFSTARTED,
	IM_ARCHIVER_R_STARTEDMORE,
	IM_ARCHIVER_N_NORMAL,
	IM_ARCHIVER_N_STOPPED,
	IM_ARCHIVER_N_STARTED,
	IM_ARCHIVER_N_HALFSTARTED,
	IM_ARCHIVER_N_STARTEDMORE,
	IM_ARCHIVER_R_INSTANCE,
	IM_ARCHIVER_N_INSTANCE,

	IM_JDBC_R_NORMAL,
	IM_JDBC_R_STOPPED,
	IM_JDBC_R_STARTED,
	IM_JDBC_R_HALFSTARTED,
	IM_JDBC_R_STARTEDMORE,
	IM_JDBC_N_NORMAL,
	IM_JDBC_N_STOPPED,
	IM_JDBC_N_STARTED,
	IM_JDBC_N_HALFSTARTED,
	IM_JDBC_N_STARTEDMORE,
	IM_JDBC_R_INSTANCE,
	IM_JDBC_N_INSTANCE,

	IM_DASVR_R_NORMAL,
	IM_DASVR_R_STOPPED,
	IM_DASVR_R_STARTED,
	IM_DASVR_R_HALFSTARTED,
	IM_DASVR_R_STARTEDMORE,
	IM_DASVR_N_NORMAL,
	IM_DASVR_N_STOPPED,
	IM_DASVR_N_STARTED,
	IM_DASVR_N_HALFSTARTED,
	IM_DASVR_N_STARTEDMORE,
	IM_DASVR_R_INSTANCE,
	IM_DASVR_N_INSTANCE
};

//
// Define the order of Folders
enum 
{
	ORDER_BRIDGE = 1,
	ORDER_DBMS,
	ORDER_DAS,
	ORDER_JDBC,
	ORDER_NET,
	ORDER_STAR,
	ORDER_GATEWAY,

	ORDER_ICE,
	ORDER_NAME,
	ORDER_RMCMD,
	ORDER_SECURITY,
	ORDER_LOCKING,
	ORDER_LOGGING,
	ORDER_TRANSACTION_PRIMARY_LOG,
	ORDER_TRANSACTION_DUAL_LOG,
	ORDER_RECOVERY,
	ORDER_ARCHIVER
};

enum ComponentState
{
	COMPONENT_NORMAL,
	COMPONENT_STOPPED,
	COMPONENT_STARTED,
	COMPONENT_STARTEDMORE,
	COMPONENT_HALFSTARTED
};

//
// Event images indicator:
enum 
{
	IM_EVENT_R_NORMAL,
	IM_EVENT_N_NORMAL,
	IM_EVENT_R_ALERT,
	IM_EVENT_N_ALERT
};


//
// Corner icons indicator: (IDB_TRAYIMAGE)
// _R_  : Read
// _N_  : Not read
// _RA_ : Read alerted
// _NA_ : Not read alerted
enum 
{
	IM_CORNER_R_NORMAL,
	IM_CORNER_R_STOPPED,
	IM_CORNER_R_STARTED,
	IM_CORNER_R_STARTEDMORE,
	IM_CORNER_R_HALFSTARTED,
	IM_CORNER_N_NORMAL,
	IM_CORNER_N_STOPPED,
	IM_CORNER_N_STARTED,
	IM_CORNER_N_STARTEDMORE,
	IM_CORNER_N_HALFSTARTED
};

#define COMP_NORMAL    0x0001
#define COMP_STOP      0x0002
#define COMP_START     0x0004
#define COMP_STARTMORE 0x0008
#define COMP_HALFSTART 0x0010




#define COMPONENT_UNCHANGED    0x0000
#define COMPONENT_ADDED        0x0001
#define COMPONENT_REMOVED      0x0002
#define COMPONENT_STARTUPCOUNT 0x0004

#define COMPONENT_CHANGEEDALL  (COMPONENT_ADDED|COMPONENT_REMOVED|COMPONENT_STARTUPCOUNT)

//
// Action of updating the Data:
#define APP_WORKER_THREAD         0
#define TREE_COMPONENT_SELCHANGED 1
#define MAIN_TAB_SELCHANGED       2


enum 
{
	IM_ENV_USER = 0,
	IM_ENV_SYSTEM,
	IM_ENV_SYSTEM_REDIFINED
};

#define IM_NOTIFY    IM_EVENT_N_NORMAL
#define IM_ALERT     IM_EVENT_N_ALERT
#define IM_DISCARD   6
#define IM_NOTIFY_U  4
#define IM_ALERT_U   5
#define IM_DISCARD_U 7

//
// FOR HELP CONTEXT MANAGEMENT:
// ----------------------------
// Because of Context ID need to be differentiated for each select item in the left tree (13-March-2000),
// we caanot use directly the dialog ID. The dialog IDs are common for most of the items in the left tree.
// For ex: the instance of DBMS and NET log event panes are the same, and dialog ID is IDD_LOGEVENT_GENERIC = 133.
// So to have the different context help id, we use constant defined for the specific Active Tab in the right
// pane of the selected item in the left pane:
//
// HELPBASE_F_XX (Folder), HELPBASE_C_XX (Component), HELPBASE_I_XX (Instance):
#define HELPBASE_INSTALLATION              1000
#define HELPBASE_INSTALLATION_STATUS       1001
#define HELPBASE_INSTALLATION_PARAMETER    1002
#define HELPBASE_INSTALLATION_EVENT        1003
#define HELPBASE_INSTALLATION_STATEVENT    1004

#define HELPBASE_F_DBMS_STATUS             1005
#define HELPBASE_F_DBMS_EVENT              1006
#define HELPBASE_F_DBMS_STATEVENT          1007
#define HELPBASE_C_DBMS_STATUS             1008
#define HELPBASE_C_DBMS_EVENT              1009
#define HELPBASE_C_DBMS_STATEVENT          1010
#define HELPBASE_I_DBMS_STATUS             1011
#define HELPBASE_I_DBMS_EVENT              1012
#define HELPBASE_I_DBMS_STATEVENT          1013

#define HELPBASE_F_NET_STATUS              1014
#define HELPBASE_F_NET_EVENT               1015
#define HELPBASE_F_NET_STATEVENT           1016
#define HELPBASE_C_NET_STATUS              1017
#define HELPBASE_C_NET_EVENT               1018
#define HELPBASE_C_NET_STATEVENT           1019
#define HELPBASE_I_NET_STATUS              1020
#define HELPBASE_I_NET_EVENT               1021
#define HELPBASE_I_NET_STATEVENT           1022

#define HELPBASE_F_BRIDGE_STATUS           1023
#define HELPBASE_F_BRIDGE_EVENT            1024
#define HELPBASE_F_BRIDGE_STATEVENT        1025
#define HELPBASE_C_BRIDGE_STATUS           1026
#define HELPBASE_C_BRIDGE_EVENT            1027
#define HELPBASE_C_BRIDGE_STATEVENT        1028
#define HELPBASE_I_BRIDGE_STATUS           1029
#define HELPBASE_I_BRIDGE_EVENT            1030
#define HELPBASE_I_BRIDGE_STATEVENT        1031

#define HELPBASE_F_STAR_STATUS             1032
#define HELPBASE_F_STAR_EVENT              1033
#define HELPBASE_F_STAR_STATEVENT          1034
#define HELPBASE_C_STAR_STATUS             1035
#define HELPBASE_C_STAR_EVENT              1036
#define HELPBASE_C_STAR_STATEVENT          1037
#define HELPBASE_I_STAR_STATUS             1038
#define HELPBASE_I_STAR_EVENT              1039
#define HELPBASE_I_STAR_STATEVENT          1040

#define HELPBASE_F_GATEWAY_STATUS          1041
#define HELPBASE_F_GATEWAY_EVENT           1042
#define HELPBASE_F_GATEWAY_STATEVENT       1043
#define HELPBASE_C_GATEWAY_STATUS          1044
#define HELPBASE_C_GATEWAY_EVENT           1045
#define HELPBASE_C_GATEWAY_STATEVENT       1046
#define HELPBASE_I_GATEWAY_STATUS          1047
#define HELPBASE_I_GATEWAY_EVENT           1048
#define HELPBASE_I_GATEWAY_STATEVENT       1049

#define HELPBASE_F_ORACLE_STATUS           1050
#define HELPBASE_F_ORACLE_EVENT            1051
#define HELPBASE_F_ORACLE_STATEVENT        1052
#define HELPBASE_C_ORACLE_STATUS           1053
#define HELPBASE_C_ORACLE_EVENT            1054
#define HELPBASE_C_ORACLE_STATEVENT        1055
#define HELPBASE_I_ORACLE_STATUS           1056
#define HELPBASE_I_ORACLE_EVENT            1057
#define HELPBASE_I_ORACLE_STATEVENT        1058

#define HELPBASE_F_INFORMIX_STATUS         1059
#define HELPBASE_F_INFORMIX_EVENT          1060
#define HELPBASE_F_INFORMIX_STATEVENT      1061
#define HELPBASE_C_INFORMIX_STATUS         1062
#define HELPBASE_C_INFORMIX_EVENT          1063
#define HELPBASE_C_INFORMIX_STATEVENT      1064
#define HELPBASE_I_INFORMIX_STATUS         1065
#define HELPBASE_I_INFORMIX_EVENT          1066
#define HELPBASE_I_INFORMIX_STATEVENT      1067

#define HELPBASE_F_SYBASE_STATUS           1068
#define HELPBASE_F_SYBASE_EVENT            1069
#define HELPBASE_F_SYBASE_STATEVENT        1070
#define HELPBASE_C_SYBASE_STATUS           1071
#define HELPBASE_C_SYBASE_EVENT            1072
#define HELPBASE_C_SYBASE_STATEVENT        1073
#define HELPBASE_I_SYBASE_STATUS           1074
#define HELPBASE_I_SYBASE_EVENT            1075
#define HELPBASE_I_SYBASE_STATEVENT        1076

#define HELPBASE_F_MSSQL_STATUS            1077
#define HELPBASE_F_MSSQL_EVENT             1078
#define HELPBASE_F_MSSQL_STATEVENT         1079
#define HELPBASE_C_MSSQL_STATUS            1080
#define HELPBASE_C_MSSQL_EVENT             1081
#define HELPBASE_C_MSSQL_STATEVENT         1082
#define HELPBASE_I_MSSQL_STATUS            1083
#define HELPBASE_I_MSSQL_EVENT             1084
#define HELPBASE_I_MSSQL_STATEVENT         1085

#define HELPBASE_F_ODBC_STATUS             1086
#define HELPBASE_F_ODBC_EVENT              1087
#define HELPBASE_F_ODBC_STATEVENT          1088
#define HELPBASE_C_ODBC_STATUS             1089
#define HELPBASE_C_ODBC_EVENT              1090
#define HELPBASE_C_ODBC_STATEVENT          1091
#define HELPBASE_I_ODBC_STATUS             1092
#define HELPBASE_I_ODBC_EVENT              1093
#define HELPBASE_I_ODBC_STATEVENT          1094

#define HELPBASE_F_NAME_STATUS             1095
#define HELPBASE_F_NAME_EVENT              1096
#define HELPBASE_F_NAME_STATEVENT          1097
#define HELPBASE_C_NAME_STATUS             1098
#define HELPBASE_C_NAME_EVENT              1099
#define HELPBASE_C_NAME_STATEVENT          1100
#define HELPBASE_I_NAME_STATUS             1101
#define HELPBASE_I_NAME_EVENT              1102
#define HELPBASE_I_NAME_STATEVENT          1103

#define HELPBASE_F_ICE_STATUS              1104
#define HELPBASE_F_ICE_EVENT               1105
#define HELPBASE_F_ICE_STATEVENT           1106
#define HELPBASE_C_ICE_STATUS              1107
#define HELPBASE_C_ICE_EVENT               1108
#define HELPBASE_C_ICE_STATEVENT           1109
#define HELPBASE_I_ICE_STATUS              1110
#define HELPBASE_I_ICE_EVENT               1111
#define HELPBASE_I_ICE_STATEVENT           1112

#define HELPBASE_F_RECOVER_STATUS          1113
#define HELPBASE_F_RECOVER_EVENT           1114
#define HELPBASE_F_RECOVER_STATEVENT       1115
#define HELPBASE_C_RECOVER_STATUS          1116
#define HELPBASE_C_RECOVER_EVENT           1117
#define HELPBASE_C_RECOVER_STATEVENT       1118
#define HELPBASE_C_RECOVER_LOGFILE         1119
#define HELPBASE_I_RECOVER_STATUS          1120
#define HELPBASE_I_RECOVER_EVENT           1121
#define HELPBASE_I_RECOVER_STATEVENT       1122

#define HELPBASE_F_RMCMD_STATUS            1123
#define HELPBASE_F_RMCMD_EVENT             1124
#define HELPBASE_F_RMCMD_STATEVENT         1125
#define HELPBASE_C_RMCMD_STATUS            1126
#define HELPBASE_C_RMCMD_EVENT             1127
#define HELPBASE_C_RMCMD_STATEVENT         1128
#define HELPBASE_I_RMCMD_STATUS            1129
#define HELPBASE_I_RMCMD_EVENT             1130
#define HELPBASE_I_RMCMD_STATEVENT         1131

#define HELPBASE_F_SECURITY_STATUS         1132
#define HELPBASE_F_SECURITY_EVENT          1133
#define HELPBASE_F_SECURITY_STATEVENT      1134
#define HELPBASE_C_SECURITY_STATUS         1135
#define HELPBASE_C_SECURITY_EVENT          1136
#define HELPBASE_C_SECURITY_STATEVENT      1137
#define HELPBASE_I_SECURITY_STATUS         1138
#define HELPBASE_I_SECURITY_EVENT          1139
#define HELPBASE_I_SECURITY_STATEVENT      1140

#define HELPBASE_F_LOCKING_STATUS          1141
#define HELPBASE_F_LOCKING_EVENT           1142
#define HELPBASE_F_LOCKING_STATEVENT       1143
#define HELPBASE_C_LOCKING_STATUS          1144
#define HELPBASE_C_LOCKING_EVENT           1145
#define HELPBASE_C_LOCKING_STATEVENT       1146
#define HELPBASE_I_LOCKING_STATUS          1147
#define HELPBASE_I_LOCKING_EVENT           1148
#define HELPBASE_I_LOCKING_STATEVENT       1149

#define HELPBASE_F_LOGGING_STATUS          1150
#define HELPBASE_F_LOGGING_EVENT           1151
#define HELPBASE_F_LOGGING_STATEVENT       1152
#define HELPBASE_C_LOGGING_STATUS          1153
#define HELPBASE_C_LOGGING_EVENT           1154
#define HELPBASE_C_LOGGING_STATEVENT       1155
#define HELPBASE_I_LOGGING_STATUS          1156
#define HELPBASE_I_LOGGING_EVENT           1157
#define HELPBASE_I_LOGGING_STATEVENT       1158

#define HELPBASE_F_LOGFILE_STATUS          1159
#define HELPBASE_F_LOGFILE_EVENT           1160
#define HELPBASE_F_LOGFILE_STATEVENT       1161
#define HELPBASE_C_LOGFILE_STATUS          1162
#define HELPBASE_C_LOGFILE_EVENT           1163
#define HELPBASE_C_LOGFILE_STATEVENT       1164
#define HELPBASE_I_LOGFILE_STATUS          1165
#define HELPBASE_I_LOGFILE_EVENT           1166
#define HELPBASE_I_LOGFILE_STATEVENT       1167

#define HELPBASE_F_ARCHIVER_STATUS         1168
#define HELPBASE_F_ARCHIVER_EVENT          1169
#define HELPBASE_F_ARCHIVER_STATEVENT      1170
#define HELPBASE_C_ARCHIVER_STATUS         1171
#define HELPBASE_C_ARCHIVER_EVENT          1172
#define HELPBASE_C_ARCHIVER_STATEVENT      1173
#define HELPBASE_C_ARCHIVER_LOGFILE        1174
#define HELPBASE_I_ARCHIVER_STATUS         1175
#define HELPBASE_I_ARCHIVER_EVENT          1176
#define HELPBASE_I_ARCHIVER_STATEVENT      1177

#define HELPBASE_F_DBMSOIDT_STATUS         1178
#define HELPBASE_F_DBMSOIDT_EVENT          1179
#define HELPBASE_F_DBMSOIDT_STATEVENT      1180
#define HELPBASE_C_DBMSOIDT_STATUS         1181
#define HELPBASE_C_DBMSOIDT_EVENT          1182
#define HELPBASE_C_DBMSOIDT_STATEVENT      1183
#define HELPBASE_I_DBMSOIDT_STATUS         1184
#define HELPBASE_I_DBMSOIDT_EVENT          1185
#define HELPBASE_I_DBMSOIDT_STATEVENT      1186

#define HELPBASE_F_JDBC_STATUS             1187
#define HELPBASE_F_JDBC_EVENT              1188
#define HELPBASE_F_JDBC_STATEVENT          1189
#define HELPBASE_C_JDBC_STATUS             1190
#define HELPBASE_C_JDBC_EVENT              1191
#define HELPBASE_C_JDBC_STATEVENT          1192
#define HELPBASE_I_JDBC_STATUS             1193
#define HELPBASE_I_JDBC_EVENT              1194
#define HELPBASE_I_JDBC_STATEVENT          1195

#define HELPBASE_F_DASVR_STATUS             1196
#define HELPBASE_F_DASVR_EVENT              1197
#define HELPBASE_F_DASVR_STATEVENT          1198
#define HELPBASE_C_DASVR_STATUS             1199
#define HELPBASE_C_DASVR_EVENT              1200
#define HELPBASE_C_DASVR_STATEVENT          1201
#define HELPBASE_I_DASVR_STATUS             1202
#define HELPBASE_I_DASVR_EVENT              1203
#define HELPBASE_I_DASVR_STATEVENT          1204
typedef enum 
{
	TREE_INSTALLATION=0,
	TREE_FOLDER,
	TREE_COMPONENTNAME,
	TREE_INSTANCE
} TreeBranchType;
//
// This function return the value of HELPBASE_F_XX, HELPBASE_C_XX, HELPBASE_I_XX
// depending on 'nTreType' and 'nComponentType'
int IVM_HelpBase(TreeBranchType nTreType, int nComponentType, UINT nDialogID);


#endif // !defined(COMPBASE_HEADER)
