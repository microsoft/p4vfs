// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.IO;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Collections.Generic;

namespace Microsoft.P4VFS.Extensions.Utilities
{
	public static class Converters
	{
		public struct Result<T>
		{
			public T Value;
			public bool Success;
		}

		public static Result<Int32> ToInt32(object value, Int32 defaultValue = 0)
		{
			return ParseType(value, defaultValue, Int32.TryParse);
		}

		public static Result<Int64> ToInt64(object value, Int64 defaultValue = 0)
		{
			return ParseType(value, defaultValue, Int64.TryParse);
		}

		public static Result<Single> ToFloat(object value, Single defaultValue = 0)
		{
			return ParseType(value, defaultValue, Single.TryParse);
		}

		public static Result<Double> ToDouble(object value, Double defaultValue = 0)
		{
			return ParseType(value, defaultValue, Double.TryParse);
		}

		public static Result<Boolean> ToBoolean(object value, Boolean defaultValue = false)
		{
			return ParseType(value, defaultValue, Boolean.TryParse);
		}

		public static Result<string> ToString(object value, string defaultValue = null)
		{
			if (value == null)
				return new Result<string>{ Value=defaultValue, Success=false };
			if (value is bool)
				return new Result<string>{ Value=(bool)value?"true":"false", Success=true };
			return new Result<string>{ Value=value.ToString(), Success=true };
		}

		public static Result<TEnum> ToEnum<TEnum>(object value, TEnum defaultValue = default(TEnum))
		{
			return ParseType(value, defaultValue);
		}

		public static Result<T> ParseType<T>(object value, T defaultValue = default(T))
		{
			try
			{
				if (typeof(T).IsEnum)
					return FromResult<T, object>(ParseType(value, (object)defaultValue, TryParseEnum<T>));
				if (typeof(T) == typeof(Int32))
					return FromResult<T, Int32>(ToInt32(value, (Int32)(object)defaultValue));
				if (typeof(T) == typeof(Int64))
					return FromResult<T, Int64>(ToInt64(value, (Int64)(object)defaultValue));
				if (typeof(T) == typeof(Single))
					return FromResult<T, Single>(ToFloat(value, (Single)(object)defaultValue));
				if (typeof(T) == typeof(Double))
					return FromResult<T, Double>(ToDouble(value, (Double)(object)defaultValue));
				if (typeof(T) == typeof(Boolean))
					return FromResult<T, Boolean>(ToBoolean(value, (Boolean)(object)defaultValue));
				if (typeof(T) == typeof(String))
					return FromResult<T, string>(ToString(value, (string)(object)defaultValue));
			}
			catch {}
			return new Result<T>{ Value=defaultValue, Success=false };
		}

		private static Result<T> ParseType<T>(object value, T defaultValue, TryParseDelegate<T> tryParse)
		{
			if (value == null)
				return new Result<T>{ Value=defaultValue, Success=false };
			if (typeof(T) != typeof(object) && value is T)
				return new Result<T>{ Value=(T)value, Success=true };
			
			string valueText = value as string;
			if (valueText == null)
				valueText = ToString(value).Value;
			
			if (String.IsNullOrWhiteSpace(valueText) == false)
			{
				T result;
				if (tryParse(valueText, out result))
					return new Result<T>{ Value=result, Success=true };
			}
			return new Result<T>{ Value=defaultValue, Success=false };
		}

		private static Result<T> FromResult<T, Q>(Result<Q> result)
		{
			return new Result<T>{ Value=(T)(object)result.Value, Success=result.Success };
		}

		private static bool TryParseEnum<TEnum>(string value, out object result)
		{
			try
			{
				if (typeof(TEnum).IsEnum)
				{
					if (Int32.TryParse(value, out int ivalue))
					{
						if (Enum.IsDefined(typeof(TEnum), ivalue))
						{
							result = Enum.ToObject(typeof(TEnum), ivalue);
							return true;
						}
					}
					else if (Enum.IsDefined(typeof(TEnum), value))
					{
						result = Enum.Parse(typeof(TEnum), value);
						return true;
					}
				}
			}
			catch {}
			result = null;
			return false;
		}

		private delegate bool TryParseDelegate<T>(string value, out T result);
	}
}

