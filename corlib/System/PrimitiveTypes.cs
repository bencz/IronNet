namespace System
{
    /// <summary>
    /// Represents a Boolean (true or false) value
    /// </summary>
    public struct Boolean
    {
        public static readonly string TrueString = "True";
        public static readonly string FalseString = "False";

        public override string ToString()
        {
            return this ? TrueString : FalseString;
        }

        public override int GetHashCode()
        {
            return this ? 1 : 0;
        }
    }

    /// <summary>
    /// Represents a Unicode character
    /// </summary>
    public struct Char
    {
        public const char MaxValue = (char)0xFFFF;
        public const char MinValue = (char)0x0000;

        public override string ToString()
        {
            char[] arr = new char[1];
            arr[0] = this;
            return new string(arr);
        }

        public static bool IsWhiteSpace(char c)
        {
            return c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
                   c == '\v' || c == '\f';
        }

        public static bool IsDigit(char c)
        {
            return c >= '0' && c <= '9';
        }

        public static bool IsLetter(char c)
        {
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        }

        public static bool IsLetterOrDigit(char c)
        {
            return IsLetter(c) || IsDigit(c);
        }

        public static bool IsUpper(char c)
        {
            return c >= 'A' && c <= 'Z';
        }

        public static bool IsLower(char c)
        {
            return c >= 'a' && c <= 'z';
        }

        public static char ToUpper(char c)
        {
            if (c >= 'a' && c <= 'z')
                return (char)(c - 32);
            return c;
        }

        public static char ToLower(char c)
        {
            if (c >= 'A' && c <= 'Z')
                return (char)(c + 32);
            return c;
        }
    }

    /// <summary>
    /// Represents an 8-bit signed integer
    /// </summary>
    public struct SByte
    {
        public const sbyte MaxValue = 127;
        public const sbyte MinValue = -128;

        public override string ToString()
        {
            return Int32Extensions.ToString((int)this);
        }
    }

    /// <summary>
    /// Represents an 8-bit unsigned integer
    /// </summary>
    public struct Byte
    {
        public const byte MaxValue = 255;
        public const byte MinValue = 0;

        public override string ToString()
        {
            return Int32Extensions.ToString((int)this);
        }
    }

    /// <summary>
    /// Represents a 16-bit signed integer
    /// </summary>
    public struct Int16
    {
        public const short MaxValue = 32767;
        public const short MinValue = -32768;

        public override string ToString()
        {
            return Int32Extensions.ToString((int)this);
        }
    }

    /// <summary>
    /// Represents a 16-bit unsigned integer
    /// </summary>
    public struct UInt16
    {
        public const ushort MaxValue = 65535;
        public const ushort MinValue = 0;

        public override string ToString()
        {
            return Int32Extensions.ToString((int)this);
        }
    }

    /// <summary>
    /// Represents a 32-bit signed integer
    /// </summary>
    public struct Int32
    {
        public const int MaxValue = 2147483647;
        public const int MinValue = -2147483648;

        public override string ToString()
        {
            return Int32Extensions.ToString(this);
        }

        public static int Parse(string s)
        {
            if (s == null)
                throw new ArgumentNullException("s");
            
            int result = 0;
            int sign = 1;
            int i = 0;

            if (s.Length > 0 && s[0] == '-')
            {
                sign = -1;
                i = 1;
            }

            for (; i < s.Length; i++)
            {
                if (s[i] < '0' || s[i] > '9')
                    throw new FormatException("Invalid character in number");
                result = result * 10 + (s[i] - '0');
            }

            return result * sign;
        }
    }

    /// <summary>
    /// Represents a 32-bit unsigned integer
    /// </summary>
    public struct UInt32
    {
        public const uint MaxValue = 4294967295;
        public const uint MinValue = 0;

        public override string ToString()
        {
            return Int32Extensions.ToStringUnsigned(this);
        }
    }

    /// <summary>
    /// Represents a 64-bit signed integer
    /// </summary>
    public struct Int64
    {
        public const long MaxValue = 9223372036854775807;
        public const long MinValue = -9223372036854775808;

        public override string ToString()
        {
            return Int64Extensions.ToString(this);
        }
    }

    /// <summary>
    /// Represents a 64-bit unsigned integer
    /// </summary>
    public struct UInt64
    {
        public const ulong MaxValue = 18446744073709551615;
        public const ulong MinValue = 0;

        public override string ToString()
        {
            return Int64Extensions.ToStringUnsigned(this);
        }
    }

    /// <summary>
    /// Represents a single-precision floating-point number
    /// </summary>
    public struct Single
    {
        public const float MaxValue = 3.40282347E+38f;
        public const float MinValue = -3.40282347E+38f;
        public const float Epsilon = 1.401298E-45f;
        public const float NaN = 0.0f / 0.0f;
        public const float PositiveInfinity = 1.0f / 0.0f;
        public const float NegativeInfinity = -1.0f / 0.0f;

        public static bool IsNaN(float f)
        {
            return f != f;
        }

        public static bool IsInfinity(float f)
        {
            return f == PositiveInfinity || f == NegativeInfinity;
        }
    }

    /// <summary>
    /// Represents a double-precision floating-point number
    /// </summary>
    public struct Double
    {
        public const double MaxValue = 1.7976931348623157E+308;
        public const double MinValue = -1.7976931348623157E+308;
        public const double Epsilon = 4.94065645841247E-324;
        public const double NaN = 0.0 / 0.0;
        public const double PositiveInfinity = 1.0 / 0.0;
        public const double NegativeInfinity = -1.0 / 0.0;

        public static bool IsNaN(double d)
        {
            return d != d;
        }

        public static bool IsInfinity(double d)
        {
            return d == PositiveInfinity || d == NegativeInfinity;
        }
    }

    /// <summary>
    /// A platform-specific type for pointer arithmetic
    /// </summary>
    public struct IntPtr
    {
        private unsafe void* _value;

        public static readonly IntPtr Zero = new IntPtr(0);

        public unsafe IntPtr(int value)
        {
            _value = (void*)value;
        }

        public unsafe IntPtr(long value)
        {
            _value = (void*)value;
        }

        public unsafe IntPtr(void* value)
        {
            _value = value;
        }

        public static unsafe int Size
        {
            get { return sizeof(void*); }
        }

        public override unsafe int GetHashCode()
        {
            return (int)_value;
        }

        public override unsafe bool Equals(object obj)
        {
            if (obj is IntPtr other)
                return _value == other._value;
            return false;
        }

        public static unsafe bool operator ==(IntPtr a, IntPtr b)
        {
            return a._value == b._value;
        }

        public static unsafe bool operator !=(IntPtr a, IntPtr b)
        {
            return a._value != b._value;
        }
    }

    /// <summary>
    /// A platform-specific type for pointer arithmetic (unsigned)
    /// </summary>
    public struct UIntPtr
    {
        private unsafe void* _value;

        public static readonly UIntPtr Zero = new UIntPtr(0);

        public unsafe UIntPtr(uint value)
        {
            _value = (void*)value;
        }

        public unsafe UIntPtr(ulong value)
        {
            _value = (void*)value;
        }

        public unsafe UIntPtr(void* value)
        {
            _value = value;
        }

        public static unsafe int Size
        {
            get { return sizeof(void*); }
        }

        public override unsafe int GetHashCode()
        {
            return (int)_value;
        }

        public override unsafe bool Equals(object obj)
        {
            if (obj is UIntPtr other)
                return _value == other._value;
            return false;
        }

        public static unsafe bool operator ==(UIntPtr a, UIntPtr b)
        {
            return a._value == b._value;
        }

        public static unsafe bool operator !=(UIntPtr a, UIntPtr b)
        {
            return a._value != b._value;
        }
    }

    /// <summary>
    /// Helper class for integer to string conversion
    /// </summary>
    internal static class Int32Extensions
    {
        public static string ToString(int value)
        {
            if (value == 0)
                return "0";

            bool negative = value < 0;
            if (negative)
                value = -value;

            char[] buffer = new char[12];
            int pos = 11;

            while (value > 0)
            {
                buffer[pos--] = (char)('0' + value % 10);
                value /= 10;
            }

            if (negative)
                buffer[pos--] = '-';

            return new string(buffer, pos + 1, 11 - pos);
        }

        public static string ToStringUnsigned(uint value)
        {
            if (value == 0)
                return "0";

            char[] buffer = new char[11];
            int pos = 10;

            while (value > 0)
            {
                buffer[pos--] = (char)('0' + value % 10);
                value /= 10;
            }

            return new string(buffer, pos + 1, 10 - pos);
        }
    }

    /// <summary>
    /// Helper class for long to string conversion
    /// </summary>
    internal static class Int64Extensions
    {
        public static string ToString(long value)
        {
            if (value == 0)
                return "0";

            bool negative = value < 0;
            if (negative)
                value = -value;

            char[] buffer = new char[21];
            int pos = 20;

            while (value > 0)
            {
                buffer[pos--] = (char)('0' + value % 10);
                value /= 10;
            }

            if (negative)
                buffer[pos--] = '-';

            return new string(buffer, pos + 1, 20 - pos);
        }

        public static string ToStringUnsigned(ulong value)
        {
            if (value == 0)
                return "0";

            char[] buffer = new char[21];
            int pos = 20;

            while (value > 0)
            {
                buffer[pos--] = (char)('0' + value % 10);
                value /= 10;
            }

            return new string(buffer, pos + 1, 20 - pos);
        }
    }
}
