/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <wcs.h>
#include <ddfhash.h>
#include <erwsf.h>
#include <wsf.h>
#include <wcs.h>

/*
**
**  Name: wcsdoc.c - Document memory management
**
**  Description:
**	This file contains the whole ice document memory management package
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**	07-Sep-1998 (fanra01)
**	    Enclosed MECOPY_VAR_MACRO in braces.
**          Fixup compiler warnings on unix.
**
**  09-Feb-2000 (chika01)
**	    Fix bug 100378 (WCSDocAdd()): Checks for error before updating 
**      document into memory space.
** 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      16-Mar-2001 (fanra01)
**          Bug 104245
**          Add additional error information to the retrieval of an unknown
**          document name.
**      29-Aug-2001 (fanra01)
**          Bug 104028
**          Don't return error status for a non-existent file just log it,
**          as copying of a business unit relies on the return not failing.
**      20-Sep-2001 (fanra01)
**          Bug 105833
**          Return error when a document is not found.  The copied out file
**          must be pre-registered.
**      09-Jul-2002 (fanra01)
**          Bug 108207
**          Add function to perform validation check of document id.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**/

typedef struct __WCS_DOCUMENT 
{
	char					*key;
	char					*name;
	char					*suffix;
	u_i4					unit;
	u_i4					id;
	i4					flag;
	u_i4					owner;
	u_i4					ext_loc;
	char					*ext_file;
	char					*ext_suffix;
	WCS_FILE				*file;
} WCS_DOCUMENT;

static DDFHASHTABLE			documents = NULL;
static WCS_DOCUMENT			**documents_id = NULL;
static u_i4						wcs_document_max = 0;
static u_i2							wcs_mem_doc_tag;

/*
** Name: WCSDocsUseUnit() - Check if the unit is empty
**
** Description:
**
** Inputs:
**	u_i4	: unit id
**
** Outputs:
**	bool*	: result
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS 
WCSDocsUseUnit (
	u_i4	unit,
	bool	*result) 
{
	u_i4	i = 1;

	for (i = 1; i < wcs_document_max; i++)
		 if (documents_id[i] != NULL &&
			 documents_id[i]->unit == unit)	
		 {
			 *result = TRUE;
			 return(GSTAT_OK);
		 }

	*result = FALSE;
return(GSTAT_OK);
}

/*
** Name: WCSPerformDocumentId() - Generate a new document id
**
** Description:
**
** Inputs:
**
** Outputs:
**	u_nat*	: document id
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS 
WCSPerformDocumentId (
	u_i4 *id) 
{
	GSTATUS err = GSTAT_OK;
	u_i4	i = 1;

	for (i = 1; i < wcs_document_max && documents_id[i] != NULL; i++) ;

	*id = i;
return(err);
}

/*
** Name: WCSDocIsOwner() - Check the document owner
**
** Description:
**
** Inputs:
**	u_i4	: document id
**	u_i4	: user id
**
** Outputs:
**	bool*	: result
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS 
WCSDocIsOwner (
	u_i4	id,
	u_i4	user,
	bool	*result) 
{
	G_ASSERT(documents == NULL, E_WS0051_WCS_MEM_NOT_OPEN);	

	if (id > 0 &&
		id < wcs_document_max &&
		documents_id[id] != NULL)
	{
		if (documents_id[id]->owner == user)
			*result = TRUE;
		else 
			return( WCSUnitIsOwner(documents_id[id]->unit, user, result) );
	}
	else
		*result = FALSE;
return(GSTAT_OK);
}

/*
** Name: WCSDocPerformName() - Generate the document key
**
** Description:
**	That key will be used in the document hash table
**
** Inputs:
**	u_i4	: unit
**	char*	: document name
**	char*	: document suffix, 
**
** Outputs:
**	char**	: key
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS
WCSDocPerformName(
	u_i4 unit,
	char *doc, 
	char *suffix, 
	char **key) 
{
	GSTATUS err = GSTAT_OK;
	u_i4 length;
	length = 10 + 
		((doc != NULL) ? STlength(doc) : 0) + 
		((suffix != NULL) ? STlength(suffix) : 0);

	err = GAlloc((PTR*)key, length, FALSE);
	if (err == GSTAT_OK)
	{
		if (suffix != NULL && suffix[0] != EOS)
			STprintf(*key, "%d_%s%c%s", 
							unit,
							doc,
							CHAR_FILE_SUFFIX_SEPARATOR,
							suffix);
		else
			STprintf(*key, "%d_%s", 
							unit,
							doc);
	}
return(err);
}

/*
** Name: WCSDocInit() - Initialize a document
**
** Description:
**
** Inputs:
**	WCS_DOCUMENT*	: object
**	u_i4			: document id
**	u_i4			: unit id
**	char*			: document name
**	char*			: document suffix
**	i4			: document flag
**	u_i4			: document owner
**	u_i4			: external location
**	char*			: external file
**	char*			: external suffix
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS 
WCSDocInit (
	WCS_DOCUMENT	*object,
	u_i4			id,
	u_i4			unit,
	char*			name, 
	char*			suffix, 
	i4			flag,
	u_i4			owner,
	u_i4			ext_loc,
	char*			ext_file,
	char*			ext_suffix) 
{
	GSTATUS err = GSTAT_OK;
	char		 *tmp = NULL;

	err = WCSDocPerformName(unit, name, suffix, &tmp);
	if (err == GSTAT_OK) 
	{
		u_i4 length;
		length = STlength(tmp) + 1;
		err = G_ME_REQ_MEM(wcs_mem_doc_tag, object->key, char, length);
		if (err == GSTAT_OK) 
		{
			MECOPY_VAR_MACRO(tmp, length, object->key);
			length = STlength(name) + 1;
			err = G_ME_REQ_MEM(wcs_mem_doc_tag, object->name, char, length);
			if (err == GSTAT_OK) 
			{
				MECOPY_VAR_MACRO(name, length, object->name);
				length = STlength(suffix) + 1;
				err = G_ME_REQ_MEM(wcs_mem_doc_tag, object->suffix, char, length);
				if (err == GSTAT_OK) 
				{
					MECOPY_VAR_MACRO(suffix, length, object->suffix);
					if (ext_file != NULL && ext_file[0] != EOS)
					{
						length = STlength(ext_file) + 1;
						err = G_ME_REQ_MEM(wcs_mem_doc_tag, object->ext_file, char, length);
						if (err == GSTAT_OK) 
							MECOPY_VAR_MACRO(ext_file, length, object->ext_file);
					}
					else
						object->ext_file = NULL;

					if (err == GSTAT_OK &&
						ext_suffix != NULL &&
						ext_suffix[0] != EOS) 
					{
						length = STlength(ext_suffix) + 1;
						err = G_ME_REQ_MEM(wcs_mem_doc_tag, object->ext_suffix, char, length);
						if (err == GSTAT_OK) 
							MECOPY_VAR_MACRO(ext_suffix, length, object->ext_suffix);
					}
					else
						object->ext_suffix = NULL;

					if (err == GSTAT_OK) 
					{
						object->ext_loc = ext_loc;
						object->unit = unit;
						object->id = id;
						object->flag = flag;
						object->owner = owner;
					}
				}
			}
		}
	}
return(err);
}

/*
** Name: WCSDocAdd() - Create a new document into the document memory space
**
** Description:
**
** Inputs:
**	u_i4			: document id
**	u_i4			: unit id
**	char*			: document name
**	char*			: document suffix
**	i4			: document flag
**	u_i4			: document owner
**	u_i4			: external location
**	char*			: external file
**	char*			: external suffix
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**
**	09-Feb-2000 (chika01)
**	    Checks for error before updating document into memory space.
** 
**/

GSTATUS 
WCSDocAdd (
	u_i4	id,
	u_i4	unit,
	char*	name, 
	char*	suffix, 
	i4 flag,
	u_i4	owner,
	u_i4	ext_loc,
	char*	ext_file,
	char*	ext_suffix) 
{
	GSTATUS err = GSTAT_OK;
	WCS_DOCUMENT *object = NULL;
	char		 *tmp = NULL;

	if (id >= wcs_document_max) 
	{
		WCS_DOCUMENT* *tmp = documents_id;
		err = G_ME_REQ_MEM(
					wcs_mem_doc_tag, 
					documents_id, 
					WCS_DOCUMENT*, 
					id + WCS_DEFAULT_DOCUMENT);

		if (err == GSTAT_OK && tmp != NULL) 
			MECOPY_CONST_MACRO(
				tmp, 
				sizeof(WCS_DOCUMENT*)*wcs_document_max, 
				documents_id);

		if (err == GSTAT_OK)
		{
			MEfree((PTR)tmp);
			wcs_document_max = id + WCS_DEFAULT_DOCUMENT;
		}
		else
			documents_id = tmp;
	}

	if (err == GSTAT_OK)
	{
		err = G_ME_REQ_MEM(wcs_mem_doc_tag, object, WCS_DOCUMENT, 1);
		if (err == GSTAT_OK)
			err = WCSDocInit (
					object,
					id,
					unit,
					name, 
					suffix, 
					flag,
					owner,
					ext_loc,
					ext_file,
					ext_suffix);

        if (err == GSTAT_OK) 
        {
            err = DDFputhash(documents, object->key, HASH_ALL, (PTR) object);
            if (err == GSTAT_OK) 
            {
                documents_id[id] = object;
            }
        }
    }
    return(err);
}

/*
** Name: WCSDocGet() - Retrieve the document id through his unit, name and suffix
**
** Description:
**
** Inputs:
**	u_i4			: unit id
**	char*			: document name
**	char*			: document suffix
**
** Outputs:
**	u_nat*			: document id
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	10-Sep-98 (marol01)
**		changed access control. If the unit is SYSTEM_ID, the document is 
**		definitly a SYSTEM_ID document. Else if we do find a document the function 
**		will return an error message.
**      16-Mar-2001 (fanra01)
**          Add additional information for unknown document.
**      29-Aug-2001 (fanra01)
**          Don't return error status for a non-existent file just log it,
**          as copying of a business unit relies on the return not failing.
**      20-Sep-2001 (fanra01)
**          Backout the change to not return the error status as this was
**          permitting access to non-registered files in the business unit.
**          If a copied business unit needs to be downloaded from the unit
**          location it must be registered first.
*/
GSTATUS
WCSDocGet ( u_i4 unit, char *name, char *suffix, u_i4 *id )
{
    GSTATUS err = GSTAT_OK;
    WCS_DOCUMENT *object = NULL;
    char *tmp = NULL;

    G_ASSERT(documents == NULL, E_WS0051_WCS_MEM_NOT_OPEN);

    if (unit == SYSTEM_ID)
        *id = SYSTEM_ID;
    else
    {
        err = WCSDocPerformName(unit, name, suffix, &tmp);
        if (err == GSTAT_OK)
            err = DDFgethash(documents, tmp, HASH_ALL, (PTR*) &object);

        if (err == GSTAT_OK)
        {
            if (object != NULL)
                *id = object->id;
            else
            {
                err = DDFStatusAlloc(E_WS0073_WCS_BAD_DOCUMENT);
                DDFStatusInfo( err, "%s.%s", name,suffix );
            }
        }
        MEfree(tmp);
    }
    return(err);
}

/*
** Name: WCSDocCheckFlag() - Check document flag
**
** Description:
**	if the flag is set the return TRUE  else return FALSE
**
** Inputs:
**	u_i4			: document id
**	u_i4			: flag 
**
** Outputs:
**	bool*			: result
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS
WCSDocCheckFlag (
	u_i4 id,
	i4 flag,
	bool *result)
{
	G_ASSERT(documents == NULL, E_WS0051_WCS_MEM_NOT_OPEN);

	if (id != SYSTEM_ID)
	{
		G_ASSERT(id < 1 || 
						 id >= wcs_document_max ||
						 documents_id[id] == NULL,
						 E_WS0066_WCS_BAD_DOC);
		*result = (flag & documents_id[id]->flag) ? TRUE : FALSE;
	}
	else
		*result = FALSE;

return(GSTAT_OK);
}

/*
** Name: WCSDocSetFile() - Set the document file.
**
** Description:
**	WCS uses this attribute to keep the file information which is PRE and PERMANENT cached
**
** Inputs:
**	u_i4			: document id
**	WCS_FILE*		: file
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS
WCSDocSetFile (
	u_i4 id,
	WCS_FILE *file)
{
	GSTATUS err = GSTAT_OK;

	G_ASSERT(documents == NULL, E_WS0051_WCS_MEM_NOT_OPEN);
	G_ASSERT(id < 1 || 
			 id >= wcs_document_max ||
			 documents_id[id] == NULL,
			 E_WS0066_WCS_BAD_DOC);

	documents_id[id]->file = file;
return(err);
}

/*
** Name: WCSDocGetFile() - Get the document file.
**
** Description:
**	see WCSDocSetFile
**
** Inputs:
**	u_i4			: document id
**
** Outputs:
**	WCS_FILE**		: file
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS
WCSDocGetFile (
	u_i4 id,
	WCS_FILE **file)
{
	GSTATUS err = GSTAT_OK;
	G_ASSERT(documents == NULL, E_WS0051_WCS_MEM_NOT_OPEN);
	G_ASSERT(id < 1 || 
			 id >= wcs_document_max ||
			 documents_id[id] == NULL,
			 E_WS0066_WCS_BAD_DOC);

	*file = documents_id[id]->file;
return(err);
}

/*
** Name: WCSDocGetUnit() - Get the unit of the document
**
** Description:
**
** Inputs:
**	u_i4		: document id
**
** Outputs:
**	u_nat*		: unit id
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS
WCSDocGetUnit (
	u_i4	id,
	u_i4	*unit_id)
{
	GSTATUS err = GSTAT_OK;
	G_ASSERT(documents == NULL, E_WS0051_WCS_MEM_NOT_OPEN);
	G_ASSERT(id < 1 || 
			 id >= wcs_document_max ||
			 documents_id[id] == NULL,
			 E_WS0066_WCS_BAD_DOC);

	*unit_id = documents_id[id]->unit;
return(err);
}

/*
** Name: WCSDocGetExtFile() - Get external file information
**
** Description:
**
** Inputs:
**	u_i4		: document id
**
** Outputs:
**	u_nat*		: external location id
**	char*		: external file name
**	char*		: external suffix
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS
WCSDocGetExtFile (
	u_i4	id,
	u_i4	*location_id,
	char*	*filename,
	char*	*suffix)
{
	GSTATUS err = GSTAT_OK;
	G_ASSERT(documents == NULL, E_WS0051_WCS_MEM_NOT_OPEN);
	G_ASSERT(id < 1 || 
			 id >= wcs_document_max ||
			 documents_id[id] == NULL,
			 E_WS0066_WCS_BAD_DOC);

	*location_id = documents_id[id]->ext_loc;
	err = G_ST_ALLOC(*filename, documents_id[id]->ext_file);
	if (err == GSTAT_OK)
		err = G_ST_ALLOC(*suffix, documents_id[id]->ext_suffix);
return(err);
}

/*
** Name: WCSDocUpdate() - Update a document into the document memory space
**
** Description:
**
** Inputs:
**	u_i4			: document id
**	u_i4			: unit id
**	char*			: document name
**	char*			: document suffix
**	i4			: document flag
**	u_i4			: document owner
**	u_i4			: external location
**	char*			: external file
**	char*			: external suffix
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
WCSDocUpdate (
	u_i4	id,
	u_i4	unit,
	char	*name, 
	char	*suffix, 
	i4 flag,
	u_i4	owner,
	u_i4	ext_loc,
	char*	ext_file,
	char*	ext_suffix)
{
	GSTATUS err = GSTAT_OK;
	WCS_DOCUMENT *object = NULL;
	char		 *tmp = NULL;

	G_ASSERT(documents == NULL, E_WS0051_WCS_MEM_NOT_OPEN);
	G_ASSERT(id < 1 || 
			 id >= wcs_document_max ||
			 documents_id[id] == NULL,
			 E_WS0066_WCS_BAD_DOC);

	if (err == GSTAT_OK)
		err = DDFdelhash(documents, documents_id[id]->key, HASH_ALL, (PTR*) &object);

	if (err == GSTAT_OK && object != NULL)
	{
		char		*tmp_key = object->key;
		char		*tmp_name = object->name;
		char		*tmp_suffix = object->suffix;
		char		*tmp_ext_file = object->ext_file;
		char		*tmp_ext_suffix = object->ext_suffix;

		object->name = NULL;
		object->ext_file = NULL;
		object->ext_suffix = NULL;

		err = WCSDocInit (
				object,
				id,
				unit,
				name, 
				suffix, 
				flag,
				owner,
				ext_loc,
				ext_file,
				ext_suffix);

		if (err != GSTAT_OK)
		{
			MEfree(object->key);
			MEfree(object->name);
			MEfree(object->suffix);
			MEfree(object->ext_file);
			MEfree(object->ext_suffix);
			object->key = tmp_key;
			object->name = tmp_name;
			object->suffix = tmp_suffix;
			object->ext_file = tmp_ext_file;
			object->ext_suffix = tmp_ext_suffix;
			CLEAR_STATUS (DDFputhash(documents, object->key, HASH_ALL, (PTR) object));
		}
		else
		{
			err = DDFputhash(documents, object->key, HASH_ALL, (PTR) object);
			MEfree(tmp_key);
			MEfree(tmp_name);
			MEfree(tmp_suffix);
			MEfree(tmp_ext_file);
			MEfree(tmp_ext_suffix);
		}
	}
return(err);
}

/*
** Name: WCSDocDelete() - delete a document from the document memory space
**
** Description:
**
** Inputs:
**	u_i4			: document id
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS 
WCSDocDelete (
	u_i4 id) 
{
	GSTATUS err = GSTAT_OK;
	WCS_DOCUMENT *object = NULL;
	char *tmp = NULL;

	G_ASSERT(documents == NULL, E_WS0051_WCS_MEM_NOT_OPEN);
	G_ASSERT(id < 1 || 
			 id >= wcs_document_max ||
			 documents_id[id] == NULL,
			 E_WS0066_WCS_BAD_DOC);

	if (err == GSTAT_OK)
		err = DDFdelhash(documents, documents_id[id]->key, HASH_ALL, (PTR*) &object);

	if (err == GSTAT_OK && object != NULL) 
	{
		MEfree((PTR)object->ext_file);
		MEfree((PTR)object->key);
		MEfree((PTR)object->name);
		MEfree((PTR)object->suffix);
		MEfree((PTR)object);
		documents_id[id] = NULL;
	}
	MEfree(tmp);
return(err);
}

/*
** Name: WCSDocCopy() - Copy document information into variables
**
** Description:
**
** Inputs:
**	u_i4			: document id
**
** Outputs:
**	u_nat**			: unit id
**	char**			: document name
**	char**			: document suffix
**	i4**		: document flag
**	u_nat**			: document owner
**	u_nat**			: external location
**	char**			: external file
**	char**			: external suffix
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
WCSDocCopy (
	u_i4	id,
	u_i4	**unit,
	char	**name,
	char	**suffix,
	u_i4	**flag,
	u_i4	**owner,
	u_i4	**ext_loc,
	char	**ext_file,
	char	**ext_suffix) 
{
	GSTATUS err = GSTAT_OK;
	u_i4 length;
	WCS_DOCUMENT *doc = documents_id[id];

	if (unit != NULL)
	{
		length = sizeof(u_i4);
		err = GAlloc((PTR*)unit, length, FALSE);
		if (err == GSTAT_OK)
			**unit = doc->unit;
	}

	if (err == GSTAT_OK && name != NULL)
	{
            length = ((doc->name) ? STlength(doc->name) : 0) + 1;
            err = GAlloc((PTR*)name, length, FALSE);
            if (err == GSTAT_OK && length > 1)
            {
                MECOPY_VAR_MACRO(doc->name, length, *name);
            }
            else
            {
                name[0] = EOS;
            }
	}

	if (err == GSTAT_OK && suffix != NULL)
	{
            length = ((doc->suffix) ? STlength(doc->suffix) : 0) + 1;
            err = GAlloc((PTR*)suffix, length, FALSE);
            if (err == GSTAT_OK && length > 1)
            {
                MECOPY_VAR_MACRO(doc->suffix, length, *suffix);
            }
            else
            {
                suffix[0] = EOS;
            }
	}

	if (err == GSTAT_OK && flag != NULL)
	{
		length = sizeof(u_i4);
		err = GAlloc((PTR*)flag, length, FALSE);
		if (err == GSTAT_OK)
			**flag = doc->flag;
	}

	if (err == GSTAT_OK && owner != NULL)
	{
		length = sizeof(u_i4);
		err = GAlloc((PTR*)owner, length, FALSE);
		if (err == GSTAT_OK)
			**owner = doc->owner;
	}

	if (err == GSTAT_OK && ext_loc != NULL)
	{
		length = sizeof(u_i4);
		err = GAlloc((PTR*)ext_loc, length, FALSE);
		if (err == GSTAT_OK)
			**ext_loc = doc->ext_loc;
	}

	if (err == GSTAT_OK && ext_file != NULL)
	{
	    length = ((doc->ext_file) ? STlength(doc->ext_file) : 0) + 1;
	    err = GAlloc((PTR*)ext_file, length, FALSE);
            if (err == GSTAT_OK && length > 1)
	    {
                MECOPY_VAR_MACRO(doc->ext_file, length, *ext_file);
            }
            else
            {
                ext_file[0] = EOS;
            }
	}

	if (err == GSTAT_OK && ext_suffix != NULL)
	{
		length = ((doc->ext_suffix) ? STlength(doc->ext_suffix) : 0) + 1;
		err = GAlloc((PTR*)ext_suffix, length, FALSE);
		if (err == GSTAT_OK && length > 1)
		{
			MECOPY_VAR_MACRO(doc->ext_suffix, length, *ext_suffix);
		}
		else
		{
			ext_suffix[0] = EOS;
		}
	}
return(err);
}

/*
** Name: WCSGetDocFromId() - retrieve document information and put them into variables
**
** Description:
**
** Inputs:
**	u_i4			: document id
**
** Outputs:
**	u_nat**			: unit id
**	char**			: document name
**	char**			: document suffix
**	i4**		: document flag
**	u_nat**			: document owner
**	u_nat**			: external location
**	char**			: external file
**	char**			: external suffix
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
WCSGetDocFromId (
	u_i4 id,
	u_i4	**unit,
	char	**name,
	char	**suffix,
	u_i4	**flag,
	u_i4	**owner,
	u_i4	**ext_loc,
	char	**ext_file,
	char	**ext_suffix) 
{
	G_ASSERT(documents_id == NULL || 
			 id < 1 ||
			 id > wcs_document_max ||
			 documents_id[id] == NULL,
			 E_WS0073_WCS_BAD_DOCUMENT);

	return(WCSDocCopy(id,
					 unit,
					 name,
					 suffix,
					 flag,
					 owner,
					 ext_loc,
					 ext_file,
					 ext_suffix));
}

/*
** Name: WCSDocBrowse() - list documents and thier information into variables
**
** Description:
**
** Inputs:
**	PTR*			: cursor
**
** Outputs:
**	u_nat**			: document id
**	u_nat**			: unit id
**	char**			: document name
**	char**			: document suffix
**	i4**		: document flag
**	u_nat**			: document owner
**	u_nat**			: external location
**	char**			: external file
**	char**			: external suffix
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
WCSDocBrowse (
	PTR *cursor,
	u_i4	**id,
	u_i4	**unit,
	char	**name,
	char	**suffix,
	u_i4	**flag,
	u_i4	**owner,
	u_i4	**ext_loc,
	char	**ext_file,
	char	**ext_suffix) 
{
	GSTATUS err = GSTAT_OK;
	u_i4 pos = (u_i4)(SCALARP) *cursor;

	while (pos < wcs_document_max &&
		   documents_id[pos] == NULL) 
		   pos++;

	if (pos < wcs_document_max &&
		documents_id[pos] != NULL)
	{
		u_i4 length = sizeof(u_i4);
		err = GAlloc((PTR*)id, length, FALSE);
		if (err == GSTAT_OK)
			**id = pos;
		if (err == GSTAT_OK)
			err = WCSDocCopy(pos,
							 unit,
							 name,
							 suffix,
							 flag,
							 owner,
							 ext_loc,
							 ext_file,
							 ext_suffix);
		pos++;
	}
	else
		pos = 0;

	*cursor = (PTR)(SCALARP) pos;
return(err);
}

/*
** Name: WCSDocInitialize() - Initialisze the document memory space
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
WCSDocInitialize ()
{
	GSTATUS err = GSTAT_OK;
	char*	size= NULL;
	char	szConfig[CONF_STR_SIZE];
	i4	numberOfDocument= 50;

	wcs_mem_doc_tag = MEgettag();

	STprintf (szConfig, CONF_DOCUMENT_SIZE, PMhost());
	if (PMget( szConfig, &size ) == OK && size != NULL)
		CVan(size, &numberOfDocument);

	err = DDFmkhash(numberOfDocument, &documents);
	if (err == GSTAT_OK)
	{
		err = G_ME_REQ_MEM(
				wcs_mem_doc_tag, 
				documents_id, 
				WCS_DOCUMENT*, 
				WCS_DEFAULT_DOCUMENT);

		wcs_document_max = WCS_DEFAULT_DOCUMENT;
	}
return(err);
}

/*
** Name: WCSDocTerminate() - free the document memory space
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
WCSDocTerminate () 
{
	GSTATUS err = GSTAT_OK;
	
	if (documents_id != NULL)
	{
		u_i4 i;
		for (i = 0; i <= wcs_document_max; i++)
			if (documents_id[i] != NULL)
				CLEAR_STATUS(WCSDocDelete(i));
	}
	CLEAR_STATUS (DDFrmhash(&documents));
	MEfreetag(wcs_mem_doc_tag);
return(err);
}

/*
** Name: WCSvaliddoc
**
** Description:
**      Test the provided document id is in range and valid.
**
** Inputs:
**	id          Document identifier
**
** Outputs:
**      valid   TRUE if document is valid.
**              FALSE if invalid.
**              if NULL only the return status is given.
**
** Returns:
**	GSTAT_OK
**      E_WS0051_WCS_MEM_NOT_OPEN   documents have not been initialized.
**      E_WS0073_WCS_BAD_DOCUMENT   document id is invalid.
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
WCSvaliddoc( u_i4 id, bool* valid )
{
    GSTATUS err = GSTAT_OK;

    if (valid != NULL)
    {
        *valid = FALSE;
    }
    G_ASSERT(documents == NULL, E_WS0051_WCS_MEM_NOT_OPEN);
    G_ASSERT(documents_id == NULL ||
        id < 1 ||
        id > wcs_document_max ||
        documents_id[id] == NULL,
        E_WS0073_WCS_BAD_DOCUMENT);
    if (valid != NULL)
    {
        *valid = TRUE;
    }
    return(err);
}
