#ifndef _REPLICATIONINTERFACE_INCLUDED
#define _REPLICATIONINTERFACE_INCLUDED

#define NO_KEYSCHOOSE               0
#define KEYS_SHADOWTBLONLY          1
#define KEYS_BOTHQUEUEANDSHADOW     2

/*
**
**      05-Sep-2000 (hanch04)
**          replace nat and longnat with i4
*/
//
// First Section : "extern "C" functions
//
#ifdef __cplusplus
extern "C"
{
#endif
	int		GetVnodeDatabaseAndFillCtrl(int hdl, LPUCHAR lpDbName,HWND hWndCAlistBox);
	char *	VdbaColl_Queue_Collision( int CurrentNodeHdl ,char *dbname );
	void	Vdba_FreeMemory( char *pCollInfo);
	char *	Vdba_check_distrib_config(int CurrentNodeHdl,char *dbname );
	int		GetCddsName (int hdl ,LPUCHAR lpDbName, int cdds_num ,LPUCHAR lpcddsname);

	int		Vdba_GetLenOfFileName		(char *FileName);
#ifndef LIBMON_HEADER
	int     GetCddsNumInLookupTblAndFillCtrl(int hdl, LPUCHAR lpDbName,
	                                         LPUCHAR cdds_lookup_tbl,HWND hWndComboCdds);
	int     MonitorReplicatorStopSvr ( int hdl, int svrNo ,LPUCHAR lpDbName  );
	int     SendEvent(char *vnode, char *dbname,char *evt_name,char *flag_name,char *svr_no);
	LPCTSTR Vdba_DuplicateStringWithCRLF(LPCTSTR pBufSrc);
	// NOTE : No need to delete the returned pointer,
	// which remains valid until next call to the same function
	char *table_integrity( int CurrentNodeHdl,char *CurDbName,short db1,short db2,long table_no,
	                       short cdds_no,char *begin_time,char *end_time,char *order_clause);
	int     GenerateSql4changeAccessDB( BOOL bDBPublic ,LPUCHAR DBname,LPUCHAR VnodeName);
	void    VdbaColl_Retrieve_Collision( LONG pCurClass ,int CurrentNodeHdl ,char *dbname );
	int     AddTransaction (LONG pClass,long transaction_id);
	int     AddCollisionSequence(LONG pClass, void *pVSequence,void *pVCol);
	int     ExecuteProcedureLocExtend(char *Procedure);
	int UpdateTable4ResolveCollision(int nStatusPriority ,
	                                 int nSrcDB ,
	                                 int nTransactionID ,
	                                 int nSequenceNo ,
	                                 char *pTblName,
	                                 int nTblNo);
#endif
#ifdef __cplusplus
}
#endif
//
// Second section : only for SC and C sources
//
#ifndef __cplusplus
#include "collisio.h"
	int		OpenNodeStruct4Replication	(LPUCHAR lpVirtNode);
	int		Getsession4Replication		( UCHAR *lpsessionname,int sessiontype,int * SessNum);
	long	Vdba_error_check			(long flags,DBEC_ERR_INFO *errinfo);
	void	Vdba_GetValidVnodeName		(LPUCHAR lpVNode,LPUCHAR lpValidVnode);
	BOOL	Vdba_InitReportFile			(HFILE *phdl, char *bufFileName, int bufSize);
	BOOL	Vdba_WriteBufferInFile		(HFILE hdl,char *buftmp,int buflen);
	BOOL	Vdba_CloseFile				(HFILE *hdl);
	//int		Vdba_GetLenOfFileName		(char *FileName);
	long	VdbaColl_tbl_fetch			(long table_no, int force);
	void	VdbaColl_print_cols			(HFILE report_fp,i2  sourcedb,long	transaction_id,
											long  sequence_no,char *target_wc,char *table_owner);
	char * VdbaColl_edit_name(i4 edit_type,char	*name,char	  *edited_name);
	int		AddTransaction (LONG pClass,long transaction_id);
	int		AddCollisionSequence(LONG pClass,void *pVSequence,void *pVCol);

#endif

//
// Third section : only for CPP sources
//

#ifdef __cplusplus
#include "starutil.h"
//#include "repitem.h"

extern "C"
{
//#include "monrepli.h"
#include "domdata.h"
}
#endif
#endif
