namespace System
{
    /// <summary>
    /// Provides the base class for enumerations
    /// </summary>
    public abstract class Enum : ValueType
    {
        public override string ToString()
        {
            // TODO: Implement proper enum name lookup
            return GetType().FullName;
        }

        public override bool Equals(object obj)
        {
            if (obj == null)
                return false;
            if (GetType() != obj.GetType())
                return false;
            return base.Equals(obj);
        }

        public override int GetHashCode()
        {
            return base.GetHashCode();
        }

        public static bool IsDefined(Type enumType, object value)
        {
            // TODO: Implement
            return false;
        }

        public static string GetName(Type enumType, object value)
        {
            // TODO: Implement
            return value?.ToString() ?? "";
        }

        public static string[] GetNames(Type enumType)
        {
            // TODO: Implement
            return new string[0];
        }

        public static Array GetValues(Type enumType)
        {
            // TODO: Implement
            return null;
        }
    }
}
