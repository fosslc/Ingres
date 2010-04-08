/*
** Copyright (c) 2002, 2008 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.Runtime.InteropServices;

namespace Ingres.ProviderInternals
{
	/*
	** Name: advanxid.cs
	**
	** Description:
	**	Implements the driver class: AdvanXID.
	**
	** History:
	**	 2-Apr-01 (gordy)
	**	    Created.
	**	11-Apr-01 (gordy)
	**	    Support tracing.
	**	26-Aug-02 (thoda04)
	**	    Ported for .NET Provider.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	06-mar-08 (thoda04)
	**	    Fixed up the ToString display of the bqual.
	*/



	/*
	** Name: AdvanXID
	**
	** Description:
	**	Driver class which implements 
	**	Ingres distributed transaction ID support.
	**
	**  Constants:
	**
	**	XID_INGRES		Ingres XID.
	**	XID_XA    		XA XID.
	**
	**  Public Properties:
	**
	**	Type     	Returns Transaction ID type: Ingres, XA.
	**	XId      	Returns Ingres Transaction ID.
	**	FormatId 	Returns the XA Format ID.
	**
	**  Public Methods:
	**
	**	getGlobalTransactionId	Returns the XA Global Transaction ID.
	**	getBranchQualifier    	Returns the XA Branch Qualifier.
	**	trace                 	Write XID to trace log.
	**
	**  Private Data
	**
	**	type 			XID type: Ingres, XA.
	**	xid  			Ingres Transaction ID.
	**	fid  			XA Format ID.
	**	gtid 			XA Global Transaction ID.
	**	bqual			XA Branch Qualifier.
	**
	** History:
	**	 2-Apr-01 (gordy)
	**	    Created.
	**	11-Apr-01 (gordy)
	**	    Added trace().*/
	
	/// <summary>
	/// Internal XID support class.
	/// </summary>
	[CLSCompliant(false)]
	public class AdvanXID
	{
		internal const int XID_INGRES = 1;
		internal const int XID_XA     = 2;
		
		internal int    type = 0;
		internal long   xid = 0;
		internal int    fid  = 0;
		internal byte[] gtid = null;
		internal byte[] bqual = null;
		
		
		/*
		** Name: AdvanXID
		**
		** Description:
		**	Class constructor for an Ingres XID.
		**
		** Input:
		**	xid	Ingres Transaction ID.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	 2-Apr-01 (gordy)
		**	    Created.*/
		
		/// <summary>
		/// Constructor for an Ingres XID.
		/// </summary>
		/// <param name="xid"></param>
		public AdvanXID(long xid)
		{
			this.type = XID_INGRES;
			this.xid = xid;
		}
		// AdvanXID
		
		
		/*
		** Name: AdvanXID
		**
		** Description:
		**	Class constructor for an XA XID.
		**
		** Input:
		**	fid	XA Format ID.
		**	gtid	XA Global Transaction ID.
		**	bqual	XA Branch Qualifier.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	 2-Apr-01 (gordy)
		**	    Created.*/
		
		/// <summary>
		/// Constructor for an XA XID.
		/// </summary>
		/// <param name="fid"></param>
		/// <param name="gtid"></param>
		/// <param name="bqual"></param>
		public AdvanXID(int fid, byte[] gtid, byte[] bqual)
		{
			this.type = XID_XA;
			this.fid = fid;
			this.gtid = gtid;
			this.bqual = bqual;
		}  // AdvanXID
		
		
		/*
		** Name: AdvanXID
		**
		** Description:
		**	Class constructor for an XA XID using XID structure.
		**
		** Input:
		**	fid	XA Format ID.
		**	gtid	XA Global Transaction ID.
		**	bqual	XA Branch Qualifier.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	13-Oct-03 (thoda04)
		**	    Created.
		*/
		
		internal AdvanXID(XAXIDStruct xid)
		{
			this.type = XID_XA;
			this.fid = xid.formatID;
			this.gtid = new Byte[xid.gtridLength];
			this.bqual= new Byte[xid.bqualLength];
			Array.Copy(xid.data,               0,
				this.gtid, 0, xid.gtridLength);
			Array.Copy(xid.data,xid.gtridLength,
				this.bqual,0, xid.bqualLength);
		}  // AdvanXID
		
		
		/*
		** Name: AdvanXID
		**
		** Description:
		**	Class constructor for an XA XID using a string.
		**
		** Input:
		**	string s	string representation of XID in 
		**				form of "formatid:gtridlen:bquallen:data:XA".
		**
		** Output:
		**	None.
		**
		** Returns:
		**	AdvanXID.
		**
		** History:
		**	 2-Apr-01 (gordy)
		**	    Created.*/
		
		/// <summary>
		/// Constructor for an XA XID for a given string input.
		/// </summary>
		/// <param name="s"></param>
		public AdvanXID(string s)
		{
			byte[] gtid = new byte[0];
			byte[] bqual= gtid;
			int i, j;
			int index;
			int gtridLength = 0;
			int bqualLength=0;
			string token;

			this.type = XID_XA;
			this.fid = -1;
			this.gtid = gtid;
			this.bqual = bqual;

			if (s == null  ||  s.Length < 4)
				return;

			index = s.IndexOf(":XA");  // check if not a known format
			if (index == -1)           // if unexpected format, return
				return;
			s = s.Substring(0, index);  // chop off the ":XA"

			index = s.IndexOf(':');  // find end of format id
			if (index == -1)
				return;
			token = s.Substring(0, index);
			this.fid = Convert.ToInt32(token, 16);// format id
			s = s.Substring(index+1);

			index = s.IndexOf(':');  // find end of gtrid length
			if (index == -1)         // if unexpected format, return
				return;
			token = s.Substring(0, index);
			gtridLength = Convert.ToInt16(token, 10);
			s = s.Substring(index+1);

			index = s.IndexOf(':');  // find end of bqual length
			if (index == -1)         // if unexpected format, return
				return;
			token = s.Substring(0, index);
			bqualLength = Convert.ToInt16(token, 10);
			s = s.Substring(index+1);

			System.Text.StringBuilder sb = new System.Text.StringBuilder(100);
			foreach (char c in s)
				if (c != ':')
					sb.Append(c);
			s = sb.ToString();

			if (s.Length == 0  ||  s.Length != 2*(gtridLength+bqualLength))
				return;

			this.gtid = gtid  = new byte[gtridLength];
			this.bqual= bqual = new byte[bqualLength];

			i = 0;

			j = 0;
			for (j = 0; j < gtridLength; j++)  // fill in gtrid byte array
			{
				gtid[j] = Convert.ToByte(s.Substring(i,2),16);
				i += 2;
			}

			j = 0;
			for (j = 0; j < bqualLength; j++)  // fill in bqual byte array
			{
				bqual[j]= Convert.ToByte(s.Substring(i,2),16);
				i += 2;
			}
		}  // AdvanXID
		
		
		/*
		** Name: Type property
		**
		** Description:
		**	Returns the XID type: Ingres or XA.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	Transaction type.
		**
		** History:
		**	 2-Apr-01 (gordy)
		**	    Created.*/
		
		/// <summary>
		/// Returns the XID type: Ingres or XA.
		/// </summary>
		public virtual int Type
		{
			get  { return type;  }
			
		} // Type
		
		
		/*
		** Name: XId property
		**
		** Description:
		**	Returns the Ingres Transaction ID.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	long	Ingres Transaction ID.
		**
		** History:
		**	 2-Apr-01 (gordy)
		**	    Created.*/
		
		/// <summary>
		/// Returns the Ingres Transaction ID.
		/// </summary>
		public virtual long XId
		{
			get  { return (xid); }
			
		} // XId
		
		
		/*
		** Name: FormatId property
		**
		** Description:
		**	Returns the XA Format ID.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	XA Format ID.
		**
		** History:
		**	 2-Apr-01 (gordy)
		**	    Created.*/
		
		/// <summary>
		/// Returns the XA Format ID.
		/// </summary>
		public virtual int FormatId
		{
			get  { return (fid); }
			
		} // FormatId
		
		
		/*
		** Name: getGlobalTransactionId
		**
		** Description:
		**	Returns the XA Global Transaction ID.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	byte[]	XA Global Transaction ID.
		**
		** History:
		**	 2-Apr-01 (gordy)
		**	    Created.*/
		
		/// <summary>
		/// Returns the XA Global Transaction ID.
		/// </summary>
		/// <returns></returns>
		public virtual byte[] getGlobalTransactionId()
		{
			return gtid;
			
		} // getGlobalTransactionId
		
		
		/*
		** Name: getBranchQualifier
		**
		** Description:
		**	Returns the XA Branch Qualifier.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	byte[]	XA Branch Qualifier.
		**
		** History:
		**	 2-Apr-01 (gordy)
		**	    Created.*/
		
		/// <summary>
		/// Returns the XA Branch Qualifier.
		/// </summary>
		/// <returns></returns>
		public virtual byte[] getBranchQualifier()
		{
			return bqual;
		} // getBranchQualifier
		
		
		/*
		** Name: Equals
		**
		** Description:
		**	Determine if this Xid is identical to another Xid.
		**
		** Input:
		**	xid	Xid to compare.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	True if identical, False otherwise.
		**
		** History:
		**	15-Mar-01 (gordy)
		**	    Created.
		**	29-Sep-03 (thoda04)
		**	    Port to .NET provider.
		*/
		
		/// <summary>
		/// Determine if this Xid is identical to another Xid.
		/// </summary>
		/// <param name="xid"></param>
		/// <returns></returns>
		public virtual bool Equals(AdvanXID xid)
		{
			if (xid == null)
				return false;

			if (this == xid)
				return (true);
			if (fid != xid.FormatId)
				return (false);
			
			byte[] gtid  = xid.getGlobalTransactionId();
			byte[] bqual = xid.getBranchQualifier();
			
			if (this.gtid.Length != gtid.Length ||
				this.bqual.Length != bqual.Length)
				return (false);
			
			for (int i = 0; i < gtid.Length; i++)
				if (this.gtid[i] != gtid[i])
					return (false);
			
			for (int i = 0; i < bqual.Length; i++)
				if (this.bqual[i] != bqual[i])
					return (false);
			
			return (true);
		}  // Equals


		/*
		** Name: ToString
		**
		** Description:
		**	Produce an XID string in human-readable form.
		**
		** Input:
		**	xid	Xid to convert to string.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	string	human-readable form of XID.
		**
		** History:
		**	30-Sep-03 (thoda04)
		**	    Created.
		*/
		
		/// <summary>
		/// Produce an XID string in human-readable form.
		/// </summary>
		/// <returns></returns>
		public override string ToString()
		{
			System.Text.StringBuilder sb = new System.Text.StringBuilder(100);

			Byte[] gtrid = this.getGlobalTransactionId();
			byte[] bqual = this.getBranchQualifier();

			sb.Append(this.FormatId.ToString("X8")); // format id in hex
			sb.Append(':');
			sb.Append(gtrid.Length.ToString());      // gtrid len in decimal
			sb.Append(':');
			sb.Append(bqual.Length.ToString());      // bqual len in decimal

			Byte[] data = new Byte[gtrid.Length + bqual.Length];
			Array.Copy(gtrid, data, gtrid.Length);
			Array.Copy(bqual, 0, data, gtrid.Length, bqual.Length);

			for (int i = 0; i < data.Length; i++)    // data in hex
			{
				if (i%4 == 0)
					sb.Append(':');
				Byte b = data[i];
				sb.Append(b.ToString("X2"));
			}
			sb.Append(":XA");                        // end with ":XA"

			return sb.ToString();
		}


		/*
		** Name: trace
		**
		** Description:
		**	Write XID to trace log.
		**
		** Input:
		**	trace	    Internal tracing.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	11-Apr-01 (gordy)*/
		
		internal virtual void  trace(Ingres.Utility.ITrace itrace)
		{
			if (itrace == null)
				return;

			switch (type)
			{
				case XID_INGRES: 
					itrace.write("II XID: 0x" + System.Convert.ToString(xid, 16));
					break;
					
				
				
				case XID_XA: 
					itrace.write("XA XID: FormatId = 0x" + System.Convert.ToString(fid, 16));
					itrace.write("GlobalTransactionId: ");
					itrace.hexdump(gtid, 0, gtid.Length);
					itrace.write("BranchQualifier: ");
					itrace.hexdump(bqual, 0, bqual.Length);
					break;
				
			}
			return ;
		} // trace
	} // class AdvanXID



	/// <summary>
	/// XA XID class.
	/// </summary>
	[StructLayout(LayoutKind.Sequential,CharSet=CharSet.Ansi)]
	public class XAXID
	{
		/// <summary>
		/// Format identifier.
		/// </summary>
		public int formatID;      // format identifier; DTC = 0x00445443
		/// <summary>
		/// Length of global transaction identifier.
		/// </summary>
		public int gtridLength;
		/// <summary>
		/// Length of branch qualifier.
		/// </summary>
		public int bqualLength;
		/// <summary>
		/// Global and branch transaction identifier.
		/// </summary>
		[MarshalAs(UnmanagedType.ByValArray, SizeConst=128)]
		public byte[] data;

		/// <summary>
		/// Constructor for an XA XID for a given format id,
		/// gtrid length, bqual length, and data.
		/// </summary>
		/// <param name="_formatId"></param>
		/// <param name="_gtridLength"></param>
		/// <param name="_bqualLength"></param>
		/// <param name="_data"></param>
		public XAXID(int _formatId, int _gtridLength, int _bqualLength, byte[] _data)
		{
			formatID     = _formatId;
			gtridLength = _gtridLength;
			bqualLength = _bqualLength;
			data         = _data;
		}
	}  // class XAXID



	[StructLayout(LayoutKind.Sequential,CharSet=CharSet.Ansi)]
	internal struct XAXIDStruct
	{
		public int formatID;      // format identifier; DTC = 0x00445443
		public int gtridLength;  // length of global tx identifier
		public int bqualLength;  // length of branch qualifier
		[MarshalAs(UnmanagedType.ByValArray, SizeConst=128)]
		public byte[] data;        // global and branch tx id

		public XAXIDStruct(
			int _formatId, int _gtridLength, int _bqualLength, byte[] _data)
		{
			formatID     = _formatId;
			gtridLength = _gtridLength;
			bqualLength = _bqualLength;
			data         = _data;
		}
	}  // struct XAXIDStruct



}  // namespace
