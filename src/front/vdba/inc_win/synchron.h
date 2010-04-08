/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : synchron.h, header file 
**    Project  : Visual DBA
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Synchronize Object
**
** History:
**
** 23-Oct-2001 (uk$so01)
**    Created
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
*/


#if !defined (CASYCHRONIZEINTERRUPT_HEADER)
#define CASYCHRONIZEINTERRUPT_HEADER
class CaSychronizeInterrupt
{
public:
	CaSychronizeInterrupt();
	~CaSychronizeInterrupt();

	void BlockUserInterface(BOOL bLock);
	void BlockWorkerThread (BOOL bLock);
	void WaitUserInterface();
	void WaitWorkerThread();

	void SetRequestCancel(BOOL bSet){m_bRequestCansel = bSet;}
	BOOL IsRequestCancel(){return m_bRequestCansel;}
protected:
	BOOL   m_bRequestCansel;
	HANDLE m_hEventWorkerThread;
	HANDLE m_hEventUserInterface;
};

#endif // CASYCHRONIZEINTERRUPT_HEADER
