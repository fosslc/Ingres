/*
** Copyright (c) 1985, 2005 Ingres Corporation
**
*/
# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<fstm.h>
# include	<er.h>
# include	<ermf.h>
# include	<ug.h>		/* 6-x_PC_80x86 */
# include	<uf.h>

/*
**    FSsave  --
**
**    Returns: nothing
**
**	(27-nov-1985) (peter)	Add ref to FSprnscr routine.
**	11/12/87 (dkh) - FTdiag -> FSdiag.
**	14-aug-89 (sylviap)	
**		Added is_file parameter to allow PRINT option.
**	16-oct-89 (sylviap)	
**		Changed the FS calls to IIUF.
**	25-jan-89 (teresal)
**		Increase location buffer to MAX_LOC+1.
**      03-oct-1996 (rodjo04)
**              Changed "ermf.h" to <ermf.h>
**	07-sep-2005 (abbjo03)
**	    Replace xxx_FILE constants by SI_xxx.
**	24-Nov-2009 (frima01) Bug 122490
**	    Added include of ug.h and uf.h to eliminate warnings from gcc 4.3.
*/
FSsave(bcb, is_file)
   BCB	 *bcb;
   bool	 is_file;	/* TRUE  if output to go to file */
			/* FALSE if output to the printer */
{
   register i4	   i;
   char	    bfr[1024];
   char	    msg[80];
   i4	     rc;

   LOCATION in_loc;
   FILE	    *in_fp;
   char	    in_locstr[MAX_LOC+1];

FSdiag(ERx("FSsave: entering\n"));

   LOcopy(&bcb->bfloc,in_locstr,&in_loc);

   /*
   **	 Flush any records from memory to disk that require it
   */
   IIUFfsh_Flush(bcb);

   /*
   **	 If necessary, close the file for write in order to flush any
   **	 buffers still in memory.
   */
   if (bcb->lrec >= bcb->nrecs)
      {
FSdiag(ERx("FSsave: closing spill file\n"));
      if ((rc=SIclose(bcb->bffd)) != OK)
	 {
	 STprintf(msg,ERget(E_MF0004_Unable_to_close_file),
		  bcb->bfname, rc);
	 IIUFpan_Panic(bcb,msg);
	 }
      }
      bcb->bffd = NULL;

   /*
   **	 Open the file for read and read all the way to end-of-file,
   **	 so we will have the bottommost records in memory.
   */
FSdiag(ERx("FSsave: opening temp spill file '%s' for input\n"),bcb->bfname);
   if ((rc=SIfopen(&in_loc,ERx("r"), SI_TXT,512,&in_fp)) != OK)
      {
      STprintf(msg,ERget(E_MF0005_Unable_open_file_read),
	       bcb->bfname, rc);
      IIUFpan_Panic(bcb,msg);
      }

   /*
   **	Call the interactive terminal monitor equivalent of FTprnscr
   **	to read through the input file, and dump it to an output file.
   */

   if (is_file)
        IIUFgtf_ToFile(bcb,in_fp);
   else
	IIUFgtp_ToPrinter(bcb,in_fp);

   /*
   **	 Close input file
   */
FSdiag(ERx("FSsave: closing temp spill file\n"));
      SIclose(in_fp);

   /*
   **	 If necessary, re-open the spill file for append so we will be
   **	 positioned to add more records.
   */
   if (bcb->lrec >= bcb->nrecs)
      {
FSdiag(ERx("FSsave: opening spill file for append\n"));
      if ((rc=SIfopen(&bcb->bfloc,ERx("a"), SI_TXT,512,&bcb->bffd)) != OK)
	 {
	 STprintf(msg,ERget(E_MF0007_Unable_open_file_apnd),
		  bcb->bfname, rc);
	 IIUFpan_Panic(bcb,msg);
	 }
      }

FSdiag(ERx("FSsave: leaving\n"));
   return;
}
