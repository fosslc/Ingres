/*
** Copyright (c) 2009 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.util;

/*
** Name: ByteBuffer.java
**
** Description:
**	Defines class which provides a segmented byte buffer.
**
**  Classes:
**
**	ByteBuffer		Segmented byte buffer.
**	Node			Buffer segment.
**	ByteBuffIS		InputStream wrapper for byte buffer.
**	ByteBuffOS		OutputStream wrapper for byte buffer.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

import	java.io.InputStream;
import	java.io.OutputStream;
import	java.io.IOException;


/*
** Name: ByteBuffer
**
** Description:
**	Class providing a segmented byte buffer.
**
**	The byte buffer is intended to represent large, variable
**	length binary arrays which primarily tend to grow.  Data
**	inserts and deletions are not permitted but data over-
**	writes, extensions and truncation are allowed.
**
**	A segmented byte buffer grows in fixed size blocks.  The 
**	segment size may be configured when the byte buffer is 
**	created and defaults to 8192 bytes.  Each segment has 
**	a buffer offset (node.offset), segment data (node.data), 
**	max segment length (node.data.length), and current data
**	length (node.length).
**
**    Linked List
**
**	Segment nodes are stored in a linked list (node.next)
**	to facilitate linear scans.  A reference to the last 
**	node is also maintained which allows quick access to 
**	the last segment to calculate buffer length and append 
**	data (see Root Node below).
**
**    Binary Tree
**
**	Segment nodes are also arranged in a binary tree to 
**	facilitate quick access to a specific buffer position.  
**	The level of a node (node.level) determines the depth
**	of the node in the tree.  Leaf nodes have no child
**	subtrees and are at the lowest level of the tree.
**	Internal (non-leaf) nodes comprise the other levels 
**	of the tree.  Segments which precede an internal node 
**	are in the left child subtree (node.left).  Segments 
**	which follow an internal node are in the right child 
**	subtree (node.right).  Trees grow upwards (and to the 
**	right) from the leaf nodes and are kept semi-balanced.
**
**    Root Node
**
**	At a minimum, a buffer consists of a root node with
**	offset 0 and a max segment length of 0.  By itself,
**	the root node holds no data and represents an empty 
**	buffer.  When the buffer is not empty, the root node 
**	points to the start of the linked list (root.next) 
**	and the actual root of the binary tree (root.right).  
**	Since no data precedes the root node, the root's 
**	left subtree will always be empty.  The root's left 
**	subtree reference is hijacked (since it would other-
**	wise always be NULL) to point at the last node in 
**	the linked list (root.left).  The level of the root
**	node is chosen such that it isn't a leaf node and
**	won't appear to be the direct parent (one level up)
**	of any other node.
**	
**
**    Tree Characteristics
**
**	If an internal node's children are leaf nodes, 
**	then the node immediatly follows the left child 
**	and immediately precedes the right child.  From 
**	the child viewpoint, if a leaf node is the left 
**	child of it's parent, then it is immediately 
**	followed by it's parent.  Likewise, if a leaf 
**	node is a right child, then it is immediately 
**	preceded by it's parent.
**
**	In general, the node which immediately precedes an 
**	internal node is the right most leaf node of it's 
**	left child subtree.  The node which immediately 
**	follows an internal node is the left most leaf node 
**	of it's right child subtree.
**
**	If a leaf node is the left child of it's parent,
**	then it is immediately preceded by the first node
**	ascending the tree where a right branch is taken.  
**	Likewise, if a leaf node is a right child, then it 
**	is immediately followed by the first node ascending
**	the tree where a left branch is taken.
**
**	Leaf nodes are immediately preceded and followed
**	by internal nodes, and vica-versa.
**
**	A (sub)tree is either full, partial, or empty.  A
**	full tree is either a single leaf node or a tree
**	whose root node has full subtrees for both the left 
**	and right children and is one level higher than both
**	child subtrees.  Since trees grow (and shrink) to 
**	the right, the root node of a partial tree has a 
**	full left subtree, is one level higher than the left
**	subtree, and has an empty or partial tree for the 
**	right child.  An empty tree has no nodes.
**
**	Trees grow from the leaf nodes up.  When a tree
**	becomes full, it is extended by adding a new root
**	node with the previous root as the left child and
**	an empty right subtree.  The right subtree then
**	grows from the leaf level until it is also full.
**
**	When the first leaf node is added to an empty
**	right subtree, a level discontinuity may exist 
**	between the leaf node and the root (the root is 
**	greater than one level higher than leaf).  This 
**	discontinuity remains until the subtree grows to 
**	the correct depth.  A partial tree either contains 
**	one or more discontinuities or there is some right 
**	subtree which is empty.
**
**    Extending the Buffer
**
**	Extending the buffer entails adding another node to
**	the end of the linked list and inserting that node
**	into the binary tree.  When the last node is a non-
**	leaf node, the new node will be a leaf node and is 
**	added as the right child of the last node (possibly
**	creating a discontinuity).  When the last node is a 
**	leaf node, the new node will be a non-leaf node and 
**	the trick is finding the proper place to insert the 
**	internal node.
**	
**	When the last node is a leaf node, it is the right-
**	most node of some full (sub)tree.  A full tree is 
**	extended by adding a new root node which has the 
**	original full tree as it's left child.  If the tree 
**	is not full, there exists some right-most subtree 
**	which is full (possibly just the last (leaf) node).  
**	The full subtree containing the last segment must 
**	be identified and the root of the subtree replaced 
**	by an internal node with the full subtree as it's 
**	left child.
**
**	An internal node can only be added at a discontinuity.
**	The addition of a new node at a discontinuity reduces
**	and may eliminate the discontinuity.  A discontinuity 
**	always exists between the root buffer node and the 
**	root of the binary tree.  Additional discontinuities 
**	may exist.  The lowest discontinuity in the tree lies 
**	immediately above the full subtree containing the 
**	last segment.
**
**	The point at which an internal node is added is found
**	by scanning down the tree from the root searching for
**	discontinuities in the tree.  The new internal node
**	is added at the lowest discontinuity found. The new
**	node replaces the root of the full subtree as the
**	right child of the parent node and becomes the
**	parent node of the subtree with the subtree as it's
**	left child.
**
**    Truncating the Buffer
**
**	The degenerate case of truncating the buffer to
**	zero length is easily accomplished by initializing
**	the root node to it's initial empty state.
**
**	Truncating the buffer in general entails dropping 
**	segments from the linked list which follow the 
**	target segment and pruning the binary tree to 
**	remove nodes which follow the target segment.  
**	Nodes in the right subtree of the target node 
**	follow the target segment and are easily pruned 
**	by dropping the entire right subtree.  Non-leaf 
**	nodes above the target node may or may-not follow 
**	the target segment.  The trick is identifying and 
**	pruning the nodes above the target node which are 
**	logically to the right of the target node.
**
**	Nodes above the target node can be identified by
**	scanning down the tree from the root.  A right
**	branch is taken when the target follows a node.
**	The node should therefore not be pruned (the scan
**	simply descends).  A left branch is taken when the
**	target node preceeds a node.  The node must there-
**	fore be pruned.  The scan ends when the target
**	node is found or the branch to be taken is empty.
**	The latter case indicates the target segment did
**	not exist in the tree, that the pruning which has
**	occured is sufficient, and the node becomes the
**	last node in the buffer.
**
**	When pruning a node due to a left branch, it is
**	advantageous to note that segments in the right 
**	child subtree of the node to be pruned follow
**	the node and should also be pruned.  The node 
**	can therefore be pruned by replacing it with it's 
**	left child.  In essence, a left branch does not 
**	descend but creates (or extends) a discontinuity 
**	in the tree at that point and then continues at 
**	that point with the child of the previous node 
**	as the current node.  Since only right branchs 
**	descend, the node removed was the right child 
**	of it's parent.  From the perspective of the
**	parent node: the left child of the current
**	right child becomes the new right child.
**
**  Constants:
**
**	DEFAULT_SIZE		Default segment size.
**
**  Public Methods:
**
**	length			Length of data.
**	read			Read data.
**	getIS			Retrieve stream to read from buffer.
**	find			Search for match.
**	truncate		Truncate data.
**	append			Append  data.
**	write			Write data.
**	getOS			Retrieve stream to write to buffer.
**
**  Private Data:
**
**	segmentSize		Size of data segments.
**	root			Root node.
**
**  Private Methods:
**
**	locate			Find segment holding position.
**	prune			Prune segments following position.
**	extend			Extend buffer with new segment.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
**	26-Feb-07 (gordy)
**	    Added read method to transfer entire value to stream.
*/

public class
ByteBuffer
{

    /*
    ** Constants
    */
    public static final int	DEFAULT_SIZE = 8192;

    /*
    ** Private data
    */
    private Node		root = new Node();
    private int			segmentSize = DEFAULT_SIZE;


/*
** Name: ByteBuffer
**
** Description:
**	Class constructor.  Constructs an empty byte buffer
**	with default segment size.
**
** Input:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public
ByteBuffer()
{
    /*
    ** As constructed, the root node represents an empty buffer.  
    ** To facilitate appending data, a quick reference to the
    ** last segment is needed.  Since the offset of the root node 
    ** guarantees it will always be the first node, the root
    ** node will never have a left sub-tree.  The left sub-tree
    ** reference of the root node is therefore used to point
    ** to the current last segment.
    */
    root.left = root;

} // ByteBuffer


/*
** Name: ByteBuffer
**
** Description:
**	Class constructor.  Constructs an empty byte buffer
**	with requested segment size.
**
** Input:
**	size	Segment size.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public
ByteBuffer( int size )
{
    this();
    if ( size > 0 )  segmentSize = size;
} // ByteBuffer


/*
** Name: ByteBuffer
**
** Description:
**	Class constructor.  Constructs a byte buffer and 
**	fills it with a portion of a byte array.
**
** Input:
**	data	Byte array.
**	offset	Starting position.
**	length	Number of bytes.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public
ByteBuffer( byte data[], int offset, int length )
{
    this();
    append( data, offset, length );
} // ByteBuffer


/*
** Name: ByteBuffer
**
** Description:
**	Class constructor.  Constructs a byte buffer with 
**	requested segment size and fills it with a portion
**	of a byte array.
**
** Input:
**	size	Segment size.
**	data	Byte array.
**	offset	Starting position.
**	length	Number of bytes.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public
ByteBuffer( int size, byte data[], int offset, int length )
{
    this( size );
    append( data, offset, length );
} // ByteBuffer


/*
** Name: ByteBuffer
**
** Description:
**	Class constructor.  Constructs a byte buffer and 
**	fills it data read from a binary stream.  The
**	binary stream is closed.
**
** Input:
**	stream	Binary stream.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public
ByteBuffer( InputStream stream )
{
    this();
    append( stream );
} // ByteBuffer


/*
** Name: ByteBuffer
**
** Description:
**	Class constructor.  Constructs a byte buffer with 
**	requested segment size and fills it data read from
**	a binary stream.  The binary stream is closed.
**
** Input:
**	size	Segment size.
**	stream	Binary stream.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public
ByteBuffer( int size, InputStream stream )
{
    this( size );
    append( stream );
} // ByteBuffer

/*
** Name: free
**
** Description:
**      Release resources.
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**      void.
**
** History:
**      18-Jun-09 (rajus01)
**          Created.
*/

public void
free()
{
    /*
    ** Truncating to 0 length will release internal
    ** data for garbage collection.
    */
    truncate( 0 );
} // free


/*
** Name: length
**
** Description:
**	Returns the current length of the buffered data.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	long	Data length.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public long
length()
{
    /*
    ** Since we have a quick reference to the last
    ** segment, we can easily calculate the current
    ** length rather than maintaining it separately.
    ** The length is simply the starting offset of
    ** the last segment plus it's current length.
    ** The last segment may be empty, but preceding
    ** segments are always full and the segment
    ** offset is correct.
    */
    return( root.left.offset + root.left.length );
} // length


/*
** Name: read
**
** Description:
**	Read bytes from buffer into an array and return
**	the actual number of bytes read, which may be
**	less than requested due to source and destination 
**	space restrictions.  An invalid parameter results 
**	in 0 bytes being read.
**
** Input:
**	start	Starting position in buffer.
**	offset	Offset in output array.
**	length	Number of bytes.
**
** Output:
**	data	Output array.
**
** Returns:
**	long	Number of bytes.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
**	26-Feb-07 (gordy)
**	    Changed return signature to long.
*/

public long
read( long pos, byte data[], int offset, int length )
{
    long total = 0;

    /*
    ** Validate parameters.  Rather than throwing an exception,
    ** simply return an indication that 0 bytes have been read.
    */
    if ( 
	 data == null  ||  pos < 0  ||  length < 1  ||
	 offset < 0  ||  offset >= data.length
       )
	return( 0 );

    /*
    ** Ensure there is no data overrun on output array.
    */
    length = Math.min( length, data.length - offset );

    /*
    ** Target data may be split across multiple segments.
    ** Retrieve segment containing the starting position
    ** and then scan linearly copying the data.
    **
    ** Note: position within the segment should be valid
    ** due to validation unless there is a structural
    ** problem.  Position is verified just to be safe.
    */
    for( 
	 Node node = locate( pos );  
	 length > 0  &&  node != null  &&  pos >= node.offset; 
	 node = node.next 
       )
    {
	/*
	** Restrict source bytes to data in current segment.
	*/
	int off = (int)Math.min( pos - node.offset, (long)node.length );
	int len = Math.min( length, node.length - off );

	if ( len > 0 )
	{
	    System.arraycopy( node.data, off, data, offset, len );
	    pos += len;
	    offset += len;
	    total += len;
	    length -= len;
	}
    }

    return( total );
} // read


/*
** Name: read
**
** Description:
**	Read bytes from buffer into a binary stream and 
**	return the actual number of bytes read, which may 
**	be less than requested due to source buffer space 
**	restrictions.  An invalid parameter results in 0 
**	bytes being read.  The binary stream is not closed.
**
** Input:
**	start	Starting position.
**	stream	Binary stream.
**	length	Number of bytes.
**
** Output:
**	None.
**
** Returns:
**	long	Number of bytes.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
**	26-Feb-07 (gordy)
**	    Changed signature to long values to properly 
**	    handle all possible conditions.
*/

public long 
read( long pos, OutputStream stream, long length )
{
    long total = 0;

    /*
    ** Validate parameters.  Rather than throwing an exception,
    ** simply return an indication that 0 bytes have been read.
    */
    if ( stream == null  ||  pos < 0  ||  length < 1 )
	return( 0 );

    /*
    ** Target data may be split across multiple segments.
    ** Retrieve segment containing the starting position
    ** and then scan linearly copying the data.
    **
    ** Note: position within the segment should be valid
    ** due to validation unless there is a structural
    ** problem.  Position is verified just to be safe.
    */
    for( 
	 Node node = locate( pos );  
	 length > 0  &&  node != null  &&  pos >= node.offset; 
	 node = node.next 
       )
    {
	/*
	** Restrict source bytes to data in current segment.
	*/
	int off = (int)Math.min( pos - node.offset, (long)node.length );
	int len = (int)Math.min( length, (long)(node.length - off) );

	if ( len > 0 )
	{
	    try { stream.write( node.data, off, len ); }
	    catch( IOException ex ) { break; }

	    pos += len;
	    total += len;
	    length -= len;
	}
    }

    return( total );
} // read


/*
** Name: read
**
** Description:
**	Read bytes from buffer into a binary stream and return 
**	the actual number of bytes read.  An invalid parameter 
**	results in 0 bytes being read.  The binary stream is 
**	not closed.
**
** Input:
**	stream	Binary stream.
**
** Output:
**	None.
**
** Returns:
**	long	Number of bytes.
**
** History:
**	26-Feb-07 (gordy)
**	    Created.
*/

public long
read( OutputStream stream )
{
    return( read( 0, stream, length() ) );
} // read


/*
** Name: getIS
**
** Description:
**	Returns an InputStream capable of reading the byte buffer.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	InputStream	Stream for reading.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public InputStream
getIS()
{
    return( new ByteBuffIS() );
} // getIS

/*
** Name: getIS
**
** Description:
**	Returns an InputStream capable of reading subset the byte buffer.
**
** Input:
**      pos     Starting position.
**      len     Length to read.
**
** Output:
**	None.
**
** Returns:
**	InputStream	Stream for reading.
**
** History:
**	 18-Jun-09 (rajus01)
**	    Created.
*/

public InputStream
getIS( long pos, long len )
{
    return( new ByteBuffIS( pos, len ) );
} // getIS


/*
** Name: find
**
** Description:
**	Search the buffer for a matching pattern, starting 
**	at the requested position, and return the position 
**	of the match.  A negative value is returned if no 
**	match is found or an invalid parameter is provided.
**
** Input:
**	pattern	Patten to be matched.
**	start	Starting position in buffer.
**
** Output:
**	None.
**
** Returns:
**	long	Buffer position of match, negative 
**		value if error or no match found.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public long
find( byte pattern[], long start )
{
    long pos = -1;
    Node node;

    /*
    ** Validate parameters.  Rather than throwing an exception,
    ** simply return an indication that no match was found.
    */
    if ( pattern == null  ||  pattern.length < 1  ||  start < 0 )  
	return( -1 );

    /*
    ** Scan through segments doing simple brute force search.
    **
    ** Note: position within the segment should be valid
    ** due to validation unless there is a structural
    ** problem.  Position is verified just to be safe.
    */
  segmentScan:
    for(
	 node = locate( start );
	 node != null  &&  start >= node.offset;
	 node = node.next
       )
    {
	/*
	** Determine position in segment and prepare
	** to continue with next segment.
	*/
	int off = (int)Math.min( start - node.offset, (long)node.length );
	start = node.offset + node.length;

	/*
	** Search current segment.
	*/
      segmentSearch:
	for( ; off < node.length; off++ )
	    if ( node.data[ off ] == pattern[ 0 ] )
	    {
		/*
		** Potential match found, compare pattern to
		** buffer.  Comparison is complicated by the 
		** fact that it might span segments but we
		** don't want to lose the original position.
		*/
		Node	current = node;
		int	off1 = off + 1;	// Next comparison position

		for( int off2 = 1; off2 < pattern.length; off1++, off2++ )
		{
		    while( off1 >= current.length )
		    {
			/*
			** If we reach the end of the buffer during a
			** comparison without a match, then no match 
			** is possible because the pattern will sub-
			** sequently always extend beyond the end of 
			** the buffer.
			*/
			if ( current.next == null )  
			    break segmentScan;	// We be done!

			/*
			** Continue comparison at start of next segment.
			*/
			current = current.next;
			off1 = 0;
		    }

		    /*
		    ** If comparison fails, continue searching segment.
		    */
		    if ( current.data[ off1 ] != pattern[ off2 ] )
			continue segmentSearch;
		}

		/*
		** We only reach this point if a match is found.
		** Return current search position.
		*/
		pos = node.offset + off;
		break segmentScan;	// We be done!
	    }
    }

    return( pos );
} // find


/*
** Name: truncate
**
** Description:
**	Truncate byte buffer to requested length.  
**	Length does not change if requested length 
**	is negative or exceeds current length.
**
** Input:
**	length	New buffer length.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public void
truncate( long length )
{
    /*
    ** Validate parameter.  Do nothing if invalid.
    */
    if ( length < 0 )  return;

    /*
    ** Truncating to 0 length is easily handled 
    ** by simply initializing the root node.
    */
    if ( length == 0 )
    {
	root.next = root.right = null;
	root.left = root;	// Last node in list.
	return;
    }

    /*
    ** Search for the last data byte to ensure we find the
    ** correct segment in the case where the new buffer
    ** length fills a segment and truncated bytes are in 
    ** the following segment(s).
    */
    Node node = prune( length - 1 );

    /*
    ** Segments were pruned above, but contents of current segment
    ** were not changed.  Adjust segment length to truncation point.
    **
    ** Note: truncation point could (improperly) exceed the current
    ** length.  No actual change occurs if this is the case since
    ** we have reached the last segment and care is taken so that
    ** the segment length does not increase.
    */
    length -= node.offset;	// Segment length
    node.length = (int)Math.max( 0L, Math.min( (long)node.length, length ) );
    return;
} // truncate


/*
** Name: append
**
** Description:
**	Append portion of a byte array to the buffer and
**	return the number of bytes appended, which may be
**	less than requested due to input space restrictions.
**	An invalid parameter results in 0 bytes being
**	appended.
**
** Input:
**	data	Binary data.
**	offset	Starting position.
**	length	Number of bytes.
**
** Output:
**	None.
**
** Returns:
**	long	Number of bytes appended.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public long
append( byte data[], int offset, int length )
{
    Node node = root.left;	// Last node in list
    long total = 0;

    /*
    ** Validate parameters.  Rather than throwing an exception,
    ** simply return an indication that 0 bytes have been appended.
    */
    if (
	 data == null  ||  length < 1  ||
	 offset < 0  ||  offset >= data.length
       )
	return( 0 );

    /*
    ** Ensure there is no data overrun on input array.
    */
    length = Math.min( length, data.length - offset );

    while( length > 0 )
    {
	/*
	** If current segment is full, extend buffer.
	*/
	if ( node.length >= node.data.length )  node = extend();

	/*
	** Determine how much data can fit in current segment.
	*/
	int len = Math.min( length, node.data.length - node.length );

	if ( len > 0 )
	{
	    System.arraycopy( data, offset, node.data, node.length, len );
	    node.length += len;
	    offset += length;
	    total += len;
	    length -= len;
	}
    }

    return( total );
} // append


/*
** Name: append
**
** Description:
**	Append binary stream to the buffer and return
**	number of bytes appended.  An invalid parameter
**	results in 0 bytes being appended.  The binary
**	stream is closed.
**
** Input:
**	stream	Binary stream.
**
** Output:
**	None.
**
** Returns:
**	long	Number of bytes appended.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public long
append( InputStream stream )
{
    Node node = root.left;	// Last node in list
    long total = 0;

    /*
    ** Validate parameters.  Rather than throwing an exception,
    ** simply return an indication that 0 bytes have been appended.
    */
    if ( stream == null )  return( 0 );

    for(;;)
    {
	int len;

	/*
	** If current segment is full, extend buffer.
	*/
	if ( node.length >= node.data.length )  node = extend();

	/*
	** Fill current segment from binary stream.
	*/
	try { len = stream.read( node.data, node.length, 
				 node.data.length - node.length ); }
	catch( IOException ex ) { len = -1; }

	/*
	** !!! TODO: 
	**     Current (last) segment may be empty due 
	**     to EOF immediately following an extension.
	**     Should the empty segment be truncated?
	*/
	if ( len < 0 )  break;	// End of stream.

	node.length += len;
	total += len;
    }

    /*
    ** Close stream to ensure resources are released.
    */
    try { stream.close(); }
    catch( IOException ignore ) {}

    return( total );
} // append


/*
** Name: write
**
** Description:
**	Write portion of a byte array to the buffer and
**	return the number of bytes written, which may be
**	less than requested due to input space restrictions.
**	An invalid parameter results in 0 bytes being
**	written.
**
**	Existing data is overwritten and the buffer will
**	be extended as needed to hold appended data.  No 
**	data will be written if the initial buffer position
**	is greater than the current buffer length.
**
** Input:
**	pos	Buffer position.
**	data	Binary data.
**	offset	Starting position.
**	length	Number of bytes.
**
** Output:
**	None.
**
** Returns:
**	long	Number of bytes written.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public long
write( long pos, byte data[], int offset, int length )
{
    long total = 0;

    /*
    ** Validate parameters.  Rather than throwing an exception,
    ** simply return an indication that 0 bytes have been written.
    */
    if ( 
	 data == null  ||  pos < 0  ||  length < 1  ||
	 offset < 0  ||  offset >= data.length
       )
	return( 0 );

    /*
    ** Ensure there is no data overrun on input array.
    */
    length = Math.min( length, data.length - offset );

    /*
    ** Data may need to be split across multiple segments.
    ** Retrieve segment containing the starting position
    ** and then scan linearly copying the data.
    **
    ** Note: position within the segment should be valid
    ** due to validation unless there is a structural
    ** problem.  Position is verified just to be safe.
    */
    for( 
	 Node node = locate( pos );  
	 length > 0  &&  node != null  &&  pos >= node.offset; 
	 node = node.next 
       )
    {
	/*
	** Restrict output bytes to data in current segment.
	** 
	** Note: the following calculations restrict the output
	** to only overwrite existing data.  It may be necessary
	** to extend the segment/buffer to satisfy the write 
	** request.  That case is handled later.
	*/
	int off = (int)Math.min( pos - node.offset, (long)node.length );
	int len = Math.min( length, node.length - off );

	if ( len > 0 )
	{
	    System.arraycopy( data, offset, node.data, off, len );
	    pos += len;
	    offset += len;
	    total += len;
	    length -= len;
	}

	/*
	** If the insertion point is now at the end of the 
	** buffer, any additional data is simply appended.
	**
	** Note: if position is beyond the end of the buffer,
	** data is not appended because of the missing space 
	** between the end and target position.  In this case, 
	** we will fall out of the loop since we are at the 
	** end of the segment list.
	*/
	if ( 
	     length > 0  &&  
	     pos == node.offset + node.length  &&
	     node.next == null  
	   )
	{
	    total += append( data, offset, length );
	    break;
	}
    }

    return( total );
} // write


/*
** Name: write
**
** Description:
**	Write a binary stream to the buffer and return 
**	the number of bytes written.  An invalid parameter 
**	results in 0 bytes being written.  The binary 
**	stream will be closed.
**
**	Existing data is overwritten and the buffer will
**	be extended as needed to hold appended data.  No 
**	data will be written if the initial buffer position
**	is greater than the current buffer length.
**
** Input:
**	pos	Buffer position.
**	stream	Binary stream.
**
** Output:
**	None.
**
** Returns:
**	long	Number of bytes written.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public long
write( long pos, InputStream stream )
{
    long total = 0;

    /*
    ** Validate parameters.  Rather than throwing an exception,
    ** simply return an indication that 0 bytes have been written.
    */
    if ( stream == null  ||  pos < 0 )  return( 0 );

    /*
    ** Data may need to be split across multiple segments.
    ** Retrieve segment containing the starting position
    ** and then scan linearly copying the data.
    **
    ** Note: position within the segment should be valid
    ** due to validation unless there is a structural
    ** problem.  Position is verified just to be safe.
    */
  segmentScan:
    for( 
	 Node node = locate( pos );  
	 node != null  &&  pos >= node.offset; 
	 node = node.next 
       )
    {
	/*
	** Restrict output bytes to data in current segment.
	** 
	** Note: the following calculations restrict the output
	** to only overwrite existing data.  It may be necessary
	** to extend the segment/buffer to satisfy the write 
	** request.  That case is handled later.
	*/
	int offset = (int)Math.min( pos - node.offset, (long)node.length );
	int length = node.length - offset;

	while( length > 0 )
	{
	    int len;

	    /*
	    ** Fill current segment from binary stream.
	    */
	    try { len = stream.read( node.data, offset, length ); }
	    catch( IOException ex ) { len = -1; }

	    if ( len < 0 )  
	    {
		try { stream.close(); }
		catch( IOException ignore ) {}
		break segmentScan;	// End of stream.
	    }

	    pos += len;
	    offset += len;
	    total += len;
	    length -= len;
	}

	/*
	** If the insertion point is now at the end of the 
	** buffer, any additional data is simply appended.
	**
	** Note: if position is beyond the end of the buffer,
	** data is not appended because of the missing space 
	** between the end and target position.  In this case, 
	** we will fall out of the loop since we are at the 
	** end of the segment list.
	*/
	if ( 
	     pos == node.offset + node.length  &&
	     node.next == null  
	   )
	{
	    total += append( stream );
	    break;
	}
    }

    return( total );
} // write


/*
** Name: getOS
**
** Description:
**	Returns an OutputStream capable of writing data
**	to the byte buffer.
**
**	Existing data is overwritten and the buffer will
**	be extended as needed to hold appended data.  No 
**	stream (NULL) is returned if the initial buffer 
**	position is greater than the current buffer length.
**
** Input:
**	pos		Buffer position.
**
** Output:
**	None.
**
** Returns:
**	OutputStream	Stream for writing or NULL.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public OutputStream
getOS( long pos )
{
    /*
    ** Validate parameters.
    */
    return( (pos < 0 || pos > length()) ? null : new ByteBuffOS( pos ) );
} // getOS


/*
** Name: locate
**
** Description:
**	Returns the segment containing the requested 
**	buffer position.  If position lies outside the 
**	current buffer, the nearest segment is returned
**	(position should not be negative, but the first
**	segment will still be returned in that case).
**
** Input:
**	pos	Target buffer position.
**
** Output:
**	None.
**
** Returns:
**	Node	Node associated with position.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

private Node
locate( long pos )
{
    /*
    ** Search begins at root.  Since root is always
    ** empty, search automatically follows right sub-
    ** tree.  If the buffer is empty, the right sub-
    ** tree won't exist and the root will be returned.
    */
    Node node = root;
    Node child = root.right;

    /*
    ** Scan down tree searching for segment
    ** which holds the target position.
    */
    while( child != null )
    {
	node = child;

	/*
	** Determine where position lies relative
	** to current node.  If position is outside
	** the buffer and we are at first or last
	** node, then child won't exist and loop
	** will terminate.
	*/
	if ( pos < node.offset )  
	    child = node.left;	// Position precedes segment
	else  if ( pos < node.offset + node.length )
	    child = null;	// Position in segment
	else  
	    child = node.right;	// Position follows segment
    }

    return( node );
} // locate


/*
** Name: prune
**
** Description:
**	Prunes all segments which follow the segment
**	containing the provided buffer position and
**	returns the target segment.  If position 
**	lies outside the current buffer, the nearest 
**	segment is returned (position should not be 
**	negative, but the first segment will still 
**	be returned in that case).
**
** Input:
**	pos	Target buffer position.
**
** Output:
**	None.
**
** Returns:
**	Node	Node associated with position.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

private Node
prune( long pos )
{
    /*
    ** Search begins at root.  Since root is always
    ** empty, search automatically follows right sub-
    ** tree.  If the buffer is empty, the right sub-
    ** tree won't exist and the root will be returned.
    **
    ** Algorithm is the same as in locate() with the
    ** addition of pruning branchs which follow the
    ** target segment (right of target).  Pruning
    ** may require updating a parent node, so a track
    ** behind reference is kept (parent).  Since we 
    ** immediately traverse to the child node, parent 
    ** is set the same as the current node initially.
    */
    Node parent = root;
    Node node = root;
    Node child = root.right;

    while( child != null )
    {
	node = child;

	if ( pos < node.offset )  
	{
	    /*
	    ** Target segment precedes current segment.
	    ** Descend to the left while pruning the
	    ** current segment (and all to the right).  
	    ** This effectively results in the left
	    ** child replacing the current node from
	    ** the parent viewpoint.
	    **
	    ** Note: Since target precedes current
	    ** segment, left child should always exist.  
	    ** There are only two cases when this won't 
	    ** be true: either the target position is 
	    ** negative or the binary tree structure is
	    ** corrupt.  Neither case is expected and
	    ** the natural result, terminating loop at
	    ** current segment, is reasonable in both 
	    ** cases.  Only prune right sub-tree in
	    ** this case.
	    */
	    if ( node.left == null )  
	    {
		node.right = null;
		child = null;
	    }
	    else
	    {
		parent.right = node.left;
		child = node.left;	
	    }
	}
	else  if ( pos < node.offset + node.length )
	{
	    /*
	    ** Current segment is target segment.
	    ** Prune segments which follow and 
	    ** terminate loop.
	    */
	    node.right = null;
	    child = null;
	}
	else  
	{
	    /*
	    ** Target segment follows current segment.
	    ** Descend to the right with no pruning.  
	    **
	    ** Note: if right child does not exist, then
	    ** target position is outside current buffer 
	    ** and loop will terminate at current segment
	    ** as desired.
	    */
	    parent = node;
	    child = node.right;
	}
    }

    /*
    ** Update linked list now that binary tree is 
    ** pruned. Make current segment last segment.
    */
    node.next = null;
    root.left = node;
    return( node );
} // prune


/*
** Name: extend
**
** Description:
**	Extends byte buffer by appending a new empty segment.
**	Should only be called when current last segment is full.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Node	The appended node.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

private Node
extend()
{
    Node node, last = root.left;
    long offset = last.offset + last.data.length;

    if ( last.level != Node.LEAF )
    {
	/*
	** When the last node is an internal node, simply
	** add a new leaf node as the right sub-tree.
	*/
	node = new Node( offset, segmentSize );
	last.right = node;
    }
    else
    {
	/*
	** When the last node is a leaf node, the next
	** node will be an internal node. The point of
	** insertion is the lowest discontinuity in the
	** binary tree.  The level of the internal node
	** is one greater than the full right sub-tree
	** below the discontinuity.
	**
	** The lowest discontinuity is found by scanning
	** down the right sub-trees comparing node levels
	** and saving the last found.
	**
	** Note that a target node will always be found
	** because the root's right sub-tree is not empty
	** and the root always has a discontinuity with 
	** it's right sub-tree.
	*/
	Node target = root;	// Save the root just in case.

	for( node = root; ; node = node.right )
	{
	    if ( node.right == null )  break;	// Bottom of tree
	    if ( node.level != node.right.level + 1 )  target = node;
	}

	/* 
	** The new internal node becomes the parent of
	** the full right sub-tree at the discontinuity
	** with the full tree as it's left sub-tree,
	** and replaces the sub-tree as the right sub-
	** tree at the discontinuity.
	*/
	node = new Node( target.right.level + 1, offset, segmentSize );
	node.left = target.right;
	target.right = node;
    }

    /*
    ** Link new node as last node in list.
    */
    root.left = last.next = node;
    return( node );
}


/*
** Name: Node
**
** Description:
**	Class which defines a segment of a byte buffer.
**
**	A segment provides storage (data and length), can be linked 
**	into a list as part of a larger buffer (next segment), and 
**	can be arranged as part of a binary tree for quicker access 
**	(position offset, node level, left sub-tree, right sub-tree).
**
**	A ROOT node represents an empty segment and provides no data 
**	storage.  An empty buffer is fully represented by a single
**	ROOT node.  LEAF nodes represents the lowest level of a full
**	binary tree and have no child sub-trees.  Internal nodes may 
**	have child sub-trees and are at a tree level above the LEAF 
**	nodes.  The ROOT node is neither a LEAF nor internal node
**	(level is distinct from any other node).
**	
**
**  Constants:
**
**	ROOT			Root level.
**	LEAF			Leaf level.
**
**  Public Data:
**
**	level			Level of node in tree.
**	offset			Offset of segment.
**	length			Length of segment.
**	data			Segment data.
**	next			Next node in list.
**	left			Left sub-tree.
**	right			Right sub-tree.
**
**  Private Data:
**
**	empty			Empty data array.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

private static class
Node
{

    /*
    ** Constants
    */
    public static final int	ROOT = 0;
    public static final int	LEAF = 1;

    /*
    ** Public Data
    */
    public int			level = ROOT;
    public long			offset = 0;
    public int			length = 0;
    public byte			data[];
    public Node			next = null;
    public Node			left = null;
    public Node			right = null;

    /*
    ** Private data
    */
    private static final byte	empty[] = new byte[0];


/*
** Name: Node
**
** Description:
**	Class contructor.  Constructs a ROOT node.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public
Node()
{
    data = empty;
} // Node


/*
** Name: Node
**
** Description:
**	Class contructor.  Constructs a LEAF node.
**
** Input:
**	offset	Offset of data.
**	size	Size of data array.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public
Node( long offset, int size )
{
    this( LEAF, offset, size );
} // Node


/*
** Name: Node
**
** Description:
**	Class contructor.  Constructs an general node.
**
** Input:
**	level	Level of node.
**	offset	Offset of data.
**	size	Size of data array.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public
Node( int level, long offset, int size )
{
    this.level = level;
    this.offset = offset;
    data = new byte[ size ];
} // Node


} // class Node


/*
** Name: ByteBuffIS
**
** Description:
**	InputStream class which wraps a byte buffer.
**
**  Overriden methods:
**
**	available		Buffered bytes available.
**	close			Close stream.
**	mark			Mark current position.
**	markSupported		Is mark method supported?
**	read			Read bytes.
**	reset			Set position to mark.
**	skip			Skip bytes.
**
**  Private Data:
**
**	buff			Byte buffer.
**	position		Current position.
**	limit			Max position.
**	mark			Marked position.
**	ba			Byte buffer for read method.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
**       18-Jun-09 (rajus01)
**          Added limit and constructor for reading data subset.
*/

private class
ByteBuffIS
    extends InputStream
{

    private long		position = 0;
    private long		limit = -1;
    private long		mark = -1;
    private byte		ba[] = new byte[1];


/*
** Name: ByteBuffIS
**
** Description:
**	Class constructor.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public 
ByteBuffIS()
{
} // ByteBuffIS

/*
** Name: ByteBuffIS()
**
** Description:
**      Class constructor to read subset of buffer.
**
** Input:
**      pos     Starting position.
**      len     Length.
**
** Output:
**      None.
**
** Returns:
**      None.
**
** History:
**       18-Jun-09 (rajus01)
**          Created.
*/

public
ByteBuffIS( long pos, long len )
{
    if ( pos < 0  ||  len < 0 )  pos = len = 0;
    position = pos;
    limit = position + len;
} // ByteBuffIS


/*
** Name: close
**
** Description:
**      Close stream and free resources.
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**      void.
**
** History:
**       2-Feb-07 (gordy)
**          Created.
*/

public void
close()
    throws IOException
{
    /*
    ** Disable future read, skip, and reset requests.
    */
    position = mark = -1;
    return;
} // close


/*
** Name: markSupported
**
** Description:
**      Are mark() and reset() supported?
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**      boolean         TRUE if mark() supported.
**
** History:
**       2-Feb-07 (gordy)
**          Created.
*/

public boolean
markSupported()
{
    return( true );
} // markSupported


/*
** Name: mark
**
** Description:
**      Mark current position in stream.
**
** Input:
**      readLimit       Max required number of bytes to save.
**
** Output:
**      None.
**
** Returns:
**      void.
**
** History:
**       2-Feb-07 (gordy)
**          Created.
*/

public void
mark( int readLimit )
{
    /*
    ** Since all data is already buffered, 
    ** only need to save current position 
    ** (limit is meaningless).
    */
    mark = position;
    return;
} // mark


/*
** Name: reset
**
** Description:
**      Repositions stream to previously marked position.
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**      void.
**
** History:
**       2-Feb-07 (gordy)
**          Created.
*/

public void
reset()
    throws IOException
{
    if ( position < 0 )  throw new IOException( "Stream closed" );

    /*
    ** If mark is not set, mark start of stream.
    */
    if ( mark < 0 )  mark = 0;
    position = mark;
    return;
} // reset


/*
** Name: available
**
** Description:
**	Returns the number of bytes which can be read
**	without blocking.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Number of bytes available.
**
** History
**	 2-Feb-07 (gordy)
**	    Created.
*/

public int
available()
    throws IOException
{
    if ( position < 0 )  throw new IOException( "Stream closed" );

    int		avail = 0;
    long	limit = ByteBuffer.this.length();

    if ( position < limit )
    {
	/*
	** The downcast from long to int may lose
	** significant precision - limit to max
	** positive int value.
	*/
	avail = (limit - position > Integer.MAX_VALUE)
		? Integer.MAX_VALUE : (int)(limit - position);
    }

    return( avail );
} // available


/*
** Name: skip
**
** Description:
**      Skip over and discard bytes from stream.
**
** Input:
**      bytes   Number of bytes to skip
**
** Output:
**      None.
**
** Returns:
**      long    Number of bytes skipped.
**
** History:
**       2-Feb-07 (gordy)
**          Created.
*/

public long
skip( long bytes )
    throws IOException
{
    if ( position < 0 )  throw new IOException( "Stream closed" );

    /*
    ** Limit bytes skipped to actual/valid amount.
    */
    long end = ByteBuffer.this.length();
    if ( limit >= 0 )  end = Math.min( end, limit );
    bytes = Math.max( 0L, Math.min( bytes, end - position ) );
    position += bytes;
    return( bytes );
} // skip


/*
** Name: read
**
** Description:
**      Reads a single byte from the stream.  Returns
**      -1 when no data available.
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**      int     Data byte or -1.
**
** History:
**       2-Feb-07 (gordy)
**          Created.
*/

public int
read()
    throws IOException
{
    return( (read( ba, 0, 1 ) == 1) ? (int)ba[0] & 0xff : -1 );
} // read


/*
** Name: read
**
** Description:
**      Read bytes into byte array from stream.  Returns
**      the number of bytes actually read or -1 if end-
**      of-data has been reached.
**
** Input:
**      data	Target byte array.
**      offset  Position in target array.
**      length  Number of bytes to read.
**
** Output:
**      ba      Data bytes.
**
** Returns:
**      int     Number of bytes read or -1.
**
** History:
**       2-Feb-07 (gordy)
**          Created.
*/

public int
read( byte data[], int offset, int length )
    throws IOException
{
    if ( position < 0 )  throw new IOException( "Stream closed" );

    /*
    ** Validate parameters.
    */
    if ( data == null )  throw new NullPointerException();
    if ( offset < 0  ||  length < 0  ||  length > (data.length - offset) )
        throw new IndexOutOfBoundsException();

    /*
    ** Check for end-of-data.
    */
    long end = ByteBuffer.this.length();
    if( limit >= 0 ) end = Math.min( end, limit );
    if ( position >= end )  return( -1 );
    if ( length == 0 )  return( 0 );

    /*
    ** Limit length to available data.
    */
    length = (int)ByteBuffer.this.read( position, data, offset, 
			(int)Math.min( (long)length, end - position ) );

    /*
    ** There should be no problem reading data due to the 
    ** validations performed above.  If a problem is 
    ** indicated, respond with an end-of-data indication.
    */
    if ( length > 0 )
	position += length;
    else
	length = -1;

    return( length );
} // read


} // class ByteBuffIS


/*
** Name: ByteBuffOS
**
** Description:
**	OutputStream class which wraps a byte buffer.
**
**  Overriden methods:
**
**	close			Close stream.
**	write			Write bytes.
**
**  Private Data:
**
**	position		Current position.
**	ba			Byte buffer for write method.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

private class
ByteBuffOS
    extends OutputStream
{

    private long		position = 0;
    private byte		ba[] = new byte[1];


/*
** Name: ByteBuffOS
**
** Description:
**	Class constructor.
**
** Input:
**	pos	Starting position.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public 
ByteBuffOS( long pos )
{
    position = Math.max( 0L, pos );
} // ByteBuffOS


/*
** Name: close
**
** Description:
**      Close stream and free resources.
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**      void.
**
** History:
**       2-Feb-07 (gordy)
**          Created.
*/

public void
close()
    throws IOException
{
    position = -1;	/* Disable future write requests */
    return;
} // close


/*
** Name: write
**
** Description:
**      Writes a single byte (low order 8 bits) to the stream.
**
** Input:
**	data	Byte value to be written.
**
** Output:
**      None.
**
** Returns:
**	void
**
** History:
**       2-Feb-07 (gordy)
**          Created.
*/

public void
write( int data )
    throws IOException
{
    ba[0] = (byte)(data & 0xff);
    write( ba, 0, 1 );
    return;
} // write


/*
** Name: write
**
** Description:
**      Write bytes from byte array from stream.
**
** Input:
**      data	Byte array.
**      offset  Position in array.
**      length  Number of bytes to write.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**       2-Feb-07 (gordy)
**          Created.
*/

public void
write( byte data[], int offset, int length )
    throws IOException
{
    if ( position < 0 )  throw new IOException( "Stream closed" );

    /*
    ** Validate parameters.
    */
    if ( data == null )  throw new NullPointerException();
    if ( offset < 0  ||  length < 0  ||  length > (data.length - offset) )
        throw new IndexOutOfBoundsException();

    /*
    ** No data is written if position is beyond the current 
    ** buffer end.  As long as the position is within or at 
    ** the end of the current buffer, the buffer length will 
    ** be extended if needed to hold the output.
    */
    if ( length > 0  &&  position <= ByteBuffer.this.length() )
	position += ByteBuffer.this.write( position, data, offset, length );
    return;
} // write


} // class ByteBuffOS


} // class ByteBuffer

