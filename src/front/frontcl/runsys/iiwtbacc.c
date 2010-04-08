/* 
** Copyright (c) 2004 Ingres Corporation  
*/ 
# include <compat.h>
# include <cm.h>
# include <gl.h>
# include <sl.h>
# include <st.h>
# include <iicommon.h>
# include <fe.h>
# include <eqrun.h>

# ifdef	NT_GENERIC

/**
+*  Name: iiwtbacc.c - Windows interface to TBACC to handle character names/data.
**
**  Description:
**	These routines (for Windows non-C environments ONLY) are called from an 
**	embedded program when character data is being sent/received in 
**	database statements.  Their purpose is to transform a Windows 
**	data structure (i.e., a descriptor) into a format that TBACC/ADH
**	knows about.  Data copying is avoided if at all possible for
**	performance reasons.
**
**	In 6.0, ADH has a convention regarding character types from embedded
**	programs as follows:
**	
**	DB_CHR_TYPE 
**	w/len of zero (Input)	Assume to be null terminated (input) and 
**				take len = STlength.
**	  "    "   "  (Output)	Assume to have len of DB_MAXSTRING.  Null
**				terminate.
**
**	DB_CHR_TYPE		
**	w/len > 0     (Input)	Use given length.  Don't trim blanks. 
**	"      "   "  (Output)	Use given length.  Null terminate.
**
**	DB_CHA_TYPE 	
**	w/len > 0     (Input)	Don't assume null termination.  Trim blanks.
**	"     "    "  (Output)	Blank pad out to length.  Don't null terminate.
**
**	DB_VCH_TYPE
**		      (Input)	No trimming.
**		      (Output)	Don't blank pad.
**
**	In general, in 6.0, non-C Windows descriptor character data is sent as 
**	DB_CHA_TYPE because we require it to be blank trimmed on input and 
**	blank padded on output.  However, there are some exceptions.  See the 
**	table in iivmsstr.c for a complete mapping of Windows string descriptors to 
**	internally understood descriptions.
**
**	The pre-6.0 interface routines are also contained in this file.
**
**  Defines: 
**    New Interface for 6.0:
**	IIxtcoputio	- Interface to IItcoputio
**	IIxtcogetio	- Interface to IItcogetio
**    Pre 6.0 interface:
** 	IIxtcolret( isvar, type, len, addr, column )
**	IIxtrc_param( tlist, argv )
**	IIxtsc_param( tlist, argv )
**
**  Notes:
** 	These routines should not be ported to non-Windows environments.  
-*
**  History:
**      22-Sep-2003 (fanra01)
**          Bug 110909
**          Created based on VMS version iivtbacc.c
**/

/* External data */
FUNC_EXTERN	char	*IIsd(), *IIsd_no();
FUNC_EXTERN	void	IIsd_undo(), IIsd_lenptr(), IIsd_convert(), IIsd_fill();

/*
** IIxtcoputio - Interface to IItcoputio
*/
void
IIxtcoputio(colname, ind, isref, type, length, data)
char	*colname;
i2	*ind;
i4	isref, type, length;
PTR	data;
{
    i4	llen;

    if (type != DB_CHR_TYPE)
    {
        _VOID_ IItcoputio( colname, ind, isref, type, length, data );
        return;
    }
    if (data != NULL)
    {
        llen = STlength( data );
    }
    _VOID_ IItcoputio( colname, ind, isref, type, llen, data );
}

/*
** IIxtcogetio - Interface to IItcogetio
*/
void
IIxtcogetio(ind, isref, type, length, addr, colname)
i2	*ind;
i4	isref, type, length;
PTR	addr;
char	*colname;
{
    char	buf[DB_MAXSTRING +2];
    i4		llen;
    i4  	ltype;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
	_VOID_ IItcogetio( ind, isref, type, length, addr, colname );
	return;
    }
    ldata = buf;
    _VOID_ IItcogetio( ind, isref, type, length, ldata, colname );
    llen = STlength( ldata );
    if (addr != NULL)
    {
        MEfill( length, ' ', addr );
    }
    if (llen != 0)
    {
        CMcopy( ldata, llen, addr );
    }
}

/*
** IIxtcolret - Pre-6.0 generated call.  Interface to C IItcogetio.
*/
void
IIxtcolret( isref, type, len, addr, colname )
i4	isref, type, len;
PTR	addr;
char	*colname;
{
    char	buf[DB_MAXSTRING +2];
    i4		llen;
    i4  	ltype;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
	_VOID_ IItcogetio( (i2 *)0, isref, type, len, addr, colname );
	return;
    }
    ldata = buf;
    _VOID_ IItcogetio( (i2 *)0, isref, ltype, llen, ldata, colname );
    llen = STlength( ldata );
    if (addr != NULL)
    {
        MEfill( len, ' ', addr );
    }
    if (llen != 0)
    {
        CMcopy( ldata, llen, addr );
    }
}

# else
static	i4	ranlib_dummy;
# endif /* NT_GENERIC */
