using System.Runtime.CompilerServices;

namespace System
{
    /// <summary>
    /// Provides the base class for enumerations
    /// </summary>
    public abstract class Enum : ValueType
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        public override extern string ToString();

        [MethodImpl(MethodImplOptions.InternalCall)]
        public override extern bool Equals(object obj);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public override extern int GetHashCode();

        public static bool IsDefined(Type enumType, object value)
        {
            ValidateEnumType(enumType);
            if (value == null)
                throw new ArgumentNullException("value");

            return IsDefinedCore(enumType, value);
        }

        public static string GetName(Type enumType, object value)
        {
            ValidateEnumType(enumType);
            if (value == null)
                throw new ArgumentNullException("value");

            return GetNameCore(enumType, value);
        }

        public static string[] GetNames(Type enumType)
        {
            ValidateEnumType(enumType);
            return GetNamesCore(enumType);
        }

        public static Array GetValues(Type enumType)
        {
            ValidateEnumType(enumType);
            return GetValuesCore(enumType);
        }

        private static void ValidateEnumType(Type enumType)
        {
            if (enumType == null)
                throw new ArgumentNullException("enumType");
            if (!enumType.IsEnum)
                throw new ArgumentException("Type provided must be an Enum.");
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool IsDefinedCore(Type enumType, object value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern string GetNameCore(Type enumType, object value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern string[] GetNamesCore(Type enumType);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern Array GetValuesCore(Type enumType);
    }
}
