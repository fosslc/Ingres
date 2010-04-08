/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<cv.h>
#include	<lo.h> 
#include	<me.h> 
#include	<nm.h>
#include	<si.h> 
#include	<st.h> 
#include	<ds.h>
#include	<tm.h>
#include	<er.h>
#include	<pm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fedml.h>
#include	<fe.h>
#include	<ug.h>

# include	<abclass.h>
# include	<feconfig.h>
# include	<fdesc.h>
# include	<abfrts.h>
# include	<abftrace.h>
# include	<abfcnsts.h>

#include	<eqlang.h>
#include	<gn.h>
#include	"gncode.h"
#include	"erab.h"

#include        <oocat.h>
#include        <oocatlog.h>
#include        "abcatrow.h"

/**
** Name:	abftab.c - Build Link Extract Files Module.
**
** Description:
**	Contains routines that build some of the runtime tables
**	needed by abf.	These runtime tables describe the objects
**	manipulated by the runtime system.
**
**	Essentially, this hides the ABF interface to DS.  Defines:
**
**	iiabGnImgExtract()	build data structures for application image.
**	IIABieIntExt
**	IIABtfTabFile
**	IIABibImageBuild
**
** History:
**	Revision 2.0  83  joe
**	Initial revision.
**
**	Revision 6.2  89/02  bobm
**	add IIABieIntExt, IIABtfTabFile
**	89/08  wong  Numerous cascading problems with database procedure
**	function reference (which should be NULL) generation.  JupBugs #7077
**	(now corrected in "unix!clf!ds!ds.c"), #7535 (caused by incorrect fix
**	for #7077), and #7658 (caused by Sun compiler failing to detect or
**	correctly generate code for a local declaration "HLPROC *x = x".)
**
**	Revision 6.3/03/00  89/07  wong
**	Added support for system functions.
**	12/19/89 (dkh) - VMS shared lib changes - References to IIarDbname
**			 is now through IIar_Dbname.
**	90/07  wong  Added support for language-dependent constants image
**	files generation.
**
**	Revision 6.4
**	02-dec-91 (leighb) DeskTop Porting Change:
**		Added routines to pass data across ABF/IAOM boundary.
**		IIABibImageBuild() - Generate Interpreted Data Structures -
**		added.  This is a revival of the 5.0 MSDOS code.
**
**	Revision 6.5
**	11-jan-92 (davel)
**		Changes for global constants.
**	10-aug-92 (davel)
**		Changes to support decimal global constants, and overall
**		changes to the way lengths of non-numeric global constants
**		are determined.
**	11/21/92 (dkh) - For system objects, changed the code to use
**			 iiarSysHash instead of iiarHash.  This is needed
**			 since the name of the system objects are READONLY
**			 on the alpha and can't be changed.
**		IT IS IMPORTANT THAT HASHING OF ANY PREALLOCATED SYSTEM
**		OBJECTS USE THE IIarSysHash ROUTINE TO PREVENT AVs ON ALPHA.
**	07-dec-92 (davel)
**		Initial changes for PM constant override.  Also has fixes
**		related to change in the way DS handles DSD_STR and DSD_SPTR.
**	07-dec-92 (davel)
**		Pass HLPROC flags info in fid member of ABRTSPRO, which is
**		unused for host language procedures.
**	14-dec-92 (essi)
**		Implemented the global constant file override for both 'go'
**		and 'image' options. 
**		
**	14-jul-93 (essi)
**		Changed the PM calls with PMm calls. PMm calls provide
**		explicit context. 
**      10-sep-93 (kchin) bug # 51513
**              Change type of 'value' from i4 to PTR in iiabGnImgExtract(), 
**		as it will be holding address.  On 64-bit pointer machine,
**		assigning a 64-bit address to a i4 (32-bit integer) will 
**		result in truncation.
**		Add a temp variable, 'i4_value', since CVal() expects an
**		i4 variable.
**	25-oct-1993 (mgw)
**		Bring kchin's 6.4 change into 6.5 and use i4 for value
**		variable sent down to CVal instead of i4 (if CVal is taking
**		an i4 that's counter to it's documented interface).
**      16-Aug-95 (fanra01)
**              Added the definition of NT_GENERIC to enable USE_ABHDRINCL.
**	10-feb-1998 (fucch01)
**		Had to cast 2nd param's in most calls of DSwrite as PTR,
**		which is char *, to satisfy sgi_us5's picky compiler...
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	12-Jan-2005 (schka24)
**	    DB_DATA_VALUE now contains collID, need to fix here since
**	    we output C structures textually.
**    25-Oct-2005 (hanje04)
**        Add prototype for makeRec() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
[@history_template@]...
*/

GLOBALREF APPL_COMP	*IIam_Frames;	/* system frames */			 
GLOBALREF APPL_COMP	*IIam_Procs;	/* system procedures */			 
GLOBALREF APPL_COMP	*IIam_Globals;	/* system global variables */		 

GLOBALREF char	**IIar_Dbname;

GLOBALREF char	*IIabRoleID;
GLOBALREF char	*IIabPassword;

FUNC_EXTERN char      *UGcat_now();
FUNC_EXTERN STATUS    afe_dec_info();
FUNC_EXTERN char      *IIABqcfQueryConstantFile();
 
static STATUS	make_rtt();
 
static VOID	_genFrame();
static VOID	_genFrmExtract();
static VOID	_genAttrs();
static VOID	_genTypExtract();
static VOID	_genProcExtract();
static VOID	_genGlobalExtract();
static char	*_formSym();
static VOID	dsd_wrform();
static VOID	dbd_dswr();
static VOID	dsd_paddr();
static i4	get_language();
static char	*get_val_string();
static PTR	rt_alloc();
static VOID	rt_forms();
static VOID	rt_frames();
static VOID	rt_procs();
static VOID	rt_globs();
static VOID	rt_types();

static RECDEF	*getTypes();

static makeRec(
		RECDEF 		*rdp,
		ABRTSTYPE	*sp);

FUNC_EXTERN i4	iiarThTypeHash();

/*
** stuff for in-memory version of runtime table.  There are three
** arrays of structures, with their current lengths.  We just keep
** high water marks rather than thrashing the allocation routines.
** This is somewhat wasteful in that we allocate an ABRTSVAR for each
** frame, rather than the size for the union element we actually need.
** The array pointers are in Rtobj, together with the actual applicable
** lengths for the current instance.  The "outside" length variables
** are the current allocated sizes of the arrays.
*/
static ABRTSOBJ Rtobj ZERO_FILL;

static i4  Rt_forms = 0, 
	Rt_frames = 0, 
	Rt_procs = 0, 
	Rt_globs = 0,
	Rt_types = 0;

/* Constants data area */
static PTR	Rt_data = NULL;
static i4	Rt_dsize = 0;

# ifdef UNIX
# define USE_ABHDRINCL
# endif

# ifdef DGC_AOS
# define USE_ABHDRINCL
# endif

# ifdef hp9_mpe
# define USE_ABHDRINCL
# endif

# if defined (PMFE) || defined (NT_GENERIC)
# define USE_ABHDRINCL
# endif

# ifdef USE_ABHDRINCL
/*
** Name:	_includeFile() -	Generate #include Statement.
**
** Description:
**	Generate the include statement for the header file that must be
**	be included into the "abextract.c" file.
*/
static VOID
_includeFile ( file )
FILE		*file;
{
	LOCATION	path;
	char		*cp;

	NMgtAt(ERx("II_ABFURTS"), &cp);
	if ( cp == NULL || *cp == EOS )
	{
		NMloc(FILES, FILENAME, ERx("abfurts.h"), &path);
		LOtos(&path, &cp);
	}
	SIfprintf( file, ERx("# include\t\"%s\"\n\n"), cp);
}
# endif

/*{
** Name:	iiabGnImgExtract() -	Build the Link Extract File for
**						Linking an Application Image.
** Description:
**	Generate the runtime data structures for an application image and write
**	them into the link extract file.
**		If any of the frames in the application need their
**		tables created then go do that.
**
** Inputs:
**	app	{APPL *}  The application for which to create the extract file.
**	file	{FILE *}  The file in which to write the runtime tables.
**
** Returns:
**	{STATUS}  OK	success
**		  FAIL	failure
**
** Side Effects:
**	Allocates some dynamic space.
**
** Trace Flags:
**	ROUT
**
** History:
**	Written 7/26/82 (jrc)
**	11-oct-89 (mgw)
**		add NULL arg to IIGNssStartSpace() call since it expects one.
**	11-sep-86 (mgw) bug # 10327
**		Don't write out frame structure if frame doesn't exist.
**	07/89 (jhw) - Abstracted frame and procedure extract generation
**		code into static functions, and added generation of system
**		frames and procedures.
**	07/90 (jhw) -- Added call to 'iiabGnConstImg()' to generate language-
**		dependent constants image files.
**	1/94 (donc) Bug 56979
**		Entries in abextract file for global constants declared a
**		char_array with a dimension equal to the # of letters the
**	 	constant not the declared length of the constant. As a result
**		when data space is allocated for the constants, too little
**		is allocated causing access to constants via adf routines to  
**		potentially contain garbage characters or the contents of 
**		some different global constant.
**      27-Jul-98 (islsa01)
**              Fixed bug# 86115 in the section of DB_TXT_TYPE.  Allows you to
**              override the varchar value with constant string.
**              So the db_length would return the actual length of constant
**              string, not the length of varchar string.
**      22-SEP-98 (kinpa04)
**		Adjusted above fix so that the db_length is not truncated by a
**		small default constant. Above fix prevented the run-time
**		override with -constants_file with length of constants greated
**		than default.
**      08-July-99 (islsa01)
**            Fixed bug# 93232 in the section of DB_CHR_TYPE / DB_CHA_TYPE.
**            Allows you to override the char value with constant string.
**            So the db_length would return the actual length of constant
**            string, not the length of char string.
**      06-Jan-2004 (zhahu02)
**            Updated to avoid SIGSEGV (INGCBT505/bug111555) .
*/
VOID
iiabGnImgExtract ( app, file )
APPL	*app;
FILE	*file;
{
	register APPL_COMP	*objl;
	register i4		cnt;
	register i4		*hp;
	i4			hvalue;
	i4			frmcnt;
	i4			procnt;
	i4			glocnt;
	i4			typcnt;
	char			*nm_frm;
	char			*nm_pro;
	char			*nm_glo;
	char			*nm_typ;
	char			*nm_fhash;
	char			*nm_phash;
	char			*nm_ghash;
	char			*nm_thash;
	i4			hash[ABHASHSIZE];
	RECDEF			*typelist;
	char                    *datestamp = UGcat_now();
	i4			app_lang;
	char			*cons_file;
	PM_CONTEXT		*cntx;

#ifdef txROUT
	if (TRgettrace(ABTROUTENT, -1))
		abtrsrout(ERx(""), (char *) NULL);
#endif

	DSbegin(file, ABEXT_DSL, ABEXTNAME);
#ifdef CMS
	/*
	**  Generate a reference to C@TO@LN for CMS language interface
	**  routines
	*/
	DSinit(file, DSL_MACRO, DSA_UN, ABCTOLANG, DSV_GLOBAL);
	DSwrite(DSD_XADDR, ERx("C@TO@LN"), 0);
	DSfinal();
#endif

#ifdef USE_ABHDRINCL
	/* Write the include statement, "abfurts.h" */

	_includeFile( file );
#endif

	/* Unique name space for local symbols.  */

	if (IIGNssStartSpace(AGN_LOCAL, AGN_LPFX, AGN_SULEN, AGN_STLEN,
				GN_MIXED, GNA_XUNDER, (VOID (*)())NULL) != OK)
		IIUGerr(S_AB001F_IIGNssStartSpace, UG_ERR_FATAL, 0);

	/* Set up component names */

	IIGNgnGenName(AGN_LOCAL, ERx("frames"), &nm_frm);
	IIGNgnGenName(AGN_LOCAL, ERx("procs"), &nm_pro);
	IIGNgnGenName(AGN_LOCAL, ERx("globs"), &nm_glo);
	IIGNgnGenName(AGN_LOCAL, ERx("types"), &nm_typ);
	IIGNgnGenName(AGN_LOCAL, ERx("fhash"), &nm_fhash);
	IIGNgnGenName(AGN_LOCAL, ERx("phash"), &nm_phash);
	IIGNgnGenName(AGN_LOCAL, ERx("ghash"), &nm_ghash);
	IIGNgnGenName(AGN_LOCAL, ERx("thash"), &nm_thash);

	/* Write out the frame structures and associated form structures */

	/* System frames */
	for ( objl = IIam_Frames ; objl != NULL ; objl = objl->_next )
		_genFrame(file, objl);

	/* Application frames */
	for ( objl = (APPL_COMP *)app->comps ; objl != NULL ;
			objl = objl->_next )
	{
		switch ( objl->class )
		{
		  case OC_OSLFRAME:
		  case OC_MUFRAME:
		  case OC_APPFRAME:
		  case OC_UPDFRAME:
		  case OC_BRWFRAME:
		  case OC_OLDOSLFRAME:
		  case OC_RWFRAME:
		  case OC_QBFFRAME:
		  case OC_GRFRAME:
			_genFrame( file, objl );
		}
	}

	/* Fill in an empty hash table */

	for (hp = hash ; hp < &hash[ABHASHSIZE] ; ++hp)
	{
		*hp = -1;
	}

	/*
	** Write out the frame extract table, an array of ABRTSFRM structures.
	**
	**	Note:  System frames must be written out first so that
	**	user-defined frames will override the system ones because
	**	of hash table overflow chaining.  (That is, user-defined
	**	frames should cause the system frames to be chained and since
	**	they have the same name, the user-defined frames will always
	**	be found first.)
	*/
	DSinit(file, ABEXT_DSL, DSA_C, nm_frm, DSV_LOCAL, ERx("FRMARRAY"));
	cnt = 0;

	/* System frames */
	for ( objl = IIam_Frames ; objl != NULL ; objl = objl->_next )
	{
		hvalue = iiarSysHash(objl->name);
		_genFrmExtract(objl, hash[hvalue]);
		hash[hvalue] = cnt++;
	}

	/* Application frames */
	for ( objl = (APPL_COMP *)app->comps ; objl != NULL ;
			objl = objl->_next )
	{
		switch ( objl->class )
		{ /* frames only */
		  case OC_MUFRAME:
		  case OC_APPFRAME:
		  case OC_UPDFRAME:
		  case OC_BRWFRAME:
		  case OC_OSLFRAME:
		  case OC_OLDOSLFRAME:
		  case OC_RWFRAME:
		  case OC_QBFFRAME:
		  case OC_GRFRAME:
			hvalue = iiarHash(objl->name);
			_genFrmExtract(objl, hash[hvalue]);
			hash[hvalue] = cnt++;
			break;
		} /* end frames */
	}
	frmcnt = cnt;
	DSfinal();

	/* Write the hash table, just an array of i4's */

	DSinit(file, ABEXT_DSL, DSA_C, nm_fhash, DSV_LOCAL, ERx("natarray") );
	for (hp = hash; hp < &hash[ABHASHSIZE] ; ++hp)
	{
		DSwrite(DSD_NAT, (PTR)(i4)*hp, 0);
		*hp = -1;
	}
	DSfinal();

	/*
	** Write out the procedure table, an array of ABRTSPRO structures.
	**
	**	Note:  System procedures must be written out first so that
	**	user-defined procedures will override the system ones because
	**	of hash table overflow chaining.  (That is, user-defined
	**	procedures will cause the system procedures to be chained and
	**	since they have the same name, the user-defined procedures will
	**	always be found first.)
	*/
	DSinit(file, ABEXT_DSL, DSA_C, nm_pro, DSV_LOCAL, ERx("PROCARRAY"));
	cnt = 0;

	/* System procedures */
	for ( objl = IIam_Procs ; objl != NULL ; objl = objl->_next )
	{
		hvalue = iiarSysHash(objl->name);
		_genProcExtract(objl, hash[hvalue]);
		hash[hvalue] = cnt++;
	}

	/* Application procedures */
	for ( objl = (APPL_COMP *)app->comps ; objl != NULL ;
			objl = objl->_next )
	{
		switch ( objl->class )
		{
		  case OC_HLPROC:
		  case OC_DBPROC:
		  case OC_OSLPROC:
			hvalue = iiarHash(objl->name);
			_genProcExtract(objl, hash[hvalue]);
			hash[hvalue] = cnt++;
			break;
		} /* end switch */
	} /* end for */
	procnt = cnt;
	DSfinal();

	/* Procedure table hash, an array of i4's */
	DSinit(file, ABEXT_DSL, DSA_C, nm_phash, DSV_LOCAL, ERx("natarray"));
	for (hp = hash; hp < &hash[ABHASHSIZE] ; ++hp)
	{
		DSwrite(DSD_NAT, (PTR)(i4)*hp, 0);
		*hp = -1;
	}
	DSfinal();

	/* Application constants: for backward compatibilty, we need to 
	** determine which of the constant values will be used - by default
	** use the zeroth value.  If the II_APPLICATION_LANGUAGE logical is
	** set, and it translates to a valid natural langauge, use the
	** constant value associated with that language code.
	**
	** Also, check to see if a Constant's file has been specified - if so,
	** open that file now (using PM), and remember to check for an
	** overridden value for each constant.
	*/
	app_lang = get_language();
	cons_file = IIABqcfQueryConstantFile();
	if (cons_file != NULL)
	{
		LOCATION	loc;
		char		locbuf[MAX_LOC+1];

		_VOID_ IIUGvfVerifyFile ( cons_file, UG_IsExistingFile, 
			FALSE, TRUE, UG_ERR_ERROR );
		STcopy(cons_file, locbuf);
		LOfroms( PATH & FILENAME, locbuf, &loc );

		PMmInit( &cntx );
	        if ( PMmLoad( cntx, &loc,(PM_ERR_FUNC *) NULL)  != OK )
		{
			IIUGerr(E_AB00CA_BadConstantFile,
				UG_ERR_ERROR, 1, cons_file);
		}
	}

	for ( objl = (APPL_COMP *)app->comps ; objl != NULL ;
			objl = objl->_next )
	{
		if ( objl->class == OC_CONST )
		{
			f8			float_val;
			register CONSTANT	*xconst;
			DB_DATA_VALUE		*dbv;
			char			*ctype_name;
			i4			i4_value;
			PTR			value;
			i4			ds_type;
			i4			length = 0;
			char			str_type[ABBUFSIZE+8+1];
			char 			dec_buffer[DB_MAX_DECLEN];
			char 			cha_buffer[DB_MAXSTRING+1];
			bool			is_varchar = FALSE;
			char			*value_str;
			STATUS			status;	
                        AB_CONSTANT             *max_len_const_str;
                        AB_CONSTANT             *max_char_len_const_str;

			IIGNgnGenName(AGN_LOCAL, objl->name, &(objl->locsym));
			xconst    = (CONSTANT *)objl;
			dbv       = (DB_DATA_VALUE *)&xconst->data_type;

			/* if there's a constant's file open, check to see if
			** should be overriden.
			*/
			if (cons_file && app_lang == 0)
			{
				status = PMmGet(cntx, objl->name, &value_str);
                                if (status != OK)
					value_str = get_val_string(xconst,
						app_lang);
			}
			else
			{
				value_str = get_val_string(xconst, app_lang);
			}
			switch ( dbv->db_datatype )
			{
			  case DB_INT_TYPE:
			  {
				status = CVal(value_str, &i4_value);
				if (status != OK)
				{
					IIUGerr(E_AB00CE_IntConvFailed,
						UG_ERR_ERROR, 1, objl->name);
				}
				value = (PTR)i4_value;
				if ( dbv->db_length == 1 )
				{
					ds_type = DSD_I1;
					ctype_name = ERx("i1");
				}
				else if ( dbv->db_length == 2 )
				{
					ds_type = DSD_I2;
					ctype_name = ERx("i2");
				}
				else /* ( dbv->db_length == 4 ) */
				{
					ds_type = DSD_I4;
					ctype_name = ERx("i4");
				}
				length = dbv->db_length;
				break;
			  }
				
			  case DB_FLT_TYPE:
			  {
				status = CVaf(value_str, '.', &float_val);
				if (status != OK)
				{
					IIUGerr(E_AB00CF_FloatConvFailed,
						UG_ERR_ERROR, 1, objl->name);
				}
				value = (PTR)&float_val;
				if ( dbv->db_length == 4 )
				{
					ds_type = DSD_F4;
					ctype_name = ERx("f4");
				}
				else /* ( dbv->db_length == 8 ) */
				{
					ds_type = DSD_F8;
					ctype_name = ERx("f8");
				}
				length = dbv->db_length;
				break;
			  }

			  case DB_DYC_TYPE:
			  case DB_VCH_TYPE:
			  case DB_TXT_TYPE:
			  {
				if (value_str == NULL)
				length = 0;
				else
				length = STlength(value_str);

                             /*
                              Resets the db_length if file constant is greater
                              */
			if (( length > dbv->db_length - DB_CNTSIZE ) && (length > 0)) 
			{
                                dbv->db_length = STlength(value_str)+DB_CNTSIZE;

                               /*
                                 if db_length is greater than maximum length,
                                 the db_length is set to maximum length to
                                 prevent any buffer overflow.
                               */
                                if (dbv->db_length > sizeof(max_len_const_str->value ))
                                {
                                        dbv->db_length=sizeof(max_len_const_str->value);
                                }

				length = dbv->db_length - DB_CNTSIZE;
				*( value_str + length ) = EOS;
			}
				value = (PTR)value_str;
				ctype_name = STprintf(str_type, 
					ERx("TEXT_STR(%d)"), length);
				ds_type = DSD_STR;
				length++;	/* + EOS */
				is_varchar = TRUE;
				break;
			  }
			  case DB_CHR_TYPE:
			  case DB_CHA_TYPE:
			  {    
				if (value_str == NULL)
				length = 0;
				else
				length = STlength(value_str);
                                /*
                                ** Resets the db_length if file constant
                                ** is greater
                                */ 
				if (length > dbv->db_length)
				{
                                   dbv->db_length = STlength(value_str);
                                 
                                   /*
                                   ** if db_length is greater than maximum 
                                   ** length,the db_length is set to maximum
                                   ** length to prevent any buffer overflow. 
                                   */
                                   if (dbv->db_length > sizeof(max_len_const_str->value))
                                   {
                                        dbv->db_length=sizeof(max_len_const_str ->value);
                                   }     
				   length = dbv->db_length;
				   *( value_str + length ) = EOS;
				}
				ctype_name = STprintf(str_type, 
						 ERx("CHAR_ARRAY(%d)"),
						 dbv->db_length);
				ds_type = DSD_STR;
				STmove( value_str,' ', dbv->db_length,
					cha_buffer );
				*( cha_buffer + dbv->db_length ) = EOS;
				value = (PTR)cha_buffer;
				length = dbv->db_length + 1;	/* + EOS */
				break;
			  }
			  case DB_DEC_TYPE:
			  {
				STATUS  stat;
				i4 prec = DB_P_DECODE_MACRO(dbv->db_prec);
				i4 scale = DB_S_DECODE_MACRO(dbv->db_prec);
				length = ((prec / 2) + 1);

				status = CVapk( value_str, '.', prec,
						scale, dec_buffer);
				if (status != OK)
				{
		                        IIUGerr(E_AB00DA_PackConvFailed,
						UG_ERR_ERROR, 1, objl->name);
				}
				ctype_name = 
					STprintf(str_type, 
						 ERx("CHAR_ARRAY(%d)"),
						 length);

				ds_type = DSD_STR;
				value = (PTR)dec_buffer;
				/* note: we only add 1 byte for an EOS here
				** so that DS will write the entire encoded
				** value out as a string.  Since we're telling
				** DSwrite that the value is a DSD_STR, we have
				** to account for an EOS.
				*/
				length++;	/* + EOS */
				break;
			  }

			} /* end switch */
			DSinit( file, ABEXT_DSL, DSA_C, objl->locsym, DSV_LOCAL,
				ctype_name
			);
			if (is_varchar)
				DSwrite(DSD_I2, (PTR)(i4)length - 1, 0);
			DSwrite(ds_type, (char *)value, length);
			DSfinal();
		}
	}
	/*
	** Do the runtime global/constant table, an array of ABRTSGL structs.
	*/
	DSinit(file, ABEXT_DSL, DSA_C, nm_glo, DSV_LOCAL, ERx("GLOBARRAY"));
	cnt = 0;

	/* System Global Variables */
	for ( objl = IIam_Globals ; objl != NULL ; objl = objl->_next )
	{
		hvalue = iiarSysHash(objl->name);
		_genGlobalExtract(objl, hash[hvalue]);
		hash[hvalue] = cnt++;
	}

	for (objl = (APPL_COMP *)app->comps ; objl != NULL; objl = objl->_next)
	{
		if ( objl->class == OC_GLOBAL || objl->class == OC_CONST )
		{
			hvalue = iiarHash(objl->name);
			_genGlobalExtract(objl, hash[hvalue]);
			hash[hvalue] = cnt++;
		}
	} /* end for */
	glocnt = cnt;
	DSfinal();

	/* Globals table hash, an array of i4's */
	DSinit(file, ABEXT_DSL, DSA_C, nm_ghash, DSV_LOCAL, ERx("natarray"));
	for (hp = hash; hp < &hash[ABHASHSIZE] ; ++hp)
	{
		DSwrite(DSD_NAT, (PTR)(i4)*hp, 0);
		*hp = -1;
	}
	DSfinal();

	/*
	** Do the runtime class attributes tables, arrays of ABRTSATTRs
	*/

	/* attributes of types defined in ABF */
	for (objl = (APPL_COMP *)app->comps; objl != NULL; objl = objl->_next)
	{
		if ( objl->class != OC_RECORD )
			continue;

		/* load record attributes if necessary. */
		if ( ((RECDEF *) objl)->recmems == NULL )
		{
			_VOID_ IIAMraReadAtts((RECDEF *) objl);
		}
		_genAttrs(file, (RECDEF *) objl);
	}

	/* attributes of types defined in 4GL (like CLASS OF FORM FZWQV) */
	typelist = getTypes(app, (i4 *) NULL);
	for (objl = (APPL_COMP *)typelist; objl != NULL; objl = objl->_next)
	{
		/* These are all records, and have all their attributes. */
		_genAttrs(file, (RECDEF *) objl);
	}

	/*
	** Do the runtime typedefs (class) table, an array of ABRTSTYPE structs.
	*/
	DSinit(file, ABEXT_DSL, DSA_C, nm_typ, DSV_LOCAL, ERx("TYPEARRAY"));
	cnt = 0;
	/* types defined in ABF */
	for (objl = (APPL_COMP *)app->comps; objl != NULL; objl = objl->_next)
	{
		if ( objl->class != OC_RECORD )
			continue;

		hvalue = iiarThTypeHash(objl->name);
		_genTypExtract((RECDEF*) objl, hash[hvalue]);
		hash[hvalue] = cnt++;
	}
	/* types defined in 4GL (like CLASS OF FORM FZWQV) */
	for (objl = (APPL_COMP *)typelist; objl != NULL; objl = objl->_next)
	{
		/* These are all records. */
		hvalue = iiarThTypeHash(objl->name);
		_genTypExtract((RECDEF*) objl, hash[hvalue]);
		hash[hvalue] = cnt++;
	}
	typcnt = cnt;
	DSfinal();

	/* Typedefs table hash, an array of i4's */
	DSinit(file, ABEXT_DSL, DSA_C, nm_thash, DSV_LOCAL, ERx("natarray"));
	for (hp = hash; hp < &hash[ABHASHSIZE] ; ++hp)
	{
		DSwrite(DSD_NAT, (PTR)(i4) *hp, 0);
		*hp = -1;
	}
	DSfinal();

	/*
	** now the object table, which links the whole mess together.
	** This is the externally known object
	*/
	DSinit(file, ABEXT_DSL, DSA_C, ABEXTNAME, DSV_GLOB, ERx("ABRTSOBJ"));

	/* version number */
	DSwrite(DSD_NAT, (PTR)(i4)ABRT_VERSION, 0);

	/* DB name, starting frame, literal strings */
	DSwrite(DSD_SPTR, *IIar_Dbname, 0);
	DSwrite(DSD_SPTR, app->start_name, 0);

	/* Application DML */
	DSwrite(DSD_I2, (PTR)(i4)app->dml, 0);

	/* Application Start Procedure Flag */
	DSwrite(DSD_I1, (PTR)(i4)app->start_proc, 0);

	/* Application Batch Flag */
	DSwrite(DSD_I1, (PTR)(i4)app->batch, 0);

	/* size of hash table */
	DSwrite(DSD_NAT, (PTR)(i4)ABHASHSIZE, 0);

	/* hash tables, local i4  arrays */
	DSwrite(DSD_NATARY, nm_fhash, 0);
	DSwrite(DSD_NATARY, nm_phash, 0);

	/* table counts, nats (forms currently unused) */
	DSwrite(DSD_NAT, (PTR)(i4)frmcnt, 0);
	DSwrite(DSD_NAT, (PTR)(i4)procnt, 0);
	DSwrite(DSD_NAT, (i4)0, 0);

	/* table pointers, local arrays (forms currently unused) */
	DSwrite(DSD_ARYADR, nm_frm, 0);
	DSwrite(DSD_ARYADR, nm_pro, 0);
	DSwrite(DSD_NULL, (PTR)NULL, 0);

	/* Version 2 */

	/* Default application Natural Language: in 6.5, 0 indicates default */
	DSwrite(DSD_NAT, (PTR)(i4)app_lang, 0);

	/* Application Role ID (including password) */
	DSwrite(DSD_SPTR, IIabRoleID, 0);

	/* User password (not used) */
	DSwrite(DSD_NULL, (PTR)NULL, 0);

	/* global, type hash tables, local i4  arrays */
	DSwrite(DSD_ARYADR, nm_ghash, 0);
	DSwrite(DSD_ARYADR, nm_thash, 0);

	/* global, type table counts, nats */
	DSwrite(DSD_NAT, (PTR)(i4)glocnt, 0);
	DSwrite(DSD_NAT, (PTR)(i4)typcnt, 0);

	/* global, type table pointers, local arrays */
	DSwrite(DSD_ARYADR, nm_glo, 0);
	DSwrite(DSD_ARYADR, nm_typ, 0);

	/* Timestamp for constants files */
	DSwrite(DSD_SPTR, datestamp, 0);
	
       	/* Application name */
       	DSwrite(DSD_SPTR, app->name, 0);

	DSfinal();

#ifdef txROUT
	if (TRgettrace(ABTROUTEXIT, -1))
		abtrerout(ERx(""), (char *) NULL);
#endif
	/* now throw away the local symbol space */
	IIGNesEndSpace(AGN_LOCAL);

	DSend(file, ABEXT_DSL);

	/* Now generate language-dependent constants image files. 
	** Do this only if the II_APPLICATION_LANGUAGE logical is defined
	** to a valid language.
	*/
	if ( glocnt > 0 && app_lang > 0)
	{
		if ( iiabGnConstImg(app, app_lang, datestamp) != OK )
			IIUGerr(E_NoConstsGen, UG_ERR_ERROR, 0);
	}

	if (cons_file != NULL)
	{
		PMmFree( cntx );
	}
	return;
}

/*{
** Name:	_genFrame() -	Generate the RunTime Data Structures for Frame.
**
** Description:
**	Create the runtime tables for an application frame component.
**
** Inputs:
**	file	{FILE *}  The output file.
**	frm	{APPL_COMP *}  The application frame component object.
**
** Side Effects:
**	Changes the state of the frame and dynamically allocates some space.
**
** Trace Flags:
**	ROUT
**
** History:
**	Written 7/26/82 (jrc)
**	Modified 8/23/84 (jrc) Added ABFRQDEF
**	20-jan-1987 (Joe)
**		Bug fix for bug 11106.  The DBname put
**		in the extract file should be Abrdb since
**		it has any node:: or /d specifications in it.
**	30-dec-1993 (mgw) Bug #58227
**		DSwrite() the frm->qry_object string itself, not the pointer
**		to the frm->qry_object string, in the OC_QBFFRAME case. Do
**		likewise for frm->report in the OC_RWFRAME case and for
**		frm->graph in the OC_GBFFRAME case. And likewise for the
**		frm->cmd_flags and frm->output in all cases. This has the
**		same effect on Unix (the quoted string is generated in the
**		abextract.c file in either case), but makes a big difference
**		on VMS (the DSD_SPTR yields a .address in the abextract.mar
**		file and DSD_STR yields a .asciz)
**	20-jan-1994 (mgw) Bug #58227
**		Correct previous fix. Must send full length of buffer, not
**		STlength of string.
*/
static VOID
_genFrame ( file, comp )
FILE		*file;
APPL_COMP	*comp;
{
#ifdef txROUT
	if (TRgettrace(ABTROUTENT, -1))
		abtrsrout(ERx(""), (char *) NULL);
#endif

	/* Generate a name for the local variable. */

	IIGNgnGenName(AGN_LOCAL, comp->name, &(comp->locsym));

	/*
	** Do the correct DSinit for the type of frame,
	** then write initializers for every member of
	** the structure.
	*/

	switch ( comp->class )
	{
	  case OC_OLDOSLFRAME:
	  { /* Write an ABRTSUSER structure. */
		register USER_FRAME	*frm = (USER_FRAME *)comp;

		DSinit( file, ABEXT_DSL, DSA_C, comp->locsym, DSV_LOCAL,
			ERx("ABRTSUSER")
		);

#ifndef CMS
		DSwrite(DSD_MENU,  frm->symbol, 0);	/* the Menu. */
#else
		DSwrite(DSD_XADDR, frm->symbol, 0);	/* the Menu. */
#endif
		dbd_dswr( &frm->return_type, (char*)NULL );/* The return type.*/
		break;
	  }

	  case OC_OSLFRAME:
	  case OC_MUFRAME:
	  case OC_APPFRAME:
	  case OC_UPDFRAME:
	  case OC_BRWFRAME:
	  { /* Write an ABRTSNUSER structure. */
		register USER_FRAME	*frm = (USER_FRAME *)comp;

		DSinit( file, ABEXT_DSL, DSA_C, comp->locsym, DSV_LOCAL,
			ERx("ABRTSNUSER")
		);

		DSwrite(DSD_FNADR, frm->symbol, 0);	/* procedure name */
		dbd_dswr( &frm->return_type, (char*)NULL );/* The return type.*/
		DSwrite(DSD_I4, (PTR)frm->fid, 0);		/* The FID. */
		break;
	  }

	  case OC_QBFFRAME:
	  { /* Write an ABRTSQBF structure. */
		register QBF_FRAME	*frm = (QBF_FRAME *)comp;

		DSinit( file, ABEXT_DSL, DSA_C, comp->locsym, DSV_LOCAL,
			ERx("ABRTSQBF")
		);

		/* the query object name */
		DSwrite(DSD_STR, frm->qry_object, FE_MAXNAME+1);
		/* the command flags */
		DSwrite(DSD_STR, frm->cmd_flags, ABCOMSIZE+1);
		/* whether it is a JoinDef */
		DSwrite(DSD_NAT, (PTR)(i4)(frm->qry_type == OC_JOINDEF), 0);
		break;
	  }

	  case OC_RWFRAME:
	  { /* Write an ABRTSRW structure. */
		register REPORT_FRAME	*frm = (REPORT_FRAME *)comp;

		DSinit( file, ABEXT_DSL, DSA_C, comp->locsym, DSV_LOCAL,
			ERx("ABRTSRW")
		);

		/* the report name */
		DSwrite(DSD_STR, frm->report, FE_MAXNAME+1);
		/* the command flags */
		DSwrite(DSD_STR, frm->cmd_flags, ABCOMSIZE+1);
		/* the output file */
		DSwrite(DSD_STR, frm->output, ABFILSIZE+1);
		break;
	  }

	  case OC_GBFFRAME:
	  case OC_GRFRAME:
	  { /* Write an ABRTSGBF structure. */
		register GRAPH_FRAME	*frm = (GRAPH_FRAME *)comp;

		DSinit( file, ABEXT_DSL, DSA_C, comp->locsym, DSV_LOCAL,
			ERx("ABRTSGBF")
		);

		/* the graph name */
		DSwrite(DSD_STR, frm->graph, FE_MAXNAME+1);
		/* the command flags */
		DSwrite(DSD_STR, frm->cmd_flags, ABCOMSIZE+1);
		/* the output file */
		DSwrite(DSD_STR, frm->output, ABFILSIZE+1);
		break;
	  }
	}

	DSfinal();

	/*
	** for frames which have them, write out the form reference
	*/
	if ( comp->class != OC_GRFRAME && comp->class != OC_GBFFRAME )
		dsd_wrform(file, comp, comp->class);

#ifdef txROUT
	if (TRgettrace(ABTROUTEXIT, -1))
		abtrerout(ERx(""), (char *) NULL);
#endif
}

/*{
** Name:	_genAttrs() -	Generate an array of attributes for a record.
**
** Description:
**	Create the runtime tables for attributes of a class.
**
** Inputs:
**	file	{FILE *}  The output file.
**	rec	{RECDEF *}  The class definition component object.
**
** Trace Flags:
**	ROUT
**
** History:
**	Written 11/8/89 (billc)
*/

static VOID
_genAttrs(file, rec)
FILE	*file;
RECDEF 	*rec;
{
	RECMEM	*ap;
	char	*nm_attr;

	IIGNgnGenName(AGN_LOCAL, ERx("attrs"), &nm_attr);
	DSinit(file, ABEXT_DSL, DSA_C, nm_attr, DSV_LOCAL, ERx("ATTRARRAY"));
	rec->locsym = nm_attr;

	for (ap = rec->recmems; ap != NULL; ap = ap->next_mem)
	{
		DSwrite(DSD_SPTR, ap->name, 0);   /* the name */
		/* the data-type name */
		DSwrite(DSD_SPTR, ap->data_type.db_data, 0);

		dbd_dswr( &ap->data_type, (char *)NULL );  /* The data type. */

		DSwrite(DSD_NAT, (PTR)(i4) -1, 0);	/* data area offset */
	}
	DSfinal();
}

/*
** Name:	_genTypExtract() -	Generate a runtime entry for a class.
**
** Description:
**	Create the runtime table entry for a class definition.
**
** Inputs:
**	rec	{RECDEF *}  class-definition frame component.
**	bucket	{nat}  Hash table bucket index for name.
**	
** History:
**	Written 11/8/89 (billc)
*/

static VOID
_genTypExtract ( rec, bucket )
register RECDEF	*rec;
i4		bucket;	
{
	RECMEM	*ap;
	i4	attcnt = 0;

	for (ap = rec->recmems; ap != NULL; ap = ap->next_mem)
		attcnt++;

	DSwrite(DSD_SPTR, rec->name, 0);	/* the name */
	DSwrite(DSD_ARYADR, rec->locsym, 0);	/* the fields */
	DSwrite(DSD_NAT, (PTR)(i4) attcnt, 0);	/* dbdv count */
	DSwrite(DSD_NAT, (PTR)(i4) -1, 0);		/* data area size */

	DSwrite(DSD_NAT, (PTR)bucket, 0);
}

/*
** Name:	_genFrmExtract() -	Generate Frame Extract Entry.
**
** Descripiton:
**	Generates a frame extract table entry for an application frame.
**
** Inputs:
**	frm	{APPL_COMP *}  Application frame component.
**	bucket	{nat}  Hash table bucket index for name.
**	
** History:
**	07/89 (jhw) -- Abstracted from 'iiabImageGen()' as function.
*/
static VOID
_genFrmExtract ( frm, bucket )
register APPL_COMP	*frm;
i4			bucket;	
{
	register char	*form_sym;
	OOID		wrclass;

	switch ( wrclass = frm->class )
	{ /* frames only */
	  case OC_MUFRAME:
	  case OC_APPFRAME:
	  case OC_UPDFRAME:
	  case OC_BRWFRAME:
		wrclass = OC_OSLFRAME;	/* FALL THROUGH */
		break;
	}
		
	DSwrite(DSD_SPTR, frm->name, 0);	/* the name */
	DSwrite(DSD_NAT, (PTR)(i4)wrclass, 0);	/* the class */
	/* form pointer */
	if ( (form_sym = _formSym(frm)) != NULL )
		DSwrite(DSD_FORP, form_sym, 0);
	else
		DSwrite(DSD_NULL, (PTR)NULL, 0);

	/* the local symbol, an ABRTSVAR cast to ABRTSVAR */
	DSwrite(DSD_VRADR, frm->locsym, 0);

	DSwrite(DSD_NAT, (PTR)(i4)bucket, 0);

	/* the DML for executable frames */
	switch( frm->class )
	{
	  case OC_OSLFRAME:
	  case OC_MUFRAME:
	  case OC_APPFRAME:
	  case OC_UPDFRAME:
	  case OC_BRWFRAME:
		DSwrite(DSD_NAT, (PTR)(i4)iiabExtDML(((USER_FRAME*)frm)->source), 0);
		break;
	  default:
		DSwrite( DSD_NAT, (i4)DB_NOLANG, 0 );
		break;
	}
}

/*
** Name:	_genProcExtract() -	Generate Procedure Extract Entry.
**
** Descripiton:
**	Generates a procedure extract table entry for an application procedure.
**
** Inputs:
**	proc	{APPL_COMP *}  Application procedure component.
**	bucket	{nat}  Hash table bucket index for name.
**	
** History:
**	07/89 (jhw) -- Abstracted from 'iiabImageGen()' as function.
*/
static VOID
_genProcExtract ( proc, bucket )
register APPL_COMP	*proc;
i4			bucket;	
{
	i4	lang;
	DB_LANG	dml;
	OOID	fid;

	DSwrite(DSD_SPTR, proc->name, 0);	/* the name */

	if ( proc->class == OC_DBPROC )
	{
		lang = OLSQL;
		dml = DB_SQL;
		fid = OC_UNDEFINED;
	}
	else if ( proc->class == OC_OSLPROC )
	{
		lang = OLOSL;
		dml = iiabExtDML(((_4GLPROC *)proc)->source);
		fid = ((_4GLPROC *)proc)->fid;
	}
	else
	{
		lang = ((HLPROC *)proc)->ol_lang;
		dml = iiabExtDML(((HLPROC *)proc)->source);

		/* overload the fid to hold the HLPROC's flags, some of which
		** are needed at run-time (e.g. the APC_PASSDEC bit).
		** Note well: the fid is an i4, while the HLPROC's flags
		** are a i4.
		*/
		fid = ((HLPROC *)proc)->flags;
	}
	DSwrite(DSD_NAT, (PTR)(i4)lang, 0);

	/* the return type */
	dbd_dswr( &((_4GLPROC*)proc)->return_type, (char *)NULL );

	dsd_paddr(proc);		/* the procedure address */
	DSwrite( DSD_I4, (PTR)(i4)fid, 0 );		/* the IL procedure ID */

	DSwrite(DSD_NAT, (PTR)(i4)bucket, 0);
	DSwrite(DSD_NAT, (PTR)(i4)dml, 0);
}

/*
** Name:	_genGlobalExtract() -	Generate Globals Extract Entry.
**
** Descripiton:
**	Generates a globals extract table entry for an application global
**	variable or constant.
**
** Inputs:
**	glob	{APPL_COMP *}  Application global component.
**	bucket	{nat}  Hash table bucket index for name.
**	
** History:
**	12/89 (jhw) -- Abstracted from 'iiabImageGen()' as function.
*/
static VOID
_genGlobalExtract ( glob, bucket)
register APPL_COMP	*glob;
i4			bucket;	
{
	DB_DATA_DESC	*type = &((GLOBVAR *)glob)->data_type;
	DB_DATA_DESC	const_type;

	DSwrite(DSD_SPTR, glob->name, 0);	/* the name */
	/* the datatype type name */
	DSwrite(DSD_SPTR, type->db_data, 0);	

	dbd_dswr( type, glob->locsym );

	DSwrite(DSD_NAT, (PTR)(i4)bucket, 0);

	DSwrite(DSD_NAT, (PTR)(i4)glob->class, 0);
}

/*
** Name:	_formSym() -	Return Local Symbol for RunTime
**					Form Reference Structure.
** Description:
*/
static char *
_formSym ( comp )
register APPL_COMP	*comp;
{
	register FORM_REF	*fp;

	switch ( comp->class )
	{
	  case OC_OSLFRAME:
	  case OC_MUFRAME:
	  case OC_APPFRAME:
	  case OC_UPDFRAME:
	  case OC_BRWFRAME:
	  case OC_OLDOSLFRAME:
		fp = ((USER_FRAME *)comp)->form;
		break;

	  case OC_RWFRAME:
		fp = ((REPORT_FRAME *)comp)->form;
		break;

	  case OC_QBFFRAME:
		fp = ((QBF_FRAME *)comp)->form;
		break;

	  case OC_GRFRAME:
	  case OC_GBFFRAME:
	  default:
		fp = NULL;
	}

	return ( fp != NULL && fp->locsym != NULL ) ? fp->locsym : (char *)NULL;
}

/*
** local form writer utility
**
** History:
**	17-oct-1989 (wolf)
**		For CMS, external symbol for (const) FRAME structure
**		begins with $, regardless of case.
**	9/14/90 (MIke S) Base compiled form vs. database form on 
**			 component flag
*/
static VOID
dsd_wrform ( file, comp, class )
FILE		*file;
APPL_COMP	*comp;
OOID		class;
{
	register FORM_REF	*form;
	i4			formsource;

	switch ( comp->class )
	{
	  case OC_OSLFRAME:
	  case OC_MUFRAME:
	  case OC_APPFRAME:
	  case OC_UPDFRAME:
	  case OC_BRWFRAME:
	  case OC_OLDOSLFRAME:
		form = ((USER_FRAME *)comp)->form;
		break;

	  case OC_RWFRAME:
		form = ((REPORT_FRAME *)comp)->form;
		break;

	  case OC_QBFFRAME:
		form = ((QBF_FRAME *)comp)->form;
		break;

	  default:
		form = NULL;
		break;
	}

	if ( form == NULL || (form->name)[0] == EOS )
		return;

	if (class == OC_QBFFRAME || *form->source == EOS)
		formsource = ABFOSDB;	/* QBF frame or no form source file */
	else if ((comp->appl->flags & APC_DBFORMS) == 0)
		formsource = ABFOSC;	/* Entire application compiles forms */
	else
		formsource = (comp->flags & APC_DBFORM) ? ABFOSDB : ABFOSC;

	IIGNgnGenName(AGN_LOCAL, form->name, &(form->locsym));
	DSinit(file, ABEXT_DSL, DSA_C, form->locsym, DSV_LOCAL, ERx("ABRTSFO"));

	/* abrfoname */
	DSwrite(DSD_SPTR, form->name, 0);

	/*
	** abrfoaddr.  If form is from DB, write NULL
	*/
	if ( formsource == ABFOSC )
	{
#ifdef CMS
		char	vconname[FE_MAXNAME+2];

		DSwrite( DSD_FNADR,
			STprintf(vconname, ERx("$%s"), form->symbol),
			0
		);
#else
		DSwrite(DSD_FRADR, form->symbol, 0);
#endif
	}
	else
	{
		DSwrite(DSD_NULL, (PTR)NULL, 0);
	}

	/* abrfopos */
	DSwrite(DSD_NAT, (i4)0, 0);

	/* abrfosource */
	DSwrite(DSD_NAT, (PTR)(i4)formsource, 0);

	/* abrforef */
	DSwrite(DSD_NAT, (i4)0, 0);

	DSfinal();
}

/*
** local utility for DSwriting a data descriptor.
*/
static VOID
dbd_dswr ( db, data )
register DB_DATA_DESC	*db;
char			*data;
{
	if ( data != NULL )
		DSwrite(DSD_PTR, (PTR)data, 0);
	else
		DSwrite(DSD_NULL, (PTR)NULL, 0);
	DSwrite(DSD_I4, (PTR)db->db_length, 0);
	DSwrite(DSD_I2, (PTR)(i4)db->db_datatype, 0);
	DSwrite(DSD_I2, (PTR)(i4)db->db_scale, 0);
	DSwrite(DSD_I2, (PTR)(i4)db->db_collID, 0);
}

/*
** local utility for writing a procedure address.  saves us repeating the
** CMS special code in two places.
**
** History:
**	02/89 (jhw) - Add special case for database procedures.
**	08/89 (jhw) - Note:  "0" is a special case to 'DSwrite()'.
**	11-oct-89 (wolf)  For CMS, uppercase C procedure name.
*/
static VOID
dsd_paddr ( proc )
APPL_COMP	*proc;
{
	if ( proc->class == OC_DBPROC )
	{
		/* Note:  "0" is a special case caught by 'DSwrite()'
		** to write a NULL function address.  JupBug #7077 and #7535.
		*/
		DSwrite(DSD_FNADR, ERx("0"), 0);
	}
	else
	{
		/* Note:  Both _4GLPROC and HLPROC are sub-classes that
		** share all attributes up through the 'symbol' attribute.
		*/
		register HLPROC	*sproc = (HLPROC *)proc;
		
		if ( *sproc->symbol != EOS || sproc->class == OC_OSLPROC )
		{
#ifdef CMS
			char	procname[FE_MAXNAME+1];

			STcopy(sproc->symbol, pgmname);
			CVupper(pgmname);
			DSwrite(DSD_FNADR, pgmname, 0);
#else
			DSwrite(DSD_FNADR, sproc->symbol, 0);
#endif
		}
		else
		{
			/* assert:  proc->class == OC_HLPROC
			**		&& *proc->symbol == EOS
			*/
# ifdef CMS
			/*
			** Under CMS we must make sure that the function names
			** are in uppercase for non-C languages (scl 7/15/86)
			*/
			if ( sproc->ol_lang != OLC )
			{
				char	pgmname[FE_MAXNAME+1];

				STcopy(sproc->name, pgmname);
				CVupper(pgmname);
				DSwrite(DSD_FNADR, pgmname, 0);
			}
			else
# endif
				DSwrite(DSD_FNADR, sproc->name, 0);
		}
	}
}

/*{
** Name:	IIABieIntExt() -
**
**	Create the extract file for an interpreter executable.  This
**	simply defines the table of host language procedures.
**
** Inputs:
**	app	{APPL *}  The application for which to create the table.
**	file	{FILE *}  output file.
**
**
** Returns:
**	{STATUS}  OK	success
**		  FAIL	failure
**
** History:
**	1/89 (bobm) written
*/
VOID
IIABieIntExt ( app, file )
APPL	*app;
FILE	*file;
{
	register APPL_COMP	*objl;

	DSbegin(file, ABEXT_DSL, ABEXTNAME);
#ifdef CMS
	/*
	**  Generate a reference to C@TO@LN for CMS language interface
	**  routines
	*/
	DSinit(file, DSL_MACRO, DSA_UN, ABCTOLANG, DSV_GLOBAL);
	DSwrite(DSD_XADDR, ERx("C@TO@LN"), 0);
	DSfinal();
#endif
#ifdef USE_ABHDRINCL
	/* write the include statement for "abfurts.h" */
	_includeFile( file );
#endif

	/*
	** write out array of HL procedure addresses.  Terminated
	** by NULL entry.  Doing this assures at least one item in the
	** array.  Note that we are not supposed to wind up in this
	** routine if there are no HL procedures, but this way we will
	** still work if we do.
	*/
#ifdef CMS
#define _HL_PROC_TAB	ERx("$IIOhl@proc")
#else
#define _HL_PROC_TAB	ERx("IIOhl_proc")
#endif
	DSinit( file, ABEXT_DSL, DSA_C, _HL_PROC_TAB, DSV_GLOB,
		ERx("HLARRAY")
	);

	for ( objl = (APPL_COMP *)app->comps ; objl != NULL ;
			objl = objl->_next )
	{
		if ( objl->class == OC_HLPROC )
		{
			/* the name */
			DSwrite(DSD_SPTR, ((HLPROC *)objl)->name, 0);
			/* the procedure address */
			dsd_paddr(objl);
		}
	}

	/* NULL entry */
	DSwrite(DSD_NULL, (PTR)NULL, 0);
	DSwrite(DSD_NULL, (PTR)NULL, 0);
	DSfinal();

	DSend(file, ABEXT_DSL);
}

/*{
** Name:	IIABtfTabFile() -	Generate Interpreted Data Structures.
**
** Description:
**	Create the frame table for the application in memory, and
**	write it out to a DS file.  Actually, what we do is call
**	IOI to create a "test image", which will be used by the
**	interpreter.
**
** Inputs:
**	app	{APPL *}  The application for which to make the data structures.
**	fname	{char *}  filename to create.  MAXLOC+1.
**
** Returns:
**	{STATUS}  OK, FAIL
**
** Side Effects:
**	Allocates some dynamic space.
**
** History:
**	2/89 (bobm)	written.
**	9/91 (bobm)	extracted make_rtt for common use.
*/

STATUS
IIABtfTabFile ( app, fname )
APPL	*app;
char	*fname;
{
	STATUS stat;
	i4 hstore[4*ABHASHSIZE];

	stat = make_rtt ( app, fname, hstore );
	if (stat != OK)
		return stat;

	return IIOItcTstfilCreate(fname, &Rtobj);
}

/*{
** Name:	IIABibImageBuild() -	Generate Interpreted Data Structures.
**
** Description:
**	Create an image file.  Done by creating an in-memory runtime
**	table, and then calling IIABdiDoImage()
**
** Inputs:
**	app	{APPL *}  The application for which to make the data structures.
**	fname	{char *}  filename to create.  MAXLOC+1.
**
** Returns:
**	{STATUS}  OK, FAIL
**
** Side Effects:
**	Allocates some dynamic space.
**
** History:
**	9/91 (bobm)	written.
*/

STATUS
IIABibImageBuild( app, fname )
APPL	*app;
char	*fname;
{
	STATUS stat;
	i4 hstore[4*ABHASHSIZE];

	stat = make_rtt ( app, fname, hstore );
	if (stat != OK)
		return stat;

	return IIABdiDoImage(fname, &Rtobj);
}


/*
** static routine for the common work under IIABtfTabFile and IIABibImageBuild.
**
** hash table storage is passed in because it needs to hang about while the
** caller uses the runtime table, but is appropriately scoped to the caller.
*/

static STATUS
make_rtt ( app, fname, hs )
APPL	*app;
char	*fname;
i4	*hs;
{
	i4	ofrms;
	i4	*fhash = hs;
	i4	*phash = hs + ABHASHSIZE;
	i4	*ghash = hs + 2*ABHASHSIZE;
	i4	*thash = hs + 3*ABHASHSIZE;
	register APPL_COMP	*list;
	i4	n_frms = 0;
	i4	n_procs = 0;
	i4	n_forms = 0;
	i4	n_globs = 0;
	i4	n_types = 0;
	i4	n_data = 0;
	STATUS	status;
	char	*cons_file;
	char	*value_str;

	Rtobj.language = get_language();
	Rtobj.abcndate = NULL;
	Rtobj.abroappl = app->name;

	/*
	** check allocations.  Allocate an ABRTSVAR along with each frame
	*/
	ofrms = Rt_frames;

	/* System Frames, Procedures & Global Variables */
	for ( list = IIam_Frames ; list != NULL ; list = list->_next )
		++n_frms;
	for ( list = IIam_Procs ; list != NULL ; list = list->_next )
		++n_procs;
	for ( list = IIam_Globals ; list != NULL ; list = list->_next )
		++n_globs;

	/* load the 'special' types - CLASS OF FORM, etc. */
	_VOID_ getTypes(app, &n_types);

	/* Application Frames, Procedures, and Globals 
	*/
	
	for ( list = (APPL_COMP *)app->comps ; list != NULL ;
			list = list->_next )
	{
		if ( list->class >= OC_APPLFRAME &&
				list->class <= OC_APPLFRAME + 99 )
		{
			++n_frms;
			if ( list->class == OC_QBFFRAME
				    && ((QBF_FRAME *)list)->form != NULL )
				++n_forms;
		}
		else if ( list->class == OC_GLOBAL )
		{
			++n_globs;
		}
		else if ( list->class == OC_CONST )
		{
			i4	align;
			CONSTANT *xconst = (CONSTANT *)list;

			++n_globs;
			if ( (align = n_data % sizeof(ALIGN_RESTRICT))
					!= 0 )
				align = sizeof(ALIGN_RESTRICT) - align;
			n_data += align;

			/* increment n_data, so we know how much space
			** to allocate for all of the globals.
			*/
			switch (xconst->data_type.db_datatype)
			{
			  case DB_DYC_TYPE:
			  case DB_VCH_TYPE:
			  case DB_TXT_TYPE:
			  case DB_CHR_TYPE:
			  case DB_CHA_TYPE:
				n_data += 1;	/* EOS */
				/* fall through */
			  default:
				n_data += xconst->data_type.db_length;
			} /* end switch */
		}
		else if (list->class == OC_AFORMREF || list->class == OC_FORM)
		{
			++n_forms;
		}
		else if (list->class == OC_RECORD) 
		{
			++n_types;
		}
		else if (list->class != OC_RECMEM) 
		{
			++n_procs;
		}

	}

	if ( (Rtobj.abrofcnt = n_frms) > 0 
	  && (Rtobj.abroftab = (ABRTSFRM *) rt_alloc( n_frms, 
			sizeof(ABRTSFRM)+sizeof(ABRTSVAR),
			(PTR)Rtobj.abroftab, &Rt_frames, FALSE, &status )
	    ) == NULL )
	{
		return status;
	}
	if ( (Rtobj.abropcnt = n_procs) > 0 
	   && (Rtobj.abroptab = (ABRTSPRO *) rt_alloc( n_procs, 
			sizeof(ABRTSPRO), (PTR)Rtobj.abroptab, &Rt_procs, 
			     FALSE, &status )
	    ) == NULL )
	{
		return status;
	}
	if ( (Rtobj.abrofocnt = n_forms) > 0 
	  && (Rtobj.abrofotab = (ABRTSFO *) rt_alloc( n_forms, sizeof(ABRTSFO),
			(PTR)Rtobj.abrofotab, &Rt_forms, FALSE, &status )
	    ) == NULL )
	{
		return status;
	}
	if ( (Rtobj.abroglcnt = n_globs) > 0 
	  && (Rtobj.abrogltab = (ABRTSGL *) rt_alloc( n_globs, sizeof(ABRTSGL),
			(PTR)Rtobj.abrogltab, &Rt_globs, FALSE, &status )
	    ) == NULL )
	{
		return status;
	}
	if ( n_data > 0
	  && (Rt_data = rt_alloc( n_data, sizeof(char), (PTR)Rt_data,
				&Rt_dsize, FALSE, &status ) ) == NULL )
	{
		return status;
	}
	if ( (Rtobj.abrotycnt = n_types) > 0 
	  && (Rtobj.abrotytab = (ABRTSTYPE *) rt_alloc( n_types, 
			sizeof(ABRTSTYPE), (PTR)Rtobj.abrotytab, &Rt_types, 
			TRUE, &status )
	    ) == NULL )
	{
		return status;
	}

	/*
	** if we allocated a new frame array, set the ABRTSVAR pointers
	*/
	if ( ofrms != Rt_frames )
	{
		register ABRTSVAR	*var;
		register ABRTSFRM	*frm;

		register i4	i;

		frm = Rtobj.abroftab;
		var = (ABRTSVAR *)(frm+Rt_frames);
		for ( i = 0 ; i < Rt_frames ; ++i )
		{
			frm->abrfrvar = var++;
			++frm;
		}
	}

	/*
	** It is important that forms are done before frames -
	** 'rt_forms()' sets up indexes to allow the frame structures
	** to reference the appropriate form entry.  These routines
	** fill in the 'Rtobj' object arrays.
	*/
	rt_forms((APPL_COMP *) app->comps);
	rt_frames((APPL_COMP *) app->comps, fhash, (DB_LANG) app->dml);
	rt_procs((APPL_COMP *) app->comps, phash);
	rt_globs((APPL_COMP *) app->comps, ghash);
	rt_types((APPL_COMP *) app->comps, thash);

	Rtobj.abrodbname = *IIar_Dbname;
	Rtobj.abrosframe = NULL;
	Rtobj.abroversion = ABRT_VERSION;
	Rtobj.abdml = app->dml;
	Rtobj.start_proc = app->start_proc;
	Rtobj.batch = app->batch;
	Rtobj.language = 0;		/* as of 6.5 */
	Rtobj.abroleid = IIabRoleID;
	Rtobj.abpassword = IIabPassword;
	Rtobj.abrofhash = fhash;
	Rtobj.abrophash = phash;
	Rtobj.abroghash = ghash;
	Rtobj.abrothash = thash;
	Rtobj.abrohcnt = ABHASHSIZE;

	return OK;
}

/*
** local routine to assure that enough memory is allocated for the current
** runtime table.  Basically chases a list, counts the items, compares to
** an old count, allocates a longer array if needed.  Frees the old one, and
** generates a fatal error on allocation failure.  We may not actually
** need all that's allocated, due to ABUNDEF things, but it's probably not
** enough to worry about.
*/
static PTR
rt_alloc ( cnt, size, addr, count, clear, status )
i4	cnt;
i4	size;
PTR	addr;
i4	*count;
bool	clear;
STATUS	*status;
{
	/*
	** allocate longer array if needed.  Free old one if it
	** was longer than 0.  We use MEreqlng because some of
	** these structures are rather large, and users do sometimes
	** define applications with hundreds of frames.
	*/
	if ( cnt > *count )
	{
		if ( *count > 0 )
			MEfree(addr);
		if ((addr = MEreqmem(0, (SIZE_TYPE)(size * cnt), clear, status))
				== NULL )
		{
			return NULL;
		}
		*count = cnt;
	}
	return addr;
}

static VOID
rt_forms ( list )
register APPL_COMP	*list;
{
	register ABRTSFO	*rfo;

	if ( Rtobj.abrofocnt <= 0 )
		return;

	rfo = Rtobj.abrofotab;
	for ( ; list != NULL ; list = list->_next )
	{
		if ( list->class == OC_AFORMREF || list->class == OC_FORM ||
			( list->class == OC_QBFFRAME &&
				((QBF_FRAME *)list)->form != NULL ) )
		{
			register FORM_REF	*form;

			form = ( list->class == OC_QBFFRAME )
				? ((QBF_FRAME *)list)->form : (FORM_REF *)list;

			/*
			** if we somehow have an empty form name,
			** skip it, so that the runtime table will
			** contain a NULL form reference to be handled
			** by abf runtime.
			*/
			form->locsym = NULL;
			if (form->name == NULL || *(form->name) == EOS)
				continue;

			rfo->abrfoname = form->name;
			rfo->abrfoaddr = NULL;
			rfo->abrfopos = 0;
			rfo->abrforef = 0;
			rfo->abrfosource = ABFOSDB;

			/*
			** we fill in the 'locsym' with a coerced address of an
			** ABRTSFO structure.  This allows us to fill the frame
			** array with the right reference into the form array.
			** The 'locsym' is declared as (char *) because of its
			** use to hold a local variable name when writing out
			** the run time table as compilable C structures.
			*/
			form->locsym = (char *) rfo;

			++rfo;
		}
	} /* end for */
}

static VOID
rt_frmset ( frm, lang, bucket, rf )
register APPL_COMP	*frm;
DB_LANG			lang;
i4			bucket;
register ABRTSFRM	*rf;
{
	rf->abrfrname = frm->name;
	rf->abrfrtype = frm->class;
	rf->abrdml = lang;

	switch (frm->class)
	{
	  case OC_OLDOSLFRAME:
	  case OC_OSLFRAME:
	  case OC_MUFRAME:
	  case OC_APPFRAME:
	  case OC_UPDFRAME:
	  case OC_BRWFRAME:
	  {
		register USER_FRAME	*ufrm = (USER_FRAME *)frm;

		rf->abrfrnuser.abrrettype.db_data = NULL;
		rf->abrfrnuser.abrrettype.db_datatype =
						ufrm->return_type.db_datatype;
		rf->abrfrnuser.abrrettype.db_length =
						ufrm->return_type.db_length;
		rf->abrfrnuser.abrrettype.db_prec =
						ufrm->return_type.db_scale;
		rf->abrfrnuser.abrrettype.db_collID =
						ufrm->return_type.db_collID;
		rf->abrfrtype = OC_OSLFRAME;
		rf->abrfrnuser.abrfunc = NULL;
		rf->abrfrnuser.abrstatic = (i4)ufrm->is_static;
		rf->abrfrnuser.abrfid = ufrm->fid;
		rf->abrdml = iiabExtDML(ufrm->source);
		break;
	  }
	  case OC_QBFFRAME:
	  {
		register QBF_FRAME	*qfrm = (QBF_FRAME *)frm;

		STcopy(qfrm->qry_object, rf->abrfrqbf.abrqfrelid);
		STcopy(qfrm->cmd_flags, rf->abrfrqbf.abrqfcomline);
		rf->abrfrqbf.abrqfjdef = (bool)( qfrm->qry_type == OC_JOINDEF );
		break;
	  }
	  case OC_RWFRAME:
	  {
		register REPORT_FRAME	*rfrm = (REPORT_FRAME *)frm;

		STcopy(rfrm->report, rf->abrfrrw.abrrwname);
		STcopy(rfrm->cmd_flags, rf->abrfrrw.abrrwcomline);
		STcopy(rfrm->output, rf->abrfrrw.abrrwoutput);
		break;
	  }
	  case OC_GBFFRAME:
	  case OC_GRFRAME:
	  {
		register GRAPH_FRAME	*gfrm = (GRAPH_FRAME *)frm;

		STcopy(gfrm->graph, rf->abrfrgbf.abrgfname);
		STcopy(gfrm->cmd_flags, rf->abrfrgbf.abrgfcomline);
		STcopy(gfrm->output, rf->abrfrgbf.abrgfoutput);
		break;
	  }
	  default:
		return;
	} /* end switch */

	/*
	** use the coerced 'locsym's to point to the correct
	** form structure.  The 'locsym' was filled in with
	** an ABRTSFO pointer in 'rt_forms()'.
	*/
	rf->abrform = (ABRTSFO *)_formSym(frm);

	rf->abrnext = bucket;
}

static VOID
rt_frames ( comps, hash, lang )
APPL_COMP		*comps;
register i4		*hash;
DB_LANG			lang;
{
	register APPL_COMP	*list;
	register ABRTSFRM	*rf;
	register i4		h;
	register i4		cnt;

	for ( h = 0 ; h < ABHASHSIZE ; ++h )
		hash[h] = -1;

	if ( Rtobj.abrofcnt <= 0 )
		return;

	/*
	** Generate the frame extract table, an array of ABRTSFRM structures.
	**
	**	Note:  System frames must be written out first so that
	**	user-defined frames will override the system ones because
	**	of hash table overflow chaining.  (That is, user-defined
	**	frames should cause the system frames to be chained and since
	**	they have the same name, the user-defined frames will always
	**	be found first.)
	*/
	rf = Rtobj.abroftab;
	cnt = 0;

	/* System frames */
	for ( list = IIam_Frames ; list != NULL ; list = list->_next )
	{
		h = iiarSysHash(list->name);
		rt_frmset(list, lang, hash[h], rf++);
		hash[h] = cnt++;
	}

	/* Application frames */
	for ( list = comps ; list != NULL ; list = list->_next )
	{
		if (list->locsym != NULL)
			continue;
		if ( list->class >= OC_APPLFRAME
				&& list->class <= OC_APPLFRAME + 99 )
		{
			h = iiarHash(list->name);
			rt_frmset(list, lang, hash[h], rf++);
			hash[h] = cnt++;
		}
	} /* end for */
	Rtobj.abrofcnt = cnt;
}

static VOID
rt_setproc ( proc, bucket, rp )
register APPL_COMP	*proc;
i4			bucket;
register ABRTSPRO	*rp;
{
	register DB_DATA_DESC	*dp;

	switch ( proc->class )
	{
	  case OC_HLPROC:
		dp = &((HLPROC *)proc)->return_type;
		rp->abrplang = ((HLPROC *)proc)->ol_lang;
		rp->abrpdml = iiabExtDML(((HLPROC *)proc)->source);

		/* overload the fid to hold the HLPROC's flags, some of which
		** are needed at run-time (e.g. the APC_PASSDEC bit).
		** Note well: the fid is an i4, while the HLPROC's flags
		** are a i4.
		*/
		rp->abrpfid = ((HLPROC *)proc)->flags;
		break;
	  case OC_OSLPROC:
		dp = &((_4GLPROC *)proc)->return_type;
		rp->abrplang = OLOSL;
		rp->abrpfid = ((_4GLPROC *)proc)->fid;
		rp->abrpdml = iiabExtDML(((_4GLPROC *)proc)->source);
		break;
	  case OC_DBPROC:
		dp = &((DBPROC *)proc)->return_type;
		rp->abrplang = OLSQL;
		rp->abrpfid = 0;
		rp->abrpdml = DB_SQL;
		break;
	  default:
		return;
	} /* end switch */

	rp->abrpfunc = NULL;
	rp->abrpname = proc->name;

	rp->abrprettype.db_data = NULL;
	rp->abrprettype.db_datatype = dp->db_datatype;
	rp->abrprettype.db_length = dp->db_length;
	rp->abrprettype.db_prec = dp->db_scale;
	rp->abrprettype.db_collID = dp->db_collID;

	rp->abrpnext = bucket;
}

static VOID
rt_procs ( comps, hash )
APPL_COMP		*comps;
register i4		*hash;
{
	register APPL_COMP	*list;
	register ABRTSPRO	*rp;
	register i4		h;
	register i4		cnt;

	for ( h = 0 ; h < ABHASHSIZE ; ++h )
		hash[h] = -1;

	if ( Rtobj.abropcnt <= 0 )
		return;

	/*
	** Generate the procedure extract table, an array of ABRTSPRO structures
	**
	**	Note:  System procedures must be written out first so that
	**	user-defined procedures will override the system ones because
	**	of hash table overflow chaining.  (That is, user-defined
	**	procedures will cause the system procedures to be chained and
	**	since they have the same name, the user-defined procedures will
	**	always be found first.)
	*/
	rp = Rtobj.abroptab;
	cnt = 0;

	/* System procedures */
	for ( list = IIam_Procs ; list != NULL ; list = list->_next )
	{
		h = iiarSysHash(list->name);
		rt_setproc(list, hash[h], rp++);
		hash[h] = cnt++;
	}

	/* Application procedures */
	for ( list = comps ; list != NULL ; list = list->_next )
	{
		if (list->locsym != NULL)
			continue;
		switch ( list->class )
		{
		  case OC_HLPROC:
		  case OC_OSLPROC:
		  case OC_DBPROC:
			h = iiarHash(list->name);
			rt_setproc(list, hash[h], rp++);
			hash[h] = cnt++;
		  default:
			break;
		} /* end switch */
	} /* end for */
	Rtobj.abropcnt = cnt;
}

/*
** History:
**	07/90 (jhw) - Do not include EOS in data length specification but do
**		account for it in the data allocation.  Bug #32044.
**	15-jul-93 (essi)
**		Added the cntx parameter.
**      18-Aug-2003 (zhahu02)
**              Updated for value_str causing SIGSEGV (b110734/INGCBT484) . 
*/
static i4
rt_setglob (cntx, glob, data, bucket, rp )
PM_CONTEXT		*cntx;
register APPL_COMP	*glob;
PTR			data;
i4			bucket;
register ABRTSGL	*rp;
{
	i4	size, length;
	DB_DATA_VALUE *dbv = (DB_DATA_VALUE *)&((GLOBVAR *)glob)->data_type;
	char	*value_str;
	char	*cons_file;  

	rp->abrgtype = glob->class;
	rp->abrgname = glob->name;

	rp->abrgtname = dbv->db_data;
	rp->abrgdattype.db_datatype = dbv->db_datatype;
	rp->abrgdattype.db_length = dbv->db_length;
	rp->abrgdattype.db_prec = dbv->db_prec;
	rp->abrgdattype.db_collID = dbv->db_collID;
	if ( glob->class == OC_GLOBAL )
	{
		rp->abrgdattype.db_data = NULL; 
		size = 0;
	}
	else
	{ /* set constant value */
		i4			align;
		i4			_eos = 0;	/* EOS? */
		register CONSTANT	*xconst = (CONSTANT *)glob;
		STATUS	status;
		
		if ( (align = (data - Rt_data) % sizeof(ALIGN_RESTRICT)) != 0 )
			align = sizeof(ALIGN_RESTRICT) - align;
		rp->abrgdattype.db_data = (data += align);

		cons_file = IIABqcfQueryConstantFile();
		if (cons_file != NULL)
		{
			status = PMmGet(cntx, rp->abrgname, &value_str);
			if (status != OK)
				value_str = get_val_string(xconst, 
						Rtobj.language);
		}
		else
		{
			value_str = get_val_string(xconst, Rtobj.language);
		}
		switch ( dbv->db_datatype )
		{
		  case DB_INT_TYPE:
		  {
			i4	value;

			status = CVal(value_str,  &value);
                        if (status != OK)
			{
				IIUGerr(E_AB00CE_IntConvFailed,
					UG_ERR_ERROR, 1, rp->abrgname);
			}
			if ( dbv->db_length == sizeof(i1) )
			{
				*((i1 *)data) = value;
			}
			else if ( dbv->db_length == sizeof(i2) )
			{
				*((i2 *)data) = value;
			}
			else /* ( dbv->db_length == sizeof(i4) ) */
			{
				*((i4 *)data) = value;
			}
			break;
		  }
				
		  case DB_FLT_TYPE:
		  {
			f8	float_value;

			status = CVaf(value_str, '.', &float_value);
                        if (status != OK)
			{
				IIUGerr(E_AB00CF_FloatConvFailed,
					UG_ERR_ERROR, 1, rp->abrgname);
			}
			if ( dbv->db_length == sizeof(f4) )
			{
				*((f4 *)data) = float_value;
			}
			else /* ( dbv->db_length == sizeof(f8) ) */
			{
				*((f8 *)data) = float_value;
			}
			break;
		  }

		  case DB_DYC_TYPE:
			rp->abrgdattype.db_datatype = DB_VCH_TYPE;
			/* fall through ... */
		  case DB_VCH_TYPE:
		  case DB_TXT_TYPE:
		  {
			DB_TEXT_STRING	*txt;
			if (value_str == NULL)
			{
			rp->abrgdattype.db_data = NULL;
			size = 0;
		        break;	
			}
			length = STlength( value_str );
			if ( length > 
				( rp->abrgdattype.db_length - DB_CNTSIZE ))
			{
				length = rp->abrgdattype.db_length - 
					DB_CNTSIZE;
				*( value_str + length ) = EOS;
			}
			txt = ( DB_TEXT_STRING * )data;
			txt->db_t_count = length;
			MEcopy( (PTR)value_str, 
					txt->db_t_count + 1,	/* EOS */
					(PTR)txt->db_t_text );
			_eos = 1;
			break;
		  }
		  case DB_CHR_TYPE:
		  case DB_CHA_TYPE:
		  {
			if (value_str == NULL)
			{
			rp->abrgdattype.db_data = NULL;
			size = 0;
		        break;	
			}
			length = STlength(value_str);
			if (length > rp->abrgdattype.db_length)
			{
				length = rp->abrgdattype.db_length;
				*(value_str + length) = EOS;
			}
                        STmove( value_str,' ', rp->abrgdattype.db_length, 
				data);
			*((char *)data + STlength(value_str)) = EOS;
			_eos = 1;
			break;
		  }
		  case DB_DEC_TYPE:
		  {
			i4 prec = DB_P_DECODE_MACRO(dbv->db_prec);
			i4 scale = DB_S_DECODE_MACRO(dbv->db_prec);

			status = CVapk( value_str,  '.', prec,
					scale, (char *)data);
                        if (status == CV_TRUNCATE || status == OK)
			{
				;
			}
			else
			{
			IIUGerr(E_AB00DA_PackConvFailed,
				UG_ERR_ERROR, 1, rp->abrgname);
			}
			break;
		  }
		} /* end switch */

		size = align + rp->abrgdattype.db_length + _eos;
	}

	rp->abrgnext = bucket;

	return size;
}

static VOID
rt_globs ( comps, hash )
APPL_COMP	*comps;
i4		*hash;
{
	register APPL_COMP	*list;
	register ABRTSGL	*rp;
	register i4		h;
	register PTR		data;
	register i4		cnt;
	char            	locbuf[MAX_LOC+1];
	char			*cons_file;
	LOCATION		loc;
	PM_CONTEXT		*cntx;

	for ( h = 0 ; h < ABHASHSIZE ; ++h )
		hash[h] = -1;

	if ( Rtobj.abroglcnt <= 0 )
		return;

	rp = Rtobj.abrogltab;
	data = Rt_data;
	cnt = 0;

        /*
	** Initialize and load the user specified constant file to override
	** the globals from system catalogs if indicated by user.
	*/
	cons_file = IIABqcfQueryConstantFile();
	if (cons_file != NULL)
	{
		_VOID_ IIUGvfVerifyFile ( cons_file, UG_IsExistingFile,
			FALSE, TRUE, UG_ERR_ERROR );
		STcopy(cons_file, locbuf);
		LOfroms( PATH & FILENAME, locbuf, &loc );
		PMmInit( &cntx );
	        if ( PMmLoad( cntx, &loc,(PM_ERR_FUNC *) NULL)  != OK )
		{
			IIUGerr(E_AB00CA_BadConstantFile,
				UG_ERR_ERROR, 1, cons_file);
		}
	}

	/* System globals */
	for ( list = IIam_Globals ; list != NULL ; list = list->_next )
	{
		h = iiarSysHash(list->name);
		data += rt_setglob(cntx, list, data, hash[h], rp++);
		hash[h] = cnt++;
	}

	for ( list = comps ; list != NULL ; list = list->_next )
	{
		if ( list->class == OC_GLOBAL || list->class == OC_CONST )
		{
			h = iiarHash(list->name);
			data += rt_setglob(cntx, list, data, hash[h], rp++);
			hash[h] = cnt++;
		}
	} /* end for */

	if (cons_file != NULL)
	{
		PMmFree( cntx );
	}
}

static RECDEF	*TypList = NULL;
static TAGID	AttrTag = 0;

static VOID
rt_types ( list, hash )
APPL_COMP	*list;
i4		*hash;
{
	register ABRTSTYPE *sp;
	register i4	h;
	i4	cnt = 0;
	RECDEF	*recp;

	for ( h = 0 ; h < ABHASHSIZE ; ++h )
		hash[h] = -1;

	if ( Rtobj.abrotycnt <= 0 )
		return;

	sp = Rtobj.abrotytab;
	cnt = 0;

	for ( ; list != NULL ; list = list->_next )
	{
		if (list->class != OC_RECORD)
			continue;

		h = iiarThTypeHash(list->name);
		makeRec((RECDEF*)list, sp);
		sp->abrtnext = hash[h];
		hash[h] = cnt++;

		++sp;
	} 

	for (recp = TypList; recp != NULL ; recp = (RECDEF*) recp->_next )
	{
		h = iiarThTypeHash(recp->name);
		makeRec(recp, sp);
		sp->abrtnext = hash[h];
		hash[h] = cnt++;
		++sp;
	} 
}

/*
** name: getTypes()	- load special types and return count.
*/

static RECDEF *
getTypes(app, cnt)
APPL	*app;
i4	*cnt;
{
	i4	count;

	RECDEF	*IIAMgsrGetSpecialRecs();

	if ( AttrTag == 0 )
		AttrTag = FEgettag();
	else if ( TypList != NULL )
		FEfree(AttrTag);

	TypList = IIAMgsrGetSpecialRecs(app, AttrTag, &count, (STATUS*)NULL);
	if (cnt != NULL)
		*cnt = count;
	return TypList;
}

static
makeRec(rdp, sp)
RECDEF 		*rdp;
ABRTSTYPE	*sp;
{
	register RECMEM		*rp;
	register ABRTSATTR	*ap;
	register i4		rcnt = 0;

	sp->abrtname = rdp->name;

	/* load record attributes if necessary. */
	if (rdp->recmems == NULL && rdp->ooid != OC_UNDEFINED)
		_VOID_ IIAMraReadAtts((RECDEF *) rdp);

	/* and count them. */
	for (rp = rdp->recmems; rp != NULL; rp = rp->next_mem)
	{
		++rcnt;
	}

	if (rcnt > 0)
	{
		sp->abrtflds = (ABRTSATTR *) FEreqmem(AttrTag,
			sizeof(ABRTSATTR) * rcnt, TRUE, (STATUS*)NULL
		);
	}
	else
	{
		sp->abrtflds = NULL;
	}

	for ( ap = sp->abrtflds, rp = rdp->recmems ; rp != NULL ;
			++ap, rp = rp->next_mem )
	{
		ap->abraname = rp->name;
		ap->abratname = rp->data_type.db_data;
		ap->abradattype.db_data = NULL;
		ap->abradattype.db_datatype = rp->data_type.db_datatype;
		ap->abradattype.db_length = rp->data_type.db_length;
		ap->abradattype.db_prec = rp->data_type.db_scale;
		ap->abradattype.db_collID = rp->data_type.db_collID;
		ap->abraoff = -1;
	}

	sp->abrtcnt = rcnt;
	sp->abrtsize = -1;
}

/*
** get_language
**
**	check to see if the obsolete II_APPLICATION_LANGUAGE logical
**	is set. If so, try to translate it to a langauge code - if it is
**	successful, return the language code; in all other cases return 0.
**	Raise warning errors whenever the logical is defined.
*/
static i4
get_language()
{
	i4	code = 0;
	char	*language;

	/* Check if II_APPLICATION_LANGUAGE is set */
	NMgtAt(ERx("II_APPLICATION_LANGUAGE"), &language);
	if (language != NULL && *language != EOS)
	{
		if (ERlangcode(language, &code) != OK )
		{  /* ignore the II_APPLICATION_LANGUAGE setting */
			code = 0;
			IIUGerr(E_AB0129_InvAppLang, UG_ERR_ERROR, 1,
				language);
		}
		else
		{
			IIUGerr(E_AB012A_AppLangUsed, UG_ERR_ERROR, 1,
				language);
		}
	}
	return code;
}
/*
** get_val_string  - return which value in a CONSTANT represents the current
**		     constant value string, based on the specified "language".
**
*/
static char *
get_val_string(xconst, lang)
CONSTANT *xconst;
i4	 lang;
{
	char *value;

	switch (xconst->data_type.db_datatype)
	{
		case DB_DYC_TYPE:
		case DB_VCH_TYPE:
		case DB_TXT_TYPE:
		case DB_CHR_TYPE:
		case DB_CHA_TYPE:
		{
			if (lang > 0 && xconst->value[lang] != NULL)
			{
				value = xconst->value[lang];
				break;
			}
			/* else fall through */
		}
		default:
		{
			value = xconst->value[0];
		}
	} /* end switch */
	return value;
}
