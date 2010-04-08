/*
** Copyright (c) 2004 Ingres Corporation
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


/*
**    FSrecfi  --  Get one single record from the file
**
**    FSrecfi is called only by FSmore when it wants one record of
**    output from the file being browsed. The file should already have
**    been opened; just check the return code to make sure it was opened
**    sucessfully.
**
**    Returns: An integer representing the length of the record being
**	       returned. A length of '-1' means the request is complete
**	       and there is no further output data.
**  History:
**	11/12/87 (dkh) - FTdiag -> FSdiag.
**      03-oct-1996 (rodjo04)
**              Changed "ermf.h" to <ermf.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/
i4
FSrecfi(bcb,bfr,bfrlen)
   BCB	 *bcb;
   char	 *bfr;
   i4	  bfrlen;
{
   register i4  len;
   i4	     rc;

   FSdiag(ERx("FSrecfi entered\n"));
   if (bcb->req_complete)
   {
	FSdiag(ERx("FSrecfi: complete=%d!!!\n"), bcb->req_complete);
	return(-1);
   }

   if (bcb->ifrc != OK)
   {
      STprintf(bfr,
	       ERget(E_MF0001_Unable_to_open_file),
		bcb->ifname,bcb->ifrc);
      bcb->req_complete = TRUE;
      return(STlength(bfr));
   }

   if ((rc=SIgetrec(bfr, bfrlen, bcb->iffd)) == OK)
   {
      len = STlength(bfr) - 1;
      bfr[len] = EOS;
      return(len);
   }
   else
   {
      bcb->req_complete = TRUE;
      FSdiag(ERx("FSrecfi: closing input file\n"));
      SIclose(bcb->iffd);
      bcb->iffd = NULL;
      return(-1);
   }
}
