/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

# include <compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<st.h>
# include	<iicommon.h>
# include <fe.h>
# include <me.h>
# include <er.h>
#include "iamstd.h"
#include "iamtbl.h"
#include "iamvers.h"
# include <ooclass.h>

/**
** Name:	iamenc.c - AOM object encoding
**
** Description:
**	Routine to encode an AOM object to the database.
*/

/*{
** Name:	oo_encode -
**
** Description:
**	Writes encoding string out to database through drive table template.
**	Calls drive table procedure to write out arrays.  The procedures from
**	the table, and this routine make use of db_puts and db_printf to do
**	the writing.  This routine makes the initial call to db_initbuf and
**	the final call to db_endbuf.
**
** Inputs:
**	tbl	driver table (template)
**	num	length of tbl
**	id	object id for object
**	obj	actual object
**
** Outputs:
**
**	return:
**		OK		success
**		ILE_FRDWR	write failure
**
** History:
**	8/86 (rlm)	written
**	11/08/90 (emerson)
**		Change the version number in the first token
**		from 4156 to 6303, to indicate that the format
**		of the encoded IL has changed.
**		The version number is now specified by IAOM_IL_VERSION,
**		defined in iamvers.h.  Future changes to the version number
**		will be be made in iamvers.h, and documented in its history.
**	01/09/91 (emerson)
**		Correct the call to db_printf: cast numeric literal to i4.
**	22-jun-92 (davel)
**		changed db_printfs to pass nothing but i4s.
**	09/10/93 (kchin)
**		See comment under 'case AOO_array' below.
**	14-sep-93 (mgw)
**		Suppress compile warning by casting ME_ALIGN_MACRO return
**		to obj's i4 datatype below.
**	23-Aug-1993 (fredv)
**		Included <st.h>.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	13-May-2005 (kodse01)
**	    replace %ld with %d for i4 values.
*/

STATUS
oo_encode(tbl,num,id,obj)
CODETABLE *tbl;
i4  num;
i4 id;
i4 *obj;
{
	STATUS rc;
	char bufr[ESTRLEN+1];
	i4 elen;
	i4 negid;
	i4 i;
	char floatbuf[64];

	/*
	** ESTRLEN should be the max value for IIOOencodedSize().  If
	** our table is actually larger somehow, it will still work - we
	** just won't be filling out the columns.  This is simpler than
	** dynamically allocating bufr.  It also assures that we can
	** read by allowing a buffer of size ESTRLEN.
	*/
	if ((elen = IIOOencodedSize()) > ESTRLEN)
		elen = ESTRLEN;

	db_initbuf(bufr,elen,id,id);

	/*
	** Write out an initial token indicating the version of ABF
	** that's encoding the IL.  This version number will be inspected
	** by iamdec.qsc.
	*/
	if (db_printf(ERx("V%d\n*%d{"),
		(i4)IAOM_IL_VERSION, (i4)id) != OK)
	{
		return (ILE_FRDWR);
	}

	negid = -1;

	/*
	** scan table, writing out top level object.  When an array
	** element is encountered, fill in tbl with a unique (within
	** this string) negative id, the address and number of elements
	** of the array.
	*/
#ifdef AOMDTRACE
	aomprintf(ERx("\nFIRST TABLE PASS\n>"));
#endif
	for (i=0; i < num; ++i)
	{
#ifdef AOMDTRACE
		aomprintf (ERx("\nIndex %d, type %d\n>"),i,tbl[i].type);
#endif
		switch(tbl[i].type)
		{
		case AOO_f8:
			(VOID)STprintf(floatbuf, ERx("F%g"), *((f8 *)obj), '.');
			rc = db_puts(floatbuf);
			obj += sizeof(f8)/sizeof(i4);
			break;
		case AOO_i4:
			rc = db_printf(ERx("I%d"),(i4)*obj);
			++obj;
			break;
		case AOO_array:
			rc = db_printf(ERx("A%d"), (i4)negid);
			if (rc != OK)
				break;
			tbl[i].id = negid;

			/*
			** Need to align obj to sizeof(PTR) boundary, this
			** is especially important on 64-bit platform like
			** Alpha where pointer size is 8 bytes.
			** eg.  typedef struct
			**	{
        	 	**		i4 version;
        		**		MFESC   **esc;
        		**		i4 numescs;
			**	} ESCENC;
			** ESCENC is defined in iammpop.c.
			** Since i4 is defined as int, the offset of esc
			** should be 4, which is not true on Alpha, as the
			** compiler will align the pointer element to 8 byte
			** boundary, thus esc 's offset will be 8 then.
			** The use of ME_ALIGN_MACRO here is to skip the extra
			** 4 bytes pad added by the compiler after version.
			**					[kchin] 2/3/93
			*/
			obj = (i4 *) ME_ALIGN_MACRO(obj,sizeof(PTR));

			tbl[i].addr = *((i4 **) obj);
			obj += sizeof(i4 *)/sizeof(i4);
			tbl[i].numel = *obj;
			rc = db_printf(ERx("I%d"), (i4)tbl[i].numel);
			/*
			** On 32-bit platforms, ++obj will position correctly
			** to next element in 'obj' (which is struct AOMFRAME
			** in the caller), since pointer size is 4 bytes, no
			** padding is added by the compiler to align the 
			** pointer element in structure(refer to struct AOMFRAME
			** defined in iamfrm.h).  But, on 64-bit platform like
			** Alpha, pointer size is 8 bytes, any pointer element 
			** within a structure will be aligned to 8-byte 
			** boundary by the compiler, thus ++obj will point to 
			** the padded 4 byte, which is wrong.
			**   Change  ++obj  to  obj += sizeof(PTR)/sizeof(i4);
			** Note:this change only apply to the structure AOMFRAME
			**	 defined in iafrm.h now, any change to the 
			** 	 structure need recoding of the statment below.
			**					 [kchin]
			*/
			obj += sizeof(PTR)/sizeof(i4);
			--negid;
#ifdef AOMDTRACE
			aomprintf (ERx("\ntbl[%d] id=%d addr=X%lx num=%d\n>"),
				i,tbl[i].id,tbl[i].addr,tbl[i].numel);
#endif
			break;
		}

		if (rc != OK)
			return (ILE_FRDWR);
	}

	if (db_puts(ERx("}")) != OK)
		return (ILE_FRDWR);

	/*
	** now do a second table scan, finding all the array elements,
	** and writing out THEIR strings by calling the indicated procedure.
	*/
#ifdef AOMDTRACE
	aomprintf (ERx("\nSECOND TABLE PASS\>n"));
#endif
	for (i=0; i < num; ++i)
	{
#ifdef AOMDTRACE
		aomprintf (ERx("\nIndex %d\n>"),i);
#endif
		if (tbl[i].type != AOO_array)
			continue;
		if (tbl[i].numel <= 0)
			continue;
		if (db_printf(ERx("*%d{"), (i4)tbl[i].id) != OK)
			return (ILE_FRDWR);
#ifdef AOMDTRACE
		aomprintf (ERx("\nCALL PROC %d(X%lx,%d)\n>"),
				i,(i4) tbl[i].addr,tbl[i].numel);
#endif
		if ( (*(tbl[i].wproc)) (tbl[i].addr, tbl[i].numel) != OK)
			return (ILE_FRDWR);
		if (db_puts(ERx("}")) != OK)
			return (ILE_FRDWR);
	}

	return (db_endbuf());
}
