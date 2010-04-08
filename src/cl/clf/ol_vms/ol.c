/*
**    Copyright (c) 1985, 2003 Ingres Corporation
*/

# include	<compat.h>
# include 	<gl.h>
# include	<ol.h>
# include	<me.h>
# include	<st.h>
# include	<ssdef.h>
# include	<descrip.h>
# include	<lib$routines.h>
# include	<str$routines.h>
# include	<stdlib.h>

/**
** Name:	ol.c -- OLpcall and supporting static routines, for calling
**			3GL procedures from 4GL.
**
**	Parameters:
**		func -- (input only) The address of the procedure to be called
**			with the passed parameters.
**
**		pv -- (input/output) Address of an array of parameter structures
**			describing the parameters to be passed.
**
**		argcnt -- (input only) The number of parameters to be passed.
**
**		lang -- (input only) The language in which the procedure was
**			written.
**
**		rettype -- (input only) The return type of the procedure.
**
**		retvalue -- (input/output) The place to put the return
**			  value;
**
** History:
**	xx/xxx (?) -- First written.
**
**	12/84 (jhw) -- UNIX 3.0 CL port (Pyramid & VAX 4.2).
**
**	Revision 3.0  85/05/15  19:15:35  arthur
**	Changed to return value returned by called routine.
**		
**	Revision 3.1  85/06/05  11:14:01  wong
**	Added conditional compilation and code for CCI Power6.
**		
**	Revision 3.2  85/07/05  13:46:50  wong
**	Added conditional compilation and code for ATTIS 3B5.
**		
**	Revision 3.3  85/07/17  10:41:48  wong
**	Added notes for MC68000.
**		
**	Revision 3.4  85/07/23  08:34:54  wong
**	Added conditional compilation and code for UTS/370.
**		
**	Revision 3.5  85/08/28  17:54:14  daveb
**	Fix wrong var name for SV
**
**	17-mar-1986 (joe)
**		Fix for passing strings byref to PASCAL.
**	14-apr-1987 (mgw)	Bug # 12190
**		Enable passing strings byref to Basic procedures by changing
**		mecharr[OLBASIC].ctype to VDESC (was SDESC) and appending the
**		right flag to the size descriptor when pushing the string onto
**		the stack for calling the procedure.
**	07-jul-87 (mgw)	Bug # 12605
**		Null out all chars after the input string to prevent
**		procedures from producing garbage in byref strings.
**	11-aug-87 (mgw) Bug # 12831
**		Fix for bug 12190 broke strings passed to basic procedures not
**		byref. The solution is to treat string descriptors for basic
**		procedures as VDESC's in the byref case, and as SDESC's in the
**		byvalue case. Also to Null out the part of OL_value between the
**		end of the string and the end of the maxstring allowable.
**	25-aug-87 (mgw) Bug # 13160
**		Boy do I feel stupid! I was going about this OLBASIC stuff all
**		wrong. The real fix is to pass down a Dynamic string descriptor
**		to BASIC procedures.
**	22-sep-87 (mgw) Bug # 13311
**		Fix it so that string function return values don't munge up
**		the byref fixit loop at the end.
**
**	Revision 6.2  89/04  wong
**	Corrected retrieval of return string descriptor from argument stack.
**	JupBug #4768.  Also, saved arg. count on stack could not be accessed
**	because of the stack allocation so this was changed to use a register
**	variable for a counter with the original value being the parameter
**	value.  JupBug #5378.
**
**	Revision 6.3  89/09  wong
**	Modified to use stack allocation routines (see also "ol.sed") to remove
**	dependency on Dec C compiler register allocation.
**	09/89 (jhw) -- Added support for Ada on VMS; apparently identical
**			to PL/1.  Fix for 13160 - send Dynamic string 
**			descriptor to BASIC routines using DDESC.
**	90/03  wong  Pass string length for string descriptor (SDESC) not
**			allocated length (in 'OL_size').  JupBug #4378.
**
**	Revision 6.5
**	16-apr-92 (emerson)
**		Made some changes so that ADA will return strings properly:
**		(a) Changed interface to iiolDesc (see oldesc.c).
**		(b) Changed mecharr[OLADA].srtype from VDESC to ACBSBDESC - 
**		    a new type for "area control block followed by string 
**		    descriptor with bounds".
**		(c) Modified the logic that handles strings returned by
**		    the 3GL procedure: treat srtype of ACBSBDESC and DDESC
**		    like SDESC, and use a slightly different technique
**		    to get the address of the string or its descriptor.
**	16-apr-92 (emerson)
**		Changed mecharr[OLBASIC].srtype from SDESC to DDESC. - this
**		is more nearly correct than SDESC, and allows iiolDesc 
**		to dispense with 'lang'.
**	06-jan-93 (davel)
**		Major cleanup! Eliminate the major hack, and combine 
**		olpcall.hak, oldesc.c, olret.c, oli.h all into this module.
**		Eliminate the use of a sed script to munge macro code generated
**		by compiling the pseudo C code contained in olpcall.hak. 
**		Made re-entrant (needed as now 3GL can call back into 4GL).
**		Eliminated old UNIX ifdefs.  The new approach builds a 
**		VMS calling stack in static memory, and uses the VMS 
**		LIB$CALLG function to call the 3GL procedure.
**	19-jan-93 (wolf)
**		Change #include from "ol.h" to <ol.h>
**	01/29/93 (dkh) - Added changes to allow floating point values
**			 to be passed correctly for the Alpha.
**	16-feb-93 (davel)
**		several 6.5 changes: 
**		1. Cast references to OL_PV's OL_value, which is not type PTR.
**		2. Modifications now that string arguments are passed 
**		   as OL_VSTR structures
**		3. Add support for decimal arguments (OL_DEC).
**	21-jun-93 (wolf)
**		Remove ctrl-D's that caused compiler errors on VMS.
**
**	15-Jul-1993 (donc) Added DECIMAL support for PL/1 and Basic. These
**		use the REF method under VMS.
**
**	15-Jul-1993 (donc) Bugs 34033 and 50113. VARCHAR strings passed to 
**		routines byref were having their values truncated upon return 
**		from the called procedure if the called procedure returned 
**		more characters than the calling routine provided.
**
**	16-Jul-93 (ed)
**		added gl.h
**
**	23-Jul-1993 (donc) Whiffed on submission. rev 7 was submitted with
**		no changes. Here is the real submission.
**
**	7/93 (Mike S) 
**		Add OL_PTR support.  For VMS we can treat a pointer as
**		an int; always on a VAX, for now on an ALPHA.
**	11-aug-93 (ed)
**		changed to match ol.h prototype
**	19-sep-93 (donc) Bug 55867
**		blank pad character strings in fortran up to OL_size as
**		opposed to NULL fill. 
**      2-July-1996 (strpa01)
**              Modifications to re-cast integer types to correct length as 
**              specified by OL_size. (Bug 77201)
**      26-aug-1998 (horda03)
**              In 6.4 the size of a VARCHAR passed as a parameter to a function was
**              the number of characters used. This caused problems when the function
**              invoked, returned more characters in the VARCHAR than those supplied
**              (see bugs 34033 and 50113). In OI, the maximum size of the VARCHAR is
**              supplied, Users upgrading from 6.4 to OI on AXP.VMS have come across
**              problems when their embedded fortran application use the "format(a)"
**              statement to format a character string. There is a limitation on AXP.VMS
**              of 134 characters for the "format" command. In 6.4 because we were setting
**              the size to the number of characters used, user applications were fine.
**              Now that we pass the full VARCHAR size, users are finding that their
**              applications now ACCVIO. This is NOT an INGRES problem, it is a limitation
**              on the AXP.VMS Fortran compiler. However, to allow users to upgrade, I have
**              added a user defined variable which when set will cause VARCHARs to be
**              handled as per 6.4. To utilise this feature the user must specify the
**              variable/logical II_VARCHAR_64_PAD. By default the VARCHAR size is used to
**              identify the length of the VARCHAR parameter. (Bug 91607).
**	9-feb-1998 (kinte01)
**		Standardize include statements for header files
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes and
**	   external function declarations
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**      30-oct-2002 (horda03) Bug 106895
**              ACCVIO when ADA function returns a string. Note, strings are
**              passed to ADA routines as DESCRIPTOR(S), the ADA routine
**              must specify this.
**	07-feb-2003 (abbjo03)
**	    Remove PL/1 as a supported language.
**      22-dec-2008 (stegr01)
**          Itanium VMS port.
*/

FUNC_EXTERN VOID NMgtAt();
FUNC_EXTERN VOID OLcall();

# define	MAX_ARGS	256

/*
** MECHSTRUCT Declaration:
**
** The 'mecharr' structure array describes, for each language, how
** parameters are to be passed for the four types:  integer, float, decimal,
** and string and how strings are returned.
** Each member can have a value that indicates how that
** type of parameter is to be passed.  These passing mechanisms are:
**
**	VALUE	-- by value (for return means returns like C)
**
**	REF	-- by reference (not used in returns)
**
**	SDESC	-- by string descriptor (reference, length) reference 
**		   (for return means caller passes a string descriptor).
**
**	VDESC	-- by variable string descriptor (string descriptor 
**			of the variable string ( length, string) on VMS)
**			(for return means that caller passes a varying
**			string descriptor).
**
**	VSTR	-- Used on returns, means that caller passes a varying
**		   string with no descriptor.
**
**	NONE    -- Means that the procedure can't return that type of value,
**		   or that we cannot pass this type of argument (e.g. 
**		   for decimal).
**
**	DDESC	-- by dynamic string descriptor
**
**	ACBSBDESC  by a so-called "area control block", followed
**		   immediately by a bounded string descriptor.
**		   This is the mechanism that ADA uses to return strings
**		   (assuming, as we do, that they're of an unconstrained type).
**		   See static function iiolDesc() for details.
**
*/

typedef struct
{
	i1	itype;
	i1	ftype;
	i1	dtype;
	i1	ctype;
	i1	srtype;
} MECHSTRUCT;

# define	VALUE		0
# define	REF		1
# define	SDESC		2
# define	VDESC		3
# define	VSTR		4
# define	NONE		5
# define	DDESC		6
# define	ACBSBDESC	7
# define 	PDDESC		8

static MECHSTRUCT mecharr[] = {
	VALUE,	VALUE,	NONE, VALUE,  VALUE,     /* OLC		0 */
	REF,	REF,	NONE, SDESC,  SDESC,     /* OLFORTRAN	1 */
	REF,	REF,	NONE, VDESC,  VSTR, 	 /* OLPASCAL	2 */
	REF,	REF,    REF,  DDESC,  DDESC,     /* OLBASIC	3 */
	REF,	REF,	REF,  VALUE,  NONE,	 /* OLCOBOL	4 */
	NONE,	NONE,	NONE, NONE,   NONE, 	 /* OLPL1 	5 */
	REF,	REF,	NONE, SDESC,  ACBSBDESC, /* OLADA 	6 */
};

/*
**	The structure below is the required 'extra parameter' for a called
**	ADA program that returns an 'unconstrained' string type.  (See
**	section 4.3.2 of the VAX Ada Programmer's Run-Time Reference Manual).
**	The 'desc' portion is used by other languages as well.
**	Note that other languages may technically use "different" descriptors
**	(e.g. dsc$descriptor_a or dsc$descriptor_s), but those descriptors
**	are in fact "prefixes" of dsc$descriptor_sb.
*/
typedef struct 
{
	struct
	{
		u_i4		acb_length;
		PTR			acb_pointer;
		i4			acb_zone;

	}				acb;
	struct	dsc$descriptor_sb	desc;

} DESC_ACB_SB;

typedef struct dsc$descriptor DESCRIPTOR;

static PTR iiolDesc ( i4 mech, DESC_ACB_SB *a, char *b );

STATUS
OLpcall (func, pv, argcnt, lang, rettype, retvalue)
VOID		(*func)();
register OL_PV	*pv;
i4		argcnt;
i4		lang;
i4		rettype;
OL_RET		*retvalue;
{
	f8	retf8;
	i4	reti4;
	char	*retstring;
	PTR	retptr;
	i4	olcnt = 0;
	i4	oltype;
	i4	olclass[MAX_ARGS];
	i4	*ol_class = olclass;
	i4	argpos = 0;

	f8	(*f8callg)() = (f8 (*)())lib$callg;
	i4	(*i4callg)() = (i4 (*)())lib$callg;
	char	*(*strcallg)() = (char * (*)())lib$callg;

	i4	i;
	register i4	argc = argcnt;
	struct {
		i4	c_argn;
		i4	c_arglist[MAX_ARGS];
	} callglist;
# define argn		callglist.c_argn
# define arglist	callglist.c_arglist
	register i4	*al = arglist;
	i4		descr_list[2*MAX_ARGS];
	i4		*dl = descr_list;
	i4		len;
	bool		got_byref_str = FALSE;
	OL_VSTR	*vstr;
	struct dsc$descriptor_d dyndesc;

	register MECHSTRUCT	*mech = &(mecharr[lang]);

	char		*ret_buffer;
	DESC_ACB_SB	acb;
	DESCRIPTOR	*d;
	char		*buf;

        /* B91607  - Allow users to specify the use of the VARCHAR padding method used in
        **           6.4. They can achieve this by specifying II_VARCHAR_64_PAD.
        */
        char *ii_varchar_pad;
        NMgtAt( "II_VARCHAR_64_PAD", &ii_varchar_pad);

	argn = argcnt;

	i = 0;
	/*
	** If we are returning a string by means other than like C does,
	** the first argument is a place to store the string.
	*/
	if (rettype == OL_STR && mech->srtype != VALUE)
	{
		if ( mech->srtype == NONE 
		  || retvalue == NULL
		  || (ret_buffer = retvalue->OL_string) == NULL
		   )
		{
			return FAIL;
		}

		/* assume retvalue->OL_string references allocation 
		** of OL_MAX_RETLEN.
		*/
		MEfill(OL_MAX_RETLEN, '\0', ret_buffer);
		*al++ = (i4)iiolDesc(mech->srtype, &acb, ret_buffer);
		buf = ret_buffer;
		d = (DESCRIPTOR *)&acb.desc;
		argn++;
		i = 1;
		olcnt++;
		*ol_class++ = 0;
	}
	/*
	** Push arguments left-to-right into 'arglist'.
	** i gets initialized above because returning strings
	** might put an extra argument on the arglist.
	*/
	for ( ; --argc >= 0 ; ++pv, argpos++ )
	{
		if ( ++i >= MAX_ARGS )
			return FAIL;

                if (abs(pv->OL_type) == OL_PTR)
                {
                        if (mech->itype == VALUE && pv->OL_type > 0)
                                *al++ = *(i4 *)pv->OL_value;
                        else
                                *al++ = (i4)pv->OL_value;

                        olcnt++;
                        *ol_class++ = 0;
                }
                else if (abs(pv->OL_type) == OL_I4)
                {
                        if (mech->itype == VALUE && pv->OL_type > 0)
                        {
                                switch (pv->OL_size)
                                {
                                        case sizeof(i1):
                                                *al++ = *(i1 *)pv->OL_value;
                                                break;

                                        case sizeof(i2):
                                                *al++ = *(i2 *)pv->OL_value;
                                                break;

                                        case sizeof(i4):
                                                *al++ = *(i4 *)pv->OL_value;
                                                break;
                                        default:
                                                *al++ = *(i4 *)pv->OL_value;
                                }
                        }
                        else
                                *al++ = (i4)pv->OL_value;
                        olcnt++;
                        *ol_class++ = 0;
                }
		else if (abs(pv->OL_type) == OL_F8)
		{
			if (mech->ftype == VALUE && pv->OL_type > 0)
			{
				if ( ++i >= MAX_ARGS )
					return FAIL;
				/* Assumes sizeof(f8) = 2 * sizeof(i4) */
				*al++ = *(i4 *)pv->OL_value;
				*al++ = *((i4 *)pv->OL_value + 1);
				++argn;

				olcnt++;
				*ol_class++ = 1;
			}
			else
			{
				*al++ = (i4)pv->OL_value;
				olcnt++;
				*ol_class++ = 0;
			}
		}
		else if (abs(pv->OL_type) == OL_DEC)
		{
			olcnt++;
			*ol_class++ = 0;

			/* only pass by-reference currently supported */
			if (mech->dtype == REF)
				*al++ = (i4)pv->OL_value;
			else
				return FAIL;
		}
		else  /* Must be a string */
		{
			OL_VSTR *vstr_val = (OL_VSTR *)pv->OL_value;
			char *str_val = &(vstr_val->string[0]);
                        int str_len;

			olcnt++;
			*ol_class++ = 0;

			/*
			** 12605 - first clear out extraneous garbage, including
			** the space allowed for the terminating EOS in the
			** pv->OL_size + 1'th place of pv->OL_value.
			*/
			len = vstr_val->slen;
			if ( lang == OLFORTRAN )
                        {
			    MEfill( (u_i2)(pv->OL_size - len + 1), ' ',
				 str_val + len);

                            /* B91607 - The size of the VARCHAR parameter is the number of characters
                            **          in use if II_VARCHAR_64_PAD is defined. Otherwise, its the
                            **          maximum number of characters that the VARCHAR may hold.
                            */
                            str_len = (ii_varchar_pad && *ii_varchar_pad) ? len : pv->OL_size;
                        }
			else
                        { 
			    MEfill( (u_i2)(pv->OL_size - len + 1), '\0',
				 str_val + len);

                            str_len = pv->OL_size;
                        }

			if (mech->ctype == VALUE)
				*al++ = (i4)str_val;
			else if (mech->ctype == SDESC)
			{
				*al++ = (i4)dl;
				*dl++ = str_len;
				*dl++ = (i4)str_val;
			}
			else if (mech->ctype == VDESC)
			{
				/*
				** If passed by reference, note it.
				*/
				if (pv->OL_type < 0)
					got_byref_str = TRUE;
				*al++ = (i4)dl;
				*dl++ = pv->OL_size;
				*dl++ = (i4)vstr_val;
			}
			else if (mech->ctype == DDESC)
			{
				/*
				** If passed by reference, note it.
				*/
				if (pv->OL_type < 0)
					got_byref_str = TRUE;
				dyndesc.dsc$w_length = 0;
				dyndesc.dsc$b_dtype = DSC$K_DTYPE_T;
				dyndesc.dsc$b_class = DSC$K_CLASS_D;
				dyndesc.dsc$a_pointer = NULL;
				/* call STR$COPY_R to properly fill the 
				** string pointer.
				*/
				if(str$copy_r(&dyndesc, &len, 
					str_val) != SS$_NORMAL)
					return FAIL;
				/*
				** 0x020e0000 | len is the first 4 bytes of the
				** dyndesc structure initialized above.
				** DSC$K_CLASS_D = 2 and DSC$K_DTYPE_T = 0x0e
				*/
				*al++ = (i4)dl;
				*dl++ = len | 0x020e0000;
				*dl++ = (i4) dyndesc.dsc$a_pointer;
			}
		}
	}

# if defined(ALPHA) || defined(IA64)
	switch(rettype)
	{
	  case OL_F8:
		oltype = 2;
		retptr = (PTR) &retf8;
		break;

	  case OL_STR:
		oltype = 1;
		retptr = (PTR) &retstring;
		break;

	  case OL_I4:
	  case OL_PTR:
		oltype = 1;
		retptr = (PTR) &reti4;
		break;

	  default:
		oltype = 0;
		retptr = NULL;
		break;
	}

	OLcall(func, olcnt, olclass, callglist.c_arglist, oltype, retptr);

# else
	/* call the stupid thing */
	switch (rettype)
	{
	    case OL_F8:
		retf8 = (* f8callg)(&callglist, func);
		break;
	    case OL_STR:
		retstring = (* strcallg)(&callglist, func);
		break;
	    default:
		reti4 = (* i4callg)(&callglist, func);
		break;
	}
# endif
	al = arglist;

	/*
	** If we are returning a string other than how C returns
	** a string, move the right pointer into retstring.
	** 
	** In some languages (PL/1 and BASIC) the called procedure
	** allocates a new buffer to contain the return value.
	** In these cases we move the value using
	** the length given into our buffer which has NULLs in it.
	** In other languages (FORTRAN and PASCAL), the original buffer
	** (specified in the descriptor) contains the return value, so
	** so no copy has  to be done.
	*/
	if (rettype == OL_STR && mech->srtype != VALUE)
	{
		al++;
		if (mech->srtype == VDESC)
		{
			vstr = (OL_VSTR *) d->dsc$a_pointer;
			if ((char *) vstr != buf)
				STlcopy(vstr->string, buf, vstr->slen);
			else
				buf += 2;   /* Make it point to the string */
		}
		else if (mech->srtype == VSTR)
		{
			vstr = (OL_VSTR *) *arglist;
			if ((char *) vstr != buf)
				STlcopy(vstr->string, buf, vstr->slen);
			else
				buf += 2;   /* Make it point to the string */
		}
		else
		{
			if (d->dsc$a_pointer != buf)
                        {
                                /* New storage allocated for the return value, so
                                ** copy it to the return buffer, and release the memory
                                **
                                ** If the srtype is ACBSDDESC, then the length of the
                                ** string returned is the acb.acb_length.
                                */
                                if (mech->srtype == ACBSBDESC)
                                {
                                   d->dsc$w_length = acb.acb.acb_length;
                                }

                                if (d->dsc$w_length > OL_MAX_RETLEN)
                                   d->dsc$w_length = OL_MAX_RETLEN;

				STlcopy(d->dsc$a_pointer, buf, d->dsc$w_length);
                                free(d->dsc$a_pointer);
                        }
		}
		retstring = buf;
	}
	/*
	** If we passed any strings to a VDESC or DDESC byref, then fix them
	** up here.
	*/
	if (got_byref_str)
	{
		/* note that the al++ below isn't quite right: for floats
		** that are passed by value, there needs to be an additional
		** al++.  Since floats are only passed by value to C, and
		** C doesn't have any weird "got_byref_str" possibilities,
		** there's no need currently to worry about this...
		*/
		for (pv -= (argc = argcnt); --argc >= 0; pv++, al++)
		{
			if (pv->OL_type == -OL_STR)
			{
				OL_VSTR *vstr_val = (OL_VSTR *)pv->OL_value;
				char *str_val = &(vstr_val->string[0]);
				i2   *slen    = &(vstr_val->slen);
				/*
				** assumes all descriptors in use have the
				** same structure (they do, they're just
				** used differently).
				*/
				d = (DESCRIPTOR *) *al;
				if (mech->ctype == VDESC)
				{
					vstr = (OL_VSTR *) d->dsc$a_pointer;
					if (vstr->slen > pv->OL_size)
						vstr->slen = pv->OL_size;
					*slen = vstr->slen;
					STlcopy(vstr->string, str_val, *slen);
				}
				else if (mech->ctype == DDESC)
				{
					STlcopy(d->dsc$a_pointer, str_val,
					(i2) (
					((i4) d->dsc$w_length > pv->OL_size) ?
							pv->OL_size :
							d->dsc$w_length));
				}
			}
		}
	}

	/* set the return value */
	if (retvalue != NULL)
	{
	    switch (rettype)
	    {
		case OL_I4:
		case OL_PTR:
		    retvalue->OL_int = reti4;
		    break;
	
		case OL_F8:
		    retvalue->OL_float = retf8;
		    break;

		case OL_STR:
		    retvalue->OL_string = retstring;
		    break;

		default:
		    break;
	    }
	}
	return OK;
}

/*{
** Name: 	iiolDesc() -	Build a VMS descriptor for returning a string.
**
** Description:
**	Builds a VMS descriptor for a string.  The descriptor is always
**	built the same way no matter what language is being called.  It
**	points to a fixed size buffer which is big enough for a string
**	of size 4096.  The buffer is 3 bigger in case the language expects
**	a varying string in which case it will use 2 characters for the
**	length of the string.  The buffer is initialized with NULLs so
**	that after the procedure runs it is a C string.
**
** Inputs
**		mech	The mechanism used for returning strings.
** Outputs
**		desc	A pointer to a descriptor.
**		buf	A pointer to the buffer it will use.
**	Returns:
**		A pointer to a buffer, descriptor, or area control block,
**		as appropriate for the specified mechanism.
**		This pointer will be used as the 'extra parameter'
**		that represents the string returned by the 3GL procedure.
**
** History:
**	07/90 (jhw) - Increased character buffer size to 4096.  Bug #21462.
**	08/90 (jhw) - Correct last change; compare with 'lang' not 'mech' for
**			OLBASIC.  Bug #32967.
**	04/11/92 (Emerson)
**		Added logic to handle the new 'mech' value ACBSBDESC (for Ada).
**		Changed the return type to PTR.
**		Added logic for new srtype 'mech' value DDESC (for BASIC);
**		removed logic that explicitly tested for lang == OLBASIC.
**		Removed 'lang' parameter.  Added 'desc' parameter.
**		Cleaned up the logic a bit.
**      30-oct-2002 (horda03) Bug 106895.
**              The documentation on how ADA function return strings is a little
**              flaky to say the least. You are supposed to pass an Area Control
**              Block and a Descriptor (our ACBSBDESC) as the first parameter.
**              Now, the documentation states that the ACB is a (i4) len, ptr
**              and (i4) zone. Where len is 0 (meaning no area created) or the
**              size of the area created to hold the return string, ptr is a
**              pointer to the return string and zone is 0 (use dynamic memory)
**              or a zone id. What I've found is if you populate the ACB as
**              specified an ACCVIO occurs, if the zone is set to the address of
**              the return string (ptr is also the address of the return string)
**              then the string is returned. BUT, if the area isn't big enough
**              then rather than a new piece of memory being created, it just
**              overwrites the return string buffer.
**
**              One other fault of this is that the length of the string returned
**              isn't specified. So, not only do you risk corrupting memory, you
**              don't know the length of the returned string.
**
**              The workaround, is not to specify the return area, set the ACBSBDESC
**              structure to 0. The ADA function will then allocate storage for the
**              return string, and returns the length of the string too. Phew!!
*/

static PTR
iiolDesc ( i4 mech, DESC_ACB_SB *a, char *b )
{
	DESCRIPTOR *d;

	d = (DESCRIPTOR *)&a->desc;
	d->dsc$a_pointer = b;
	d->dsc$w_length = OL_MAX_RETLEN;
	d->dsc$b_dtype = DSC$K_DTYPE_T;

	switch (mech)
	{
	  case ACBSBDESC:	/* ADA */
		/* DOPE_VECTOR is the default PARAM MECH for a STRING
		** returned by a function. In testing I couldn't find
		** any values that worked for the ACB structure apart
		** for setting the memory to 0. (horda03)
		*/
		MEfill(sizeof(DESC_ACB_SB), 0, a);

		return (PTR)a;

	  case DDESC:		/* BASIC */
		d->dsc$b_class = DSC$K_CLASS_D;
		d->dsc$a_pointer = 0;
		return (PTR)d;
	  case SDESC:
		d->dsc$b_class = DSC$K_CLASS_S;
		return (PTR)d;
	  case VDESC:
		d->dsc$b_class = DSC$K_CLASS_VS;
		return (PTR)d;
	  case VSTR:
		return (PTR)b;
	  default:
		return NULL;
	}
}

