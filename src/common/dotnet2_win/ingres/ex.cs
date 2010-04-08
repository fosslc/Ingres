/*
** Copyright (c) 2002, 2006 Ingres Corporation. All Rights Reserved.
*/

	/*
	** Name: ex.cs
	**
	** Description:
	**	IngresException class.
	**	IngresExceptionFactory class.
	**
	** History:
	**	 5-Aug-02 (thoda04)
	**	    Created for .NET Provider.
	**	24-Feb-04 (thoda04)
	**	    Reworked SqlEx and IngresException to avoid forcing the user
	**	    to add a reference to their application to reference the
	**	    another assembly to resolve the SqlEx class at compile time.
	**	    We want the application to only reference the Client assembly.
	**	27-Feb-04 (thoda04)
	**	    Added additional XML Intellisense comments.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	14-jul-04 (thoda04)
	**	    Corrected XML comment for StackTrace.
	*/


using System;
using System.Runtime.Serialization;
using System.Security;
using System.Security.Permissions;
using Ingres.Utility;


namespace Ingres.Client 
{
	/*
	** Name: IngresException
	**
	** Description:
	**	Ingres exception class.
	**
	**  Public Properties:
	**
	**	Errors        	Gets a collection of IngresError objects.
	**	InnerException	Gets the Exception object that caused the current exception
	**	Message       	Gets the text of message that describes the exception
	**	StackTrace		Get the stack trace as a string at the time of exception
	**
	**  Public Methods:
	**
	**	GetObjectData	Used by .NET for object serialization.
	**
	*/

	/// <summary>
	/// The exception that is thrown when the data source
	/// returns a warning or error.
	/// </summary>
	//	Allow this type to be visible to COM.
	[System.Runtime.InteropServices.ComVisible(true)]
	[Serializable]
	public sealed class IngresException : SystemException
	{
		internal IngresException(String message) : base(message)
		{
			AddError(message, "00000", 0);
		}

		internal IngresException(String message, Exception innerException)
			:base(message, innerException)
		{
			AddError(message, "00000", 0);
			while ( innerException != null)
			{
				AddError(innerException.Message, "00000", 0);
				innerException = innerException.InnerException;
			}
		}

		internal IngresException(String message,
		                         String sqlState,
		                         int    nativeError)
			: base(message)
		{
			AddError(message, sqlState, nativeError);
		}

		internal IngresException(String message,
		                         String sqlState,
		                         int    nativeError,
		                         Exception innerException,
		                         string    stackTrace)
		: base(message, innerException)
		{
			this._stackTrace = stackTrace;
			AddError(message, sqlState, nativeError);
		}

		internal IngresException(SqlEx.ErrInfo errinfo)
			: base(errinfo.Msg)
		{
			AddError(errinfo.Msg, errinfo.sqlState, errinfo.code);
		}

		internal IngresException(SqlEx.ErrInfo errinfo, IngresException _innerException)
			:base(errinfo.Msg, _innerException)
		{
			AddError(errinfo.Msg,            errinfo.sqlState, errinfo.code);
			Exception innerException = _innerException;
			while ( innerException != null)
			{
				AddError(innerException.Message, errinfo.sqlState, errinfo.code);
				innerException = innerException.InnerException;
			}
		}

		internal IngresException(SqlEx.ErrInfo errinfo, Exception innerException)
			:base(errinfo.Msg, innerException)
		{
			AddError(errinfo.Msg,            errinfo.sqlState, errinfo.code);
			while ( innerException != null)
			{
				AddError(innerException.Message, errinfo.sqlState, errinfo.code);
				innerException = innerException.InnerException;
			}
		}


		/*
		** Name: AddError
		**
		** Description:
		**	Using the message string, SQLState, and nativeError
		**	(ususally coming from an ErrInfo object), create a new
		**	error collection and one error object in the collection.
		*/
		private void AddError(String message,
		                                          String sqlState,
		                                          int    nativeError)
		{
			// add IngresError object into list
			if (errors == null)
				errors = new IngresErrorCollection();
			errors.Add(message, sqlState, nativeError);
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
		private IngresException(SerializationInfo info, StreamingContext context)
			:base(info, context)     // Deserialization constructor
		{
			errors = (IngresErrorCollection)
				(info.GetValue("Errors", typeof(IngresErrorCollection)));
		}

		/// <summary>
		/// Sets the SerializationInfo with data about the IngresException.
		/// </summary>
		/// <param name="info"></param>
		/// <param name="context"></param>
		[SecurityPermissionAttribute(
			SecurityAction.Demand, SerializationFormatter=true)]
		public override void GetObjectData(SerializationInfo info,
		                          StreamingContext context)
			                         // Serialization method
		{
			info.AddValue("Errors", errors, typeof(IngresErrorCollection));

			// Let the base type serialize its fields
			base.GetObjectData(info, context);
		}

		/*
		** Name: Errors property
		**
		** Description:
		**	A collection of one or more IngresError objects that give
		**	more detailed information on the exception generated
		**	by the provider.
		**
		*/
		private IngresErrorCollection errors;
		/// <summary>
		/// A collection of one or more Error objects that give
		/// more detailed information on the exception generated
		/// by the provider.
		/// </summary>
		public  IngresErrorCollection Errors
		{
			get {return (errors); }
		}

		/*
		** Name: Message property
		**
		** Description:
		**	A concatenation of all the messages in the Errors collection.
		**
		*/
		/// <summary>
		/// A concatenation of all the messages in the Errors collection.
		/// </summary>
		public override string Message
		{
			get
			{
				bool newlineNeeded = false;
				string errString = "";
				foreach (IngresError err in this.Errors)
				{
					if  (newlineNeeded)   errString += "\n";
					else newlineNeeded = true;
					errString += err.Message;
				}
				return (errString);
			}
		}

		/*
		** Name: Source property
		**
		** Description:
		**	Name of the provider that generated the error.
		**
		*/
		/// <summary>
		/// Gets the name of the provider that created the error or warning.
		/// </summary>
		public override String Source
		{
			get {return (errors[0].Source); }
		}

		/*
		** Name: StackTrace property
		**
		** Description:
		**	The contents of the call stack.
		**
		*/
		private String _stackTrace;
		/// <summary>
		/// Gets the description of the frames on the call stack
		/// at the time when the exception was thrown.
		/// </summary>
		public override String StackTrace
		{
			get
			{
				if ( this._stackTrace == null )
					return (base.StackTrace);
				return this._stackTrace;
			}
		}

		/*
		** Name: ToString
		**
		** Description:
		**	A description of the error.
		**
		*/
		/// <summary>
		/// Full exception information returned by the data provider.
		/// </summary>
		public override String ToString()
		{
			return (this.GetType().ToString() + ": "
				                // "IngresException:"
				+ this.Message);
		}

	
	}  // class IngresException

	internal class IngresExceptionFactory : ISqlExFactory
	{
		#region ISqlExFactory Members

//		Exception ISqlExFactory.CreateInstance(
//			SqlEx.ErrInfo errinfo)
//		{
//			return new IngresException(errinfo);
//		}
//
//		Exception ISqlExFactory.CreateInstance(
//			SqlEx.ErrInfo errinfo, Exception innerException)
//		{
//			return new IngresException(errinfo, innerException);
//		}
//
//		Exception ISqlExFactory.CreateInstance(
//			String message)
//		{
//			return new IngresException(message);
//		}
//
//		Exception ISqlExFactory.CreateInstance(
//			String message, Exception innerException)
//		{
//			return new IngresException(message, innerException);
//		}
//
//		Exception ISqlExFactory.CreateInstance(
//			String message, String sqlState, int nativeError)
//		{
//			return new IngresException(message, sqlState, nativeError);
//		}
//
		Exception ISqlExFactory.CreateInstance(
			String    message,
			String    sqlState,
			int       nativeError,
			Exception innerException,
			String    stackTrace)
		{
			return new IngresException(
				message, sqlState, nativeError, innerException, stackTrace);
		}

		Exception ISqlExFactory.AddError(
			Exception ex,
			String message,
			String sqlState,
			int    nativeError)
		{
			IngresException provEx = ex as IngresException;
			if (provEx == null)  // if not an IngresException, return
				return ex;       // should not happen

			provEx.Errors.Add(
				new IngresError(message, sqlState, nativeError) );
			return ex;
		}

		#endregion

	}

}  // namespace
