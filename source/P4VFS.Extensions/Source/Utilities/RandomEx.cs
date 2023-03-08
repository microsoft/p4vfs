// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Threading;
using System.Text;

namespace Microsoft.P4VFS.Extensions.Utilities
{
	/// <summary>
	/// Random number generator that is not thread-safe but it is safe to create on multiple threads at the same time (unlike System.Random)  
	/// It also has additional random number functionality.
	/// </summary>
	public class RandomEx
	{
		protected static int seed = 0;
		protected static int initialSeed = 0;
		protected Random RandGen;

		static RandomEx()
		{
			initialSeed = (int)DateTime.Now.Ticks;
			seed = initialSeed;
		}

		public RandomEx()
		{
			RandGen = new Random(Interlocked.Increment(ref seed));
		}

		public int Next()
		{
			return RandGen.Next();
		}

		public int Next(int maximum)
		{
			return RandGen.Next(maximum);
		}

		public int Next(int minimum, int maximum)
		{
			return RandGen.Next(minimum, maximum);
		}

		public float Next(float maximum)
		{
			return (float)RandGen.NextDouble() * maximum;
		}

		public double NextDouble()
		{
			return RandGen.NextDouble();
		}

		public bool NextBool()
		{
			return RandGen.Next() > (Int32.MaxValue / 2);
		}

		public void NextBytes(byte[] buffer)
		{
			RandGen.NextBytes(buffer);
		}

		static public int GetInstanceCount()
		{
			return seed - initialSeed;
		}

		/// <summary>This will generate a random string.  The entire string is composed of chars in the 65-127 range</summary>
		static readonly string validChars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
		static readonly int validCharCount = validChars.Length;

		public string GetRandomString(int length)
		{
			StringBuilder name = new StringBuilder(length);

			// Add some time dependent chatacters
			for (int i = 0; i < length; i++)
			{
				int id = (char)RandGen.Next(validCharCount);
				name.Append(validChars[id]);
			}

			return name.ToString();
		}

		/// <summary>Returns an Int32 that logarithmically tends towards 0</summary>
		#region Random number distribution
		// (calculated from 10,000,000 samples)
		//49.99799
		//25.00818
		//12.49884
		// 6.25495
		// 3.12409
		// 1.55705
		// 0.77807
		// 0.39020
		// 0.19451
		// 0.09827
		// 0.04867
		// 0.02443
		// 0.01233
		// 0.00618
		// 0.00315
		// 0.00151
		// 0.00084
		// 0.00034
		// 0.00019
		// 0.00011
		// 0.00005
		// 0.00001
		// 0.00003
		// 0.00001
		#endregion
		public int NextLog()
		{
			return (int)(31 - Math.Log(RandGen.Next() + 1, 2));
		}
	}

	public class RandomFast
	{
		private UInt32 m_Seed;
		
		public RandomFast()
		{
			InitSeed((UInt32)DateTime.Now.Ticks);
		}

		public RandomFast(UInt32 seed)
		{
			InitSeed(seed);
		}

		public void InitSeed(UInt32 seed)
		{
			m_Seed = seed;
		}

		public UInt32 Next()
		{
			m_Seed = (UInt32)(((UInt64)m_Seed * 214013UL + 2531011UL) & UInt32.MaxValue);
			return (UInt32)((m_Seed >> 16) & 0x7fff); 
		}
	}
}
