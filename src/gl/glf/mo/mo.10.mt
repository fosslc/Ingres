.\"==== Mo' MO
.\"
.\"As evidence of review through endurance, MO is nearing architecture
.\"approval.  The following relatively points were brought up at this
.\"round of discussion.
.\"
.\"(1)  A number of organizational inconsistencies and typos were
 \"	found. FIXED.
.\"
.\"(2)  The string-space/garbage collection details can and should be
.\"     omitted. FIXED
.\"
.\"(3)  Why can't the GC just be done with reference counts, rather than
.\"     computed during GC?  FIXED
.\"
.\"(4)  Persistence discussion is wrong for the current model.  FIXED
.\"
.\"(5)  MOmutex and MOunmutex are unloved.  Can they be hidden under
 \"	the covers?  FIXED
.\"
.\"(6)  Maybe collapse valid_perms and output_perms into one argument in some
.\"     functions.  NO, DON'T like that.
.\"
.\"(7)  The FIXME problem of index methods and GET-NEXT for virtual
.\"     classes remains open.  "Solved" with documentation and
.\"	techniques.
.\"
.\" (8) Need to call methods on set/del monitor.  Neet mdata for set/del
.\"	monitor.  This is for handling SQL events through one class,
.\"	using the instance as the SQL event name?
.\"
.(T "General Library Specification" "MO 1.10" mo
CL Committee
.)T
.(A
This is a proposed specification of an MO facility to be provided by
the General Library for access to management objects.

Revision: 1.10, 30-Apr-93
.)A
.C "Document History"
.P
Revision 0.1, split from original MI and renamed MO for
experimentation.  This version is more explicitly object oriented,
with name-instance treated separately.
.P
Revision 0.2, add more permission concepts for five different roles,
.q System ,
.q Security ,
.q Installation ,
.q DBA ,
and
.q Session.
.P
Revision 0.3, remove MOscan; can't think of a real need and it was a
complicated bag of worms.  Remove CL_ERR_DESCs and other CL
references.
.P
Revision 0.4, remove memory stuff into separate GLalloc / GLfree /
GLset_alloc; it's a common problem so solve it in a general way\**.
Add MOstring, MOla, and MOlax; maybe some of these are bad ideas.
.(f
\** ``I always speak Generally.'' \- Norman Schwartzkopf
.)f
.P
Revision 0.5, add thoughts about extending .0 access functions to
eliminate the need to attach each instance, and contemplate ditching
getnext_name.
.P
Revision 0.6, make more object like; names are now classes, access
functions are now "methods", and objects mean {class, instance}.
Ditch next_name; can't figure out how to make it work reasonably in
the face of methods.  Would require a "GETNEXT_CLASS" message to be
handled by the method, which seems practically very difficult.  Each
method would need to know what the "next" class was, and that would
lose most of the value of the index in the tree.
.P
Revision 0.7, add user permission; add pile of stuff for object-ids.
Sigh.
.P
Revision 0.8, clean up some object-id related stuff.  Remove get_oid
and get_name; they are redundant with get_meta.  Add flags for field
validity in meta data, allowing for garbage collection.
.P
Revision 0.9.  Remove "classid" args to methods and monitors.  Neither
can constructively use this information, particularly if handed a
mapped object-id instead of the class the client used for the
attachment.  Add
.i valid_perms
as an argument to a number of functions to allow some enforcement by
MO.  Add
.i session_context
to get and get_next for use as a context by methods that may benefit
having it for getnext.  Remove get_meta, because you can just MOget
the meta data.
.P
Revision 1.0.  Investigate things suggested in the first review.
Clear up classes; allow only one caller of methods/monitors; collapse
meta data to oid mapping, allow only char input to set, and char
output from get/getnext.  Make session_context a PTR *, so the method
can store things there itself.  Add method arg to attach, and add many
arguments to the method.  Attach can now define a class independent of
an instance, and methods may be handed object addresses and sizes.
.P
Revision 1.1.  Following CL review of 31-Jan-92, make the following
changes.  (1) Split class definition and instance attachment; (2)
delete single class/instance operations and do everything with tables
of stuctures; (3) get rid of ATTACH and DETACH event messages; (4)
separate "value" and "index" operations into separate methods; (5)
eliminate use of flags to identify intrinsic methods. Supply them
instead as functions; (6) Get rid of MOstring etc.from the public
interface, and make that operation part of the definition/attachment;
(7) various document organization and typo changes.
.P
Revision 1.2.  Following CL review of 7-Feb-92, (a) fixed typos and
organization problems; (b) Remove mutex/unmutex\-handle it internally;
(c) remove discussion of gargage collection, hiding the issue.
.P
Revision 1.3.  Internal cleanup.  Remove user permission.
.P
Revision 1.4,  Add methods/events for set/del monitor.  Each needs to
be handed the actual object type, and standard set/del method funcs
are provided.
.P
Revision 1.5, Remove hex/oct conversion flags, make the flags field
now be an offset for use with structures, change get/set methods to
use the offset.  Remove defaulting of index method; user must specify
MOidata_index instead of handing NULL.  Allow VAR for class index and
cdata.  Allow defaulting index to classid, and cdata to classid or
index.  Provide MOcdata_index for one-stop object definition.  Remove
MO_INSTANCE_DEF structure and turn MOattach into a "one object" only
interface taking arguments.  Document MOstrout and MOlongout as public
interfaces.  Add MOivget.  Remove dumb monitor methods.
.P
Revision 1.6, At request of BruceK, the first user of monitors,
change set_method to allow multiple methods per-classid, and have both
cookie and qual_regexp args.
.P
Revision 1.7  Have tell_monitor and monitor functions take a value
string as well.  Monitor calls driven by MOget[next]/MOset give a real
value, other internal calls pass NULL, and external calls pass
whetever the client user wishes.
.P
Revision 1.8, fix some gross typos.
.P
Revision 1.9, More typos, clarifications etc.  In particulat, getnext
methods must return the first instance of the class if an empty
instance is presented.  Recover some function descriptions that got
lost somehow (MOnoset, MOstrget, MOstrout, MOuintget, MOuintset, and
MOuivget).  Note that qual_regexp's don't work in this version.
.P
Revision 1.10, Sigh, recover 1.9 by hand from postscript output as
only the 1.7 source can be found.  Add MOptrget and MOpvget methods,
MOlstrout, and MOptrout.  Note lack of ACLs.
.bp
.C "Specification"
.S 1 Introduction

The MO module provides ways of defining and working with
.i "management objects" .
Clients export meter data and control knobs via a named class-instance
space; other clients make this space visible to users via
.i "management protocols"
One such protocol is SQL, where the objects are presented as rows or
columns in tables; another is the TCP/IP Simple Network Management
Protocol (SNMP), and a third is the OSI Common Management Interface
Protocol (CMIP).  MO is intended to support all of these protocols,
and a number of it's restrictions reflect those of these other
interfaces.
.P
Objects may be arbitrary program variables, accessed via
client-supplied or standard
.q method
functions.  Object may also be completely virtual, instantiated by
user-provided method functions.  Object classes provide permissions
that may be used to enforce access control.
.P
The following executable interfaces are provided:
.TS
center;
cfB s
lfB lfB
l l.
Table 1: MO functions
Name	Description
_
	\fIControl\fP
MOon_off	enable/disable MO operations (for application clients).
_
	\fIDefinition\fP
MOclassdef	define a class type.
MOattach	attach an instance to the object space.
MOdetach	detach an instance from the object space.
_
	\fIQuery\fR
MOget	get data from the object.
MOgetnext	get next object in name, instance order.
MOset	set value for an instance.
_
	\fIMonitors\fR
MOset_monitor	set monitor function for a class.
MOtell_monitor	call any monitor functions set for a class.
_
	\fIAccess Methods \- get\fR
MOintget	get integer at pointer
MOivget	get integer value
MOlstrget	get from string buffers, length bounded.
MOstrget	get from string buffers.
MOstrpget	get from pointers to string buffers.
MOptrget	get from pointer to pointer
MOpvget	get from pointer value.
MOuintget	get unsigned integer at pointer
MOuivget	get unsigned integer value
MOzeroget	get value of 0.
MOblankget	get all blanks.
	\fIAccess Methods \- set\fR
MOintset	set integers
MOnoset	set nothing, error method for non-writable objects.
MOstrset	set string buffers.
MOuintset	set unsigned integers
_
	\fIIndex Methods\fR
MOcdata_index	method to get cdata from class definition for one instance.
MOidata_index	method to get idata from attached instances.
MOname_index	method to get name from attached instances.
_
	\fIUser Method Utilities\fR
MOlongout	convert and put a longnat with error checking.
MOptrout	convert and put a PTR with error checking.
MOstrout	put a string with error checking.
MOulongout	convert and put an unsigned longnat with error checking.
.TE
.P
A
.q "Users guide to writing objects"
is attached in Appendix A.
.S 2 "Intended Uses"
.P
MO is intended to provide an underlying mechanism for entity
management under different management protocols.  For instance, an SQL
gateway might use the MOget, MOgetnext and MOset functions to objects
made visible by the program.  Similarly, the five Internet SNMP (RFC
1157) operations could map cleanly to MO if that were desired:
.(b
.TS
center;
cfB s
lfB lfB
l l.
Table 2: Mapping SNMP to MO
SNMP	MO
_
GetRequest-PDU	MOget
GetRequestNext-PDU	MOgetnext
GetResponse-PDU	MOget[next] return
SetRequest-PDU	MOset
Trap-PDU	MOtell_monitor
.TE
.)b
.S 2 "Concepts and Assumptions"
.S 3 "Managed Objects"
.P
Managed objects are identified with a process-unique two-part key,
consisting of a
.i classid
and an
.i instance .
There may be multiple instances of the same classid.  Each object may
have an associated value that is manipulated with a number of built in
or client supplied methods.  Objects are transient in MO; they have no
persistence across the life of a process.
.P
All objects within MO are scalars; there are no arrays or structure
aggregate types (except the various character types).  Arrays or
aggregates are constructed out of scalars using the object naming
conventions described below under
.b "Modeling" .
.S 3 "Formal and experimental objects"
.P
.i "Formal MIB objects"
are those that are documented for customer use, and which are also
described/defined using the ASN.1 for operation with standard
protocols.  Objects in the formal MIB will be documented as part of
various projects beyond the scope of MO proper.  It is expected to be
difficult to add things to the formal MIB, because of the support and
documentation burden.  All objects in the formal MIB will have object
identifiers (oids), described below.
.P
\fBAt this time, there are no objects in the formal MIB.\fP
.P
Because the formal MIB will be fairly rigid and inflexible, there is also
an
.i "experimental MIB"
name space to be used for things that are not to be used by clients in
portable product applications.  These objects may be added with
impunity, though it is wise to restrict them to a reasonably small
number.  The experimental MIB can be used for what has traditionally
been done though tracepoints, NM symbols, and TRdisplay statements to
the log output.
.S 3 "Object Naming Conventions"
.P
Within MO, there is one {classid, instance}
.i object-space
per process.  The
.i classid
is actually a union of two disjoint sets used for class
identification.  The
.i class
or
.i "class name"
is the human sensible name, and is what all human originated queries
will use as the key.  The other method is the use of
.i "object ids" ,
or
.i oids .
These are by products of the need to product standards amendable
management objects defined with the ISO ASN.1 language.  The syntax of
each is shown below.
.ip \fBClass\fR 12
From the alphabet \f(CW[a-zA-Z_\e-\e.]\fP, as in
.q ingres.server.serverPid .
There may be class names that do not correspond to object ids.
.ip \fBObject-ID\fR 12
From the alphabet \f(CW[0-9\e.]\fP, as in
.q 1.3.6.1.4.1.3.3.3 .
Each object id corresponds to a single class name.  The numbers in
embedded object ids may not contain leading zeros.  While `1.0.5' is a
legal oid, `01.00.06' is not.
.ip \fBInstance\fR 12
From the alphabet \f(CW[a-zA-Z0-9_\e-\e.]\fR.  \fBFor objects that are
within the formal MIB, instance values may not contain letters, only
decimal digits and the decimal point.\fR
.P
All are manipulated by MO as EOS-terminated non-multi-byte character
strings.
.P
Object-instances are occasionally described to humans with a composite
key of the form
.(E
	"classid.instance"
.)E
Note that classid and instance may both contain their own embedded
.q dot
separators, indicating hierarchies within each.  In particular,
instances are not numbers, they are strings.  All the MO functions
take classid and the instance as separate string arguments.
.S 3 "Modeling"
.P
.PB "Scalar objects"
having only one possible
.i instance
per-process are to be given the
.i instance
.q 0 .
For example, a scalar variable holding the number of connected users
might be attached as
.(E
	"exp.common.gcf.gca.connected_users.0"
.)E
.PB "Columnar data"
(an array) is represented with multiple objects having the same
.i classid ,
but having a different
.i instance
value for each logical row, acting as a row index.  None of the rows
may be the
.q 0
instance.  If the
.q 0
instance exists as an accessible object, there should be no other
instances of the class.
.q 0
instance.
.PB "Tabular data"
is represented using a different classid for each column, joined by
identical
.i instance
values for each logical row.  There must be an explicit index column
for each table, which can be scanned for all the valid row instances.
.(b
Given a logical table that looks like this, with ``id'' as the index
column:
.TS
center;
cfB s s s
lfB lfB lfB lfB
l l l l.
Table 3:  Example abstract table
id	user	group	role
_
1000	me	wheel	admin
2000	him	users	user
.TE
.)b
.(b
The typical MO representation would be as four classes, with the
following instances (in MOgetnext order):
.TS
center;
cfB s s
lfB lfB lfB
lfCW lfCW lfCW.
Table 4:  Example objects implementing an abstract table
Class	Instance	Value
_
group	1000	wheel
group	2000	users
id	1000	1000
id	2000	2000
role	1000	admin
role	2000	user
user	1000	me
user	2000	him
.TE
.)b
.SB "Multiple-access paths"
.P
Actual objects may be multiply attached under different {classid,
instance} combinations.  This allows something to be attached once as
an undocumented object, and again as a documented object.  For
instance,
.b exp.cl.clf.tm.cpums.0
might be the CL internal representation, while
.b server.cpums.0
might be a documented interface.
.SB "Twins \- Mapping Class Names to Object IDs and back"
.P
As a particularly important case of multiple access paths, objects
that are attached by name are also implicitly attached by their object
id.  Clients may examine the MO_META_CLASS_CLASS with an object id to
see the
.q twin
name, and the MO_META_OID_CLASS with a name to see the twin object-id.
.P
Actions on the class are reflected in the oid-twin, and vice-versa.
That is, when a set is made on the class, a monitor for the oid-twin
will be called; when the object is detached, the oid-twin goes away as
well.  Thus, the code providing objects needs only to be concerned
with the class names, and never the OID values.
.P
Object Ids may be assigned to named classed by setting the
MO_META_OID_CLASS for the name to the string value of the object-id.
When this is done, any existing attached objects for the class will be
duplicated as if they had been attached by the oid as well.
.SB "Experimental Objects"
.P
Objects that are not defined for use in the formal MIB with ASN.1
should use the following convention for naming:
.(l

\f(CWexp\fR.\fIfacility\fP.\fIsubpart\fI.\fIwhatever\fR.\fIinstance\fR
.)l
The use of
.q dot
separation of parts is preferred ton indicate hierarchical divisions,
with underscore used within a subpart for readability, e.g.,
.(E
	exp.clf.cs.info.norm_prio.0
.)E
.S 3 "Permissions"
.P
Logical permissions are associated with each class, reflecting a
number of different access roles: Session, DBA, Installation,
System, and Security.  MO relies on the client code to provide it with
a mask of currently valid roles to use to enforce these permission
bits. and enforces them as appropriate.  If the client wants to do
it's own enforcement, it may hand in a mask of all ones (~0), and look
at the output permissions returned with the object.
.P
Each class may have a
.i permanent
attribute, which indicates that objects in the class may not be
detached by anyone.
.P
If a class has neither read nor write permission specified, the object
exists, but can't be seen on gets, and can't be written on sets.
Write-only types are strongly discouraged.
.P
There are permissions for \fIcurrent user\fP, because MO does not provide for
object ownership.  All objects are owned by the process.
.P
MO also does not presently aprovide permissions against individual
users (ACLs) because doing so would add a great deal of overhead.
(This may need revisiting in the future because of C2/B1 and SNMPv2).
.P
The five roles are
.PB MO_SES,
a ``session'' may access the class;
.PB MO_DBA,
the ``data base administrator'' may access the class;
.PB MO_SERVER,
the ``server'' or ``installation''  administrator\** may access the class;
.(f
\** Perhaps ingres or someone acting as ingres.
.)f
.PB MO_SYSTEM,
the ``system'' administrator\** may access the class;
.(f
\** Perhaps, root, backup, someone with SYSPRIV, or the SNMP agent.
.)f
.PB MO_SECURITY,
perhaps the ``security officer'' in the
.q "Orange Book"
world may access the class;
.P
The definition of these roles is at the discretion of the calling
code, in terms of the
.i valperms
masks it supplies to get and set calls.
.S 3 "Message Types"
.P
There are
.i "message types"
associated with methods and monitors, both discussed in sections below.
.(b
.TS
center;
cfB s
lfB lfB
l l.
Table 5: MO method/monitor message types
Message	Means
_
MO_ATTACH	Instance being attached.
MO_DETACH	Instance being detached.
MO_GET	Retrieve value and permissions
MO_GETNEXT	Retrieve value and permissions of next
MO_SET	Set value
MO_SET_MONITOR	Monitor being set for this class
MO_DEL_MONITOR	Monitor being deleted for this class
.TE
.)b
.S 3 "Class Methods"
.P
Each class definition identifies
.i method
functions that are called when access to an instance is needed.  These
can be coded by the client defining the class, or you may use a number
of pre-defined methods provided by MO for common operations.
Provision for user-defined methods allows clients to do more
complicated actions than simply reading or setting a variable.
.P
There are three methods that must be provided in a class definition.
.ip "GET METHOD" 16
is handed an offset and an object size provided by the class
definition, and a pointer provided by an index method, the length of
an output buffer, and an output buffer.
.ip "SET METHOD" 16
is handed the same arguments as a get method, in order identifying the
inputs and outputs.  It takes the input character buffer and makes the
instance identitified by the object take that value.
.ip "INDEX METHOD" 16
is given a message type (either MO_GET or MO_GETNEXT), a pointer of
data associated with the class, the length of the instance buffer, an
instance buffer, and the address of a pointer that will be filled in
with instance-specific data.  If the message is GET NEXT, then the
instance will be modified to contain the instance returned.  GET
handles the direct mapping of instance to instance-data, and GET NEXT
enforces the ordering of instances.
.P
Some reasons to use user-methods include:
.BU
Changing a limit might need shut down an existing configuration and
restart it with the new value \- provide a special SET method.
.BU
Making visible instances that can't be explicitly attached \- provide a
special INDEX method.
.BU
Providing different conversions from string to internal type (and
back) than are provided by the internal methods \- provide special GET
and SET methods.
.P
Two of the three provided index methods (MOidata_index and
MOname_index) look up instances that have been explicitly attached.
The other, MOcdata_index is for single instance classes whose objects
are known at the time of class definition.  A typical user index method
walks through it's own tree or array to find objects by instance.
Examples are presented in Appendix A.
.P
There are a number of standard methods provided for use in client
class definitions.
.TS
center;
cfB s s
lfB lfB lfB
l l l.
Table 6: MO standard method functions
Name	Type	Description
_
		\fIGet methods for unreadable control objects\fR
MOblankget	MO_GET_METHOD	Get a buffer full of blanks.
MOzeroget	MO_GET_METHOD	Get a buffer with string of value zero.
_
		\fISet methods for unwritable monitor objects\fR
MOnoset	MO_SET_METHOD	Set method for read-only classes.
_
		\fIMethods for integers\fP
MOintget	MO_GET_METHOD	Get string of value at instance data.
MOivget	MO_GET_METHOD	Get string of integer of instance data
MOintset	MO_SET_METHOD	Set integer from input string.
_
		\fIMethods for unsigned integers\fR
MOuintget	MO_GET_METHOD	Get string of unsigned value at instance data.
MOuivget	MO_GET_METHOD	Get string of unsigned integer of instance data
MOuintset	MO_SET_METHOD	Set unsigned integer from input string.
_
		\fIMethods for pointers\fR
MOptrget	MO_GET_METHOD	Get string of pointer at instance data.
MOpvget	MO_GET_METHOD	Get string of pointer of instance data.
_
		\fIMethods for unsigned integers\fR
MOptrget	MO_GET_METHOD	Get string of unsigned value at instance data.
		\fIMethods for strings\fR
MOstrget	MO_GET_METHOD	Get string from string buffer.
MOstrpget	MO_GET_METHOD	Get string from pointer to string buffer.
MOstrset	MO_SET_METHOD	Set string buffer from string buffer.
_
		\fIIndex Methods\fR
MOidata_index	MO_INDEX_METHOD	Get idata from attached instance idata.
MOcdata_index	MO_INDEX_METHOD	Get idata from class definition cdata.
MOname_index	MO_INDEX_METHOD	Get name from attached instances.
.TE
.P
MO does not provide floating point methods, and their use is
discouraged for compatibility with other management protocols.  If you
are dealing with an accumulator that may exceed the range of a
longnat, consider scaling it.  For instance, a variable keeping count
of GC I/O volume might be scaled to Kbytes or Mbytes rather than being
raw bytes.  Perhaps you keep the counter as floating, but present it
as scaled Kbytes.
.S 3 "The GETNEXT Problem for index methods"
.P
There are classes where it is inconvenient or impossible to explicitly
attach each instance.  In these cases, a method must be defined for
the class, and it is called for all operations.  This means that it
must provide the appropriate ordering of instances for the GETNEXT
operation.  This can be difficult, because the instance may not known.
For instance, the string of an control block address might be used as
the instance, but there might not be an ordered list of control blocks
visible to the method.  The solutions to this problem are discussed in
Appendix A.
.P
.S 3 "Monitors"
.P
MO provides
.i "monitor functions"
to assist support of event driven programs, where things are called
when something happens rather than polling to observe changes.
Monitors are attached to classes by clients, and are distinct from
method functions.  Methods are owned by the person writing the object;
Monitors are owned by clients that have no visibility on the objects.

The expectation is that there will be relatively few objects written
requiring these calls, and that they be driven rarely.  ``Relatively
few'' means handfulls, and ``rarely'' means a total monitor rate of
1-10/second or so.  It is not intended to handle hundreds or thousands
or monitor calls per second for hundreds of classids.

Coordination of meanings between objects and monitor functions is
defined by clients and object providers.  MO merely coordinates the
rendezvous though its object space.

Multiple monitors may be hung onto a classid with the MOset_monitor
call.  The monitors on a classid are distinguished by having unique
.i "monitor data" ,
values, similar to the class data given to an class definition or the
instance data provided with an MOattach.  This might be the address of
a control block for the monitor operation, especially if one monitor
function is being used to watch many different classids.

When MOset, MOget or MOgetnext are called on that instance or class,
the monitor functions for the class will be called.  Monitors may also
be explicitly driven by client code calling MOtell_monitor.

At present, monitors will be called for all instances of a classid;
the 
.i qual_regexp
is ignored.  In the future, the instances will be filtered by
considering the qual_regexp handed in when the monitor was set.  If it
was NULL, or if the actual instancematches theregulat expression, then
the monitor would be called.  This will be done when there is a
regular expression module in the GL for MO to call.

A called monitor is provided the classid, the classid of it's twin,
(if one exists), the instance, a value and a message.  If the message
is MO_GET or MO_SET, and the call was made implicitly by MO, then the
value is the current value of the object.  Other internal calls pass a
NULL value.  Explicit calls to MOtell_monitor deliver whatever value
the client wishes to send.

Changes to objects are only signaled to monitors if (a) the the change
was made by an MOset operation, or (b) the code that made the change
explicitly calls MOtell_monitor that something happened.  Many objects
are written with get methods like MOintget, which report the value of
an existing variable when queried.  Changes to these variables will
not drive monitors without the explicit MOtell_monitor calls.  

.S 3 "Memory Allocation"
.P
Some MO routines allocate and release memory by calling MEreqmem and
MEfree.  These are documented in the function descriptions.  There are
limits defined for the amount of memory that may be allocated by MO,
and within that limit, how much may be used for string space.  These
may be manipulated with the MO_MEM_LIMIT and MO_STR_LIMIT classes,
described below.
.P
String space is used to cache non-constant strings that are provided
as classids, indexes, and instances in calls to MOclassdef and
MOattach.  Strings that become unreferenced through calls to MOdetach
are released.
.S 3 "Ordering"
.P
MO provides efficient ordered keyed access to items addressed by
.q "{classid, instance}" with
MOgetnext calls.  The ordering is compatible with STcompare over the
limited alphabet described in in
.b "Object Naming Conventions" .
.P
This may cause a problem matching database ordering for joins.
STcompare is not the fancy ADF database comparison functions, which is
not visible to the General Library.  Use of ADF stuff would preclude
use of MO by CL code, and enlarge otherwise small executables.  The
current GWF code is incapable of dealing with ordering from the GW
that is different from that of the database.  Thus, MO data made
visible in a database with non\-STcompare ordering may result in
improper joins.
.S 3 "Initialization"
.P
MO is self-initializing.  Calls can be made in any order, and must
always produce the expected results.  This is because we can't predict
when something will start attaching things, and it might be before it
is possible to explicitly initialize MO.
.S 3 "Re-entrancy"
.P
The MO routines are re-entrant, but the underlying object space is
not.  MO protects it's object space by acquiring MU_SEMAPHOREs at
appropriate places to ensure consistency.  
.S 3 "Persistence"
.P
The strings provided as classes and instances to attach calls must
remain valid until they can no longer be accessed unless the
appropriate VAR flags are handed in to cause them to be saved by MO.
Strings provided as compile time constants are constant and don't need
to be stashed.  Strings built into local buffers are volatile, and do
need to be saved.
.P
The MOget routines alway fill an output buffer with a copy of the
value, so there is no obligation on the caller to maintain validity
after a detach.
.S 3 "Self Management"
.P
MO defines a number of internal classes.  One set provides statistics
on call counts to MO.  Another set is for control of the string cache
and memory allocation.  The last is for accessing the meta data
contained in classid descriptions.  The only meta-data instances that
must be defined are those identifying twin class/object-id pairs;
others are optional.
.P
The following classes provide counts of calls to external routines.
.TS
center;
c s
l l.
Table 7: MO Call counters
Class	Description
_
	\fIpublic interfaces\fP
MO_NUM_ATTACH	Calls to MOattach
MO_NUM_CLASSDEF	Calls to MOclassdef
MO_NUM_DETACH	Calls to MOdetach
MO_NUM_GET	Calls to MOget
MO_NUM_GETNEXT	Calls to MOgetnext
MO_NUM_DEL_MONITOR	For client use
MO_NUM_SET_MONITOR	For client use
_
	\fIInternal interfaces\fR
MO_NUM_ALLOC	Calls to allocate memory.
MO_NUM_FREE	Calls to free memory.
.TE
.P
The following self-management classes are provided
.TS
center;
c s
l l.
Table 8: MO Meters
Class	Description
_
MO_MEM_LIMIT	Max memory that MO may allocate
MO_MEM_BYTES	Current amount of memory MO has allocated.
MO_MEM_FAIL	Allocator calls that hit the limit.
_
MO_STR_LIMIT	Max memory that may be used by strings.
MO_STR_BYTES	Current memory used for strings.
MO_STR_FREED	Bytes recovered by garbage collection.
MO_STR_FAIL	String saves that hit the limit.
.TE
.P
The strings classes are indexed by the string values.
.TS
center;
c s
l l.
Table 9: MO String table classes
Class	Description
_
MO_STRING_CLASS	String value in the cache.
MO_STRING_REF_CLASS	References to the string.
.TE
.S 3 "Meta Data"
.P
A number of classes are provided for access to the meta data contained
in the class definitions.  The live meta data is stored in the MO
space indexed by classid in these classes.  Some ancilliary classes
are defined for use by clients, but are not maintained by MO.  All of
these classes are indexec by MO_META_CLASSID_CLASS.
.TS
center;
c s
l l.
Table 10: MO Meta Data Classes
Class	Description
_
	\fIAlways present\fR
MO_META_CLASSID_CLASS	The classid as defined.
MO_META_OID_CLASS	The twin OID of the classid.
MO_META_CLASS_CLASS	The twin class name of the classid.
MO_META_SIZE_CLASS	The size of the objects in the class.
MO_META_PERMS_CLASS	The permissions for objects in the class.
MO_META_INDEX_CLASS	The index classes for objects in the class.
_
	\fIPossibly present\fR
MO_META_SYNTAX_CLASS	The SNMP syntax for the class.
MO_META_ACCESS_CLASS	The SNMP access for the class.
MO_META_DESC_CLASS	The description for the class.
MO_META_REF_CLASS	The SNMP reference for the class.
.TE
.P
The index is the name of the classid providing an index column for the table.
.bp
.S 1 "Header file <mo.h>"
.S 2 "Manifest Constants"
.S 3 "Error codes"
.P
The following error codes are defined and may be checked by calling
code as specified for the individual functions.
.SD MO_ALREADY_ATTACHED
Attempt to re-attach an existing instance.  This is a programming
error.
.SD MO_BAD_MSG
An index method or a monitor function got a msg that it didn't expect.
This is a programming error.
.SD MO_BAD_SIZE
A get or set method was given an object size that didn't expect.  This
is a programming error.
.SD MO_CLASSID_TRUNCATED
The classid buffer handed to an index method wasn't large enough, and the
return value was truncated to fit.  This is a programming error.
.SD MO_INCONSISTENT_CLASS
A classid was being defined a second time, and the new definition
wasn't the same as the first one.  This is a programming error.
.SD MO_INSTANCE_TRUNCATED
The instance buffer handed to an index method wasn't large enough, and the
return value was truncated to fit.  This is a programming error.
.SD MO_NO_CLASSID
The classid specified isn't defined.
.SD MO_NO_DETACH
Attempt to detach an instance of a class that had the MO_PERMANENT
flag set.  This is a programming error.
.SD MO_NO_INSTANCE
The instance requested doesn't exist, or permissions to read the class
don't exist.
.SD MO_NO_NEXT
There is no successor to the requested instance.
.SD MO_NO_READ
You don't have permissions to get instances of the classid.
.SD MO_NO_STRING_SPACE
A class definition or instance attach would use up too much string
space memory.
.SD MO_NO_WRITE
You don't have permissions to set instances of the classid.
.SD MO_NULL_METHOD
The class definition did not provide necessary get or set methods.
This is a programming error.
.SD MO_VALUE_TRUNCATED
The instance buffer handed to an index method wasn't large enough, and the
return value was truncated to fit.  This is a programming error.
.SD MO_MEM_LIMIT_EXCEEDED
An internal allocation request would have caused MO to exceed it's
allocated memory limit
.S 3 "Wired in classes"
.P
These classes may be used to store meta data.  Clients should use the
defined symbol whenever possible.
.(E
# define MO_NUM_ATTACH          "exp.gl.glf.mo.num.attach"
# define MO_NUM_CLASSDEF        "exp.gl.glf.mo.num.classdef"
# define MO_NUM_DETACH          "exp.gl.glf.mo.num.detach"
# define MO_NUM_GET             "exp.gl.glf.mo.num.get"
# define MO_NUM_GETNEXT         "exp.gl.glf.mo.num.getnext"
# define MO_NUM_SET_MONITOR     "exp.gl.glf.mo.num.set_monitor"
# define MO_NUM_TELL_MONITOR    "exp.gl.glf.mo.num.tell_monitor"

# define MO_NUM_MUTEX           "exp.gl.mo.num.mutex"
# define MO_NUM_UNMUTEX         "exp.gl.mo.num.unmutex"

# define MO_NUM_ALLOC           "exp.gl.glf.mo.num.alloc"
# define MO_NUM_FREE            "exp.gl.glf.mo.num.free"

# define MO_MEM_LIMIT           "exp.gl.glf.mo.mem.limit"
# define MO_MEM_BYTES           "exp.gl.glf.mo.mem.bytes"

# define MO_STR_LIMIT           "exp.gl.glf.mo.strings.limit"
# define MO_STR_BYTES           "exp.gl.glf.mo.strings.bytes"
# define MO_STR_FREED           "exp.gl.glf.mo.strings.freed"

# define MO_STRING_CLASS        "exp.gl.glf.mo.strings.vals"
# define MO_STRING_REF_CLASS    "exp.gl.glf.mo.strings.refs"

# define MO_META_CLASSID_CLASS  "exp.gl.glf.mo.meta.classid"
# define MO_META_OID_CLASS      "exp.gl.glf.mo.meta.oid"
# define MO_META_CLASS_CLASS    "exp.gl.glf.mo.meta.class"
# define MO_META_SIZE_CLASS     "exp.gl.glf.mo.meta.size"
# define MO_META_PERMS_CLASS    "exp.gl.glf.mo.meta.perms"
# define MO_META_INDEX_CLASS    "exp.gl.glf.mo.meta.index"

# define MO_META_SYNTAX_CLASS   "exp.gl.glf.mo.meta.syntax"
# define MO_META_ACCESS_CLASS   "exp.gl.glf.mo.meta.access"
# define MO_META_DESC_CLASS     "exp.gl.glf.mo.meta.desc"
# define MO_META_REF_CLASS      "exp.gl.glf.mo.meta.ref"
.)E
.S 3 "Operations for MOon_off(), and returned as state"
.P
These codes are passed as operations to MOon_off, and it returns
MO_ENABLE or MO_DISABLE as the old state.
.(E
# define MO_ENABLE      1
# define MO_DISABLE     2
# define MO_INQUIRE     4
.)E
.S 3 "Permissions"
.P
These define access permissions for classes.  They are used in calls
to MOclassdef, and are returned in calls to MOget and MOgetnext.
.P
The defined permissions are:
.SD MO_PERM_NONE
no permission to read or write.
.SD MO_SES_READ
instance may be read by the session.
.SD MO_SES_WRITE
instance may be written by the session.
.SD MO_DBA_READ
instance may be read by the dba.
.SD MO_DBA_WRITE
instance may be written by the dba.
.SD MO_SERVER_READ
instance may be read by the server administrator.
.SD MO_SERVER_WRITE
instance may be written by the server administrator.
.SD MO_SYSTEM_READ
instance may be read by the system administrator.
.SD MO_SYSTEM_WRITE
instance may be written by the system administrator.
.SD MO_SECURITY_READ
instance may be read by the security officer.
.SD MO_SECURITY_WRITE
instance may be written by the security officer.
.P
Permissions may be modified with the following flag:
.SD MO_PERMANENT
may not be detached by anyone.
.P
The actual definitions are:
.(E
# define MO_SES_READ                    00000002
# define MO_SES_WRITE                   00000004

# define MO_DBA_READ                    00000020
# define MO_DBA_WRITE                   00000040

# define MO_SERVER_READ                 00000200
# define MO_SERVER_WRITE                00000400

# define MO_SYSTEM_READ                 00002000
# define MO_SYSTEM_WRITE                00004000

# define MO_SECURITY_READ               00020000
# define MO_SECURITY_WRITE              00040000

# define MO_PERMS_MASK                  00066666

/* all may read, or mask for someone may read */

# define MO_READ        ( MO_SES_READ | \e
                          MO_DBA_READ | MO_SERVER_READ | \e
                          MO_SYSTEM_READ | MO_SECURITY_READ )

/* all may write, or mask for someone may write */

# define MO_WRITE       ( MO_SES_WRITE | \e
                          MO_DBA_WRITE | MO_SERVER_WRITE | \e
                          MO_SYSTEM_WRITE | MO_SECURITY_WRITE )

# define MO_PERMANENT                   01000000
.)E
.S 3 "Message Types"
.P
Method functions and monitors are sent the the following message types
indicating the event to be handled.
.SD MO_GET
The instance is being queried for its value.
.SD MO_GETNEXT
This instance is being queried for the value of the next instance.
.SD MO_SET
The instance is being set to a new value.
.SD MO_SET_MONITOR
The class is getting a new monitor; used by clients only, never used
internally.
.SD MO_DEL_MONITOR
The class is deleting a monitor; used by clients only, never used
internally.
.S 3 "String persistance flags"
.P
The following values may be or-ed together in the flags fields of the
MO_CLASS_DEF and MO_INSTANCE_DEF structures discussed below.  If one
is set, it means the matching string needs to be saved in the string
cache because the caller knows it is unstable.
.SD MO_CLASSID_VAR
the classid string must be saved, because what is here is transient.
.SD MO_INDEX_VAR
the index string must be saved, because what is here is transient.
.SD MO_CDATA_VAR
the cdata is a string and must be saved, because what is here is transient.
.SD MO_INSTANCE_VAR
the instance string must be saved, because what is here is transient.
.SD MO_INDEX_CLASSID
Use the classid as the index, and ignore MO_INDEX_VAR.
.SD MO_CDATA_CLASSID
Use the classid as the cdata, and ignore MO_CDATA_VAR.
.SD MO_CDATA_INDEX
Use the index as the cdata, and ignore MO_CDATA_VAR.
.S 2 "Function Types"
.S 3 "MO_GET_METHOD Function type"
.P
.(E
typedef STATUS MO_GET_METHOD( nat offset,
                              nat objsize,
                              PTR object,
                              nat luserbuf,
                              char *userbuf );

.)E
.S 3 "MO_SET_METHOD Function Type"
.P
.(E
typedef STATUS MO_SET_METHOD( nat offset,
                              nat luserbuf,
                              char *userbuf,
                              nat objsize,
                              PTR object );

.)E
.S 3 "MO_INDEX_METHOD Function Type"
.P
.(E
/* Msg is either MO_GET or MO_GETNEXT */

typedef STATUS MO_INDEX_METHOD( nat msg,
                                PTR cdata,
                                nat linstance,
                                char *instance,
                                PTR *instdata );
.)E
.S 3 "MO_MONITOR_FUNC Function Type"
.P
A monitor function as expected by MOset_monitor and called directly by
MOtell_monitor, or indirectly by other functions.
.(E
typedef STATUS MO_MONITOR_FUNC( char *classid,
                                char *twinid,
                                char *instance,
                                char *value,
                                nat msg,
                                PTR mon_data );
.)E
.S 2 "Data Structures"
.S 3 "MO_CLASS_DEF structure"
.P
This is an element in a table passed to MOclassdef
.P
If the idx method is either MOidata_index or MOname_index, the cdata
must be the name of a classid; this may either be the classid (by
specifying MO_CDATA_CLASSID) or the index (by specifying
MO_CDATA_INDEX).
.(E
typedef struct
{
    nat         flags;          /* MO_CLASSID_VAR | MO_INDEX_VAR |
                                   MO_INDEX_CLASSID | MO_CDATA_CLASSID |
                                   MO_CDATA_INDEX */
    char        *classid;       /* name of the class */
    nat         size;           /* size of object */
    nat         perms;          /* permissions on all instances */
    char        *index;         /* classids forming the index,
                                   comma separated */
    nat         offset;         /* value given to get/set methods */
    MO_GET_METHOD *get;         /* get conversion */
    MO_SET_METHOD *set;         /* set conversion */

    PTR         cdata;          /* global data for classid, passed
                                   to index methods */
    MO_INDEX_METHOD *idx;       /* for get/getnext. */
} MO_CLASS_DEF;

.)E
.S 3 "MO_INSTANCE_DEF structure"
.P
This is an element in a table handed to MOattach or MOdetach.
MOdetach only looks at the classid and instance fields.
.(E
typedef struct
{
    nat         flags;          /* MO_INSTANCE_VAR or 0 */
    char        *classid;       /* the classid name */
    char        *instance;      /* the instance string */
    PTR         idata;          /* the data for the instance */
} MO_INSTANCE_DEF;
.)E
.S 2 "MACROS"
.S 3 "MO_SIZEOF_MEMBER \- compile time size of a structure member"

MO_SIZEOF_MACRO is a macro which yields the size in bytes of a member
within its structure in a way that is suitable for use in initializing
static variables at compile time.

MO_SIZEOF_MACRO is similar in feel to the CL_OFFSETOF the macro, and
used for similar purposes.
.SB Inputs:
.SI s_type
the name of a structure type
.SI m_name
the name of a member of the structure
.SB Outputs:
.SR None
.SB Returns:
.P
The size in bytes of member \fIm_name\fP in the structure \fIs_type\fP.
.SB Definition:
.(E
u_nat
MO_SIZEOF_MACRO(s_type, m_name)
.)E
.SB "Example:"
.(E
MO_CLASS_DEF My_classes[] =
{
    {
        0,
        "exp.back.myfacility.my_struct.field_one",
        MO_SIZEOF_MEMBER( MY_STRUCT, field_one ),
        MO_READ,
        0,
        CL_OFFSETOF( MY_STRUCT, field_one ),
        MOintget,
        MOnoset,
        (PTR)&My_struct,
        MOcdata_index
    },
    { 0 }
};
.)E
.P
.SB "Implementation Notes:"
While the``zero-points-to'' trick is appealing:
.(E
# define MO_SIZEOF_MEMBER(struct, member) (sizeof((struct *)0)->member)
.)E
It does not compile everywhere.  Instead, it's better to define
a dummy object and use it instead:
.(E
GLOBALREF ALIGN_RESTRICT        MO_sizeof_trick;

# define MO_SIZEOF_MEMBER(s_type, m_name) \e
		(sizeof( ((s_type *)&MO_sizeof_trick)->m_name) )
.)E
.bp
.S 1 "Executable Interface"
.S 2 "MOattach \- attach instances"

If MOon_off has been called with MO_DISABLE, returns OK without doing
anything.

Attachs an instance of a classid.  If a classid has a corresponding
object-id (as determined by looking up the classid in the
MO_META_OBJID class), it is attached under the oid as well.

If the
.i flags
field contains MO_INSTANCE_VAR, then the instance string will be saved
in allocated space, otherwise only a pointer will be kept.  The
.i idata
may be passed by the MOidata_index method to the get and set methods
for the class.  This pointer will usually identify the actual object
in question.

Allocates memory for the object using MEreqmem.

If the call succeeds, it must also call the monitors for the class
with the MO_ATTACH event, the instance id, and a NULL value.
.SB Inputs:
.SI flags
either 0 or MO_INSTANCE_VAR if the instance string is not in stable storage.
.SI classid
the classid of this class.
.SI instance
being attached.
.SI idata
the value to associate with the instance, picked out by the
MOidata_index method and handed to get and set methods.
.SB Returns:
.SI OK
is the object was attached successfully.
.SI MO_NO_CLASSID
if the classid is not defined.
.SI MO_ALREADY_ATTACHED
the object has already been attached.
.SI MO_NO_MEMORY
if the MO_MEM_LIMIT would be exceeded.
.SI MO_NO_STRING_SPACE
if the MO_STR_LIMIT would be exceeded.
.SI other
error statuses, possibly indicating allocation failure of some sort.
.SB Prototype:
.(E
STATUS MOattach( nat flags, char *classid, char *instance, PTR idata );
.)E
.bp
.S 2 "MOblankget \- get integer buffer of blanks method"
.P
This get method is for use with writable control objects.  It fills in
the output buffer with a string of blanks.  The offset, object, and
size are ignored.
.SB Inputs:
.SI offset
ignored.
.SI objsize
ignored.
.SI object
ignored.
.SI luserbuf
the length of the user buffer.
.SI userbuf
the buffer to use for output.
.SB Outputs:
.SI userbuf
Filled with blanks.
.SB Returns:
.SI OK
if the operation succeseded
.SB Prototype:
.(E
STATUS MOblankget( nat offset,
                 nat objsize,
                 PTR object,
                 nat luserbuf,
                 char *userbuf )
.)E
.bp
.S 2 "MOcdata_index \- standard index method for single instance class"
.P
Index for using the cdata of the class definition as the data
belonging to instance "0" of the class.  MO_GETNEXT never succeeds.
.P
If the instance is not the string
.i "0" ,
returns MO_NO_INSTANCE.  If the
.i msg
is MO_GET, uses the input
.i cdata
value as the output
.i instdata.
If
.i msg
is MO_GETNEXT, returns MO_NO_NEXT.
.SB Inputs:
.SI msg
either MO_GET or MO_GETNEXT
.SI cdata
class specific data that will be used as the output instdata for
instance "0" on an MO_GET.
.SI linstance
the length of the instance buffer
.SI instance
the instance in question.
.SI instdata
a pointer to a place to put instance-specific data.
.SB Outputs:
.SI instdata
filled in with appropriate idata for the attached instance.
.SB Returns:
.SI OK
if the operation succeeded.
.SI MO_NO_INSTANCE
if the requested instance does not exist.
.SI MO_NO_NEXT
if there is no instance following the specified one in the class.
.SB Prototype:
.(E
MO_INDEX_METHOD MOcdata_index;

STATUS MOcdata_index(nat msg,
                     PTR cdata,
                     nat linstance,
                     char *instance,
                     PTR *instdata );
.)E
.bp
.S 2 "MOclassdef \- define a class"

If MOon_off has been called with MO_DISABLE, returns OK without doing
anything.

Interprets up to
.i nelem
elements in the
.i classes
array as definitions of classes.  The array may be terminated by a
member having a NULL classid value.

For each class,
if the
.i flags
contain MO_CLASSID_VAR or MO_INDEX_VAR, then the string will be saved
in allocated space, otherwise only a pointer will be kept.  The object
.i size
is subsequently passed along to get and set methods, and is the same
for all instances.  The
.i perms
field will be AND-ed with valid permissions given to the MOget and
MOset functions to provide some security.  The
.i offset
field will be passed uninterpreted to the get and set methods for
their use.  The
.i get
band
.i set
fields define the functions that will be used to convert back and
forth between internal representations and character form.  The
.i cdata
field contains data that will be passed uninterpreted to the
.i idx
index method. The cdata allows the index method to know what class it
is being used for.  In particular, the MO provided MOname_index and
MOidata_index methods require that this be the name of a classid.
This is usually either the actual classid, by specifying
MO_CDATA_CLASSID, or the index class, by specifying MO_CDATA_INDEX.

Returns OK if the class was defined sucessfully.  If the class is
already defined, MOattach will return OK if the current definition is
the same as the old one.  If it is not, returns MO_INCONSISTENT_CLASS,
and the original definition remains unaffected.

Allocates memory for definition using MEreqmem.
.SB Inputs:
.SI nelem
the maximum number of elements in the table to define
.SI classes
A pointer to an array of class definitions, terminated with one having
a null classid field.
.SB Outputs:
.SI none
.SB Returns:
.SI OK
is the object was attached successfully.
.SI MO_INCONSISTENT_CLASS
the object has already been attached.
.SI MO_NO_MEMORY
if the MO_MEM_LIMIT would be exceeded.
.SI MO_NO_STRING_SPACE
if the MO_STR_LIMIT would be exceeded.
.SI MO_NULL_METHOD
A null get or set method was provided, but the permissions said get or
set was allowed.
.SI other
system specific error status, possibly indicating allocation failure.
.SB Prototype:
.(E
STATUS MOclassdef( nat nelem, MO_CLASS_DEF *classes );
.)E
.SB Example:
.(E
.)E
.bp
.S 2 "MOdetach \- detach an attached instance"

Detaches an attached instance.  Subsequent attempts to get or set
it will fail, and an attempts to attach it will succeed.

Frees memory allocated by MOattach using MEfree.

If the call succeeds, it must also call the monitors for the class
with the MO_DETACH event, the instance id, and a NULL value.
.SB Inputs:
.SI classid
the classid of the object.
.SI instance
the instance of the object.
.SB Outputs:
.SI none
.SB Returns:
.SI OK
if the classid was detached.
.SI MO_NO_INSTANCE
if classid is not attached.
.SI MO_NO_DETACH
if the object was attached as MO_PERMANENT.
.SB Prototype:
.(E
STATUS MOdetach( char *classid, char *instance );
.)E
.bp
.S 2 "MOget \- get data associated with an instance."

Return the string value of the the requested instance.  Returns
MO_NO_INSTANCE if the object doesn't exist.  If the object exists but
is not readable, returns MO_NO_READ.
.P
Existance of the object is determined by calling the index method for
the class with the MO_GET method.  If this returns OK, then the
instance value it provided is passed to the get method for the class,
which fills in the buffer supplied to MOget.

If the call succeeds, it must also call the monitor for the class with
the MO_GET event, the instance id, and the current value.
.SB Inputs:
.SI valid_perms
the roles currently in effect, to be and-ed with object permissions to
determine access rights.
.SI classid
the classid in question.
.SI instance
the instance in question.
.SI lsbufp
pointer to length of the string buffer handed in, to be used for
output if needed.
.SB Outputs:
.SI lsbufp
filled in with the length of the string written to
sbuf.
.SI sbuf
contains the retrieved string value.
.SI perms
the permissions for the classid.
.SB Returns:
.SI OK
if the retrieval returned data.
.SI MO_NO_INSTANCE
if class is not attached, or if the object permissions are non-zero
but do not include the
.i valid_perms
bits.
.SI MO_NO_READ
if a user with 
.i valid_perms
isn't allowed to read it.
.SI MO_VALUE_TRUNCATED
if the value wouldn't fit in the provided sbuf.
.SI "other error status"
returned by a method.
.SB Prototype:
.(E
STATUS MOget(   nat valid_perms,
                char *classid,
                char *instance,
                nat *lsbufp,
                char *sbuf;
                nat *perms );
.)E
.SB Example:
.(E
.)E
.bp
.S 2 "MOgetnext \- get next object in class, instance order."

Given an object, attempts to return the next one that is readable.
Ordering is as by STcompare of { classid, instance } of objects
including the input
.i valid_perms
bits.

As important special cases, if the classid and instance are both a 0
length strings, then return the first of any instances in the class
space that match its valid_perms.  If only the instance is a 0 length
string, then the return is the first instance of that or a subseqent
class.

If the input instance does not exist or doesn't include valid_perms,
returns MO_NO_INSTANCE, and the output class is untouched.  If there
is no readable successor to the input object, returns MO_NO_NEXT.

If the classid of the returned object won't fit in the provided
buffer, returns MO_CLASSID_TRUNCATED.  If the instance of the returned
object won't fit in the provided buffer, returns
MO_INSTANCE_TRUNCATED.  If the value returned won't fit in the
supplied buffer, returns MO_VALUE_TRUNCATED.  Any of these truncation
errors is a serious programming error, which should abort processing
as it may be impossible to continue a get next scan from that point.
Client buffers should always be sized big enough to avoid truncation,
so truncation is a serious programming error.  MOgetnext will not work
as desired when handed a truncated classid or instance for a
subsequent call.

Existance of the input object is determined by calling the idx method
for the class with the MO_GETNEXT message.

If the call succeeds, it must also call the monitors for the class
with the MO_GET event, the instance id, and the current value.
.SB Inputs:
.SI valid_perms
the roles currently in effect, to be and-ed with object permissions to
determine access rights.
.SI lclassid
length of the buffer for the classid.
.SI classid
the classid in question.
.SI linstance
length of the buffer for the instance.
.SI instance
the instance in question.
.SI lsbufp
pointer to the length of the output string buffer.
.SB Outputs:
.SI classid
filled in with the new classid.
.SI instance
filled in with the new instance.
.SI lsbufp
if *sval is NULL, contains the actual length of the
value in sbuf.
.SI sbuf
if sval is NULL, a copy of the output string.  If it wasn't long
enough, returns MO_TRUNCATED.
.SI perms
the permissions for the returned instance.
.SB Returns:
.SI OK
if the retrieval returned data.
.SI MO_CLASSID_TRUNCATED
buffer for classod was too small, and output was truncated.
.SI MO_INSTANCE_TRUNCATED
buffer for instance was too small and output was truncated.
.SI MO_VALUE_TRUNCATED
buffer for value was too small, and output was truncated.
.SI MO_NO_INSTANCE
the input class/instance did not exist, or it's permissions didnot
include readable
.i valid_perms
bits
.SI MO_NO_NEXT
ther was no readable object following the input one.
.SI other
error status, returned by a method.
.SB Prototype:
.(E
STATUS MOgetnext(nat valid_perms,
                nat lclassid,
                nat linstance,
                char *classid,
                char *instance,
                nat *lsbufp,
                char *sbuf;
                nat *perms );
.)E
.SB Example:
.(E
.)E
.bp
.S 2 "MOidata_index \- standard index method for data of attached objects"
.P
Index for instance data belonging to objects known through MOattach
calls, for handing the user object pointer to the get/set/method.
This is the normally used index method cdata of a classid, where there
is only one instance of the class, known at the time of class
definition.
.P
The input
.i cdata
is assumed to be a pointer to a string holding the classid in
question, usually by defining the class with MO_CDATA_CLASSID or
MO_CDATA_INDEX.  If
.i msg
is MO_GET, determine whether the requested instance exists.  If it
does, fill in
.i instdata
with the
.i idata
pointer provided in the attach call for use by a get or set method,
and return OK.  If the requested instance does not exist, return
MO_NO_INSTANCE.
.P
If the msg is MO_GETNEXT, see if the requested instance exists; if not
return MO_NO_INSTANCE.  Then locate the next instance in the class.
If there is no successor instance in the class, return MO_NO_NEXT.  If
there is a sucessor, replace the input instance with the one found.
Fill in
.i instdata
with the idata supplied when the instance was attached, for use by the
get/set methods defined for the class.  If the new instance won't fit
in the given buffer (as determined by
.i linstance ),
return MO_INSTANCE_TRUNCATED, otherwise return OK.
.SB Inputs:
.SI msg
either MO_GET or MO_GETNEXT
.SI cdata
class specific data from the class definition, assumed to be a pointer
to the classid string.
.SI linstance
the length of the instance buffer
.SI instance
the instance in question.
.SI instdata
a pointer to a place to put instance-specific data.
.SB Outputs:
.SI instdata
filled in with appropriate idata for the attached instance.
.SB Returns:
.SI OK
if the operation succeeded.
.SI MO_NO_INSTANCE
if the requested instance does not exist.
.SI MO_NO_NEXT
if there is no instance following the specified one in the class.
.SI MO_INSTANCE_TRUNCATED
if the output instance of a GET_NEXT would not fit in the provided
buffer.
.SB Prototype:
.(E
MO_INDEX_METHOD MOidata_index;

STATUS MOidata_index(nat msg,
                     PTR cdata,
                     nat linstance,
                     char *instance,
                     PTR *instdata );
.)E
.bp
.S 2 "MOintget \- standard get integer at an address method"
.P
Convert the signed integer at the object location to a character
string.  This is often the wrong thing to use, as negative numbers are rare.
The
.i offset
is treated as a byte offset to the input
.i object .
They are added together to get the object location.


The output
.i userbuf ,
has a maximum length of
.i luserbuf
that will not be exceeded.  If the output is bigger than the buffer, it
will be chopped off and MO_VALUE_TRUNCATED returned.
.P
The
.i objsize
comes from the class definition, and is the length of the integer,
one of sizeof(i1), sizeof(i2), or sizeof(longnat).
.SB Inputs:
.SI offset
value from the class definition, taken as byte offset to the object
pointer where the data is located.
.SI objsize
the size of the integer, from the class definition.
.SI object
a pointer to the integer to convert.
.SI luserbuf
the length of the user buffer.
.SI userbuf
the buffer to use for output.
.SB Outputs:
.SI userbuf
contains the string of the value of the instance.
.SB Returns:
.SI OK
if the operation succeseded
.SI MO_VALUE_TRUNCATED
the output buffer was too small, and the string was truncated.
.SI other
conversion-specific failure status as appropriate.
.SB Prototype:
.(E
STATUS MOintget( nat offset,
                 nat objsize,
                 PTR object,
                 nat luserbuf,
                 char *userbuf )
.)E
.bp
.S 2 "MOintset \- standard set integer method"
.P
Convert a caller's string to an signed integer for the instance.
This is often the wrong thing to use, as there aren't many things that
can happily take negative values.
.P
The
.i offset
is treated as a byte offset to the input
.i object .
They are added together to get the object location.

The input
.i objsize
is the size of the output integer, as in sizeof(i1), sizeof(i2), or
sizeof(i4).  The object location is taken to be the address of the
integer.  The
.i userbuf
contains the string to convert, and
.i luserbuf.
is ignored.
.SB Inputs:
.SI offset
value from the class definition, taken as byte offset to the object
pointer where the data is located.
.SI luserbuf
the length of the user buffer, ignored.
.SI userbuf
the user string to convert.
.SI objsize
the size of the integer, from the class definition.
.SI object
a pointer to the integer.
.SB Outputs:
.SI object
Modified by the method.
.SB Returns:
.SI OK
if the operation succeseded
.SI MO_BAD_SIZE
if the object size isn't something that can be handled.
.SI other
conversion-specific failure status as appropriate.
.SB Prototype
.(E
MO_SET_METHOD MOintset;

STATUS MOintset( nat offset,
                nat luserbuf,
                char *userbuf,
                nat objsize,
                PTR object )
.)E
.bp
.S 2 "MOivget \- standard get integer value method"
.P
The input object location is treated as a signed longnat and retrieved.
The output
.i userbuf ,
has a maximum length of
.i luserbuf
that will not be exceeded.  If the output is bigger than the buffer, it
will be chopped off and MO_VALUE_TRUNCATED returned.
.P
The
.i objsize
is ignored.
.P
The
.i offset
is treated as a byte offset to the input
.i object .
They are added together to get the object location.
.SB Inputs:
.SI offset
value from the class definition, taken as byte offset to the object
pointer where the data is located.
.SI objsize
the size of the integer, from the class definition.
.SI object
a pointer to the integer to convert.
.SI luserbuf
the length of the user buffer.
.SI userbuf
the buffer to use for output.
.SB Outputs:
.SI userbuf
contains the string of the value of the instance.
.SB Returns:
.SI OK
if the operation succeseded
.SI MO_VALUE_TRUNCATED
the output buffer was too small, and the string was truncated.
.SI other
conversion-specific failure status as appropriate.
.SB Prototype:
.(E
STATUS MOivget( nat offset,
                 nat objsize,
                 PTR object,
                 nat luserbuf,
                 char *userbuf )
.)E
.bp
.S 2 "MOlongout \- output utility for MO methods to put signed longnats"
.P
Turn the input longnat into a fixed width, 0 padded string and copy it
to the output buffer up to a maximum length.  Will write a leading
minus sign if needed.  The fixed width is so string comparisons will
yield numeric sort ordering.
.SB Inputs:
.SI errstat
the status to return if the output buffeer isn't big enough.
.SI val
the value to write as a string.
.SI destlen
the length of the user buffer.
.SI dest
the buffer to use for output
.SB Outputs:
.SI dest
contains a copy of the string value.
.SB Returns:
.SI OK
if the operation succeeded.
.SI errstat
if the output buffer was too small and the value was chopped off at
the end to fit.
.SB Prototype:
.(E
STATUS MOlongout( STATUS errstat, longnat val, nat destlen, char *dest );
.)E
.bp
.S 2 "MOlstrout \- string output with bounded input"

Copy the input string to the output, knowing the length of both the
input and output. If input won't fit, return the given error status.
Copy terminates OK on EOS in the input or copying all the characters
in the known input length.
.SB Re-entrancy:
yes.
.SB Inputs
.SI errstat
value to return on error.
.SI srclen
maximum length of the source string
.SI src
the source string
.SI destlen
length of the output buffer.
.SI dest
the destination buffer.
.SB Outputs:
.SI dest
gets a copy of the source string.
.SB Returns:
.SI OK
or input errstat value.
.SB Prototype:
.(E
STATUS MOlstrout( STATUS errstat, 
		  nat srclen, 
		  char *src, 
		  nat destlen, 
		  char *dest );
.)E
.S 2 "MOname_index \- standard index for names of attached instances"

.P
Method for producing ``index'' objects whose value is the instance.
This is almost like MOidata_index, except that the instdata is filled
with a pointer to the instance string (for use with MOstrget) instead
of the idata associated with the instance.
.P
The
.i cdata
is assumed to point to the classid string.
If
.i msg
is MO_GET, determine whether the requested instance exists.  If it
does, fill in
.i instdata
with a pointer to the instance string and return OK.  If the requested
instance does not exist, return MO_NO_INSTANCE.
.P
If the msg is MO_GETNEXT, see if the requested instance exists; if not
return MO_NO_INSTANCE.  Then locate the next instance in the class.
If there is no successor instance in the class, return MO_NO_NEXT.  If
there is a sucessor, replace the input instance with the one found.
Fill in
.i instdata
with a pointer to the instance string.  If the new instance won't fit
in the given buffer (as determined by
.i linstance ),
return MO_INSTANCE_TRUNCATED, otherwise return OK.
.SB Inputs:
.SI msg
either MO_GET or MO_GETNEXT
.SI cdata
class specific data pointing to the class string.
.SI linstance
the length of the instance buffer
.SI instance
the instance in question
.SI instdata
place to put a pointer to the instance string.
.SB Outputs:
.SI instdata
filled in with a pointer to the instance string.
.SB Returns:
.SI OK
if the operation succeeded.
.SI MO_NO_INSTANCE
if the requested instance does not exist.
.SI MO_NO_NEXT
if there is no instance following the specified one in the class.
.SI MO_INSTANCE_TRUNCATED
if the output instance of a GET_NEXT would not fit in the provided
buffer.
.SB Prototype:
.(E
MO_INDEX_METHOD MOname_index;

STATUS MOname_index(nat msg,
                    PTR cdata,
                    nat linstance,
                    char *instance,
                    PTR *instdata );
.)E
.bp
.S 2 "MOnoset \- standard set method for read-only objects"
.P
Don't do anything, and return MO_NO_WRITE.  Commonly used as the set
method for non-writable classids.
.SB Inputs:
.SI offset
ignored.
.SI luserbuf
ignored.
.SI userbuf
ignored.
.SI objsize
ignored.
.SI object
ignored.
.SB Outputs:
.SI object
ignored.
.SB Returns:
.SI MO_NO_WRITE
.SB Prototype
.(E
MO_SET_METHOD MOnoset;

STATUS MOnoset( nat offset,
                nat luserbuf,
                char *userbuf,
                nat objsize,
                PTR object )
.)E
.bp
.S 2 "MOon_off \- enable/disable MO attach (for client programs)"
.P
Turns MOattach off while having calls to it return OK.  This is
intended to be used by client programs that do not want the expense of
MO operations, but which are constructed from libraries that would
otherwise define MO objects.  All requests return the old state;
MO_INQUIRE returns the old state without changing it.

By default, the state is enabled.

Queries with MOget and MOgetnext on objects attached when MO was
enabled will always work.
.SB Inputs:
.SI operation
one of MO_ENABLE. MO_DISABLE, or MO_INQUIRE.
.SB Outputs:
.SI old_state
the previous state, either MO_ENABLED or MO_DISABLED.
.SB Returns:
.SI OK
or other error status.
.SB Prototype:
.(E
STATUS MOon_off( nat operation, nat *old_state );
.)E
.bp
.S 2 "MOptrget \- get pointer at a pointer method"
.P
Convert the pointer at the object location to a character string.
The
.i offset
is treated as a byte offset to the input
.i object .
They are added together to get the object location.

The output
.i userbuf ,
has a maximum length of
.i luserbuf
that will not be exceeded.  If the output is bigger than the buffer, it
will be chopped off and MO_VALUE_TRUNCATED returned.
.P
The
.i objsize
is ignored.
.SB Inputs:
.SI offset
value from the class definition, taken as byte offset to the object
pointer where the data is located.
.SI objsize
ignored
.SI object
a pointer to the pointer to convert.
.SI luserbuf
the length of the user buffer.
.SI userbuf
the buffer to use for output.
.SB Outputs:
.SI userbuf
contains the string of the value of the instance.
.SB Returns:
.SI OK
if the operation succeseded
.SI MO_VALUE_TRUNCATED
the output buffer was too small, and the string was truncated.
.SI other
conversion-specific failure status as appropriate.
.SB Prototype:
.(E
STATUS MOptrget( nat offset,
                 nat objsize,
                 PTR object,
                 nat luserbuf,
                 char *userbuf )
.)E
.bp
.S 2 "MOptrout \- output routine for methods to put a pointer value"
.P
Turn the input PTR into a fixed width, 0 padded string and copy it to
the output buffer up to a maximum length.  The fixed width is so
string comparisons will yield numeric sort ordering.
.SB Inputs:
.SI errstat
the status to return if the output buffeer isn't big enough.
.SI ptr
the pointer value to write as a string.
.SI destlen
the length of the user buffer.
.SI dest
the buffer to use for output
.SB Outputs:
.SI dest
contains a copy of the string value.
.SB Returns:
.SI OK
if the operation succeeded.
.SI errstat
if the output buffer was too small and the value was chopped off at
the end to fit.
.SB Prototype:
.(E
STATUS MOptrout( STATUS errstat, PTR ptr, nat destlen, char *dest );
.)E
.bp
.S 2 "MOpvget \- get pointer method"
.P
The input object location is treated as a pointer and retrieved.
The output
.i userbuf ,
has a maximum length of
.i luserbuf
that will not be exceeded.  If the output is bigger than the buffer, it
will be chopped off and MO_VALUE_TRUNCATED returned.
.P
The
.i objsize
is ignored.
.P
The
.i offset
is treated as a byte offset to the input
.i object .
They are added together to get the object location.
.SB Inputs:
.SI offset
value from the class definition, taken as byte offset to the object
pointer where the data is located.
.SI objsize
ignored.
.SI object
a pointer.
.SI luserbuf
the length of the user buffer.
.SI userbuf
the buffer to use for output.
.SB Outputs:
.SI userbuf
contains the string of the value of the instance.
.SB Returns:
.SI OK
if the operation succeseded
.SI MO_VALUE_TRUNCATED
the output buffer was too small, and the string was truncated.
.SI other
conversion-specific failure status as appropriate.
.SB Prototype:
.(E
STATUS MOpvget( nat offset,
		nat objsize,
		PTR object,
		nat luserbuf,
		char *userbuf )
.)E
.bp
.S 2 "MOset \- set value associated with an instance"

Attempts to set the value associated with the instance.  

If the call succeeds, it must also call the monitors for the class
with the MO_SET event, the instance id, and the new value.
.SB Inputs:
.SI valid_perms
the roles currently in effect, to be and-ed with object permissions to
determine access rights.
.SI class
the class whose value should be altered.
.SI instance
the instance in question.
.SI val
pointer to the string containing the new value.
.SB Outputs:
.SI none
.SB Returns:
.SI OK
if the value was set
.SI MO_NO_INSTANCE
if the class is not attached
.SI MO_NO_WRITE
if the class may not be changed.
.SI other
error status returned by a method.
.SB Prototype:
.(E
STATUS MOset(   nat valid_perms,
                char *classid,
                char *instance,
                char *val );
.)E
.SB Example:
.(E
STATUS
mysetint( char *classid, char *instance, longnat val )
{
    char buf[ 20 ];
    CVla( val, buf );
    return( MOset( ~0, classid, instance, buf ) );
}
.)E
.bp
.S 2  "MOset_monitor \- set monitor function for a class"

Arranges to have a monitor function called as a direct result of an
MOtell_monitor call or indirectly from other operations.  The monitors
for a classid and it's twin are kept identical.  There may be multiple
monitors for a classid.  Monitors are installed for both the classid
and it's twin.  The monitor is uniquely identified by it's
.i classid
and
.i mon_data
value.  (The same mon_data may be used for multiple classids.)

A monitor is deleted by specifying a NULL monitor function for a
{classid, mon_data} pair.

The output parameter
.i old_monitor
is filled in with the previous monitor for the {classid, mon_data}.

At present the 
.i qual_regexp
is ignored and all potential calls to the monitor are delivered.  In
the future (when there's a GL regular expression module to call), the
candidate instances will be qualified by the regular expression
provided when the monitor was set.  It the instance matches, or the
expression was NULL, then the monitor would be called.

The monitor is called with the classid, the classid of the twin, if
any, the qualified instance, a value, a message type, and the mon_data
value.

If the class is not currently defined, the call fails with
MO_NO_CLASSID.  It may also fail for inability to get memory.

The new monitor function is not called by the MOset_monitor call.
.SB Inputs:
.SI classid
the classid of the object to be monitored.
.SI mon_data
data to be handed to the MO_MONITOR_FUNC for the classid.
.SI monitor
the function to call for events affecting the instance.
.SB Outputs:
.SI none
.SB Returns:
.SI OK
if the monitor was attached.
.SI MO_NO_CLASSID
if the classid wasn't defined.
.SI MO_BAD_MONITOR
if the {classid, mon_data} doesn't exist and the new monitor is NULL
(trying to delete one that doesn't exist).
.SI MO_MEM_LIMIT_EXCEEDED
couldn't allocate memory for the monitor.
.SB Prototype:
.(E
STATUS
MOset_monitor(  char *classid,
                PTR mon_data,
                char *qual_regexp
                MO_MONITOR_FUNC *monitor
                MO_MONITOR_FUNC **old_monitor );
.)E
.bp
.S 2 "MOstrget \- standard get string buffer method"
.P
Copy the instance string to the output buffer
.i userbuf ,
to a maximum length of
.i luserbuf .
If the output is bigger than the buffer, chop it off and return
MO_VALUE_TRUNCATED.
.P
The
.i objsize
from the class definition is ignored.  The
.i object
plus the
.i offset
is treated as a pointer to a character string to be copied.
.SB Inputs:
.SI offset
value from the class definition, taken as byte offset to the object
pointer where the data is located.
.SI objsize
the size of the object from the class definition, ignored.
.SI object
a pointer to a character string for the instance to copy out.
.SI luserbuf
the length of the user buffer.
.SI userbuf
the buffer to use for output.
.SB Outputs:
.SI userbuf
contains a copy of the the string value.
.SB Returns:
.SI OK
if the operation succeseded
.SI MO_VALUE_TRUNCATED
if the output buffer was too small and the value was chopped off to fit.
.SI other
method-specific failure status as appropriate.
.SB Prototype:
.(E
MO_GET_METHOD MOstrget;

STATUS MOstrget( nat offset,
                nat objsize,
                PTR object,
                nat luserbuf,
                char *userbuf )
.)E
.bp
.S 2 "MOstrout \- output utility for MO methods to put strings"
.P
Copy the input string to the output buffer up to a maximum length.
If the length is exceeded, return the specified error status.
.SB Inputs:
.SI errstatus
the status to return if the output buffer isn't big enough.
.SI str
the string to write out
.SI l
a pointer to a character string for the instance to copy out.
.SI luserbuf
the length of the user buffer.
.SI userbuf
the buffer to use for output.
.SB Outputs:
.SI userbuf
contains a copy of the the string value.
.SB Returns:
.SI OK
if the operation succeseded
.SI MO_VALUE_TRUNCATED
if the output buffer was too small and the value was chopped off to fit.
.SI other
method-specific failure status as appropriate.
.SB Prototype:
.(E
MO_GET_METHOD MOstrget;

STATUS MOstrget( nat offset,
                nat objsize,
                PTR object,
                nat luserbuf,
                char *userbuf )
.)E
.bp
.S 2 "MOstrpget \- standard get string pointer method"
.P
Copy the string pointed to by instance object to the output
.i userbuf ,
to a maximum length of
.i luserbuf .
If the output is bigger than the buffer, chop it off and return
MO_VALUE_TRUNCATED.
.P
The
.i offset
and
.i objsize
from the class definition are ignored.
The
.i object
is considered a pointer to a string pointer, and will be dereferenced
to find the source string.
.SB Inputs:
.SI offset
value from the class definition, taken as byte offset to the object
pointer where the data is located.
.SI objsize
the size of the object from the class definition, ignored.
.SI object
treated as a pointer to a string pointer.
.SI luserbuf
the length of the user buffer.
.SI userbuf
the output buffer.
.SB Outputs:
.SI userbuf
contains the string of the value of the instance.
.SB Returns:
.SI OK
if the operation succeseded
.SI MO_VALUE_TRUNCATED
if the output buffer was too small, and the value was chopped off to fit.
.SI other
method-specific failure status as appropriate.
.SB Prototype:
.(E
MO_GET_METHOD MOstrpget;

STATUS MOstrpget( nat offset,
                 nat objsize,
                 PTR object,
                 nat luserbuf,
                 char *userbuf )
.)E
.bp
.S 2 "MOstrset \- standard set string buffer method"
.P
Copy a caller's string to the buffer for the instance.
.P
The
.i object
plus the
.i offset
is treated as a pointer to a character buffer having
.i objsize
bytes.  The input
.i luserbuf
is ignored.  The string in
.i userbuf
is copied up to objsize; if it wouldn't fit, returns
MO_VALUE_TRUNCATED, otherwise OK.
.SB Inputs:
.SI offset
value from the class definition, taken as byte offset to the object
pointer where the data is located.
.SI luserbuf
the length of the user buffer, ignored.
.SI userbuf
the user string to convert.
.SI objsize
the size of the output buffer.
.SI object
interpreted as a character buffer.
.SB Outputs:
.SI object
gets a copy of the input string.
.SB Returns:
.SI OK
if the operation succeseded
.SI MO_VALUE_TRUNCATED
if the output buffer wasn't big enough.
.SB Prototype
.(E
MO_SET_METHOD MOstrset;

STATUS MOstrset( nat offset,
                nat luserbuf,
                char *userbuf,
                nat objsize,
                PTR object );
.)E
.bp
.S 2 "MOtell_monitor \- call class monitor function for an instance"

Consider call any monitor functions for the classid and it's twin with
the specified value and message.  The instance provided to the
MOtell_monitor call will be qualified by the qual_regexps provided
with each monitor instance; only those which match will be called.

If the class is not defined or no monitor exists, returns OK.

Returns the status returned by the called monitor functions.  It stops
calling monitors when one returns error status.  In most cases this
should be ignored, because the caller of MOtell_monitor has no idea
who is being called.
.SB Prototype:
.(E
STATUS MOtell_monitor( char *classid, nat instance, char *value, nat msg );
.)E
.bp
.S 2 "MOuintget \- get method for unsigned integer to string"
.P
Convert the unsigned integer at the object location to a character
string.  This is often the wrong thing to use, as negative numbers are
rare.  The
.i offset
is treated as a byte offset to the input
.i object .
They are added together to get the object location.

The output
.i userbuf ,
has a maximum length of
.i luserbuf
that will not be exceeded.  If the output is bigger than the buffer, it
will be chopped off and MO_VALUE_TRUNCATED returned.
.P
The
.i objsize
comes from the class definition, and is the length of the unsigned
integer, one of sizeof(i1), sizeof(i2), or sizeof(longnat).
.SB Inputs:
.SI offset
value from the class definition, taken as byte offset to the object
pointer where the data is located.
.SI objsize
the size of the integer, from the class definition.
.SI object
a pointer to the integer to convert.
.SI luserbuf
the length of the user buffer.
.SI userbuf
the buffer to use for output.
.SB Outputs:
.SI userbuf
contains the string of the value of the instance.
.SB Returns:
.SI OK
if the operation succeseded
.SI MO_VALUE_TRUNCATED
the output buffer was too small, and the string was truncated.
.SI other
conversion-specific failure status as appropriate.
.SB Prototype:
.(E
STATUS MOuintget( nat offset,
                 nat objsize,
                 PTR object,
                 nat luserbuf,
                 char *userbuf )
.)E
.bp
.S 2 "MOuintset \- standard set unsigned integer method"
.P
Convert a caller's string to an unsigned integer for the instance.
.P
The
.i offset
is treated as a byte offset to the input
.i object .
They are added together to get the object location.

The input
.i objsize
is the size of the output integer, as in sizeof(i1), sizeof(i2), or
sizeof(i4).  The object location is taken to be the address of the
output.  The
.i userbuf
contains the string to convert, and
.i luserbuf.
is ignored.
.SB Inputs:
.SI offset
value from the class definition, taken as byte offset to the object
pointer where the data is located.
.SI luserbuf
the length of the user buffer, ignored.
.SI userbuf
the user string to convert.
.SI objsize
the size of the integer, from the class definition.
.SI object
a pointer to the integer.
.SB Outputs:
.SI object
Modified by the method.
.SB Returns:
.SI OK
if the operation succeseded
.SI MO_BAD_SIZE
if the object size isn't something that can be handled.
.SI other
conversion-specific failure status as appropriate.
.SB Prototype
.(E
MO_SET_METHOD MOuintset;

STATUS MOuintset( nat offset,
                nat luserbuf,
                char *userbuf,
                nat objsize,
                PTR object )
.)E
.bp
.S 2 "MOuivget \- standard get unsigned integer value method"
.P
The input object location is treated as a unsigned longnat and retrieved.
The output
.i userbuf ,
has a maximum length of
.i luserbuf
that will not be exceeded.  If the output is bigger than the buffer, it
will be chopped off and MO_VALUE_TRUNCATED returned.
.P
The
.i objsize
is ignored.
.P
The
.i offset
is treated as a byte offset to the input
.i object .
They are added together to get the object location.
.SB Inputs:
.SI offset
value from the class definition, taken as byte offset to the object
pointer where the data is located.
.SI objsize
the size of the integer, from the class definition.
.SI object
a pointer to the integer to convert.
.SI luserbuf
the length of the user buffer.
.SI userbuf
the buffer to use for output.
.SB Outputs:
.SI userbuf
contains the string of the value of the instance.
.SB Returns:
.SI OK
if the operation succeseded
.SI MO_VALUE_TRUNCATED
the output buffer was too small, and the string was truncated.
.SI other
conversion-specific failure status as appropriate.
.SB Prototype:
.(E
STATUS MOuivget( nat offset,
                 nat objsize,
                 PTR object,
                 nat luserbuf,
                 char *userbuf )
.)E
.bp
.S 2 "MOulongout \- output utility for MO methods to put unsigned longnats"
.P
Turn the input unsigned longnat into a fixed width, 0 padded string
and copy it to the output buffer up to a maximum length.  The fixed
width is so string comparisons will yield numeric sort ordering.
.SB Inputs:
.SI errstat
the status to return if the output buffeer isn't big enough.
.SI val
the value to write as a string.
.SI destlen
the length of the user buffer.
.SI dest
the buffer to use for output
.SB Outputs:
.SI dest
contains a copy of the string value.
.SB Returns:
.SI OK
if the operation succeeded.
.SI errstat
if the output buffer was too small and the value was chopped off at
the end to fit.
.SB Prototype:
.(E
STATUS MOulongout( STATUS errstat, u_longnat val, nat destlen, char *dest );
.)E
.bp
.S 2 "MOzeroget \- get buffer with zero value method"
.P
This get method is for use with writable control objects.  It writes
a buffer of the string of the value zero, as if MOivget had been
called with object of zero.
.SB Inputs:
.SI offset
ignored.
.SI objsize
ignored.
.SI object
ignored.
.SI luserbuf
the length of the user buffer.
.SI userbuf
the buffer to use for output.
.SB Outputs:
.SI userbuf
Filled with stirng value of zero.
.SB Returns:
.SI OK
if the operation succeseded
.SB Prototype:
.(E
STATUS MOzeroget( nat offset,
                 nat objsize,
                 PTR object,
                 nat luserbuf,
                 char *userbuf )
.)E
.bp
.S 2 "MO_INDEX_METHOD \- example index method"
.P
The input
.i cdata
is provided from the class definition to allow one index method to be
used for multiple classes.
If
.i msg
is MO_GET, determine whether the requested instance exists.  If it
does, fill in
.i instdata
with a pointer that can be handed to a get or set method, and return OK.
If the requested instance does not exist, return MO_NO_INSTANCE.
.P
If the msg is MO_GETNEXT, see if the instance is an empty string.  If
so, returnthe first instance in the class (or MO_NO_NEXT if there
aren't any.)  If not, see if the requested instance exists; if it doesn't
return MO_NO_INSTANCE.  Then locate the next instance in the class.
If there is no successor instance in the class, return MO_NO_NEXT.  If
there is a sucessor, replace the input instance with the one found.
Fill in
.i instdata
with a pointer appropriate for the get/set methods defined for the
class.  If the new instance won't fit in the given buffer (as
determined by
.i linstance ),
return MO_INSTANCE_TRUNCATED, otherwise return OK.
.SB Inputs:
.SI msg
either MO_GET or MO_GETNEXT
.SI cdata
class specific data from the class definition.
.SI linstance
the length of the instance buffer
.SI instance
the instance in question
.SI instdata
a pointer to a place to put instance-specific data.
.SB Outputs:
.SI instdata
filled in with appropriate instance-specific data for use by get and
set methods.
.SB Returns:
.SI OK
if the operation succeeded.
.SI MO_NO_INSTANCE
if the requested instance does not exist.
.SI MO_NO_NEXT
if there is no instance following the specified one in the class.
.SI MO_INSTANCE_TRUNCATED
if the output instance of a GET_NEXT would not fit in the provided
buffer.
.SB Prototype:
.(E
MO_INDEX_METHOD my_index_method;

STATUS my_index_method(nat msg,
                       PTR cdata,
                       nat linstance,
                       char *instance,
                       PTR *instdata )
.)E
.bp
.S 2 "MO_GET_METHOD \- example get method"
.P
Convert the internal representation of an object to a character string
in
.i userbuf ,
to a maximum length of
.i luserbuf .
If the output is bigger than the buffer, chop it off and return
MO_VALUE_TRUNCATED.
.P
The
.i offset
and
.i objsize
are those from the class definition, and may be used in any way
desired to affect the conversion.  The
.i object
is provided by an index method to identify the instance in question.
.SB Inputs:
.SI offset
value from the class definition, often taken as byte offset to the
object pointer where the data is located.
.SI objsize
the size of the object,  from the class definition.
.SI object
the object pointer provided by an index method.
.SI luserbuf
the length of the user buffer.
.SI userbuf
the buffer to use for output.
.SB Outputs:
.SI userbuf
contains the string of the value of the instance.
.SB Returns:
.SI OK
if the operation succeseded
.SI other
method-specific failure status as appropriate.
.SB Prototype:
.(E
MO_GET_METHOD my_get_method;

STATUS my_get_method( nat offset,
                    nat objsize,
                    PTR object,
                    nat luserbuf,
                    char *userbuf )
.)E
.bp
.S 2 "MO_SET_METHOD \- example set method"
.P
Convert a caller's string to an internal representation of an object.
The
.i offset
and
.i objsize
are those provided by the class definition.  The output
.i object
is provided by an index method.  The input
.i userbuf
is of length
.i luserbuf.
.SB Inputs:
.SI offset
value from the class definition, often taken as byte offset to the
object pointer where the data is located.
.SI luserbuf
the length of the user buffer.
.SI userbuf
the user string to convert.
.SI objsize
the size of the object,  from the class definition.
.SI object
the object pointer provided by an index method.
.SB Outputs:
.SI object
May be modified by the method.
.SB Returns:
.SI OK
if the operation succeseded
.SI other
method-specific failure status as appropriate.
.SB Prototype
.(E
MO_SET_METHOD my_set_method;

STATUS my_set_method( nat offset,
                    nat luserbuf,
                    char *userbuf,
                    nat objsize,
                    PTR object )

.)E
.A "Guide to Writing Objects"
.S 1 Introduction

This appendix provides a guide for writers of MO objects, showing
techniques and examples of the approaches that became
.q "obvious"
only after a good number were written.  This is low-level coding
information.  We do not talk about the higher level semantics of the
objects themselves.  This means we don't talk about consistant data
views or ways to model transient situations, which will be the subject
of another note[4].

Section A.2 below gives a few suggestions about how to navigate this
document.  To avoid frightening anyone, we start in Section A.3 with
the ways to get and set values.  This isn't so bad.  The grotty
business is handling indexing, which is belabored in Section A.4.
Section A.5 talks about ways to organize your code.  A brief list of
things to avoid is in Section A.6.  The obligatory summary in in
Section A.7, and references are listed in Section A.8.
.S 1 "How to use this Appendix"
.P
Sliced vertically:
.BU
If you want to know how to use the get/set methods, go to section A.3.
.BU
If you want to know about indexing and index methods,
go to section A.4.
.P
Sliced horizontally:
.BU
If you are writing an object for a variable
that can be simply read or set go to Section A.3.1.
.BU
If you are writing an object for a variable that needs
active code to be read or set, go to Section A.3.2.
.BU
If you are writing objects for accessing elements of
structures, go to Section A.3.3
.BU
If you are writing objects for single global instances,
go to section A.4.1
.BU
If you are writing objects for accessing multiple instances
of individual objects or structures, go to section A.4.2
.S 1 "Value access"

One view of MO is that it's a general way of getting and setting
the values of named objects.  The way that objects are identified
are discussed in Section A.4, "Instances and Indexes".  This section
descirbed the ways you can get and set values once an object is
identified.  Section A.3.1 describes how you can use the few
built-in get/set methods provided by MO for access to program
variables.  When you need something to happen behind the curtain,
you need to provide you own code, which is described in Section
3.2 on the use of user get/set methods.  Section A.3.3 talks about
ways to reuse get/set methods when accessing elements of
structures.
.S 2 "Simple access to variable using built-in get/set methods"

If you have a variable that is a string character buffer or an
integer type that you want to make visible, life is pretty simple.
MO provides methods for direct access of three different data
types: integers, pointers to integers, string buffers, and pointer
to string buffers.  All you need to do are set up a class that
uses them and make an instance of the class appear.
.P
The MO provided methods are listed in Table 6, and each has it's own
function documentation.
.P
The first thing you need to do is define an MO class with a call
to MOclassdef.  For example,
.(E
GLOBALDEF longnat My_long;
.)E
Somewhere you prepare a class definition, usually as part of an
array, as follows:
.(E
MO_CLASS_DEF My_classes[] =
{
    {
        MO_CDATA_CLASSID,               /* flags */
        "exp.back.myfacility.my_long",  /* classid */
        sizeof(My_long),                /* size */
        MO_READ,                        /* perms */
        0,                              /* index */
        0,                              /* offset */
        MOintget,                       /* get method */
        MOnoset,                        /* set method */
        0,                              /* cdata */
        MOidata_index                   /* index method */
    },
    { 0 }
};
.)E
You make this known by calling MOclassdef:
.(E
stat = MOclassdef( MAXI2, My_classes );
.)E
(Use of the array form let's you define all the classes for a
module in one call to MOclassdef.)

You then declare the instance(s) with a call to MOattach:
.(E
stat = MOattach( 0,
                 "exp.back.myfacility.my_long",
                 "0",
                 (PTR)&My_long );
.)E
That's all there is to it.
.S 3 "Simple class definition explained"
.P
Let's look at the class definition in some detail.
.(E
    {
        MO_CDATA_CLASSID,               /* flags */
.)E
This says to use the classid as the cdata handed
to the index method, and to ignore the cdata in the
definition.  We use this because the MOidata_index
method we've chosen to use requires the classid as
the cdata.
.(E
        "exp.back.myfac.my_long",       /* classid */
.)E
This gives the name of the class.  There are conventions
for picking names.  Unless you are doing something that
is intentionally visible to customers, put things into the
"exp." space as shown here.
.(E
        sizeof(My_long),                /* size */
.)E
This is the size of the objects in the class, which will
be handed to get and set methods when they are called.
.(E
        MO_READ,                        /* perms */
.)E
These are the permissions associated with the object,
as defined in the MO spec.
.(E
        0,                              /* index */
.)E
This is the name of the index class, if any is present.
In this case there is no index, so we zero it.  (Indexes
are used for classes where there will be more than one
instance).
.(E
        0,                              /* offset */
.)E
This is a value handed to the get/set methods on access.
The MO provided methods treat it as a byte offset into
the object they are handed where the item will be found.
This is used to decode structures, which we discuss below.
.(E
        MOintget,                       /* get method */
.)E
This is the "get" method used to retrieve a value for
objects in the class.  In this case we are using the
MO provided integer method.  There are two other MO
methods:  MOstrget, when the object is a buffer of
the size specified, and MOstrpget, where the object
is a pointer to a buffer of the specified size.
.(E
        MOnoset,                        /* set method */
.)E
This is the "set" method used to write values for objects
in a class.  In this case we do not allow writes, and we
use the MO provided method.
.(E
        0,                              /* cdata */
.)E
This is data handed to the index method below when we
are looking up instances in the class.  It is usually
used to identify the class being indexed if we are using
one method to index many classes.  The MO provided index
methods assume this is the character name of a class that
has explicitly attached instances to lookup.  This is so
common that the "flags" field above supports two special
cases: MO_CDATA_CLASSID and MO_CDATA_INDEX, which force
the cdata to either the classid or the index value, and
ignore the value here.  That's what we're doing in this
definition.
.(E
        MOidata_index                   /* index method */
.)E
This is the method function to use to index the class.  In
this case, we are using the MO provided method for getting
instances of objects that have been added with calls to
MOattach.  If you don't MOattach the instances, you need
to provide your own index methods.  This is discussed in
detail in section A.4.
.(E
    },
.)E
.S 3 "Simple attach explained"

Here is a look at the instance attach call.
.(E
stat = MOattach( 0,
.)E
Flags: There are no modification to this instance.  It is plain
vanilla.  If the instance string were volatile, say if it was
constructed in a local buffer that was about to vanish, you'd specify
MO_INSTANCE_VAR.
.(E
                 "exp.back.myfacility.my_long",
.)E
This is the name of the classid of which this is
an instance.  It must match the name given in the
class definition.  The class must be defined before
instances are attached.
.(E
                 "0",
.)E
This is the identity of the instance in question.  By
convention, "0" is used for objects that have only
one instance.  Classes that have more than one instance
never use "0" as an instance value.
.(E
                 (PTR)&My_long );
.)E
This is a data value handed to the get and set methods
for the instance.  It is usually a pointer to the appropriate
data object.
.S 3 "An even simpler alternative"

We've shown the use of the MOidata_index and the MOattach call
above because it is the useful general case.  For our particular
example there is a simpler one-step method for defining a
single-instance object using a different index method.  Index
methods are described completely in section A.4.

Here you would use the MOcdata_index rather than MOidata_index.
It assumes only instance "0" exists, and that it's
instance-specific idata is the cdata in the class definition.

Here is a complete class/instance definition using this technique:
.(E
MO_CLASS_DEF My_classes[] =
{
    {
        0,                              /* flags */
        "exp.back.myfacility.my_long",  /* classid */
        sizeof(My_long),                /* size */
        MO_READ,                        /* perms */
        0,                              /* index */
        0,                              /* offset */
        MOintget,                       /* get method */
        MOnoset,                        /* set method */
        (PTR)&My_long,                  /* cdata */
        MOcdata_index                   /* index method */
    },
    { 0 }
};
.)E
.S 2 "User get/set method functions"

Sometimes you will not want to use the built in get or set
methods.  The job of these methods is to convert back and forth
between the string format present at the MO value interfaces to
whatever internal representation is used for the object, and cause
whatever side-effects the object-writer thinks is appropriate.

You'll write your own get method when you want to decode a field into
characters in a way that isn't obvious.  You write set methods to
allow a complicated conversion, or cause some action to take place
when the value is changed.  For instance, a writeable cache size
object would need a method that did something like lock the cache;
flush it; free it's memory; allocate new memory; re-initialize it; and
then unlock it.

MO provides three utility functions for user-written access methods:
.ip MOstrout 16
put an string output, checking for the end of the output buffer and
returing the error status specified it it won't fit.
.ip MOlongout 16
put a longnat converted to a string, checking for the end of the
output buffer and returing the error status specified it it won't fit.
The conversion puts out leading zeros so that lexical sort ordering
matches the numeric sort ordering.
.ip MOptrout 16
put a pointer converted to a string, checking for the end of the
output buffers and returning the error status specified if it won't fit.
The conversion puts out leading zeros to that lexical sort ordering
matches the numeric sort ordering.
.ip MOulongout 16
put a unsigned longnat converted to a string, checking for the end of
the output buffer and returing the error status specified if it won't
fit.  The conversion puts out leading zeros so that lexical sort
ordering matches the numeric sort ordering.
.P
Here is an example of a user get method:
.(E
/* offset ignored */

STATUS
CS_sem_scribble_check_get(nat gsflags,
                          nat objsize,
                          PTR object,
                          nat luserbuf,
                          char *userbuf)
{
    CS_SEMAPHORE *sem = (CS_SEMAPHORE *)object;
    char *str;

    if( sem->cs_sem_scribble_check == CS_SEM_LOOKS_GOOD )
        str = "looks ok";
    else if( sem->cs_sem_scribble_check = CS_SEM_WAS_REMOVED )
        str = "REMOVED";
    else
        str = "CORRUPT";

    return( MOstrout( MO_VALUE_TRUNCATED, str, luserbuf, userbuf ));
}
.)E
.S 2 "Structure member access"

Very often you'll have a structure for which you are creating
classes for each member.  It would be very inefficient to need to
attach each member individually, with the instance data being a
pointer to the element.  Instead, we want to store them in a way
that the instance data is a pointer to the entire structure.  We
could then write access methods for each member, but this is would
be a lot of redundant code if there were no special conversions
needed.

This situation is cleanly handled by using the "CL_OFFSETOF" macro
to set up the "offset" field of the class definition, the
"MO_SIZEOF_MEMBER" macro to provide the value for the "size"
field, and then using the MO provided access methods.  The
standard access methods take the class-definition provided offset
and add it to the instance data, treating the result as the
address of the data element to be converted.

Recall that the CL_OFFSETOF macro provides the byte offset into a
structure of a element member in a way suitable for compile time
initialization.  MO_SIZEOF_MEMBER is an analagous macro that
returns the size of the member.

In the example in section A.3.1 all the "offset" fields were set to 0.
To provide access to a global structure you might set the classes up
using MO_SIZEOF_MEMBER and CL_SIZEOF as shown below.
.(E
typedef struct
{
        longnat         field_one;
        longnat         field_two;
        longnat         field_three;
} MY_STRUCT;

GLOBALDEF MY_STRUCT My_struct;

MO_CLASS_DEF My_classes[] =
{
    {
        0,
        "exp.back.myfacility.my_struct.field_one",
        MO_SIZEOF_MEMBER( MY_STRUCT, field_one ),
        MO_READ,
        0,
        CL_OFFSETOF( MY_STRUCT, field_one ),
        MOintget,
        MOnoset,
        (PTR)&My_struct,
        MOcdata_index
    },
    {
        0,
        "exp.back.myfacility.my_struct.field_two",
        MO_SIZEOF_MEMBER( MY_STRUCT, field_two ),
        MO_READ,
        0,
        CL_OFFSETOF( MY_STRUCT, field_two ),
        MOintget,
        MOnoset,
        (PTR)&My_struct,
        MOcdata_index
    },
    {
        0,
        "exp.back.myfacility.my_struct.field_three",
        MO_SIZEOF_MEMBER( MY_STRUCT, field_three ),
        MO_READ,
        0,
        CL_OFFSETOF( MY_STRUCT, field_three )
        MOintget,
        MOnoset,
        (PTR)&My_struct,
        MOcdata_index
    },
    { 0 }
};
.)E
.S 1 "Instances and indexes"

This section describe the way classes are defined regarding their
"instance" indexes.  Section A.4.1 discusses the simple way of using
the MOattach/MOdetach function to use the internal index for
single-instance objects.   Multiple instance objects in all their
complexity are described in section A.4.2

All classes define the index method that will be used to map
instance strings to a PTR of instance-specific data (the idata).
They need to handle two messages, MO_GET and MO_GETNEXT.

For MO_GET, the instance string given is the object requested.  The
index method merely needs to validate the instance and return it's
associated idata.  The MO_GET case is usually trivial.

The ``interesting'' case is handling MO_GETNEXT, and we will spend
much time discussion ways to create reasonable ordering.  Handling
MO_GETNEXT message requires validation of the current instance,
identification of it's successor (if any), and return of the
successor's idata and it's instance string.  The successor is defined
as instance whose instance string is lexically next greater than the
current instance string.  As an important special case, if the input
instance to a MO_GETNEXT is an empty string (""), then the index
method must return the first instance in the class, or MO_NO_NEXT.

MO provides three index methods that are appropriate for most
easily forseen cases.
.ip MOcdata_index 16
provides a minimal index used to define single
instance classes and objects in the class
definition.  The cdata provided in the class
definition is the idata.
.ip MOidata_index 16
provides an index that looks things up in the
objects that have been MOattached.  The cdata
in the class definition is the the name of a
class to lookup, which will not always be the
name of the class actually being referenced.
The returned idata is what was supplied in the
MOattach call.
.ip MOname_index 16
provides an index into the MOattach space as in MOidata_index, but the
idata returned is the instance string itself.  This has some
occasional uses, but won't be further discussed.
.P
For cases not covered by these methods, you need to provide your
own index methods.  Many cases can be covered with a built-in
method, at some run-time expense, or by a user supplied index
method at the cost of writing more code.
.S 2 "Single instance objects using the built-in index methods"

As used in the examples in section A.3, you will usually use either
MOcdata_index or MOidata_index as the index method for global
objects.  You'll use MOidata_index for things you want to
explcitly attach, or MOcdata_index when you don't want to MOattach
each one, and/or want the simplest possible representation.
.S 2 "Multiple Instance objects"

Multiple instance objects are those you are modeling in a table or an
array.  This section discusses the issues involved in maintaining the
indexes for these kinds of objects.  It doesn't discuss the issues of
access method choices, which are covered above.  Everything said there
about the use of get and set methods applies equally well to multiple
instances.

When there are multiple instances of an object class, they are
distinguished by different instance strings.  These instance strings
contain only decimal digits and dots if they are to be used in the
supported, formal MIB\**.
.(f
\**  This is because some standards (eg, SNMP) allow only
number-dot-number style instances.  If you have an index that cries to
be a string, you may need to create a surrogate numeric key to be used
instead, which may then be looked up to geet the string value.  This
is unpalatable, but necessary for standards conformance.
.)f
They must form be stable,
ordered, and lead to reasonable location of an object and the one that
follows.  By stable, we mean an index value will always refer to the
same actual object.  Thus, position in a queue in a queue is not
likely to be stable, while the address of a structure in the queue is
stable.  Ordering is on the lexical value of the string representation
of the index values for a class.  Reasonable location of an object and
its successor means that given a valid index to one object, it is easy
to find it and the one that follows.  For instance, even if we have a
stable linked list, it would be a bad idea to use the ordinal position
as the index if the list is likely to be large.  Finding the 975th
element costs a scan of 975 elements, though finding the next is easy.
Using an object address makes finding the first one easy, but unless
there is a "next" pointer in the object locating the successor may be
difficult.
.BU
If your instances have a known natural ordering (given one
you know where the next is), go to section A.4.2.1
.BU
If you there is an ordering, but you don't know what the next is,
go to section A.4.2.2
.BU
If there is no natural ordering, go to section A.4.2.3
.S 3 "Using natural known indexes"

A natural known index would be something like an array index,
where simple increment of the current value leads you to the next
(or at lest on the way to the next).  These are easily handled,
and have the a major dvantage of not requiring explicit
attach/detach actions in all processes that wish to make objects
in shared memory visible.  You only need to make sure that the
classes are defined with appropriate index/access methods in all
the processes that want to know.

For instance, an array that had entries that are both active and
free has a natural known index.  Examples are the shared-memory
logging and locking tables.

Upsides:  ordering is obvious, and there is no additional space
needed to maintain this index.

Downsides:  it is necessary to write your own index methods, and
it may be difficult to control "dirty-reads" of the index since
you probably don't want to lock the index array while you are
using it.

Here is an example (untested) index method for a fixed sized
array, where the instance strings are the decimal slot numbers.
.(E
typedef struct
{
        bool in_use;
        longnat x;
        longnat y;
} MY_ARRAY_ELEMENT;

MY_ARRAY_ELEMENT My_array[ MY_ARRAY_SIZE ];

STATUS
My_array_index(nat msg,
               PTR cdata,
               nat linstance,
               char *instance,
               PTR *instdata )
{
    STATUS stat = OK;
    longnat ix;

    CVal( instance, &ix );

    if( ix > 0 && ix < MY_ARRAY_SIZE && My_array[ ix ].in_use )
        stat == MO_NO_INSTANCE;

    switch( msg )
    {
    case MO_GET:
        if( stat == OK )
            *instdata = (PTR)&My_array[ ix ];
        break

    case MO_GETNEXT:
        if( stat == OK )
        {
            stat = MO_NO_NEXT;
            while( ++ix < MY_ARRAY_SIZE )
            {
                *instdata = (PTR)&My_array[ ix ];
                if( My_array[ix].in_use )
                {
                    stat = MOlongout( MO_INSTANCE_TRUNCATED,
                                     ix, linstance, instance );
                    break;
                }
            }
        }
        break;

    default:
        stat = MO_BAD_MSG;
        break;
    }

    return( stat );
}
.)E
.S 3 "Using natural unknown indexes"

A
.q "natural unknown"
index is a common case with arbitrary allocated structures of a common
type.  Their addresses form a natural ordering, but from one it is
often difficult to know the location of the next one in the proper
order.
.BU
If you have a sorted linked list of structures, go to
section A.4.2.2.1.
.BU
If you have structures where you can easily add a 20 byte
position-dependent member to create an ordered index,
go to section A.4.2.2.2
.BU
If you have structures that can't have a position dependant
element added, go to section A.4.2.2.3
.S 4 "Using a pre-existing sorted linked list"

We haven't seen any of these yet, perhaps because it isn't a very
good idea.  Keeping a linked list sorted can be computationally
expensive compared to using an intrinsically sorted structure,
such as a splay-tree using the GL SP library.  You'll need to
write your own index method, and lock the list during the index
operation to prevent re-entrancy problems.  (Of course you already
had it locked when you were adding and deleting things from it,
right?  Weren't you?)

Upsides:  maybe little code needs to be changed in creating the list,
no additional memory needed per object for index maintenance.

Downsides:  need your own index methods, may be hard to get
a stable index, may be costly for simple GET operations if there
is a very large number of objects in the chain.
.S 4 "Embedding an index into the structure"

If you can stick a 20 byte object into your structure, you can
easily maintain an ordered index yourself.  It is computationally
cheap, and easy enough to do when you swipe the code from this
example.

Downsides: You'll need to provide a global tree root, a way of
locking the tree to prevent re-entrancy problems, a comparision
function, and your own index method.  You need to known where
objects are created, and more critically when they go out of scope
to prevent leaks.  It costs another 20 bytes or so per object.

An example of this techique is the way CS_CONDITIONs are handled in
the UNIX CL.  The CS_CONDITION is defined with an embedded SPBLK for
an SPTREE to use.
.(E
struct _CS_CONDITION
{
        CS_CONDITION    *cnd_next, *cnd_prev;
        CS_SCB          *cnd_waiter;
        char            *cnd_name;
        SPBLK           cnd_spblk;
};
.)E
A tree is initialized at some point early on, though we check it's
pointer before using it in case people try to use conditions before
the initialization takes place.  When a condition is set up in
CScnd_init, it is linked into a tree:
.(E
    cnd->cnd_spblk.uplink = NULL;
    cnd->cnd_spblk.leftlink = NULL;
    cnd->cnd_spblk.rightlink = NULL;
    cnd->cnd_spblk.key = (PTR)cnd;
    cnd->cnd_name = "";

    if( Cs_srv_block.cs_cnd_ptree != NULL )
        SPinstall( &cnd->cnd_spblk, Cs_srv_block.cs_cnd_ptree );
.)E
And it is unlinked in CScnd_free:
.(E
    if( Cs_srv_block.cs_cnd_ptree != NULL )
        SPdelete( &cnd->cnd_spblk, Cs_srv_block.cs_cnd_ptree );
.)E
Elsewhere, classes are defined using an index method on
this tree, and standard get/set methods:
.(E
MO_INDEX_METHOD CS_cnd_index;

static char index_name[] =  "exp.cl.clf.cs.cnd";

GLOBALDEF MO_CLASS_DEF CS_cnd_classes[] =
{
  { 0, index_name, 0, MO_READ, index_name, 0,
        MOuivget, MOnoset, NULL, CS_cnd_index },

  { 0, "exp.cl.clf.cs.cnd_next",
        MO_SIZEOF_MEMBER( CS_CONDITION, cnd_next ),
        MO_READ, index_name,
        CL_OFFSETOF( CS_CONDITION, cnd_next ),
        MOptrget, MOnoset, NULL, CS_cnd_index },

  { 0, "exp.cl.clf.cs.cnd_prev",
        MO_SIZEOF_MEMBER( CS_CONDITION, cnd_prev ),
        MO_READ, index_name,
        CL_OFFSETOF( CS_CONDITION, cnd_prev ),
        MOptrget, MOnoset, NULL, CS_cnd_index },

  { 0, "exp.cl.clf.cs.cnd_waiter",
        MO_SIZEOF_MEMBER( CS_CONDITION, cnd_waiter ),
        MO_READ, index_name,
        CL_OFFSETOF( CS_CONDITION, cnd_waiter ),
        MOptrget, MOnoset, NULL, CS_cnd_index },

  { 0, "exp.cl.clf.cs.cnd_name",
        MO_SIZEOF_MEMBER( CS_CONDITION, cnd_name ),
        MO_READ, index_name,
        CL_OFFSETOF( CS_CONDITION, cnd_name ),
        MOstrpget, MOnoset, NULL, CS_cnd_index },

  { 0 }
};
.)E
And the index method looks things up in that tree.
.(E
STATUS
CS_cnd_index(nat msg,
             PTR cdata,
             nat linstance,
             char *instance,
             PTR *instdata )
{
    STATUS stat = OK;
    PTR ptr;

    switch( msg )
    {
    case MO_GET:
        if( OK == (stat = CS_get_block( instance,
                                       Cs_srv_block.cs_cnd_ptree,
                                       &ptr ) ) )
            *instdata = ptr;
        break;

    case MO_GETNEXT:
        if( OK == ( stat = CS_nxt_block( instance,
                                        Cs_srv_block.cs_cnd_ptree,
                                        &ptr ) ) )
        {
            *instdata = ptr;
            stat = MOulongout( MO_INSTANCE_TRUNCATED,
                              (u_longnat)ptr,
                              linstance,
                              instance );
        }
        break;

    default:
        stat = MO_BAD_MSG;
        break;
    }
    return( stat );
}
.)E
The get_block and nxt_block utilities are fairly simple as well, and
perhaps should be provided by MO for your use.
.(E
/*
**  Given address of block, and a tree, return pointer to the
**      actual block if found.
*/
STATUS
CS_get_block( char *instance, SPTREE *tree, PTR *block )
{
    SPBLK spblk;
    SPBLK *sp;
    STATUS ret_val = OK;
    u_longnat lval;

    str_to_uns( instance, &lval );
    spblk.key = (PTR)lval;

    if( (sp = SPlookup( &spblk, tree ) ) == NULL )
        ret_val = MO_NO_INSTANCE;
    else
        *block = (PTR)sp->key;
    return( ret_val );
}

/* ---------------------------------------------------------------- */

/* utilities for index methods */

STATUS
CS_nxt_block( char *instance, SPTREE *tree, PTR *block )
{
    SPBLK spblk;
    SPBLK *sp;
    STATUS ret_val = OK;
    u_longnat lval;


    if( *instance == EOS )      /* no instance, take the first one */
    {
        if( (sp = SPfhead( tree )) == NULL )
            ret_val = MO_NO_NEXT;
        else
            *block = (PTR)sp->key;
    }
    else                        /* given an instance */
    {
        str_to_uns( instance, &lval );
        spblk.key = (PTR)lval;
        if( (sp = SPlookup( &spblk, tree )) == NULL )
            ret_val = MO_NO_INSTANCE;
        else if( (sp = SPfnext( sp ) ) == NULL )
            ret_val = MO_NO_NEXT;
        else
            *block = (PTR)sp->key;
    }
    return( ret_val );
}
.)E
.S 4 "Explicitly using the MO instance index"

If you can't put the index block into your structure, you can have
MO maintain the index itself.  This is more expensive, because MO
will need to allocate memory for an index block.  The good news is
that you can use the bone-stock MOidata_index method without further
modification.

You must find places to explicitly attach and detach the objects,
the latter being important to prevent object leaks.

Upsides:  simple, elegant, the index is away from the object (good
for shared memory), and no changes need to be made to the objects.

Downsides: costs more memory per object (40 bytes or so) and at
least one more allocation.

Examples of this are the SPTREE objects provided by
MOsptree_attach.  This whole thing is pretty trivial, just making
the elements of the existing structures visible using built-in
methods:
.(E
static char index_class[] = "exp.gl.glf.mo.sptrees.name";

static MO_CLASS_DEF MO_tree_classes[] =
{
/* this one is really attached */

{  MO_INDEX_CLASSID|MO_CDATA_INDEX, index_class,
       MO_SIZEOF_MEMBER(SPTREE, name), MO_READ, 0,
       CL_OFFSETOF(SPTREE, name), MOstrpget, MOnoset,
       0, MOidata_index },

/* The rest are virtual, created with builtin methods */

{  MO_CDATA_INDEX, "exp.gl.glf.mo.sptrees.lookups",
       MO_SIZEOF_MEMBER(SPTREE, lookups), MO_READ, index_class,
       CL_OFFSETOF(SPTREE, lookups), MOintget, MOnoset,
       0, MOidata_index },

{  MO_CDATA_INDEX, "exp.gl.glf.mo.sptrees.lkpcmps",
       MO_SIZEOF_MEMBER(SPTREE, lkpcmps), MO_READ, index_class,
       CL_OFFSETOF(SPTREE, lkpcmps), MOintget, MOnoset,
       0, MOidata_index },

{  MO_CDATA_INDEX, "exp.gl.glf.mo.sptrees.enqs",
       MO_SIZEOF_MEMBER(SPTREE, enqs), MO_READ, index_class,
       CL_OFFSETOF(SPTREE, enqs), MOintget, MOnoset,
       0, MOidata_index },

{  MO_CDATA_INDEX, "exp.gl.glf.mo.sptrees.enqcmps",
       MO_SIZEOF_MEMBER(SPTREE, enqcmps), MO_READ, index_class,
       CL_OFFSETOF(SPTREE, enqcmps), MOintget, MOnoset,
       0, MOidata_index },

{  MO_CDATA_INDEX, "exp.gl.glf.mo.sptrees.splays",
       MO_SIZEOF_MEMBER(SPTREE, splays), MO_READ, index_class,
       CL_OFFSETOF(SPTREE, splays), MOintget, MOnoset,
       0, MOidata_index },

{  MO_CDATA_INDEX, "exp.gl.glf.mo.sptrees.splayloops",
       MO_SIZEOF_MEMBER(SPTREE, splayloops), MO_READ, index_class,
       CL_OFFSETOF(SPTREE, splayloops ), MOintget, MOnoset,
       0, MOidata_index },

{  MO_CDATA_INDEX, "exp.gl.glf.mo.sptrees.prevnexts",
       MO_SIZEOF_MEMBER(SPTREE, prevnexts ), MO_READ, index_class,
       CL_OFFSETOF(SPTREE, prevnexts), MOintget, MOnoset,
       0, MOidata_index },

{ 0 }
};

/* name must be a unique non-volatile string */

STATUS
MOsptree_attach( SPTREE *t )
{
    STATUS stat = OK;

    stat = MOattach( 0, index_class, t->name, t );
    return( stat );
}
.)E
.S 2 "Creating un-natural indexes"

An unnatural index is one where the ordering is not obvious, and
where one needs to be created arbitrarily.  An example is the
index for CS_SEMAPHORES, which may be in position-independant
shared memory and which must be uniquely indexed across the
installation.  The need to have the same index in all processes
means the process-dependant address cannot be used.

This problem can be solved by compbining some things that will be
unique.  Doing so will require construction of an instance at
attach time, and an access method for retriving a constructed
index.  It is not particularly space or time efficient, but does
get you out of the hole.

For semaphores, we use the pid of the process that created the
semaphore, and its address in that process.  We create an instance
variable encoding this, and attach it with the MO_INSTANCE_VAR
flag so the string is saved by MO.
.(E
struct _CS_SEMAPHORE
{
    . . .

    CS_SEMAPHORE    *cs_sem_init_addr;  /* address in CSi_sem process */
    longnat         cs_sem_init_pid;    /* pid of CSi_sem process */

    . . .
}
.)E
Classes are defined as usual, though we need a special method to
get the index object value:
.(E
static char index_class[] = "exp.cl.clf.cs.sem_index";

GLOBALDEF MO_CLASS_DEF CS_sem_classes[] =
{
    /* this is really attached, and uses the default index method */

  { MO_INDEX_CLASSID|MO_CDATA_INDEX, index_class,
        0, MO_READ, 0,
        0, CS_sem_index_get, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.cl.clf.cs.sem_value",
        MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_value), MO_READ, index_class,
        CL_OFFSETOF(CS_SEMAPHORE, cs_value), MOintget, MOnoset,
        0, MOidata_index },

   . . .
};
.)E
We create the index value and attach semaphores to the process as follows:
.(E
void
CS_sem_make_instance( CS_SEMAPHORE *sem, char *buf, MO_INSTANCE_DEF *idp )
{
    char pidbuf[ 80 ];
    char addrbuf[ 80 ];

    (void) MOulongout( 0, sem->cs_sem_init_pid, sizeof(pidbuf), pidbuf );
    (void) MOulongout( 0, sem->cs_sem_init_addr, sizeof(addrbuf), addrbuf );
    STpolycat( 3, pidbuf, ".", addrbuf, buf );

    idp->flags = MO_INSTANCE_VAR;
    idp->classid = index_class;
    idp->instance = buf;
    idp->idata = (PTR)sem;
}
.)E
.(E
/*
** Make this semaphore known.
*/
STATUS
CS_sem_attach( CS_SEMAPHORE *sem )
{
    MO_INSTANCE_DEF instance;
    char buf[ 80 ];

    CS_sem_make_instance( sem, buf, &instance );
    return( MOattach( 1, &instance ) );
}
.)E
.(E
/*
** Make this semaphore unknown through MO.
*/
STATUS
CS_detach_sem( CS_SEMAPHORE *sem )
{
    MO_INSTANCE_DEF instance;
    char buf[ 80 ];

    CS_sem_make_instance( sem, buf, &instance );
    return( MOdetach( 1, &instance ) );
}
.)E
Our index value method can reuse code and be simply:
.(E
STATUS
CS_sem_index_get(nat gsflags,
                 nat objsize,
                 PTR object,
                 nat luserbuf,
                 char *userbuf)
{
    MO_INSTANCE_DEF instance;
    char buf[ 80 ];

    CS_sem_make_instance( (CS_SEMAPHORE *)object, buf, &instance );
    return( MOstrout( MO_VALUE_TRUNCATED, buf, luserbuf, userbuf ));
}
.)E
.S 1 "Code Organization"

Here are some hints for organizing your MO object definitions in
your code.
.BU
Put complicated multi-instance class definitions and methods in
their own file.  Provide object "attach" and an object
"detach" calls (if relevant) in the same file.
.BU
Put global variable class definitions in the file that defines
the object.
.BU
Call MOclassdef at the earliest opportunity.
.S 1 "Don't do this"
.BU
Don't explicitly attach every member of a structure.  It's horribly
space inefficient.  Use an index method that returns a pointer to the
structure.  Either use built-in methods and an offset for get/set, or
write user get/set methods that can look inside the structure.
.BU 
Don't put the address of structure members in the cdata for a
single-instance structure.  It's much cleaner to use the structure
base as the cdata, and then use the offset (with CL_OFFSETOF) to
locate the member.  This lets you use standard get/set methods.
.BU
Don't use non-standard index formats, like a hex address.
You'll mess up interoperability with some standard management
schemes, like SNMP if you do.

This can be a bitch to avoid; the MO meta data is indexed on the
classid string.  That means the non-oid objects can't be made
visible across SNMP.  (Or maybe that's good).  It would have
been better to avoid this if a way could have been found.
.BU
Don't try to compute an index on each index call.  It will be horribly
expensive, either in cycles or memory, or both.  You might be tempted
to sort and index a pile of things on each call to the index method.
This would be a Bad Idea.  Use one of the techniques described in
Section A.4.2.2 above.
.BU
Don't forget to include an index object for each indexed structure.
An index object is one whose value is the string index.  This is
necessary to provide the key for a table with onerow per instance of
the structure.
.S 1 "Summary"

We've shown with examples almost all the good ways to define
object classes using MO, and identified a few mistakes you might
be tempted to make.  The hard problems are usually in providing an
index and/or index methods for things that don't have obvious or
natural ordering.  Several techniques have been shown, and the
tradeoffs of each considered.

With this, the references and some informed courage, you should be
able write the MO objects for just about anything.
.S 1 "References"

[] MO design...

[] DBMS MIB design...

[] Modeling wacky data for the IMA...

[] SP spec

[] SP design...


