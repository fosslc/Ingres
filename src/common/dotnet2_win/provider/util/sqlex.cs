/*
** Copyright (c) 2003, 2009 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.Runtime.Serialization;
using System.Security;
using System.Security.Permissions;

namespace Ingres.Utility
{

	/*
	** Name: SqlEx.cs
	**
	** Description:
	**	Defines the internal SqlEx exception class and an error information 
	**	class used to define the errors associated with exceptions.
	**
	**  Classes:
	**
	**	SqlEx		Extends Exception and CreateInstance of IngresException
	**	ErrInfo
	**
	** History:
	**	10-Sep-99 (gordy)
	**	    Created.
	**	18-Nov-99 (gordy)
	**	    Made ErrInfo a nested class.
	**	20-Apr-00 (gordy)
	**	    Moved to package util.  Converted from interface to class.
	**	12-Apr-01 (gordy)
	**	    Tracing changes require tracing to be passed as parameter.
	**	11-Sep-02 (gordy)
	**	    Moved to GCF.  Renamed to remove specific product reference.
	**	    Extracted SQLWarning methods to SqlWarn class.
	**	22-Nov-03 (thoda04)
	**	    Ported to .NET Provider.
	**	24-Feb-04 (thoda04)
	**	    Reworked SqlEx and IngresException to avoid forcing the user
	**	    to add a reference to their application to reference the
	**	    another assembly to resolve the SqlEx class at compile time.
	**	    We want the application to only reference the Client assembly.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	20-Aug-09 (thoda04)
	**	    Trap SecurityException if running with less than FullTrust
	**	    permission when we access System.Environment.StackTrace
	**	    property and user application does not have
	**	    EnvironmentPermission=Unrestricted.  Fixes Bug 122511.
	*/


	/*
	** Name: SqlEx
	**
	** Description:
	**	Class which extends standard SQL exception class to provide
	**	construction from resource references and other useful sources.
	**
	**  Public Methods:
	**
	**	Get		Factory methods for generating SqlEx exceptions.
	**	trace		Write exception info to trace log.
	**
	** History:
	**	10-Sep-99 (gordy)
	**	    Created.
	**	18-Nov-99 (gordy)
	**	    Made ErrInfo a nested class.
	**	20-Apr-00 (gordy)
	**	    Converted from interface to class, added constructors,
	**	    and implemented factory methods get().
	**	11-Sep-02 (gordy)
	**	    Renamed to remove specific product reference.
	**	    Extracted SQLWarning methods to SqlWarn class.
	*/

	[Serializable]
	internal class SqlEx : SystemException
	{
		static private ISqlExFactory providerExFactory;

		private int errCode = 0;
		private string SQLState = "HY000";

		/*
		** Name: SqlEx constructor for IngresException factory
		**
		** Description:
		**	Record where to find the factory that can create
		**	an instance of IngresException.  This resolves the
		**	circular reference problem across the assemblies
		**	and yet still restrict access away from the public.
		**
		** Input:
		**	sqlExFactory	IngresExceptionFactory.
		**
		** Output:
		**	None.
		**
		** History:
		**	23-Nov-03 (thoda04)
		**	    Created.
		*/

		internal SqlEx(ISqlExFactory _providerExFactory)
		{
			// return if we already have the ProviderExceptionFactory
			if (providerExFactory != null)
				return;

			// this factory will help SqlEx get
			// IngresException object
			providerExFactory = _providerExFactory;
		}

		/*
		** Name: SqlEx
		**
		** Description:
		**	Class constructors.
		**
		** Input:
		**	msg	    Detailed message for exception.
		**	sqlState    Associated SQL State.
		**	err	    Associated error code.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	20-Apr-00 (gordy)
		**	    Created.
		**	11-Sep-02 (gordy)
		**	    Renamed to remove specific product reference.
		*/

		public SqlEx()
			:base() {}

		public SqlEx(String message)
			:base(message) {}

		public SqlEx(String message, Exception innerException)
			:base(message, innerException) {}

		protected SqlEx(
				String message,
				String sqlState,
				int    nativeError)
			: base(message)
		{
			errCode = nativeError;
			if (sqlState != null)
				SQLState = sqlState;
			else
				SQLState = "00000";
		}

		/*
		** Name: Class's deserialization constructor and 
		**       GetObjectData serialization method
		**
		** Description:
		**	Serialization is the mechanism to support persistence of
		**	objects in some form of storage or to transfer the objects
		**	from one place to another like from one process to another.
		**
		** Input:
		**	SerializationInfo	Holds the serialization info such as 
		**	                    AssemblyName, FullTypename, and MemberCount.
		**	StreamingContext	Describes the source and destination of a 
		**	                    serialized stream so we know if serializing
		**	                    to storage or across processes or machines.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		*/
		protected SqlEx(SerializationInfo info, StreamingContext context)
			:base(info, context)     // Deserialization constructor
		{
			errCode  = info.GetInt32( "errCode");
			SQLState = info.GetString("SQLState");
		}

		[SecurityPermissionAttribute(
		SecurityAction.Demand, SerializationFormatter=true)]
		public override void GetObjectData(
			SerializationInfo info, StreamingContext context)
		{
			info.AddValue("errCode",  errCode,  typeof(int));
			info.AddValue("SQLState", SQLState, typeof(string));

			// Let the base type serialize its fields
			base.GetObjectData(info, context);
		}

		protected internal string getSQLState()
		{
			return SQLState;
		}

		protected internal int getErrorCode()
		{
			return errCode;
		}

		/*
		** Name: getNextException
		**
		** Description:
		**	Get the next SqlEx chained to this SqlEx.
		**
		** Input:
		**	ex	    SqlEx.
		**
		** Ouptut:
		**	None.
		**
		** Returns:
		**	SqlEx	    The exception object.
		**
		** History:
		**	24-Feb-04 (thoda04)
		**	    Created to match the specification.
		*/

		private System.Collections.ArrayList exList;
		int     exCursor = 0;

		internal Exception getNextException()
		{
			if (exList == null  ||  exCursor >= exList.Count)
			{
				exCursor = 0;
				return null;
			}

			return (Exception)(exList[exCursor++]);
		}


		/*
		** Name: setNextException
		**
		** Description:
		**	Chain the next SqlEx chained into this SqlEx.
		**
		** Input:
		**	ex	    SqlEx.
		**
		** Ouptut:
		**	None.
		**
		** Returns:
		**	SqlEx	    The exception object.
		**
		** History:
		**	24-Feb-04 (thoda04)
		**	    Created to match the specification.
		*/

		internal void setNextException(Exception ex)
		{
			if (exList == null)
			{
				exList = new System.Collections.ArrayList();
			}

			exList.Add(ex);
			return;
		}



		/*
		** Name: createProviderException
		**
		** Description:
		**	Create an IngresException from the SqlEx exception
		**	using the IngresExceptionFactory interface.
		**
		** Input:
		**	this.
		**
		** Ouptut:
		**	None.
		**
		** Returns:
		**	IngresException	    The exception object.
		**
		** History:
		**	24-Feb-04 (thoda04)
		**	    Created.
		**	20-Aug-09 (thoda04)
		**	    Trap SecurityException if running with less than
		**	    FullTrust and user application does not have
		**	    EnvironmentPermission=Unrestricted.  Fixes Bug 122511.
		*/

		/// <summary>
		/// Create an IngresException from an SqlEx exception.
		/// </summary>
		/// <returns>An IngresException.</returns>
		internal Exception createProviderException()
		{
			Exception innerException;
			string callStack;

			while( (innerException = this.getNextException()) != null )
			{
				if ( !(innerException is SqlEx) )  // look for a non-SqlEx
					break;  // and use it as the InnerException
			}
			exCursor = 0;  // reset the exlist cursor

			try
			{
				callStack = Environment.StackTrace;

				// find and strip the frames prior to the current stack frame
				int i = callStack.IndexOf("createProviderException");
				if (i > -1)  // should not happen
					i = callStack.IndexOf(Environment.NewLine, i);  // find end
				if (i > -1)  // should not happen
					i = callStack.IndexOf(Environment.NewLine,
						i + Environment.NewLine.Length);  // find end of one more
				if (i > -1)
					callStack = callStack.Substring(i);  // strip prior and current
			}
			catch (SecurityException)
			{
				// System.Environment.StackTrace required
				//    EnvironmentPermission=Unrestricted
				callStack = String.Empty;
			}


			Exception providerEx =  // create an IngresException
				providerExFactory.CreateInstance(
					this.Message,
					this.getSQLState(),
					this.getErrorCode(),
					innerException,
					this.StackTrace + callStack);

			Exception ex;
			while( (ex = this.getNextException()) != null )
			{
				// look for all SqlEx and append them as error records
				SqlEx sqlEx = ex as SqlEx;
				if ( sqlEx != null )
				{
					// add new error messages to existing IngresException
					providerExFactory.AddError(
						providerEx,
						sqlEx.Message,
						sqlEx.getSQLState(),
						sqlEx.getErrorCode());
				}
			}

			return providerEx;
		}



		/*
		** Name: get
		**
		** Description:
		**	Generate an SqlEx exception from an ErrInfo constant.
		**
		** Input:
		**	err	    ErrInfo constant.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	SqlEx	    The new exception object.
		**
		** History:
		**	20-Apr-00 (gordy)
		**	    Created.
		**	11-Sep-02 (gordy)
		**	    Renamed class to remove specific product reference.
		*/

		public static SqlEx
			get( ErrInfo err )
		{
			String msg;
			try { msg = ErrRsrc.getResource().getString( err.id ); }
			catch( Exception ){ msg = err.name; }

			return( new SqlEx( msg, err.sqlState, err.code ) );
		} // Get


		/*
		** Name: get
		**
		** Description:
		**	Generate an SqlEx exception from an message, SQLState,
		**	and a native code.  The general exception is
		**	converted into an SqlEx exception and chained to the
		**	SqlEx exception created from the error information.
		**
		** Input:
		**	err	    ErrInfo constant
		**	ex	    Associated exception.
		**
		** Ouptut:
		**	None.
		**
		** Returns:
		**	SqlEx	    The new exception object.
		**
		** History:
		**	09-Dec-03 (thoda04)
		**	    Created.
		*/

		public static SqlEx
			get( String message, String sqlState, int nativeError )
		{
			return( new SqlEx( message, sqlState, nativeError ) );
		} // get


		/*
		** Name: get
		**
		** Description:
		**	Generate an SqlEx exception from an message, SQLState,
		**	and a native code.  The general exception is
		**	converted into an SqlEx exception and chained to the
		**	SqlEx exception created from the error information.
		**
		** Input:
		**	err	    ErrInfo constant
		**	ex	    Associated exception.
		**
		** Ouptut:
		**	None.
		**
		** Returns:
		**	SqlEx	    The new exception object.
		**
		** History:
		**	09-Dec-03 (thoda04)
		**	    Created.
		*/

		public static SqlEx
			get( SqlEx ex, Exception exAddition )
		{
			ex.setNextException( exAddition );
			return ex;
		} // get


		/*
		** Name: get
		**
		** Description:
		**	Generate an SqlEx exception from an ErrInfo constant
		**	and another SqlEx exception.  The existing SqlEx
		**	exception is chained to the SqlEx exception created
		**	from the error information.
		**
		** Input:
		**	err	    ErrInfo constant.
		**	ex	    Associated exception,
		**
		** Output:
		**	None.
		**
		** Returns:
		**	SqlEx	    The new exception object.
		**
		** History:
		**	20-Apr-00 (gordy)
		**	    Created.
		**	11-Sep-02 (gordy)
		**	    Renamed class to remove specific product reference.
		*/

		public static SqlEx get( ErrInfo err, SqlEx ex )
		{
			SqlEx ex_list = get( err );
			ex_list.setNextException( ex );
			return( ex_list );
		} // get


		/*
		** Name: get
		**
		** Description:
		**	Generate an SqlEx exception from an ErrInfo constant
		**	and a general exception.  The general exception is
		**	converted into an SqlEx exception and chained to the
		**	SqlEx exception created from the error information.
		**
		** Input:
		**	err	    ErrInfo constant
		**	ex	    Associated exception.
		**
		** Ouptut:
		**	None.
		**
		** Returns:
		**	SqlEx	    The new exception object.
		**
		** History:
		**	20-Apr-00 (gordy)
		**	    Created.
		**	11-Sep-02 (gordy)
		**	    Renamed class to remove specific product reference.
		*/

		public static SqlEx get( ErrInfo err, Exception ex )
		{
			SqlEx   sqlEx;
			String  msg = ex.Message;

			if ( msg == null )  msg = ex.ToString();
			sqlEx = (msg == null) ? get( err ) : get( err, new SqlEx( msg ) );

			return( sqlEx );
		} // Get


		/*
		** Name: trace
		**
		** Description:
		**	Writes the exception information to the trace log.
		**	Any chained exceptions are also traced.
		**
		** Input:
		**	trace	Tracing output.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	20-Apr-00 (gordy)
		**	    Created.
		**	12-Apr-01 (gordy)
		**	    Tracing changes require tracing to be passed as parameter.
		**	11-Sep-02 (gordy)
		**	    Renamed class to remove specific product reference.
		*/

		protected internal virtual void
			trace( ITrace itrace )
		{
			SqlEx ex = this;
			//			for( SqlEx ex = this; ex != null; ex = (SqlEx)ex.getNextException() )
		{
			//				itrace.write( "Exception: " + ex.getSQLState() + ", 0x" + 
			//					Integer.toHexString( ex.getErrorCode() ) );
			itrace.write( "  Message: " + ex.Message );
		}
			return;
		}


		/*
		** Name: ErrInfo
		**
		** Description:
		**	Class containing information for a single error code.
		**
		**  Public Data
		**
		**	code	    Numeric error code.
		**	sqlState    Associated SQL State.
		**	id	    Resource key.
		**	name	    Full name.
		**
		** History:
		**	10-Sep-99 (gordy)
		**	    Created.
		*/

		internal sealed class ErrInfo
		{

			/*
			** Name: ErrInfo
			**
			** Description:
			**	Class constructor.
			**
			** Input:
			**	code	    Error code.
			**	sqlState    SQL State.
			**	id	    Resource key.
			**	name	    Error name.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	None.
			**
			** History:
			**	10-Sep-99 (gordy)
			**	    Created.
			*/

			public
				ErrInfo( int code, String sqlState, String id, String name )
			{
				this._code     = code;
				this._sqlState = sqlState;
				this._id       = id;
				this._name     = name;
			} // ErrInfo

			private  int     _code;
			public int code
			{
				get { return _code; }
			}

			private  String  _sqlState;
			public   String   sqlState
			{
				get { return _sqlState; }
			}

			private  String  _id;
			public   String   id
			{
				get { return _id; }
			}

			private  String  _name;
			public   String   name
			{
				get { return _name; }
			}

			public String Msg
			{
				get
				{
					String msg;
			
					try
					{
						msg = ErrRsrc.getResource().getString(this.id);
					}
					catch (System.Exception /* ignore */)
					{
						msg = this.name;
					}
					return msg;
				}
			}

		} // class ErrInfo

	} // class SqlEx


	internal class SQLWarningCollection : System.Collections.ArrayList
	{
	}  // SQLWarningCollection


	internal interface ISqlExFactory
	{
//		Exception CreateInstance(
//			SqlEx.ErrInfo errinfo);

//		Exception CreateInstance(
//			SqlEx.ErrInfo errinfo, Exception innerException);

//		Exception CreateInstance(
//			String message);
//
//		Exception CreateInstance(
//			String message, Exception innerException);

//		Exception CreateInstance(
//			String message,
//			String sqlState,
//			int    nativeError);

		Exception CreateInstance(
			String message,
			String sqlState,
			int    nativeError,
			Exception innerException,
			String    stackTrace);

		Exception AddError(
			Exception ex,
			String message,
			String sqlState,
			int    nativeError);


	}  // ISqlExFactory

}  // namespace
