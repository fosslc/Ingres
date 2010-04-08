/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : errout.h
//
//     
// History:
// uk$so01 (20-Jan-2000, Bug #100063)
//         Eliminate the undesired compiler's warning
********************************************************************/
#ifdef DEBUG_OUT

#define		WARNING()			CrtSetColor(255,0,0) 
#define		NORMAL()			CrtSetColor(0,0,0)
#define		NOTIFY()			CrtSetColor(0,0,255) 
#define		SPECIAL()			CrtSetColor(0, 155,155)
#define		OUT(s)				CrtPrintF s
#define		OUTP(s,p)			CrtPrintF(s, p)
#define		OUTPP(s,p1,p2)	CrtPrintF(s, p1,p2)
#define		WAIT()				CrtWaitChar()

#define		NOTIFYOUT(s)		{ NOTIFY(); OUT(s); NORMAL(); }
#define		WARNOUT(s)		{ WARNING(); OUT(s); NORMAL(); }
#define		SPECIALOUT(s)	{ SPECIAL(); OUT(s); NORMAL(); }

#define		NOTIFYOUTP(s,p)	{ NOTIFY(); OUTP(s,p); NORMAL(); }
#define		WARNOUTP(s,p)	{ WARNING(); OUTP(s,p); NORMAL(); }
#define		SPECIALOUTP(s,p)	{ SPECIAL(); OUTP(s,p); NORMAL(); }

#define		NOTIFYOUTPP(s,p1,p2)		{ NOTIFY(); OUTPP(s,p1,p2); NORMAL(); }
#define		WARNOUTPP(s,p1,p2)		{ WARNING(); OUTPP(s,p1,p2); NORMAL(); }
#define		SPECIALOUTPP(s,p1,p2)		{ SPECIAL(); OUTPP(s,p1,p2); NORMAL(); }

#else

#define		WARNING()		
#define		NORMAL()		
#define		NOTIFY()		
#define		SPECIAL()	
#if !defined (_WINDEF_)
#define		OUT(s)
#endif
#define		OUTP(s,p)	
#define		WAIT()


#define		NOTIFYOUT(s)	
#define		WARNOUT(s)	
#define		SPECIALOUT(s)	

#define		NOTIFYOUTP(s,p)
#define		WARNOUTP(s,p)
#define		SPECIALOUTP(s,p)	

#define		NOTIFYOUTPP(s,p1,p2)	
#define		WARNOUTPP(s,p1,p2)	
#define		SPECIALOUTPP(s,p1,p2)	


#endif


#ifdef 	DEBUG_MEM
	#define		ERR_MEMWALK(s)				{ if (!MemWalk()) { ErrPrintF("Error in MemWalk at %s", s); DebugBreak(); } } 
#else
	#define		ERR_MEMWALK(s)	
#endif
