/*
** Copyright (c) 2004 Ingres Corporation
*/
# include       <compat.h>
# include	<si.h>
# include       <st.h>
# include	<cv.h>
# include	<er.h>
# include	<pc.h>
# include 	<erfac.h>

/*
**	History
**
**	26-may-95 (hanch04)
**	    Ported for 64
**	15-jun-95 (albany)
**	    Fixed bogus non-call to usage.  Brought up
**	    to CL spec standards.
**	19-mch-96 (prida01)
**	    Make errfac global put it in common!hdr!hdr stops linking problem.
**	    so diag tools can use it. Saves duplication of code and
**	    easier when updating error. 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-sep-2002 (abbjo03)
**	    Add a proper exit from main to avoid bogus error messages on VMS.
**       3-Apr-2008 (hanal04) Bug 120202
**          Use SIprintf(s) new %X to get upper case hex values so that
**          E_LQ00e4 is correctly displayed as E_LQ00E4.
*/


static VOID usage( VOID );
static VOID get_num( char *nstr );
static VOID print_mess( char *fac, i4  mlow, i4  mhi );
static VOID what_fac( i4  num );
static VOID what_num( char *fac );
static VOID fac_mess( char *fac, i4  fbit, i4  mnum, i4  has_range, i4  rval );


char *Cmd;


main(i4 argc, char **argv)
{
	u_i4 mlow,mhi;
	char *range;
	i4 fbit;

/*	Cmd = *argv; */
	Cmd = "ERRHELP";

	if (argc == 1)
		usage();

	for (++argv; argc > 1; --argc,++argv)
	{
		CVupper(*argv);

		range = (char *)STindex((*argv)+1,"-",0);

		if (range != NULL)
		{
			*range = '\0';
			++range;
			if (CVahxl(range,&mhi) != OK)
				usage();
		}

		fbit = 0;

		switch(**argv)
		{
		case 'F':
			fbit = 1;	/* fall through */
		case 'S':
		case 'E':
			++(*argv);
			if (**argv != '_')
				usage();
			++(*argv);
			if (CVahxl((*argv)+2,&mlow) != OK)
				usage();
			(*argv)[2] = '\0';                       
			fac_mess(*argv,fbit,mlow,(range != NULL),mhi);
			break;
		case '-':
			++(*argv);
			switch(**argv)
			{
			case 'D':
				++(*argv);
				get_num(*argv);
				break;

			case 'X':
				++(*argv);
				if (CVahxl(*argv,&mlow) != OK)
					usage();
				what_fac((i4) mlow);
				break;
			case 'N':
				++(*argv);
				if (CVan(*argv,&fbit) != OK)
					usage();
				what_fac(fbit);
				break;
			case 'F':
				++(*argv);
				what_num(*argv);
				break;
			default:
				usage();
				break;
			}
			break;
		default:
			if (CVal(*argv,&mlow) != OK)
				usage();
			if (range != NULL)
				mhi |= mlow & 0xffff0000L;
			else
				mhi = mlow;
			print_mess(NULL,mlow,mhi);
/*
			if (CVahxl(*argv,&mlow) != OK)
				usage();
			if (range != NULL)
				mhi |= mlow & 0xffff0000L;
			else
				mhi = mlow;
			print_mess(NULL,mlow,mhi);
*/
			break;
		}
	}
	PCexit(OK);
}

static VOID fac_mess( char *fac, i4  fbit, i4  mnum, i4  has_range, i4  rval )
{
	i4 i;

#ifdef MT_DBG
	SIprintf("fac = %s\nfbit=%d\nmnum=%d\nhas_range=%d\nrval=%d\n",
		fac, fbit, mnum, has_range, rval);
#endif
	for (i=0; i < NUMFAC; ++i)
	{
		if (!STcompare(Factab[i].fac,fac))
		{
			mnum |= ((i4)fbit) << 28;
			mnum |= ((i4)(Factab[i].num)) << 16;
			if (has_range)
				rval |= mnum & 0xffff0000L;
			else
				rval = mnum;
			print_mess(fac,mnum,rval);
			return;
		}
	}

	SIprintf("No facility '%s'\n",fac);
}

static VOID what_num( char *fac )
{
	i4 i;

	for (i=0; i < NUMFAC; ++i)
	{
		if (!STcompare(Factab[i].fac,fac))
		{
			SIprintf("Facility %s = %d (x%x)\n",
					fac,Factab[i].num,Factab[i].num);
			return;
		}
	}

	SIprintf("No facility '%s'\n",fac);
}

static VOID what_fac( i4  num )
{
	i4 i;

	for (i=0; i < NUMFAC; ++i)
	{
		if (Factab[i].num == num)
		{
			SIprintf("Facility %d (x%x) = %s\n",
						num,num,Factab[i].fac);
			return;
		}
	}

	SIprintf("No facility number %d (%x)\n",num,num);
}

static VOID print_mess( char *fac, i4  mlow, i4  mhi )
{
	char nfac[12];

	if (fac == NULL)
	{
		STprintf(nfac,"<%d>",(mlow >> 16) & 0x7fff);
		fac = nfac;
	}

#ifdef MT_DBG
	SIprintf("fac=%d, mlow=%d, mhi=%d\n", fac, mlow, mhi);
#endif

	for ( ; mlow <= mhi; ++mlow)
		SIprintf("(%d)\n%s%04X %s\n\n",
				mlow, fac,(i4) (mlow & 0xffff),ERget(mlow));

}

static VOID usage( VOID )
{
	SIprintf("usage:\t%s <error-code>\n",Cmd);
	SIprintf("\t%s -f<facility-name>\n",Cmd);
	SIprintf("\t%s -n<decimal-facility-code>\n",Cmd);
	SIprintf("\t%s -x<hex-facility-code>\n",Cmd);
	SIprintf("\t%s -d<decimal-error-number>\n",Cmd);

	SIprintf("examples:\n\t%s E_US0067\n\t%s -n99\n\t%s -fQE\n\t%s -d6164481\n",
				Cmd,Cmd,Cmd,Cmd);
	PCexit(1);
}

static VOID get_num( char *nstr )
{
	char	ebuf[132];
	i4	i, fbit;
	u_i4 fno, eno;

	CVan(nstr,&eno);
	CVlx(eno, ebuf);
	fbit = 0;
	if (ebuf[0] == '1')
		fbit = 1;
	if (CVahxl(&ebuf[4], &eno) != OK)
	{
		SIprintf("Can't convert %s\n", &ebuf[2]);
		PCexit(1);
	}

	ebuf[4] = 0;
	if (CVahxl(&ebuf[2], &fno) != OK)
	{
		SIprintf("Can't convert %s\n", &ebuf[2]);
		PCexit(1);
	}

	for (i = 0; Factab[i].num != fno && i < NUMFAC; i++)
		;
	if (i == NUMFAC)
	{
		SIprintf("No such facility # %d \n", fno);
		return;
	}

	fac_mess(Factab[i].fac, fbit, eno, 0,0);

}
