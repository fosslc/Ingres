/*
** Copyright (c)  2005 Ingres Corporation
**
*/
# include <compat.h>
/*
** The code in this file was create from a code example supplied by the
** Apple Developer Website.
** (http://developer.apple.com/samplecode/CryptNoMore/CryptNoMore.html)
** As such the follow has been retained:
**
** IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
** ("Apple") in consideration of your agreement to the following terms, and your
** use, installation, modification or redistribution of this Apple software
** constitutes acceptance of these terms.  If you do not agree with these terms,
** please do not use, install, modify or redistribute this Apple software.
**
** In consideration of your agreement to abide by the following terms, and 
** subject to these terms, Apple grants you a personal, non-exclusive license, 
** under Apple's copyrights in this original Apple software (the "Apple 
** Software"), to use, reproduce, modify and redistribute the Apple Software, 
** with or without modifications, in source and/or binary forms; provided that
** if you redistribute the Apple Software in its entirety and without
** modifications, you must retain this notice and the following text and
** disclaimers in all such redistributions of the Apple Software.  Neither the
** name, trademarks, service marks or logos of Apple Computer, Inc. may be used
** to endorse or promote products derived from the Apple Software without
** specific prior written permission from Apple.  Except as expressly stated in
** this notice, no other rights or licenses, express or implied, are granted by
** Apple herein, including but not limited to any patent rights that may be
** infringed by your derivative works or by other works in which the Apple
** Software may be incorporated.
**
** The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
** WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
** WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
** PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
** COMBINATION WITH YOUR PRODUCTS.
**
** IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION
** AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER
** THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR
** OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
*/

/* Only compile for Mac OSX */
# ifdef OSX

# include <compat.h>
# include <gl.h>
# include <st.h>
# include <gc.h>
# include <clconfig.h>
# include <clsigs.h>
# include <systypes.h>
# include <errno.h>

# include <CoreServices/CoreServices.h>
# include <DirectoryService/DirectoryService.h>

/*
** Name: usrauthosx.c
**
** Description:
**	Module for user authentication under Mac OSX
**	using Dircetory Services. This replaces the crypt() method
**	used in GCusrpwd() which is no longer supported for Mac OSX.
**
**	    usrauthosx() - Entry point for authenication
**	    getdsnodepath() - Get path info for search node
**	    FindUsersAuthInfo() - Get user authenication info from search node
**	    dsDataBufferAppendData() - Put auth info into buffer
**	    AuthenticateWithNode() - Do user authentication
**
** 	The following are wrappers for OS calls to handle special failure
**	cases.
**	    dsDataListAndHeaderDeallocate()
**	    dsFindDirNodesQ()
**	    dsGetRecordListQ()
**	    dsDoDirNodeAuthQ()
**	    DoubleBufferSize()
**	    
**
** History:
**	28-Jun-2005 (hanje04)
**	    Created.
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
*/

/* Defines */
# define eDSBadParam (tDirStatus)FAIL
# define MAX_BUFF_GROW ( 16 * 1024 * 1024 )

/* Globals */
static bool authlocalonly=FALSE;  /* Only allow authentication of local users */
enum { kDefaultDSBufferSize = 1024 };  

/* Prototypes */
tDirStatus
getdsnodepath( tDirReference dirRef, tDataListPtr *dsnodepath);
tDirStatus
FindUsersAuthInfo( tDirReference dirRef, tDirNodeReference nodeRef, 
			const char *username,
			tDataListPtr *pathListToAuthNodePtr );
tDirStatus
dsDataBufferAppendData( tDataBufferPtr  buf, const void *dataPtr,
			size_t dataLen);
tDirStatus
AuthenticateWithNode( tDirReference dirRef, tDataListPtr pathListToAuthNode,
			const char *username, const char *password);
tDirStatus
dsDataListAndHeaderDeallocate( tDirReference inDirReference,
				tDataListPtr inDataList);
void
DoubleBufferSize( tDirStatus *errPtr, tDirNodeReference dirRef,
			tDataBufferPtr *    bufPtrPtr);

tDirStatus
dsFindDirNodesQ( tDirReference inDirReference,
			tDataBufferPtr *inOutDataBufferPtrPtr,
			tDataListPtr inNodeNamePattern,
			tDirPatternMatch inPatternMatchType,
			unsigned long *outDirNodeCount,
			tContextData *inOutContinueData );
tDirStatus
dsGetRecordListQ( tDirNodeReference inDirReference,
			tDirNodeReference inDirNodeReference,
			tDataBufferPtr *inOutDataBufferPtr,
			tDataListPtr inRecordNameList,
			tDirPatternMatch inPatternMatchType,
			tDataListPtr inRecordTypeList,
			tDataListPtr inAttributeTypeList,
			dsBool inAttributeInfoOnly,
			unsigned long *inOutRecordEntryCount,
			tContextData *inOutContinueData );
tDirStatus
dsDoDirNodeAuthQ( tDirNodeReference   inDirReference,
			tDirNodeReference inDirNodeReference,
			tDataNodePtr inDirNodeAuthName,
			dsBool inDirNodeAuthOnlyFlag,
			tDataBufferPtr inAuthStepData,
			tDataBufferPtr *outAuthStepDataResponsePtr,
			tContextData *inOutContinueData );

/* Functions */
/*
** Name: DoubleBufferSize
**
** Description:
**	Handles case where Open Directory returns eDSBufferTooSmall.
**	buffer size will ONLY be doubled if that error as been passed in.
**
** Input:
**	errPtr - Error returned by failing OD call.
**	tDataBufferPtr - Buffer to be doubled.
**	dirRef - Open Directory connection.
**
** Output:
**	tDataBufferPtr - Buffer to be doubled.
**
** History:
**	30-Jun-2005 (hanje04)
**	    Created.
*/
void
DoubleBufferSize(
    tDirStatus *        errPtr, 
    tDirNodeReference   dirRef, 
    tDataBufferPtr *    bufPtrPtr
)
{
    tDirStatus      err;
    tDirStatus      junk;
    tDataBufferPtr  tmpBuf;
    
    if (*errPtr == eDSBufferTooSmall)
    {
	/*
        ** If the buffer size is already bigger than MAX_BUFF_GROW, don't try to 
        ** double it again; something has gone horribly wrong.
	*/
        
        err = eDSNoErr;
        if ( (*bufPtrPtr)->fBufferSize >= MAX_BUFF_GROW )
            err = eDSAllocationFailed;
        
        if (err == eDSNoErr)
	{
            tmpBuf = dsDataBufferAllocate(dirRef, (*bufPtrPtr)->fBufferSize * 2);
            if (tmpBuf == NULL)
                err = eDSAllocationFailed;
            else 
	    {
                err=dsDataBufferDeAllocate(dirRef, *bufPtrPtr);
                if(err == eDSNoErr);
                    *bufPtrPtr = tmpBuf;
            }
        }

        /*
	** If err is eDSNoErr, the buffer expansion was successful 
        ** so we leave *errPtr set to eDSBufferTooSmall.  If err 
        ** is any other value, the expansion failed and we set 
        ** *errPtr to that error.
	*/
        
        if (err != eDSNoErr) {
            *errPtr = err;
        }
    }
}

/*
** Name: dsFindDirNodesQ
**
** Description:
**	Wrapper for system call dsFindDirNodes(). Handles the case where it
**	returns eDSBufferTooSmall by calling DoubleBufferSize and
**	re-calling dsFindDirNodes().
**
** History:
**	30-Jun-2005 (hanje04)
**	    Created.
*/
tDirStatus
dsFindDirNodesQ(
    tDirReference		inDirReference,
    tDataBufferPtr *	inOutDataBufferPtrPtr,
    tDataListPtr		inNodeNamePattern,
    tDirPatternMatch	inPatternMatchType,
    unsigned long		*outDirNodeCount,
    tContextData		*inOutContinueData
)
{
    tDirStatus  err;
    
    do
    {
        do
  	  {
            err = dsFindDirNodes(
                inDirReference, 
                *inOutDataBufferPtrPtr, 
                inNodeNamePattern, 
                inPatternMatchType, 
                outDirNodeCount, 
                inOutContinueData
            );
            DoubleBufferSize(&err, inDirReference, inOutDataBufferPtrPtr);
        } while (err == eDSBufferTooSmall);
    } while ( (err == eDSNoErr) && (*outDirNodeCount == 0) &&
					 (*inOutContinueData != NULL) );
    
    return err;
}

/*
** Name: dsGetRecordListQ
**
** Description:
**	Wrapper for system call dsGetRecordList(). Handles the case where it
**	returns eDSBufferTooSmall by calling DoubleBufferSize and
**	re-calling dsGetRecordList().
**
** History:
**	30-Jun-2005 (hanje04)
**	    Created.
*/
tDirStatus
dsGetRecordListQ(
    tDirNodeReference       inDirReference,
    tDirNodeReference		inDirNodeReference,
    tDataBufferPtr *		inOutDataBufferPtr,
    tDataListPtr			inRecordNameList,
    tDirPatternMatch		inPatternMatchType,
    tDataListPtr			inRecordTypeList,
    tDataListPtr			inAttributeTypeList,
    dsBool					inAttributeInfoOnly,
    unsigned long			*inOutRecordEntryCount,
    tContextData			*inOutContinueData
)
{
    tDirStatus      err;
    unsigned long   originalRecordCount;
    
    originalRecordCount = *inOutRecordEntryCount;

    do
    {
        *inOutRecordEntryCount = originalRecordCount;
        do
	{
            err = dsGetRecordList(
                inDirNodeReference,
                *inOutDataBufferPtr,
                inRecordNameList,
                inPatternMatchType,
                inRecordTypeList,
                inAttributeTypeList,
                inAttributeInfoOnly,
                inOutRecordEntryCount,
                inOutContinueData
            );
            DoubleBufferSize(&err, inDirReference, inOutDataBufferPtr);
        } while (err == eDSBufferTooSmall);
    } while ( (err == eDSNoErr) && (*inOutRecordEntryCount == 0) &&
				(*inOutContinueData != NULL) );
    
    return err;
}

/*
** Name: dsDoDirNodeAuthQ
**
** Description:
**	Wrapper for system call dsDoDirNodeAuth(). Handles the case where it
**	returns eDSBufferTooSmall by calling DoubleBufferSize and
**	re-calling dsDoDirNodeAuth().
**
** History:
**	30-Jun-2005 (hanje04)
**	    Created.
*/
tDirStatus
dsDoDirNodeAuthQ(
    tDirNodeReference   inDirReference,
    tDirNodeReference	inDirNodeReference,
    tDataNodePtr 		inDirNodeAuthName,
    dsBool				inDirNodeAuthOnlyFlag,
    tDataBufferPtr		inAuthStepData,
    tDataBufferPtr *	outAuthStepDataResponsePtr,
    tContextData		*inOutContinueData
)
{
    tDirStatus  err;
    
    do
    {
        err = dsDoDirNodeAuth(
            inDirNodeReference, 
            inDirNodeAuthName, 
            inDirNodeAuthOnlyFlag, 
            inAuthStepData, 
            *outAuthStepDataResponsePtr, 
            inOutContinueData
        );
        DoubleBufferSize(&err, inDirReference, outAuthStepDataResponsePtr);
    } while (err == eDSBufferTooSmall);
    
    return err;
}
/*
** Name: getdsnodepath
**
** Description:
**	Returns the path to the Open Directory search node.
**
** Input:
**	dirRef - connection to Open Directory.
**
** Output:
**	dsnodepath - PTR to list that contains the search 
**				node's path components.
**
** History:
**	30-Jun-2005 (hanje04)
**	    Created.
*/

tDirStatus
getdsnodepath(tDirReference dirRef, tDataListPtr *dsnodepath)
{               
    tDirStatus          err;
    tDirStatus          junk;
    tDataBufferPtr      buf;
    tDirPatternMatch    patternToFind;
    unsigned long       nodeCount;
    tContextData        context;
                
    if (!( (dirRef != NULL) || ( dsnodepath != NULL) || (*dsnodepath == NULL) ) )
	return( eDSBadParam );

    if (authlocalonly)
        patternToFind = eDSLocalNodeNames;
    else
        patternToFind = eDSAuthenticationSearchNodeName;
   
    context = NULL;

    /* 
    ** Allocate a buffer for the node find results.  We'll grow
    ** this buffer if it proves to be to small.
    */

    buf = dsDataBufferAllocate(dirRef, kDefaultDSBufferSize);
    err = eDSNoErr;
    if (buf == NULL)
        err = eDSAllocationFailed;
   
    /*
    ** Find the node.  Note that this is a degenerate case because
    ** we're only looking for a single node, the search node, so
    ** we don't need to loop calling dsFindDirNodes, which is the
    ** standard way of using dsFindDirNodes.
    */

    if (err == eDSNoErr)
    {
        err = dsFindDirNodesQ(
            dirRef,
            &buf,                               /* place results here */
            NULL,                               /* no pattern, rather... */
            patternToFind,                      /* ... hardwired search type */
            &nodeCount,
            &context
        );
    }

    /* If we didn't find any nodes, that's bad. */

    if ( (err == eDSNoErr) && (nodeCount < 1) )
        err = eDSNodeNotFound;

    /*
    ** Grab the first node from the buffer.  Note that the inDirNodeIndex
    ** parameter to dsGetDirNodeName is one-based, so we pass in the constant
    ** 1.
    **
    ** Also, if we found more than one, that's unusual, but not enough to
    ** cause us to error.
    */

    if (err == eDSNoErr)
        err = dsGetDirNodeName(dirRef, buf, 1, dsnodepath);

    /* Clean up. */

    if (context != NULL)
         dsReleaseContinueData(dirRef, context);
    if (buf != NULL)
        dsDataBufferDeAllocate(dirRef, buf);

    return err;
}

/*
** Name: dsDataListAndHeaderDeallocate
**
** Description:
**	dsDataListDeallocate deallocates the list contents but /not/ the
**	list header (because the list header could be allocated on the
**	stack).  This routine is a wrapper that deallocates both.  It's
**	the logical opposite of the various list allocation routines
**	(for example, dsBuildFromPath).
**
** Input:
**	inDirReference - data list to be deallocated
**	inDataList - list header
** Output:
**	None.
**
** History:
**	30-Jun-2005 (hanje04)
**	    Created.
*/
tDirStatus
dsDataListAndHeaderDeallocate(
    tDirReference   inDirReference,
    tDataListPtr    inDataList
)
{
    tDirStatus  err;

    if (!(inDirReference != NULL) || (inDataList != NULL) )
	return(eDSBadParam);

    err = dsDataListDeallocate(inDirReference, inDataList);
    if (err == eDSNoErr) {
        free(inDataList);
    }

    return err;
}

/*
** Name: FindUsersAuthInfo
**
** Description:
**	Finds the authentication node path for a given user.
**
** Input:
**	dirRef - connection to Open Directory.
**	nodeRef - node to use to do the searching.
**	username - user whose info we're looking for.
** Output:
**	pathListToAuthNodePtr - PTR to list that contains the authentication 
**				node's path components.
**
** History:
**	30-Jun-2005 (hanje04)
**	    Created.
*/

tDirStatus
FindUsersAuthInfo(
    tDirReference       dirRef, 
    tDirNodeReference   nodeRef, 
    const char *        username, 
    tDataListPtr *      pathListToAuthNodePtr
)
{
    tDirStatus          err;
    tDirStatus          junk;
    tDataBufferPtr      buf;
    tDataListPtr        recordType;
    tDataListPtr        recordName;
    tDataListPtr        requestedAttributes;
    unsigned long       recordCount;
    tAttributeListRef   foundRecAttrList;
    tContextData        context;
    tRecordEntryPtr     foundRecEntry;
    tDataListPtr        pathListToAuthNode;

    if (!( (dirRef != NULL) || (nodeRef != NULL) || (username != NULL) ||
	( pathListToAuthNodePtr != NULL) || (*pathListToAuthNodePtr == NULL) ) )
	return(eDSBadParam);

    recordType = NULL;
    recordName = NULL;
    requestedAttributes = NULL;
    foundRecAttrList = NULL;
    context = NULL;
    foundRecEntry = NULL;
    pathListToAuthNode = NULL;
    
    /*
    ** Allocate a buffer for the record results.  We'll grow this 
    ** buffer if it proves to be too small.
    */
    
    err = eDSNoErr;
    buf = dsDataBufferAllocate(dirRef, kDefaultDSBufferSize);
    if (buf == NULL)
        err = eDSAllocationFailed;

    /*
    ** Create the information needed for the search.  We're searching for 
    ** a record of type kDSStdRecordTypeUsers whose name is "username".  
    ** We want to get back the kDSNAttrMetaNodeLocation and kDSNAttrRecordName 
    ** attributes.
    */
    
    if (err == eDSNoErr)
    {
        recordType = dsBuildListFromStrings(dirRef, kDSStdRecordTypeUsers, NULL);
        recordName = dsBuildListFromStrings(dirRef, username, NULL);
        requestedAttributes = dsBuildListFromStrings(dirRef,
		kDSNAttrMetaNodeLocation, kDSNAttrRecordName, NULL);

        if ( (recordType == NULL) || (recordName == NULL) ||
		(requestedAttributes == NULL) )
            err = eDSAllocationFailed;
    }
    
    /* Search for a matching record. */
    
    if (err == eDSNoErr) {
        recordCount = 1;            /* we only want one match (the first) */
        
        err = dsGetRecordListQ(
            dirRef,
            nodeRef,
            &buf,
            recordName,
            eDSExact,
            recordType,
            requestedAttributes,
            false,
            &recordCount,
            &context
        );
    }

    if ( (err == eDSNoErr) && (recordCount < 1) )
        err = eDSRecordNotFound;
    
    /*
    ** Get the first record from the search.  Then enumerate the attributes for 
    ** that record.  For each attribute, extract the first value (remember that 
    ** attributes can by multi-value).  Then see if the attribute is one that 
    ** we care about.  If it is, remember the value for later processing.
    */  
    if (err == eDSNoErr)
        err = dsGetRecordEntry(nodeRef, buf, 1, &foundRecAttrList, 
				&foundRecEntry);

    if (err == eDSNoErr)
    {
        unsigned long attrIndex;
        
        /* Iterate over the attributes. */
        
        for (attrIndex = 1; 
		attrIndex <= foundRecEntry->fRecordAttributeCount;
		attrIndex++)
	{
            tAttributeValueListRef  thisValue;
            tAttributeEntryPtr      thisAttrEntry;
            tAttributeValueEntryPtr thisValueEntry;
            const char *            thisAttrName;
            
            thisValue = NULL;
            thisAttrEntry = NULL;
            thisValueEntry = NULL;
            
            /* Get the information for this attribute. */
            
            err = dsGetAttributeEntry(nodeRef, buf, foundRecAttrList, attrIndex,
					 &thisValue, &thisAttrEntry);

            if (err == eDSNoErr)
	    {
                thisAttrName = thisAttrEntry->fAttributeSignature.fBufferData;

                /* We only care about attributes that have values. */
                
                if (thisAttrEntry->fAttributeValueCount > 0)
		{
                    /*
		    ** Get the first value for this attribute.  This is common
		    ** code for the two potential attribute values listed below,
		    ** so we do it first.
		    */

                    err = dsGetAttributeValue(nodeRef, buf, 1, thisValue,
						 &thisValueEntry);
                    
                    if (err == eDSNoErr)
		    {
                        const char *    thisValueDataPtr;
                        unsigned long   thisValueDataLen;

                        thisValueDataPtr = thisValueEntry->fAttributeValueData.fBufferData;
                        thisValueDataLen = thisValueEntry->fAttributeValueData.fBufferLength;

			/*
                        ** Handle each of the two attributes we care about;
			** ignore any others.
	 		*/
                        
                        if ( STcompare(thisAttrName, 
					kDSNAttrMetaNodeLocation) == 0 )
			{
                            
			    /*
                            ** This is the kDSNAttrMetaNodeLocation attribute,
			    ** which contains a path to the node used for
			    ** authenticating this record; convert its value
			    ** into a path list.  
			    */

                            pathListToAuthNode = dsBuildFromPath(
                                dirRef, 
                                thisValueDataPtr, 
                                "/"
                            );
                            if (pathListToAuthNode == NULL)
                                err = eDSAllocationFailed;

			}
		    }
		    else
		    {
			fprintf(stderr, "FindUsersAuthInfo: Unexpected no-value attribute '%s'.", thisAttrName);
		    }
		}

            /* Clean up. */
            
		if (thisValueEntry != NULL)
		    dsDeallocAttributeValueEntry(dirRef, thisValueEntry);
		if (thisValue != NULL)
		    dsCloseAttributeValueList(thisValue);
		if (thisAttrEntry != NULL)
		    dsDeallocAttributeEntry(dirRef, thisAttrEntry);
            
		if (err != eDSNoErr)
		    break;
	    }
	} /* Check attr loop */
    }
    
    /* Copy results out to caller. */
    
    if (err == eDSNoErr)
    {
        if (pathListToAuthNode != NULL) 
	{
            /* Copy out results. */
            
            *pathListToAuthNodePtr = pathListToAuthNode;
            
            /* NULL out locals so that we don't dispose them. */
            
            pathListToAuthNode = NULL;
        }
	else
            err = eDSAttributeNotFound;
    }

    /* Clean up. */

    if (pathListToAuthNode != NULL)
        dsDataListAndHeaderDeallocate(dirRef, pathListToAuthNode);
    if (foundRecAttrList != NULL)
        dsCloseAttributeList(foundRecAttrList);
    if (context != NULL)
        dsReleaseContinueData(dirRef, context);
    if (foundRecAttrList != NULL)
        dsDeallocRecordEntry(dirRef, foundRecEntry);
    if (requestedAttributes != NULL)
        dsDataListAndHeaderDeallocate(dirRef, requestedAttributes);
    if (recordName != NULL)
        dsDataListAndHeaderDeallocate(dirRef, recordName);
    if (recordType != NULL)
        dsDataListAndHeaderDeallocate(dirRef, recordType);
    if (buf != NULL)
	dsDataBufferDeAllocate(dirRef, buf);

    return err;
}

/*
** Name: dsDataBufferAppendData
**
** Description:
** 	Appends a value to a data buffer.  dataPtr and dataLen describe
**	the value to append.  buf is the data buffer to which it's added.
**
** Input:
**	dataPtr - Data to be appended
**	dataLen - Length of data
** Output:
**	buf - Data buffer to which it's appended.
**
** History:
**	30-Jun-2005 (hanje04)
**	    Created.
*/

tDirStatus
dsDataBufferAppendData(
    tDataBufferPtr  buf,
    const void *    dataPtr,
    size_t          dataLen
)
{
    tDirStatus      err;

    if (!( (buf != NULL) || (dataPtr != NULL) || 
		(buf->fBufferLength <= buf->fBufferSize) ) )
	return( eDSBadParam ) ;
   
    if ( (buf->fBufferLength + dataLen) > buf->fBufferSize )
        err = eDSBufferTooSmall;
    else
    {
        memcpy(&buf->fBufferData[buf->fBufferLength], dataPtr, dataLen);
        buf->fBufferLength += dataLen;
        err = eDSNoErr;
    }
   
    return err;
}

/*
** Name: AuthenticateWithNode
**
** Description:
**	Authenticate a user with their authentication node.
**
** Input:
**	dirRef - connection to Open Directory.
**	pathListToAuthNode - data list that containse path of the 
**			     authentication node for the specified user. 
**	username - user to authenticate.
**	password - password for username.
** Output:
**	None
**
** History:
**	30-Jun-2005 (hanje04)
**	    Created.
*/

tDirStatus
AuthenticateWithNode(
    tDirReference       dirRef, 
    tDataListPtr        pathListToAuthNode,
    const char 		*username, 
    const char 		*password
)
{
    tDirStatus          err;
    tDirStatus          junk;
    size_t              userNameLen;
    size_t              passwordLen;
    tDirNodeReference   authNodeRef;
    tDataNodePtr        authMethod;
    tDataBufferPtr      authOutBuf;
    tDataBufferPtr      authInBuf;
    unsigned long       length;
    
    if (!( (dirRef != NULL) || (pathListToAuthNode != NULL) ||
		 (username != NULL) || (password != NULL) ) )
	return(eDSBadParam) ;
    
    authNodeRef = NULL;
    authMethod = NULL;
    authOutBuf = NULL;
    authInBuf = NULL;

    userNameLen = STlen(username);
    passwordLen = STlen(password);

    /* Open the authentication node. */
    
    err = dsOpenDirNode(dirRef, pathListToAuthNode, &authNodeRef);
    
    /*
    ** Create the input parameters to dsDoDirNodeAuth and then call it.
    ** The most complex input parameter to dsDoDirNodeAuth is authentication
    ** data itself, held in authInBuf.  This holds the following items:
    **
    ** 4 byte length of user name (includes trailing null)
    ** user name, including trailing null
    ** 4 byte length of password (includes trailing null)
    ** password, including trailing null
    */
    
    if (err == eDSNoErr)
    {
        authMethod = dsDataNodeAllocateString(dirRef,
				kDSStdAuthNodeNativeClearTextOK);
        if (authMethod == NULL)
            err = eDSAllocationFailed;
    }
    if (err == eDSNoErr)
    {
	/*
        ** Allocate some arbitrary amount of space for the authOutBuf.  This 
        ** buffer comes back containing a credential generated by the 
        ** authentication (apparently a kDS1AttrAuthCredential).  However, 
        ** we never need this information, so we basically just create the 
        ** buffer, pass it in to dsDoDirNodeAuth, and then throw it away. 
        ** Unfortunately dsDoDirNodeAuth won't let us pass in NULL.
	*/
        
        authOutBuf = dsDataBufferAllocate(dirRef, kDefaultDSBufferSize);
        if (authOutBuf == NULL)
            err = eDSAllocationFailed;
    }
    if (err == eDSNoErr)
    {
        authInBuf = dsDataBufferAllocate(dirRef,
			sizeof(length) + userNameLen + 1 +
			sizeof(length) + passwordLen + 1 );

        if (authInBuf == NULL)
            err = eDSAllocationFailed;
    }
    /* Move the data into the buffer. */
    if (err == eDSNoErr)
    {     
        length = userNameLen + 1;   /* + 1 to include trailing null */
        err = dsDataBufferAppendData(authInBuf, &length, sizeof(length));
    }
    if (err == eDSNoErr)
    {
        err = dsDataBufferAppendData(authInBuf, username, 
						userNameLen + 1);
    }
    if (err == eDSNoErr)
    {
        length = passwordLen + 1;   /* + 1 to include trailing null */
        err = dsDataBufferAppendData(authInBuf, &length, sizeof(length));
    }
    if (err == eDSNoErr)
        junk = dsDataBufferAppendData(authInBuf, password, passwordLen + 1);

        /* Call dsDoDirNodeAuth to do the authentication. */
    if (err == eDSNoErr)
        err = dsDoDirNodeAuthQ(dirRef, authNodeRef, authMethod,
				true, authInBuf, &authOutBuf, NULL);

    /* Clean up. */

    if (authInBuf != NULL)
         dsDataBufferDeAllocate(dirRef, authInBuf);
    if (authOutBuf != NULL)
         dsDataBufferDeAllocate(dirRef, authOutBuf);
    if (authMethod != NULL)
         dsDataNodeDeAllocate(dirRef, authMethod);
    if (authNodeRef != NULL)
         dsCloseDirNode(authNodeRef);
    
    return err;
}

/*
** Name: usrauthosx
**
** Description:
**	Verify username and password on Mac OSX using Open Directory
**
** Input:
**	username - user to authenicate
**	password - password for username
** Output:
**	sys_err - errors (if any)
**
** History:
**	30-Jun-2005 (hanje04)
**	    Created.
*/
STATUS
usrauthosx(char *username, char *password, CL_ERR_DESC *sys_err)
{   
    STATUS		    status;
    tDirStatus              err;
    tDirStatus              junk;
    tDirReference           dirRef;
    tDataListPtr            pathListToSearchNode;
    tDirNodeReference       searchNodeRef;
    tDataListPtr            pathListToAuthNode;

    if (!( (username != NULL) || (password != NULL) ) )
	return(FAIL) ;

    dirRef = NULL;
    pathListToSearchNode = NULL;
    searchNodeRef = NULL;
    pathListToAuthNode = NULL;

    /* Connect to Open Directory. */

    err = dsOpenDirService(&dirRef);

    /* Open the search node. */

    if (err == eDSNoErr)
        err = getdsnodepath(dirRef, &pathListToSearchNode);

    if (err == eDSNoErr)
        err = dsOpenDirNode(dirRef, pathListToSearchNode, &searchNodeRef);

    /*
    ** Search for the user's record and extract the user's authentication
    ** node and authentication user name.
    */

    if (err == eDSNoErr)
        err = FindUsersAuthInfo(dirRef, searchNodeRef, username,
			 &pathListToAuthNode);

    /* Open the authentication node and do the authentication. */

    if (err == eDSNoErr) 
	err = AuthenticateWithNode(dirRef, pathListToAuthNode,
						username, password);

    /* Clean up. */

    if (pathListToAuthNode != NULL)
         dsDataListAndHeaderDeallocate(dirRef, pathListToAuthNode);
    if (searchNodeRef != NULL)
         dsCloseDirNode(searchNodeRef);
    if (pathListToSearchNode != NULL)
         dsDataListAndHeaderDeallocate(dirRef, pathListToSearchNode);
    if (dirRef != NULL)
	 dsCloseDirService(dirRef);

/* Set status based on Open Directory err value */
    switch (err)
    {
	case eDSNoErr:
                status = OK;
                break;
	case eDSAuthFailed:
	case eDSAuthNewPasswordRequired:
	case eDSAuthPasswordExpired:
                status = GC_USRPWD_FAIL;
                break;
	case eDSAuthAccountInactive:
	case eDSAuthAccountExpired:
	case eDSAuthAccountDisabled:
	case eDSRecordNotFound:
		status = GC_LSN_FAIL;
                break;
	default:
                status = GC_LOGIN_FAIL;
    }

    return(status);
}

# endif /* Mac OSX only */
