using System.Runtime.CompilerServices;

namespace System
{
    /// <summary>
    /// Represents type declarations: class types, interface types, array types, value types, enumeration types
    /// </summary>
    public abstract class Type
    {
        /// <summary>
        /// Gets the name of the current type
        /// </summary>
        public abstract string Name { get; }

        /// <summary>
        /// Gets the fully qualified name of the type
        /// </summary>
        public abstract string FullName { get; }

        /// <summary>
        /// Gets the namespace of the Type
        /// </summary>
        public abstract string Namespace { get; }

        /// <summary>
        /// Gets the type from which the current Type directly inherits
        /// </summary>
        public abstract Type BaseType { get; }

        /// <summary>
        /// Gets a value indicating whether the Type is a class
        /// </summary>
        public abstract bool IsClass { get; }

        /// <summary>
        /// Gets a value indicating whether the Type is a value type
        /// </summary>
        public abstract bool IsValueType { get; }

        /// <summary>
        /// Gets a value indicating whether the Type is an interface
        /// </summary>
        public abstract bool IsInterface { get; }

        /// <summary>
        /// Gets a value indicating whether the Type is an array
        /// </summary>
        public abstract bool IsArray { get; }

        /// <summary>
        /// Gets a value indicating whether the Type is an enumeration
        /// </summary>
        public abstract bool IsEnum { get; }

        /// <summary>
        /// Gets the Type with the specified name from the specified assembly
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern Type GetTypeFromHandle(RuntimeTypeHandle handle);

        /// <summary>
        /// Returns a String representing the name of the current Type
        /// </summary>
        public override string ToString()
        {
            return FullName ?? Name ?? "Type";
        }

        /// <summary>
        /// Determines whether two Type objects are equal
        /// </summary>
        public override bool Equals(object obj)
        {
            if (obj is Type other)
                return this == other;
            return false;
        }

        /// <summary>
        /// Returns the hash code for this instance
        /// </summary>
        public override int GetHashCode()
        {
            string name = FullName;
            if (name == null)
                return 0;
            return name.GetHashCode();
        }
    }
}
