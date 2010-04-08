/*
** Copyright (c) 2002, 2006 Ingres Corporation. All Rights Reserved.
*/

using System;

namespace Ingres.Utility
{
	/*
	** Name: supportclass.cs
	**
	** Description:
	**	Various utility support routines.
	**
	** History:
	**	24-Jul-02 (thoda04)
	**	    Created.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	04-may-06 (thoda04)
	**	    Removed Suspend() and Resume() as unused and obsolete.
	*/
	
	
	internal sealed class SupportClass
{
	private SupportClass()
	{
	}

	/*******************************/
	internal class ThreadClass
	{
		private System.Threading.Thread threadField;

		public ThreadClass()
		{
			threadField = new System.Threading.Thread(new System.Threading.ThreadStart(Run));
		}

		public ThreadClass(System.Threading.ThreadStart p1)
		{
			threadField = new System.Threading.Thread(p1);
		}

		public virtual void Run()
		{
		}

		public virtual void Start()
		{
			threadField.Start();
		}

		public String Name
		{
			get
			{
				return threadField.Name;
			}
			set
			{
				if (threadField.Name == null)
					threadField.Name = value; 
			}
		}

		public System.Threading.ThreadPriority Priority
		{
			get
			{
				return threadField.Priority;
			}
			set
			{
				threadField.Priority = value;
			}
		}

		public bool IsAlive
		{
			get
			{
				return threadField.IsAlive;
			}
		}

		public bool IsBackground
		{
			get
			{
				return threadField.IsBackground;
			} 
			set
			{
				threadField.IsBackground = value;
			}
		}

		public virtual void Interrupt()
		{
			threadField.Interrupt();
		}

		public void Join()
		{
			threadField.Join();
		}

		public void Join(long p1)
		{
			lock(this)
			{
				threadField.Join(new System.TimeSpan(p1 * 10000));
			}
		}

		public void Join(long p1, int p2)
		{
			lock(this)
			{
				threadField.Join(new System.TimeSpan(p1 * 10000 + p2 * 100));
			}
		}

		public void Abort()
		{
			threadField.Abort();
		}

		public void Abort(System.Object stateInfo)
		{
			lock(this)
			{
				threadField.Abort(stateInfo);
			}
		}

		public override String ToString()
		{
			return "Thread[" + Name + "," + Priority.ToString() + "," + "" + "]";
		}
	}

}  // class SupportClass

}  // namespace
