using System.Runtime.CompilerServices;

namespace System
{
    /// <summary>
    /// Base class for all .NET types
    /// </summary>
    public class Object
    {
        /// <summary>
        /// Initializes a new instance of the Object class
        /// </summary>
        public Object()
        {
        }

        /// <summary>
        /// Gets the Type of the current instance
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern Type GetType();

        /// <summary>
        /// Returns a hash code for this instance
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public virtual extern int GetHashCode();

        /// <summary>
        /// Determines whether the specified object is equal to the current object
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public virtual extern bool Equals(object obj);

        /// <summary>
        /// Determines whether two object instances are equal
        /// </summary>
        public static bool Equals(object objA, object objB)
        {
            if (objA == objB)
                return true;
            if (objA == null || objB == null)
                return false;
            return objA.Equals(objB);
        }

        /// <summary>
        /// Determines whether two object instances are the same instance
        /// </summary>
        public static bool ReferenceEquals(object objA, object objB)
        {
            return objA == objB;
        }

        /// <summary>
        /// Returns a string that represents the current object
        /// </summary>
        public virtual string ToString()
        {
            return GetType().FullName;
        }

        /// <summary>
        /// Creates a shallow copy of the current Object
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        protected extern object MemberwiseClone();

        /// <summary>
        /// Allows an object to try to free resources before garbage collection
        /// </summary>
        ~Object()
        {
        }
    }
}
