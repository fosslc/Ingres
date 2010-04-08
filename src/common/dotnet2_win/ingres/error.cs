/*
** Copyright (c) 2002, 2006 Ingres Corporation. All Rights Reserved.
*/

	/*
	** Name: error.cs
	**
	** Description:
	**	Error and error collection classes.
	**
	** .NET Framework Security:
	**	Requires EnvironmentPermission=Unrestricted.
	**
	** History:
	**	 5-Aug-02 (thoda04)
	**	    Created for .NET Provider.
	**	25-Feb-04 (thoda04)
	**	    Removed SetErrorStackTrace for better performance.
	**	27-Feb-04 (thoda04)
	**	    Added additional XML Intellisense comments.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	14-jul-04 (thoda04)
	**	    Added NativeError as synonym for Number.
	*/

using System;
using System.Collections;
using System.Security;
using Ingres.Utility;

namespace Ingres.Client
{
	/*
	** Name: IngresError
	**
	** Description:
	**	Defines one error or warning message returned from the data source.
	**
	** Public Properties:
	**	Message     Gets a short description of the error/warning.
	**	Number Gets the database specific error information.
	**	Source      Gets the name of the provider that created the error.
	**	SQLState    Gets the five character SQLState code
	**
	** Public Methods:
	**	ToString    Gets the text of the error/warning with a stack trace
	**	            in the form of "IngresError:" + <Message> + <StackTrace>.
	**
	*/

	/// <summary>
	/// Error or warning information returned by the data source.
	/// </summary>
	//	Allow this type to be visible to COM.
	[System.Runtime.InteropServices.ComVisible(true)]
	[Serializable]
	public sealed class IngresError
	{
		private String errorStackTrace; // stack trace at the time the error is created

		internal IngresError(String message, String sqlState, int nativeError)
		{
			this.message         = message;
			this.SQLstate        = sqlState;
			this.nativeError     = nativeError;
//			SetErrorStackTrace();
		}

		internal IngresError(SqlEx.ErrInfo errInfo)
		{
			this.message         = errInfo.Msg;
			this.SQLstate        = errInfo.sqlState;
			this.nativeError     = errInfo.code;
		}


		private void SetErrorStackTrace()
		{
			try
			{
				this.errorStackTrace = System.Environment.StackTrace;
			}
			catch (SecurityException)
			{
				// System.Environment.StackTrace required
				//    EnvironmentPermission=Unrestricted
				this.errorStackTrace = 
					"(stack trace not available due to security restrictions)";
			}
		}


		/*
		** Name: Message property
		**
		** Description:
		**	A description of the error.
		**
		*/
		private String message;
		/// <summary>
		/// Short description of the error or warning information
		/// returned by the data source.
		/// </summary>
		public  String Message
		{
			get {return (this.message); }
		}

		/*
		** Name: Number property
		**
		** Description:
		**	A description of the error.  A synonym for NativeError.
		**
		*/
		private int nativeError;
		/// <summary>
		/// Gets the database-specific error information
		/// returned by the data source.
		/// </summary>
		public  int Number
		{
			get {return (this.nativeError); }
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
		public String Source
		{
			get {return ("Ingres .NET Data Provider"); }
		}

		/*
		** Name: SQLState property
		**
		** Description:
		**	A description of the error.
		**
		*/
		private String SQLstate;
		/// <summary>
		/// Gets the five character error code.
		/// </summary>
		public  String SQLState
		{
			get {return (this.SQLstate); }
		}

		/*
		** Name: NativeError property
		**
		** Description:
		**	A description of the error.  A synonym for Number.
		**
		*/
		/// <summary>
		/// Gets the database-specific error information
		/// returned by the data source.
		/// </summary>
		public  int NativeError
		{
			get {return (this.nativeError); }
		}

		/*
		** Name: ToString
		**
		** Description:
		**	A description of the error.
		**
		*/
		/// <summary>
		/// Full error or warning information returned by the data provider
		/// including a stack trace.
		/// </summary>
		public override String ToString()
		{
			return (this.GetType().ToString() + ": "  // "IngresError:"
				+ this.Message);
			// + " " + this.errorStackTrace);
			// SqlError.ToString documentation says stacktrace is added to the
			// end of the message but tests demonstrate the documentation is
			// wrong.
		}
	}  // class IngresError


	/****************************************************************
	**              IngresErrorCollection
	*****************************************************************/

	/*
	** Name: IngresErrorCollection
	**
	** Description:
	**	A collection of error or warning information returned
	**	from the Ingres data source.
	**
	** Public Properties:
	**	Count       Gets the number of the errors/warnings in the collection.
	**	Item        Gets the error/warning at the specified zero-based
	**              index (C# indexer).
	**
	** Public Methods:
	**	ToString    Gets the text of the error/warning with a stack trace
	**	            in the form of "IngresError" + <Message> + <StackTrace>.
	**
	*/
	
	/// <summary>
	/// A collection of the error or warning information
	/// returned by the Ingres data source.
	/// </summary>
	//	Allow this type to be visible to COM.
	[System.Runtime.InteropServices.ComVisible(true)]
	[Serializable]
	public sealed class IngresErrorCollection : ICollection, IEnumerable
	{
		private ArrayList errorlist;

		internal IngresErrorCollection()
		{
			errorlist = new ArrayList();
		}

		/// <summary>
		/// Get the enumerator to iterate through the error collection.
		/// </summary>
		/// <returns>An IEnumerator to iterate
		/// through the error collection.</returns>
		public IEnumerator GetEnumerator()
		{
			return errorlist.GetEnumerator();
		}

		/// <summary>
		/// Get the error/warning at the specified zero-based index.
		/// </summary>
		public IngresError this[int index] 
		{
			get { return (IngresError)errorlist[index];  }
			set { errorlist[index] = value; }
		}

		/// <summary>
		/// Get the count of errors in the collection.
		/// </summary>
		public int Count 
		{
			get { return errorlist.Count; }
		}

		/// <summary>
		/// Returns true if access to the error collection is synchronized.
		/// </summary>
		public bool IsSynchronized       /* ICollection interface */
		{
			get { return false; }
		}
	
		/// <summary>
		/// Return an object that can the used to
		/// synchronize access to the collection.
		/// </summary>
		public object SyncRoot           /* ICollection interface */
		{
			get { return this; }
		}
	
		internal int Add(IngresError value)
		{
			return errorlist.Add(value);
		}

		internal int Add(String message, String sqlState, int nativeError)
		{
			return Add(new IngresError(message, sqlState, nativeError));
		}

		internal void Clear()
		{
			errorlist.Clear();
		}

		/// <summary>
		/// Copies the elements of the ErrorCollection into a one-dimension,
		/// zero-based  Array object, beginning at the specified zero-based index.
		/// </summary>
		void ICollection.CopyTo(Array array, int index)
		{
			errorlist.CopyTo(array, index);
		}

		/// <summary>
		/// Copies the elements of the ErrorCollection into a one-dimension,
		/// zero-based  Array object, beginning at the specified zero-based index.
		/// </summary>
		public void CopyTo(IngresError[] array, int index)
		{
			((ICollection)this).CopyTo(array, index);
		}

	}  // class IngresErrorCollection
}
