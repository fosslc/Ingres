/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	eroo.msg -	Object Manager Message Definitions File.
**
** Description:
**	Contains the definitions for the messages and error messages
**	output by the object manager module.
**
** History:
**	Revision 6.0  87/07/27  peterk
**	Initial revision.
**
**	Revision 6.2  89/01  wong
**	Added F_OO0001_DBA, F_OO00002_Unspecified, S_OO0029 and moved
**	S_OO0025 to "uf!eruf.msg".
**	89/11/10  terryr  Added E_OO0044.
**
**	Revision 6.3  90/03  wong
**	Changed message S_OO0029 to F_OO0004 for performance.
**	Editted S_OO002E_ExistsInDatabase for grammar.  JupBug #20946.
**	90/04  wong  Added class names.
**
**	Revision 6.3  91/03  steveh
**	Changed message E_OO0005_Not_a_legal_name.
**	This helps fix bug 36363: RBF and VIFRED are inconsistent in their
**	handling of Rename.  The change to the message involved reversing
**	the order of the arguments so that IIUGerr correctly displays the
**	name of the renamed object when the length of the name is excessively
**	long.
**
**	2-jun-93 (blaise)
**		Added E_OO0051_OLD_VERSION, for all those thousands of users
**		who are attempting to edit pre-6.0 vigraph objects - from 6.5
**		we no longer attempt to convert these.
**/

# define	_OO_CLASS	135

F_OO0001_DBA	"DBA"

F_OO0002_Unspecified	"unspecified"

/*
**	catutil.qc
*/
F_OO0004_Retrieving	"Retrieving %0c catalogs . . ."
/*	e.g., "Retrieving Application catalogs . . ." */
S_OO0001_Destroy_Prompt	"Do you wish to destroy %0c '%1c'? "
/*	e.g., "Do you wish to destroy Form 'abc'?"	*/
S_OO0002_Destroy_Status	"Destroying %0c '%1c' . . ."
/*	e.g., "Destroying Form 'abc' . . ."	*/
E_OO0003_Destroy_Error	"An error was encountered destroying %0c '%1c'"
/*	e.g., "An error was encountered destroying Form 'abc'." */
S_OO0004_Enter_new_name_Prompt	"Enter new name: "
E_OO0005_Not_a_legal_name     "Not a legal %1c name: '%0c'."
/*    e.g., "Not a legal Form name: 'abc!@&'" */
S_OO0006_None_in_table	"There are no %0cs in the table."
/*	e.g., "There are no more Forms in the table."	*/
E_OO0007_Name_must_be_specifie	"%0c name must be specified."
/*	e.g., "Form name must be specified."	*/
S_OO0008_None_found	"No %0cs found."
/*	e.g., "No Forms found." */
S_OO0009_Rename_Status	"Renaming %0c '%1c' to '%2c' . . ."
/*	e.g., "Renaming Form 'abc' to 'xyz' . . ."	*/
E_OO000A_Rename_Error	"An error was encountered renaming %0c '%1c'."
/*	e.g., "An error was encountered renaming Form 'abc'." */
S_OO000B_Save_Status	"Saving %0c '%1c' . . ."
/*	e.g., "Saving Form 'abc' . . ."	*/
E_OO000C_Save_Error	"An error was encountered saving %0c '%1c'."
/*	e.g., "An error was encountered saving Form 'abc'." */
F_OO0003_Destroy_Conf	"Confirm Request to Destroy"
/*
**	cmethod.qc
*/
E_OO000D_Error_from_MEreqmem	"Memory allocation failure:\n
Error %0d from 'MEreqmem()' in 'iiooMemAlloc()'."
/*
**	note: MEreqmem, OOalloc are routine names.
*/
E_OO000E_No_available_slot	"No available slot in OOtable for Object %0d"
E_OO000F_No_more_ids	"No more ids may be generated -- reached limit"
/*
**	co.qc
*/
E_OO0010_at_index_out_of_range	"Collection.at: index out of range"
E_OO0011_atPut_index_out_of_ra	"Collection.atPut: index out of range"
/*
**	encoding.qc
*/
/*
**	enconv60.c
*/
/*
**	memory.qc
*/
E_OO0012_No_fetch_operation	"No _fetch operation for class %0c"
/*
**	e.g., "Error return from fetch of Form 10023 (abc)"
*/
E_OO0013_Error_from_fetch_of	"Error return from _fetch of %0c %1d (%2c)"
E_OO0014_Unexpected_attribute	"Unexpected attribute type %0d encountered"
/*
**	STalloc and fetchAt are routine names
*/
E_OO0015_Error_from_STalloc	"Error from STalloc in fetchAt"
E_OO0016_Only_one_Sequenced_Te	"Only one Sequenced Text attribute per object allowed for now"
E_OO0017_Unexpected_attribute_	"Unexpected attribute type %0d encountered"
/*
**	e.g., "RETRIEVE error in ii_objects: 2602"
*/
E_OO0018_QUEL_error_in	"%0c error in %1c: %2d"
S_OO0019_row_count_for_query	"%0d row(s) %1c'd in %2c"
E_OO001A_Error_from_MEreqmem	"Error %0d from MEreqmem in OOextend()"
E_OO001B_Too_many_retrieved	"Too many %0c retrieved for %1c"
E_OO001C_Too_few_retrieved	"Too few %0c retrieved for %1c"
E_OO001D_Cant_write_sequenced	"Can't write out sequenced text for a temporary object"
/*
**	method.qc
*/
E_OO001E_Method_already_exists	"Method %0c already exists for Class %1c."
/*
**	ob.c
*/
E_OO001F_MEreqmem_err_OBcopyId	"Error %0d from MEreqmem in OBcopyId"
E_OO0020_does_not_understand	"%0c %1c: does not understand %2s"
/*	e.g., "Form abc: does not understand atPut"	*/
E_OO0021_Subclass_method	"The method '%0c' should be implemented in a sub-class."
/*
**	obdestro.qc
*/
/*
**	e.g., Destroy of Form 10023 (abc) failed: DELETE RESTRICTED on field
*/
E_OO0022_Delete_restricted	"Destroy of %0c %1d (%2s) failed: DELETE RESTRICTED on %3s"
/*
**	obencode.c
*/
E_OO0023_Unexpected_attribute_	"Unexpected attribute type %0d encountered"
/*
**	oodetail.qc
*/
E_OO0024_Cant_instantiate_obje	"Cannot instantiate object."
/*
**	ooidutil.qc
*/
/*
**	e.g. "You are not the owner of Form 'abc'."
*/
E_OO0026_NotOwner	"You are not the owner of %0c '%1c'."
/*
**	e.g., "A Form with the name 'xyz' already exists."
*/
E_OO0027_NameAlreadyExists	"A(n) %0c with the name '%1c' already exists"
/*
**	ooinit.c
*/
/*
**	oosave.qc
*/
E_OO0028_Object_not_found	"Object %0d not found in Object table."
E_OO002A_No_name_specified_	"No name specified."
E_OO002B_Invalid_name_specifie	"Invalid name specified."
/*
**	e.g., "Form 'abc' is owned by the DBA."
*/
S_OO002C_OwnedByDBA	"%0c '%1c' is owned by the DBA."
/*
**	e.g., "Do you want to save abc as a new object owned by you?"
*/
S_OO002D_SaveAsNewObject_Promp	"Do you want to save %0c as a new object owned by you?"
/*
**	e.g., "A Form 'abc' exists in database."
*/
S_OO002E_ExistsInDatabase	"%0c '%1c' already exists in the database."
S_OO002F_OverwriteIt_Prompt	"Do you want to overwrite it?"
/*
**	ooschema.qc
*/
E_OO0030_MEreqmem_err_Cfetch	"Error %0d from MEreqmem(method) in Cfetch"
E_OO0031_STalloc_err_Cfetch	"Error from STalloc(methname) in Cfetch"
E_OO0032_STalloc_err_Cfetch	"Error from STalloc(procname) in Cfetch"
E_OO0033_MEreqmem_err_Cfetch	"Error %0d from MEreqmem(attrib) in Cfetch"
E_OO0034_STalloc_err_Cfetch	"Error from STalloc(attrib name) in Cfetch"
E_OO0035_MEreqmem_err_Cfetch	"Error %0d from MEreqmem(reference) in Cfetch"
E_OO0036_STalloc_err_fetchMast	"Error from STalloc(ref name) in fetchMasters"
E_OO0037_STalloc_err_fetchMast	"Error from STalloc(ref connector) in fetchMasters"
E_OO0038_NameMustBeCapitalized	"Name for class must be capitalized."
E_OO0039_Class_already_exists	"Class %0c already exists."
/*
**	ref.qc
*/
E_OO003A_ReferenceAlreadyExist	"Reference %0c already exists for master class %1c."
E_OO003B_AttributeAlreadyExist	"Column attribute %0c already exists for master class %1c."
/*
**	send.qc
*/
E_OO003C_NULL_receiver	"NULL receiver of _%0c"
/*
**	classout.qc
*/
E_OO003D_Too_many_objects_for_	"Too many objects for startup list"
/*
**	cmethod.qc
*/
E_OO003E_ii_id_corrupted	
"System catalog II_ID is corrupted, causing the object manager to fail.\n
Catalog II_ID must contain only a single row whose single column contains
the id value of the highest numbered object id in catalog II_OBJECTS.
The II_ID catalog can be updated by the INGRES super-user, using 'ingres +U'."
/*
**	oosave.qc
*/
E_OO003F_err_writing_db	
"Error writing changes to database.\n
An error occurred writing changes to the database and changes have not 
been saved.  This could have been
caused by a deadlock or other concurrency problem, or some other
database error.  Try saving the changes again."
E_OO0040_cant_save_inMST
"Could not save changes- in MST.\n
Changes were not saved to the database because a multi statement transaction
is in effect."
E_OO0041_must_give_name
"A name must be given in order to save to the database"

E_OO0042_too_few	"Could not retrieve object from system catalogs.\n
Check to see if a concurrent user has destroyed the object."

E_OO0044_BadRef	"Unexpected attribute reference, %0d.\n
An internal attribute reference was not found.  Please report this problem
to your Site Technical Contact."

/* oolremfr.qsc */
S_OO0050_LRemHelpBanner	"Edit a Long Remark"

/* Class names */
F_OO0009_ApplComp	"Application Component"
F_OO0010_JoinDef	"JoinDef"
F_OO0011_Report		"Report"
F_OO0012_RWRep		"Report-Writer Report"
F_OO0013_RBFRep		"RBF Report"
F_OO0014_Form		"Form"
F_OO0015_QBFName	"QBFName"

E_OO0051_OLD_VERSION	"Incompatible object version.\n
You are trying to edit a graph whose version is older than 6.0, and is
not supported by the current release of Vigraph. Please check with your
Site Technical Contact for information on how to convert this graph to
a version compatible with the current release."
/*%
** Explanation:
**	The user is trying to edit a pre-6.0 encoded vigraph object. From
**	6.5, we no longer attempt to convert old objects to 6.0 format.
** System Status:
**	The program will continue, but the user will not be able to edit 
**	the selected object.
** User Action:
**	Running an earlier version of Vigraph (6.0-6.4) will convert the
**	object to 6.0 format.
*/
