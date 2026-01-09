using System.Runtime.CompilerServices;

namespace System
{
    /// <summary>
    /// Provides the base class for value types
    /// </summary>
    public abstract class ValueType
    {
        public override bool Equals(object obj)
        {
            if (obj == null)
                return false;
            if (GetType() != obj.GetType())
                return false;
            return EqualsInternal(obj);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern bool EqualsInternal(object obj);

        public override int GetHashCode()
        {
            return GetHashCodeInternal();
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern int GetHashCodeInternal();

        public override string ToString()
        {
            return GetType().FullName;
        }
    }
}
