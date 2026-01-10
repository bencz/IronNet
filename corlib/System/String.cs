using System.Runtime.CompilerServices;

namespace System
{
    /// <summary>
    /// Represents text as a sequence of UTF-16 code units
    /// </summary>
    public sealed class String
    {
        /// <summary>
        /// Represents the empty string
        /// </summary>
        public static readonly string Empty = "";

        // Required constructors for compiler
        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern String(char c, int count);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern String(char[] value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern String(char[] value, int startIndex, int length);

        public unsafe String(char* value)
        {
            // Internal call implementation
        }

        public unsafe String(char* value, int startIndex, int length)
        {
            // Internal call implementation
        }

        public char[] ToCharArray()
        {
            char[] chars = new char[Length];
            for (int i = 0; i < Length; i++)
                chars[i] = this[i];
            return chars;
        }

        public char[] ToCharArray(int startIndex, int length)
        {
            char[] chars = new char[length];
            for (int i = 0; i < length; i++)
                chars[i] = this[startIndex + i];
            return chars;
        }

        public string PadLeft(int totalWidth)
        {
            return PadLeft(totalWidth, ' ');
        }

        public string PadLeft(int totalWidth, char paddingChar)
        {
            if (totalWidth <= Length)
                return this;
            int padCount = totalWidth - Length;
            char[] result = new char[totalWidth];
            for (int i = 0; i < padCount; i++)
                result[i] = paddingChar;
            for (int i = 0; i < Length; i++)
                result[padCount + i] = this[i];
            return new string(result);
        }

        public string PadRight(int totalWidth)
        {
            return PadRight(totalWidth, ' ');
        }

        public string PadRight(int totalWidth, char paddingChar)
        {
            if (totalWidth <= Length)
                return this;
            char[] result = new char[totalWidth];
            for (int i = 0; i < Length; i++)
                result[i] = this[i];
            for (int i = Length; i < totalWidth; i++)
                result[i] = paddingChar;
            return new string(result);
        }

        /// <summary>
        /// Gets the number of characters in the current String object
        /// </summary>
        public extern int Length
        {
            [MethodImpl(MethodImplOptions.InternalCall)]
            get;
        }

        /// <summary>
        /// Gets the Char object at a specified position in the current String object
        /// </summary>
        public extern char this[int index]
        {
            [MethodImpl(MethodImplOptions.InternalCall)]
            get;
        }

        /// <summary>
        /// Concatenates two specified instances of String
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern string Concat(string str0, string str1);

        /// <summary>
        /// Concatenates three specified instances of String
        /// </summary>
        public static string Concat(string str0, string str1, string str2)
        {
            return Concat(Concat(str0, str1), str2);
        }

        /// <summary>
        /// Concatenates four specified instances of String
        /// </summary>
        public static string Concat(string str0, string str1, string str2, string str3)
        {
            return Concat(Concat(str0, str1), Concat(str2, str3));
        }

        /// <summary>
        /// Concatenates two objects
        /// </summary>
        public static string Concat(object arg0, object arg1)
        {
            return Concat(arg0?.ToString() ?? "", arg1?.ToString() ?? "");
        }

        /// <summary>
        /// Concatenates three objects
        /// </summary>
        public static string Concat(object arg0, object arg1, object arg2)
        {
            return Concat(Concat(arg0, arg1), arg2?.ToString() ?? "");
        }

        /// <summary>
        /// Concatenates an array of strings
        /// </summary>
        public static string Concat(params string[] values)
        {
            if (values == null || values.Length == 0)
                return Empty;
            string result = values[0] ?? "";
            for (int i = 1; i < values.Length; i++)
                result = Concat(result, values[i] ?? "");
            return result;
        }

        /// <summary>
        /// Concatenates an array of objects
        /// </summary>
        public static string Concat(params object[] args)
        {
            if (args == null || args.Length == 0)
                return Empty;
            string result = args[0]?.ToString() ?? "";
            for (int i = 1; i < args.Length; i++)
                result = Concat(result, args[i]?.ToString() ?? "");
            return result;
        }

        /// <summary>
        /// Determines whether two specified String objects have the same value
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool Equals(string a, string b);
        
        /// <summary>
        /// Determines whether two specified strings have the same value
        /// </summary>
        public static bool operator ==(string a, string b)
        {
            if ((object)a == null)
                return (object)b == null;
            if ((object)b == null)
                return false;
            return Equals(a, b);
        }
        
        /// <summary>
        /// Determines whether two specified strings have different values
        /// </summary>
        public static bool operator !=(string a, string b)
        {
            return !(a == b);
        }

        /// <summary>
        /// Determines whether this instance and a specified object have the same value
        /// </summary>
        public override bool Equals(object obj)
        {
            if (obj is string str)
                return Equals(this, str);
            return false;
        }

        /// <summary>
        /// Returns the hash code for this string
        /// </summary>
        public override int GetHashCode()
        {
            int hash = 0;
            for (int i = 0; i < Length; i++)
            {
                hash = hash * 31 + this[i];
            }
            return hash;
        }

        /// <summary>
        /// Returns this instance of String; no actual conversion is performed
        /// </summary>
        public override string ToString()
        {
            return this;
        }

        /// <summary>
        /// Indicates whether the specified string is null or empty
        /// </summary>
        public static bool IsNullOrEmpty(string value)
        {
            return value == null || value.Length == 0;
        }

        /// <summary>
        /// Indicates whether a specified string is null, empty, or consists only of white-space characters
        /// </summary>
        public static bool IsNullOrWhiteSpace(string value)
        {
            if (value == null) return true;
            for (int i = 0; i < value.Length; i++)
            {
                if (!char.IsWhiteSpace(value[i]))
                    return false;
            }
            return true;
        }

        /// <summary>
        /// Allocates a new string with the specified length (internal use)
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern string InternalAllocateStr(int length);

        /// <summary>
        /// Compares two strings
        /// </summary>
        public static int Compare(string strA, string strB)
        {
            if (strA == null)
                return strB == null ? 0 : -1;
            if (strB == null)
                return 1;

            int len = strA.Length < strB.Length ? strA.Length : strB.Length;
            for (int i = 0; i < len; i++)
            {
                if (strA[i] != strB[i])
                    return strA[i] - strB[i];
            }
            return strA.Length - strB.Length;
        }

        /// <summary>
        /// Reports the zero-based index of the first occurrence of a character
        /// </summary>
        public int IndexOf(char value)
        {
            for (int i = 0; i < Length; i++)
            {
                if (this[i] == value)
                    return i;
            }
            return -1;
        }

        /// <summary>
        /// Retrieves a substring from this instance
        /// </summary>
        public string Substring(int startIndex)
        {
            return Substring(startIndex, Length - startIndex);
        }

        /// <summary>
        /// Retrieves a substring from this instance
        /// </summary>
        public string Substring(int startIndex, int length)
        {
            if (startIndex < 0 || startIndex > Length)
                throw new ArgumentOutOfRangeException("startIndex");
            if (length < 0 || startIndex + length > Length)
                throw new ArgumentOutOfRangeException("length");
            if (length == 0)
                return Empty;

            string result = InternalAllocateStr(length);
            // Copy characters (would need unsafe or internal call)
            return result;
        }
    }
}
