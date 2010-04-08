/*
**    Copyright (c) 1995, 2003 Ingres Corporation
*/
#include <compat.h>
#include <rms.h>
#include <objrecdef.h>
#include <lo.h>
#include <si.h>
#include <pc.h>
#include <cv.h>
#include <st.h>
#include <tm.h>
#include <me.h>
#include <nm.h>
#include <ex.h>
#include <gsdef.h>
#include <epmdef.h>
#include <sdfdef.h>
#include <gpsdef.h>
#include <evset.h>
#include <starlet.h>

/**
**  Name: symbol.c - Routines to produce symbolic stack dumps
**
**  Description:
**
**	This module provides routines to produce symbolic 
**	stack dumps.
**
**	It is part of the VMS port of the ICL Diagnostics project.
**	
**	The DBMS symbol table is read to build a symbol table
**	in memory. 
**
**	Stack traces are read from a binary file produced by 
**	a dumping routine in the server.
**
** History:
**	20-AUG-1995 (mckba01)
**		Initial version.
**	01-Nov-1995 (mckba01)
**		Move file to VMS CL.
**      18-May-1998 (horda03)
**         Ported to OI 1.2/01.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	07-feb-01 (kinte01)
**	    Add casts to quiet compiler warnings
**	28-aug-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
*/


/*
**	RMS control blocks used for accessing the
**	DBMS symbol table (IIDBMS.STB)
*/
static struct FAB	*SYMfab = NULL;
static struct RAB	*SYMrab = NULL;
static struct NAM	*SYMnam = NULL;
static char		nam_name[NAM$C_MAXRSS];

/*
**	Symbol table entry structure
*/
typedef struct syment
	{
		char		SymName[32];
		u_i4        	SymValue;
		struct syment	*Next;
		struct syment	*Prev;
	} SYM_ENT;

static	SYM_ENT	*Sym_Table_Head = NULL;
static	SYM_ENT	*Sym_Table_Tail = NULL;


static	i4	SYM_TABread();
static	i4	SYM_TABopen();
static	VOID	SymTabSort();
static	VOID	SymTabSwap();
static	VOID	SymTabAdd();
static	VOID	ProcessStacks();	
static	VOID	ProcessNSStacks();
static	VOID	SYM_TABclose();
static VOID	SymTabFree();



/*{
**  Name: DIAG_VMS_dump_stacks - produce formatted thread stacks
**
**  Description:
**      This routine is the entry point for producing
**	formatted stack dumps for the threads in 
**	the DBMS server. 
**
**  Inputs:
**	VOID	outfcn		output function 
**	PTR	symfile		symbol table file name
**	PTR	stkfile		Binary stack dump file	
**
**  Side effects:
**
**  History:
**	20-8-1995 (mckba01)
**		Initial VMS version.
**
*/  
VOID
DIAG_VMS_dump_stacks(outfcn,symfile,stkfile)
VOID	outfcn();
PTR	symfile;
PTR	stkfile;
{
	i4 		RetVal;
	i4		size;
	PTR		recptr;
	i4		pos;
	i4		NameLen;
	char		NameBuf[60];
	i4		SymCount;
	EPMDEF		*EpmRec;
	SDFDEF		*SdfRec;
        u_i4		Address;
	GPSDEF		*GpsRec;
	LOCATION	loc;
	FILE		*SymDesc;


	MEadvise(ME_USER_ALLOC);


	if(SYM_TABopen(symfile) == 0)
	{
		/*	Opened Ok     */

		SymCount = 0;
		recptr = MEreqmem(0,(u_i4) 600,FALSE,NULL);

		while(1)
		{
                        /*	
				Get All records in Symbol Table File 
                                and build a symbol table in memory
			*/

			if(SYM_TABread(recptr, &size) == 0)
			{
				/* Read OK  */

				/* test first byte for record type */

				switch(recptr[0])
				{
				case OBJ$C_HDR:
					/*	
						Its a header record 
						get next record 
					*/
					break;
	
				case OBJ$C_GSD:
				        /*	Global Symbol Defs
						Since they are grouped
						by record, discard the
						ones we dont need
					*/
					pos = 1;
					
					
					while(pos != size) /* loop till end of record read*/
					{
						/* test second byte for GSD type */
						switch (recptr[pos])
						{
						case GSD$C_EPM:
						 
							/* 
								GSD$C_EPM :
								Entry Point Symbol.

								these are the ones we want 
						   		copy contents of read
						   		record into structure
						   		for easy reference to
						   		the fields.

						   		Taking care to check for
						   		End of record case.
							*/
							
							EpmRec = &(recptr[pos]);

							/* get name */
				
							NameLen = (i4)EpmRec->epm$b_namlng;
							MEcopy((PTR)&(EpmRec->epm$t_name), (u_i2) NameLen, (PTR) NameBuf);

							/* Null Terminate */
                                                        NameBuf[NameLen] = '\0';

							/* Get Value */

		    					Address = EpmRec->epm$l_addrs;

							SymTabAdd(Address,NameBuf);

				     
							SymCount++;
                    
							/* Move to next subrecord */
							pos = (pos + sizeof(EPMDEF)) - (31 - NameLen);
							break;


						case GSD$C_SYM:
							/* 	As yet we dont want
								this, but we must
								step over it to
								get to the next
								subrecord
							*/
                                         
                                                        SdfRec = &(recptr[pos]);

							/* get name */
				
							NameLen = (i4) SdfRec->sdf$b_namlng;
							MEcopy((PTR)&(SdfRec->sdf$t_name),(u_i2)NameLen,(PTR)NameBuf);

							/* Null Terminate */
                                                        NameBuf[NameLen] = '\0';

							/* Get Value */
							Address = SdfRec->sdf$l_value;

							SymTabAdd(Address,NameBuf);
                                                      
							SymCount++;                    

							/* Move to next subrecord */
							pos = (pos + sizeof(SDFDEF)) - (31 - NameLen);
							break;
                                         
						case GSD$C_PSC:
							/*	Psect stuff
								but there may
								be a few
								symbols hiding
								in here
							*/
                                         
                                                        GpsRec = &(recptr[pos]);

							/* get name */
				
							NameLen = (i4) GpsRec->gps$b_namlng;

							/* Move to next subrecord */
							pos = (pos + sizeof(GPSDEF)) - (31 - NameLen);
							break;
                                         
                                                default:
							/*
								Non recognised 
								Sub record Type
								get next record
							*/
							pos = size; 
							break;
						}
					}
					break;	/* Get next record  */	
				}
			}
			else
			{
				/*	Read Failed :- EOF	*/
                              
			   	MEfree(recptr);
				break;
			}
		}
		SymTabSort();	
		ProcessStacks(stkfile,outfcn);
	}
	else
	{
		/*	Open Failed 	*/
		outfcn("Can`t open Symbol Table file : %s\n",symfile,0,0,0,0,0,0);
		outfcn("Producing NON - symbolic stack traces\n\n",0,0,0,0,0,0,0);
		ProcessNSStacks(stkfile,outfcn);
		return;
	}    

	SYM_TABclose();
	SymTabFree();
	return;
}





/*{
**  Name: FindSymName - returns symbol namefor address
**
**  Description:
**      Looks up an address in the symbol table
**	and retruns the corresponding symbol name string
**	If address is out of reange, it returns the
**	address as a string.
**
**  Inputs:
**	u_i4	LookupValue	address to find
**	PTR	SymbolName	string in which result is returned
**
**  Side effects:
**
**  History:
**	20-8-1995 (mckba01)
**		Initial version.
**
*/  
static
VOID
FindSymName(LookupValue, SymbolName)
u_i4		LookupValue;
PTR		SymbolName;
{
	SYM_ENT	*SymEntPtr = Sym_Table_Head;

	/* Put Address in for now	*/
	STprintf(SymbolName,"%d",LookupValue);

	if(LookupValue < SymEntPtr->SymValue)
		return;	/*	Below range of our symbols	*/

	while(SymEntPtr != NULL)
	{
		if(SymEntPtr->Next == NULL) 
		{
			break;
		}
		else if ((SymEntPtr->Next)->SymValue > LookupValue)
		{
			STcopy(SymEntPtr->SymName,SymbolName);
			break;
		}
		SymEntPtr = SymEntPtr->Next;
	}
}




/*{
**  Name: ProcessStacks - outputs symbolic stack dump
**
**  Description:
**	Reads stack dump information from the binary file
**	specified, maps pc address to the symbol name
**	and outputs the trace.
**
**  Inputs:
**	PTR	stk_file	name of binary stack dump file
**	VOID	outfcn		procedure to do output
**
**  Side effects:
**
**  History:
**	20-8-1995 (mckba01)
**		Initial version.
**
*/  
static
VOID
ProcessStacks(stk_file,outfcn)
PTR	stk_file;
VOID	outfcn();
{
	char		tempstr[100] = "abcdefghijklmnb";
	char		RetSymName[100];
	LOCATION	loc;
	FILE		*Fdesc;
	EV_STACK_ENTRY	StackRec;
	i4		count;

	

	if(LOfroms(PATH &FILENAME,stk_file, &loc) != OK)
	{
		outfcn("Can't Locate Stack Dump file : %s\n",stk_file,0,0,0,0,0,0);
        	return;
	}

	if(SIfopen(&loc,"r",SI_RACC,(i4) sizeof(StackRec),&Fdesc)!= OK)
	{	
		outfcn("Can't Open Stack Dump file : %s\n",stk_file,0,0,0,0,0,0);	
		return;
	}

	while(SIread(Fdesc,sizeof(StackRec),&count,&StackRec) == OK)
	{		
		if(STcompare(StackRec.Vstring,tempstr))
		{
			STcopy(StackRec.Vstring,tempstr);
			outfcn("%s",tempstr);
		}
		FindSymName(StackRec.pc,RetSymName);
		outfcn("%08x:%-16s %08x,%08x,%08x,%08x,%08x,%08x\n"
			,StackRec.sp,RetSymName,StackRec.args[0]
			,StackRec.args[1],StackRec.args[2],StackRec.args[3]
			,StackRec.args[4],StackRec.args[5]);
	}

	SIclose(Fdesc);
                                            
}


/*{
**  Name: ProcessNStacks - outputs NON symbolic stack dump
**
**  Description:
**	Reads stack dump information from the binary file
**	specified and outputs it as a formatted
**	NON symbolic stack trace.
**
**  Inputs:
**	PTR	stk_file	name of binary stack dump file
**	VOID	outfcn		procedure to do output
**
**  Side effects:
**
**  History:
**	20-8-1995 (mckba01)
**		Initial version.
**
*/ 
static
VOID
ProcessNSStacks(stk_file,outfcn)
PTR	stk_file;
VOID	outfcn();
{
	char		tempstr[100] = "abcdefghijklmnb";
	LOCATION	loc;
	FILE		*Fdesc;
	EV_STACK_ENTRY	StackRec;
	i4		count;


	


	if(LOfroms(PATH &FILENAME,stk_file, &loc) != OK)
	{
		outfcn("Can't Locate Stack Dump file : %s\n",stk_file,0,0,0,0,0,0);
        	return;
	}

	if(SIfopen(&loc,"r",SI_RACC,(i4) sizeof(StackRec),&Fdesc)!= OK)
	{	
		outfcn("Can't Open Stack Dump file : %s\n",stk_file,0,0,0,0,0,0);	
		return;
	}

	while(SIread(Fdesc,sizeof(StackRec),&count,&StackRec) == OK)
	{		
		if(STcompare(StackRec.Vstring,tempstr))
		{
			STcopy(StackRec.Vstring,tempstr);
			outfcn("%s",tempstr);
		}
		outfcn("%08x:%08x (%08x,%08x,%08x,%08x,%08x,%08x)\n"
			,StackRec.sp,StackRec.pc,StackRec.args[0]
			,StackRec.args[1],StackRec.args[2],StackRec.args[3]
			,StackRec.args[4],StackRec.args[5]);
	}

	SIclose(Fdesc);
                                            
}


/*{
**  Name: SymTabAdd - Adds a symbol to the symbol table
**
**  Description:
**	Adds a new symbol entry into the linked list
**	of symbols.
**
**  Inputs:
**
**  Side effects:
**	u_i4	SymVal		Symbol Address.
**	PTR	SymName		Symbol Name.
**
**  History:
**	20-8-1995 (mckba01)
**		Initial version.
**
*/ static      
VOID
SymTabAdd(SymVal,SymName)
u_i4		SymVal;
PTR		SymName; /* Null terminated string */
{
	SYM_ENT		*NewPtr;

	/*	Allocate new Symbol entry	*/

	NewPtr = (SYM_ENT *) MEreqmem(0, (u_i4) sizeof(SYM_ENT), FALSE, NULL);
	
	/*	Fill in details		*/

	NewPtr->SymValue = SymVal;
	STcopy(SymName,NewPtr->SymName);
	NewPtr->Next = NULL;
	NewPtr->Prev = NULL;

	/*	Add to end of table	*/

	if(Sym_Table_Tail == NULL)
	{
		/*	No previous entries	*/

		Sym_Table_Tail = Sym_Table_Head = NewPtr;
	}
	else
	{
		/*	link entry into existing Table	*/
		
		Sym_Table_Tail->Next = NewPtr;
		NewPtr->Prev = Sym_Table_Tail;
		Sym_Table_Tail = NewPtr;
	}	
}



/*{
**  Name: SymTabSwap - swaps two entries in the symbol table
**
**  Description:
**	Switches the position of an entry and the entry
**	following it in the symbol table.
**
**  Inputs:
**	SYM_ENT *SymEntPtr	pointer to entry to be swapped
**
**  Side effects:
**
**  History:
**	20-8-1995 (mckba01)
**		Initial version.
**
*/ 
static
VOID
SymTabSwap(SymEntPtr)
SYM_ENT	*SymEntPtr;
{
	u_i4		*TempVal;
	char		TempName[32];

	/*	Swap this pointer with the one previous
		to it	*/

	TempVal = SymEntPtr->SymValue;
	STcopy(SymEntPtr->SymName,TempName);

	SymEntPtr->SymValue = (SymEntPtr->Prev)->SymValue;
	(SymEntPtr->Prev)->SymValue = TempVal;

	STcopy((SymEntPtr->Prev)->SymName,SymEntPtr->SymName);
	STcopy(TempName,(SymEntPtr->Prev)->SymName);
}




/*{
**  Name: SymTabSort - sorts the symbol table
**
**  Description:
**	Sorts the symbol table by ascending symbol address.
**
**  Inputs:
**
**  Side effects:
**                                          
**  History:
**	20-8-1995 (mckba01)
**		Initial version.
**
*/ 
static
VOID
SymTabSort()
{
	SYM_ENT	*HeadPtr;	
	SYM_ENT	*CurEntry;

	HeadPtr = Sym_Table_Head;

	while(HeadPtr->Next != NULL)
	{
		CurEntry = Sym_Table_Tail;

		while(CurEntry != HeadPtr)
		{
			/*	Compare with previous record	*/

                        if(CurEntry->SymValue < (CurEntry->Prev)->SymValue)
				SymTabSwap(CurEntry);
			
			/*	Move to next level;	*/

			CurEntry = CurEntry->Prev;
		}
		/*	Move to next level	*/

		HeadPtr = HeadPtr->Next;
	} 	
}




/*{
**  Name: SYM_TABopen - opens the symboltable file
**
**  Description:
**	Opens the symbol table file specified by
**	the supplied file name.
**
**  Inputs:
**	PTR	Sym_Tab_Name	Symbol table file name
**
**  Returns:
**	0	= OK
**	1	= FAIL
**  Side effects:
**                                          
**  History:
**	20-8-1995 (mckba01)
**		Initial version.
**
*/ 
static
i4
SYM_TABopen(Sym_Tab_Name)
PTR	Sym_Tab_Name;
{
	i4	RetVal;

	/* Allocate FAB	*/

	SYMfab = (struct FAB *) MEreqmem(0, (u_i4) sizeof (struct FAB), TRUE, NULL);
	SYMnam = (struct NAM *) MEreqmem(0, (u_i4) sizeof (struct NAM), TRUE, NULL);
	SYMrab = (struct RAB *) MEreqmem(0, (u_i4) sizeof (struct RAB), TRUE, NULL);	

	/* Initialize FAB, with file name */

	SYMfab->fab$b_bid = FAB$C_BID;
	SYMfab->fab$b_bln = FAB$C_BLN;
	SYMfab->fab$b_fns = STlength(Sym_Tab_Name);
	SYMfab->fab$l_fna = Sym_Tab_Name;

	/*	Allow read access only	*/

	SYMfab->fab$b_fac = FAB$M_GET;

	/*	Initialize FAB record details	*/

	SYMfab->fab$b_org = FAB$C_SEQ;
	SYMfab->fab$b_rfm = FAB$C_VAR;
	SYMfab->fab$b_rat = NULL;
	SYMfab->fab$w_mrs = 512;

	/*	Initialize RAB details	*/

	SYMrab->rab$b_bid = RAB$C_BID;
	SYMrab->rab$b_bln = RAB$C_BLN;
	SYMrab->rab$l_fab = SYMfab;
	SYMrab->rab$b_rac = RAB$C_SEQ;
/*	SYMrab->rab$l_rop = RAB$M_EOF;   */

	SYMnam->nam$b_bid = NAM$C_BID;
	SYMnam->nam$b_bln = NAM$C_BLN;
	SYMnam->nam$l_esa = nam_name;
	SYMnam->nam$b_ess = sizeof(nam_name);
	SYMfab->fab$l_nam = SYMnam;


	sys$open(SYMfab);

	if ((SYMfab->fab$l_sts & 1) == 1) 
	{	
         	/*	Success try to connect	*/

		if((sys$connect(SYMrab) & 1) == 1)
		{
			/*	return success status	*/

			/* Allocate space for user buffer area	*/

			SYMrab->rab$l_ubf = MEreqmem(0, (u_i4) 512, TRUE, NULL);
			SYMrab->rab$w_usz = 512;
			SYMrab->rab$b_rac = RAB$C_SEQ;

			return(0);	/* SUCCESS */
		}
	}
	
	/*	Its failed if we are here, so clean 
		up and return failure			*/

	sys$close(SYMfab);
	MEfree((PTR)SYMfab);
	MEfree((PTR)SYMrab);
	MEfree((PTR)SYMnam);

	return(1);  /* FAIL */
}




/*{
**  Name: SYM_TABread - reads a record from the symbol table file
**
**  Description:
**	reads a VAX object language record from the symbol table file
**
**  Inputs:
**	PTR	Rec_ptr		Area in which to retrun the record read
**	i4	*Size		Parameter in which record size read is returned
**
**  Returns:
**	0	= OK
**	1	= FAIL
**  Side effects:
**                                          
**  History:
**	20-8-1995 (mckba01)
**		Initial version.
**
*/ 
static
i4
SYM_TABread(Rec_Ptr,Size)
PTR	Rec_Ptr;
i4	*Size;
{
	if(SYMrab == NULL)
		return(1); /*FAIL*/


	if((sys$get(SYMrab) & 1) == 1)
	{

     		*Size = SYMrab->rab$w_rsz;
		MEcopy((PTR) SYMrab->rab$l_rbf,(u_i2) 512, Rec_Ptr);
		return(0);	/* SUCCESS */
	}
	else
	{
		return(1); /* FAIL */
	}
}



/*{
**  Name: SYM_TABclose - Closes the symbol table file.
**
**  Description:
**	Closes the connection to the open symbol table file.
**
**  Inputs:
**
**  Side effects:
**                                          
**  History:
**	20-8-1995 (mckba01)
**		Initial version.
**
*/ 
static
VOID
SYM_TABclose()
{
	sys$disconnect(SYMfab);
	sys$close(SYMfab);
	MEfree((PTR)SYMfab);
	MEfree((PTR)SYMrab);
	MEfree((PTR)SYMnam);
}


/*{
**  Name: SymTabFree - frees memory used by symbol table
**
**  Description:
**	Steps through the linked list of symbol definitions
**	freeing each.
**
**  Inputs:
**
**  Side effects:
**                                          
**  History:
**	20-8-1995 (mckba01)
**		Initial version.
**
*/ 
static
VOID
SymTabFree()
{
	SYM_ENT	*SymEntPtr;
	SYM_ENT	*hold;

	SymEntPtr = Sym_Table_Head;
	

	while(SymEntPtr != NULL)
	{
		hold = SymEntPtr;
		SymEntPtr = SymEntPtr->Next;
		MEfree((PTR) hold);
	}
}
