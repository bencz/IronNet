namespace System
{
    internal static class ComparableType
    {
        internal static int InvalidObject(string typeName)
        {
            throw new ArgumentException("Object must be of type " + typeName + ".");
        }
    }

    public partial struct Boolean : IComparable, IComparable<bool>, IEquatable<bool>
    {
        public int CompareTo(bool value)
        {
            return this == value ? 0 : (this ? 1 : -1);
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
            {
                return 1;
            }
            if (!(obj is bool))
            {
                return ComparableType.InvalidObject("Boolean");
            }

            return CompareTo((bool)obj);
        }

        public bool Equals(bool value)
        {
            return this == value;
        }
    }

    public partial struct Char : IComparable, IComparable<char>, IEquatable<char>
    {
        public int CompareTo(char value)
        {
            return this < value ? -1 : (this > value ? 1 : 0);
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
            {
                return 1;
            }
            if (!(obj is char))
            {
                return ComparableType.InvalidObject("Char");
            }

            return CompareTo((char)obj);
        }

        public bool Equals(char value)
        {
            return this == value;
        }
    }

    public partial struct SByte : IComparable, IComparable<sbyte>, IEquatable<sbyte>
    {
        public int CompareTo(sbyte value)
        {
            return this < value ? -1 : (this > value ? 1 : 0);
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
            {
                return 1;
            }
            if (!(obj is sbyte))
            {
                return ComparableType.InvalidObject("SByte");
            }

            return CompareTo((sbyte)obj);
        }

        public bool Equals(sbyte value)
        {
            return this == value;
        }
    }

    public partial struct Byte : IComparable, IComparable<byte>, IEquatable<byte>
    {
        public int CompareTo(byte value)
        {
            return this < value ? -1 : (this > value ? 1 : 0);
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
            {
                return 1;
            }
            if (!(obj is byte))
            {
                return ComparableType.InvalidObject("Byte");
            }

            return CompareTo((byte)obj);
        }

        public bool Equals(byte value)
        {
            return this == value;
        }
    }

    public partial struct Int16 : IComparable, IComparable<short>, IEquatable<short>
    {
        public int CompareTo(short value)
        {
            return this < value ? -1 : (this > value ? 1 : 0);
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
            {
                return 1;
            }
            if (!(obj is short))
            {
                return ComparableType.InvalidObject("Int16");
            }

            return CompareTo((short)obj);
        }

        public bool Equals(short value)
        {
            return this == value;
        }
    }

    public partial struct UInt16 : IComparable, IComparable<ushort>, IEquatable<ushort>
    {
        public int CompareTo(ushort value)
        {
            return this < value ? -1 : (this > value ? 1 : 0);
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
            {
                return 1;
            }
            if (!(obj is ushort))
            {
                return ComparableType.InvalidObject("UInt16");
            }

            return CompareTo((ushort)obj);
        }

        public bool Equals(ushort value)
        {
            return this == value;
        }
    }

    public partial struct Int32 : IComparable, IComparable<int>, IEquatable<int>
    {
        public int CompareTo(int value)
        {
            return this < value ? -1 : (this > value ? 1 : 0);
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
            {
                return 1;
            }
            if (!(obj is int))
            {
                return ComparableType.InvalidObject("Int32");
            }

            return CompareTo((int)obj);
        }

        public bool Equals(int value)
        {
            return this == value;
        }
    }

    public partial struct UInt32 : IComparable, IComparable<uint>, IEquatable<uint>
    {
        public int CompareTo(uint value)
        {
            return this < value ? -1 : (this > value ? 1 : 0);
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
            {
                return 1;
            }
            if (!(obj is uint))
            {
                return ComparableType.InvalidObject("UInt32");
            }

            return CompareTo((uint)obj);
        }

        public bool Equals(uint value)
        {
            return this == value;
        }
    }

    public partial struct Int64 : IComparable, IComparable<long>, IEquatable<long>
    {
        public int CompareTo(long value)
        {
            return this < value ? -1 : (this > value ? 1 : 0);
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
            {
                return 1;
            }
            if (!(obj is long))
            {
                return ComparableType.InvalidObject("Int64");
            }

            return CompareTo((long)obj);
        }

        public bool Equals(long value)
        {
            return this == value;
        }
    }

    public partial struct UInt64 : IComparable, IComparable<ulong>, IEquatable<ulong>
    {
        public int CompareTo(ulong value)
        {
            return this < value ? -1 : (this > value ? 1 : 0);
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
            {
                return 1;
            }
            if (!(obj is ulong))
            {
                return ComparableType.InvalidObject("UInt64");
            }

            return CompareTo((ulong)obj);
        }

        public bool Equals(ulong value)
        {
            return this == value;
        }
    }

    public partial struct Single : IComparable, IComparable<float>, IEquatable<float>
    {
        public int CompareTo(float value)
        {
            if (this < value)
            {
                return -1;
            }
            if (this > value)
            {
                return 1;
            }
            if (this == value)
            {
                return 0;
            }

            return IsNaN(this) ? (IsNaN(value) ? 0 : -1) : 1;
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
            {
                return 1;
            }
            if (!(obj is float))
            {
                return ComparableType.InvalidObject("Single");
            }

            return CompareTo((float)obj);
        }

        public bool Equals(float value)
        {
            return this == value || (IsNaN(this) && IsNaN(value));
        }
    }

    public partial struct Double : IComparable, IComparable<double>, IEquatable<double>
    {
        public int CompareTo(double value)
        {
            if (this < value)
            {
                return -1;
            }
            if (this > value)
            {
                return 1;
            }
            if (this == value)
            {
                return 0;
            }

            return IsNaN(this) ? (IsNaN(value) ? 0 : -1) : 1;
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
            {
                return 1;
            }
            if (!(obj is double))
            {
                return ComparableType.InvalidObject("Double");
            }

            return CompareTo((double)obj);
        }

        public bool Equals(double value)
        {
            return this == value || (IsNaN(this) && IsNaN(value));
        }
    }

    public partial struct IntPtr : IComparable, IComparable<IntPtr>, IEquatable<IntPtr>
    {
        public unsafe int CompareTo(IntPtr value)
        {
            long left = (long)_value;
            long right = (long)value._value;
            return left < right ? -1 : (left > right ? 1 : 0);
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
            {
                return 1;
            }
            if (!(obj is IntPtr))
            {
                return ComparableType.InvalidObject("IntPtr");
            }

            return CompareTo((IntPtr)obj);
        }

        public unsafe bool Equals(IntPtr value)
        {
            return _value == value._value;
        }
    }

    public partial struct UIntPtr : IComparable, IComparable<UIntPtr>, IEquatable<UIntPtr>
    {
        public unsafe int CompareTo(UIntPtr value)
        {
            ulong left = (ulong)_value;
            ulong right = (ulong)value._value;
            return left < right ? -1 : (left > right ? 1 : 0);
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
            {
                return 1;
            }
            if (!(obj is UIntPtr))
            {
                return ComparableType.InvalidObject("UIntPtr");
            }

            return CompareTo((UIntPtr)obj);
        }

        public unsafe bool Equals(UIntPtr value)
        {
            return _value == value._value;
        }
    }

    public sealed partial class String : IComparable, IComparable<string>, IEquatable<string>
    {
        public int CompareTo(string value)
        {
            return Compare(this, value);
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
            {
                return 1;
            }
            if (!(obj is string))
            {
                return ComparableType.InvalidObject("String");
            }

            return CompareTo((string)obj);
        }

        public bool Equals(string value)
        {
            return Equals(this, value);
        }
    }

    public partial struct TimeSpan : IComparable, IComparable<TimeSpan>, IEquatable<TimeSpan>
    {
        public int CompareTo(TimeSpan value)
        {
            return Compare(this, value);
        }

        public int CompareTo(object value)
        {
            if (value == null)
            {
                return 1;
            }
            if (!(value is TimeSpan))
            {
                return ComparableType.InvalidObject("TimeSpan");
            }

            return CompareTo((TimeSpan)value);
        }

        public bool Equals(TimeSpan value)
        {
            return _ticks == value._ticks;
        }
    }

    public partial struct DateTime : IComparable, IComparable<DateTime>, IEquatable<DateTime>
    {
        public int CompareTo(DateTime value)
        {
            return Compare(this, value);
        }

        public int CompareTo(object value)
        {
            if (value == null)
            {
                return 1;
            }
            if (!(value is DateTime))
            {
                return ComparableType.InvalidObject("DateTime");
            }

            return CompareTo((DateTime)value);
        }

        public bool Equals(DateTime value)
        {
            return _ticks == value._ticks;
        }
    }

    public partial struct DateTimeOffset : IComparable, IComparable<DateTimeOffset>, IEquatable<DateTimeOffset>
    {
        public int CompareTo(DateTimeOffset value)
        {
            return Compare(this, value);
        }

        public int CompareTo(object value)
        {
            if (value == null)
            {
                return 1;
            }
            if (!(value is DateTimeOffset))
            {
                return ComparableType.InvalidObject("DateTimeOffset");
            }

            return CompareTo((DateTimeOffset)value);
        }
    }
}
