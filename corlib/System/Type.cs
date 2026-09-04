using System.Runtime.CompilerServices;
using System.Reflection;

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
        public extern string Name { [MethodImpl(MethodImplOptions.InternalCall)] get; }

        /// <summary>
        /// Gets the fully qualified name of the type
        /// </summary>
        public extern string FullName { [MethodImpl(MethodImplOptions.InternalCall)] get; }

        /// <summary>
        /// Gets the namespace of the Type
        /// </summary>
        public extern string Namespace { [MethodImpl(MethodImplOptions.InternalCall)] get; }

        /// <summary>
        /// Gets the type from which the current Type directly inherits
        /// </summary>
        public extern Type BaseType { [MethodImpl(MethodImplOptions.InternalCall)] get; }

        /// <summary>
        /// Gets the type that declares the current nested type
        /// </summary>
        public extern Type DeclaringType { [MethodImpl(MethodImplOptions.InternalCall)] get; }

        public extern TypeAttributes Attributes { [MethodImpl(MethodImplOptions.InternalCall)] get; }

        /// <summary>
        /// Gets the assembly in which the type is declared
        /// </summary>
        public Assembly Assembly => Assembly.GetAssembly(this);

        /// <summary>
        /// Gets a value indicating whether the Type is a class
        /// </summary>
        public extern bool IsClass { [MethodImpl(MethodImplOptions.InternalCall)] get; }

        /// <summary>
        /// Gets a value indicating whether the Type is a value type
        /// </summary>
        public extern bool IsValueType { [MethodImpl(MethodImplOptions.InternalCall)] get; }

        /// <summary>
        /// Gets a value indicating whether the Type is an interface
        /// </summary>
        public extern bool IsInterface { [MethodImpl(MethodImplOptions.InternalCall)] get; }

        /// <summary>
        /// Gets a value indicating whether the Type is an array
        /// </summary>
        public extern bool IsArray { [MethodImpl(MethodImplOptions.InternalCall)] get; }

        /// <summary>
        /// Gets a value indicating whether the Type is an enumeration
        /// </summary>
        public extern bool IsEnum { [MethodImpl(MethodImplOptions.InternalCall)] get; }

        public extern bool HasElementType { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public extern bool IsPointer { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public extern bool IsByRef { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public extern bool IsGenericType { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public extern bool IsGenericTypeDefinition { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public extern bool IsGenericParameter { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public extern bool ContainsGenericParameters { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public extern int GenericParameterPosition { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public extern GenericParameterAttributes GenericParameterAttributes { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public extern MethodBase DeclaringMethod { [MethodImpl(MethodImplOptions.InternalCall)] get; }

        public bool IsAbstract => (Attributes & TypeAttributes.Abstract) != 0;
        public bool IsSealed => (Attributes & TypeAttributes.Sealed) != 0;
        public bool IsPublic => (Attributes & TypeAttributes.VisibilityMask) == TypeAttributes.Public;
        public bool IsNested => DeclaringType != null;

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern Type GetElementType();

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern int GetArrayRank();

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern Type[] GetGenericArguments();

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern Type[] GetGenericParameterConstraints();

        public Type GetGenericTypeDefinition()
        {
            if (!IsGenericType)
            {
                throw new InvalidOperationException("This operation is only valid on a generic type.");
            }

            return GetGenericTypeDefinitionCore();
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern Type GetGenericTypeDefinitionCore();

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern Type[] GetInterfaces();

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern Type MakeGenericType(Type[] typeArguments);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern Type MakeArrayType();

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern Type MakeArrayType(int rank);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern Type MakePointerType();

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern Type MakeByRefType();

        public MethodInfo[] GetMethods()
        {
            return GetMethods(BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static | BindingFlags.FlattenHierarchy);
        }

        public MethodInfo[] GetMethods(BindingFlags bindingAttr)
        {
            return GetMethodsCore(bindingAttr);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern MethodInfo[] GetMethodsCore(BindingFlags bindingAttr);

        public MethodInfo GetMethod(string name)
        {
            return GetMethod(name, BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static | BindingFlags.FlattenHierarchy);
        }

        public MethodInfo GetMethod(string name, BindingFlags bindingAttr)
        {
            if (name == null)
            {
                throw new ArgumentNullException("name");
            }

            MethodInfo match = null;
            MethodInfo[] methods = GetMethods(bindingAttr);
            bool ignoreCase = (bindingAttr & BindingFlags.IgnoreCase) != 0;
            for (int index = 0; index < methods.Length; index++)
            {
                if (!NamesEqual(methods[index].Name, name, ignoreCase))
                {
                    continue;
                }
                if (match != null)
                {
                    throw new AmbiguousMatchException("More than one method matches the specified name.");
                }

                match = methods[index];
            }

            return match;
        }

        public MethodInfo GetMethod(string name, Type[] types)
        {
            return GetMethod(name, BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static | BindingFlags.FlattenHierarchy, types);
        }

        public MethodInfo GetMethod(string name, BindingFlags bindingAttr, Type[] types)
        {
            if (name == null)
            {
                throw new ArgumentNullException("name");
            }
            if (types == null)
            {
                throw new ArgumentNullException("types");
            }

            MethodInfo[] methods = GetMethods(bindingAttr);
            bool ignoreCase = (bindingAttr & BindingFlags.IgnoreCase) != 0;
            for (int methodIndex = 0; methodIndex < methods.Length; methodIndex++)
            {
                MethodInfo method = methods[methodIndex];
                if (!NamesEqual(method.Name, name, ignoreCase))
                {
                    continue;
                }

                ParameterInfo[] parameters = method.GetParameters();
                if (parameters.Length != types.Length)
                {
                    continue;
                }

                bool matches = true;
                for (int parameterIndex = 0; parameterIndex < parameters.Length; parameterIndex++)
                {
                    if (types[parameterIndex] == null)
                    {
                        throw new ArgumentNullException("types");
                    }
                    if (parameters[parameterIndex].ParameterType != types[parameterIndex])
                    {
                        matches = false;
                        break;
                    }
                }
                if (matches)
                {
                    return method;
                }
            }

            return null;
        }

        public ConstructorInfo[] GetConstructors()
        {
            return GetConstructors(BindingFlags.Public | BindingFlags.Instance);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern ConstructorInfo[] GetConstructors(BindingFlags bindingAttr);

        public ConstructorInfo GetConstructor(Type[] types)
        {
            if (types == null)
            {
                throw new ArgumentNullException("types");
            }

            ConstructorInfo[] constructors = GetConstructors();
            for (int constructorIndex = 0; constructorIndex < constructors.Length; constructorIndex++)
            {
                ParameterInfo[] parameters = constructors[constructorIndex].GetParameters();
                if (parameters.Length != types.Length)
                {
                    continue;
                }

                bool matches = true;
                for (int parameterIndex = 0; parameterIndex < parameters.Length; parameterIndex++)
                {
                    if (types[parameterIndex] == null)
                    {
                        throw new ArgumentNullException("types");
                    }
                    if (parameters[parameterIndex].ParameterType != types[parameterIndex])
                    {
                        matches = false;
                        break;
                    }
                }
                if (matches)
                {
                    return constructors[constructorIndex];
                }
            }

            return null;
        }

        public FieldInfo[] GetFields()
        {
            return GetFields(BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static | BindingFlags.FlattenHierarchy);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern FieldInfo[] GetFields(BindingFlags bindingAttr);

        public FieldInfo GetField(string name)
        {
            return GetField(name, BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static | BindingFlags.FlattenHierarchy);
        }

        public FieldInfo GetField(string name, BindingFlags bindingAttr)
        {
            if (name == null)
            {
                throw new ArgumentNullException("name");
            }

            FieldInfo[] fields = GetFields(bindingAttr);
            bool ignoreCase = (bindingAttr & BindingFlags.IgnoreCase) != 0;
            for (int index = 0; index < fields.Length; index++)
            {
                if (NamesEqual(fields[index].Name, name, ignoreCase))
                {
                    return fields[index];
                }
            }

            return null;
        }

        public PropertyInfo[] GetProperties()
        {
            return GetProperties(BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static | BindingFlags.FlattenHierarchy);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern PropertyInfo[] GetProperties(BindingFlags bindingAttr);

        public PropertyInfo GetProperty(string name)
        {
            return GetProperty(name, BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static | BindingFlags.FlattenHierarchy);
        }

        public PropertyInfo GetProperty(string name, BindingFlags bindingAttr)
        {
            if (name == null)
            {
                throw new ArgumentNullException("name");
            }

            PropertyInfo match = null;
            PropertyInfo[] properties = GetProperties(bindingAttr);
            bool ignoreCase = (bindingAttr & BindingFlags.IgnoreCase) != 0;
            for (int index = 0; index < properties.Length; index++)
            {
                if (!NamesEqual(properties[index].Name, name, ignoreCase))
                {
                    continue;
                }
                if (match != null)
                {
                    throw new AmbiguousMatchException("More than one property matches the specified name.");
                }

                match = properties[index];
            }

            return match;
        }

        public EventInfo[] GetEvents()
        {
            return GetEvents(BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static | BindingFlags.FlattenHierarchy);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern EventInfo[] GetEvents(BindingFlags bindingAttr);

        public EventInfo GetEvent(string name)
        {
            return GetEvent(name, BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static | BindingFlags.FlattenHierarchy);
        }

        public EventInfo GetEvent(string name, BindingFlags bindingAttr)
        {
            if (name == null)
            {
                throw new ArgumentNullException("name");
            }

            EventInfo[] events = GetEvents(bindingAttr);
            bool ignoreCase = (bindingAttr & BindingFlags.IgnoreCase) != 0;
            for (int index = 0; index < events.Length; index++)
            {
                if (NamesEqual(events[index].Name, name, ignoreCase))
                {
                    return events[index];
                }
            }

            return null;
        }

        public Type[] GetNestedTypes()
        {
            return GetNestedTypes(BindingFlags.Public);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern Type[] GetNestedTypes(BindingFlags bindingAttr);

        public Type GetNestedType(string name)
        {
            return GetNestedType(name, BindingFlags.Public);
        }

        public Type GetNestedType(string name, BindingFlags bindingAttr)
        {
            if (name == null)
            {
                throw new ArgumentNullException("name");
            }

            Type[] nestedTypes = GetNestedTypes(bindingAttr);
            bool ignoreCase = (bindingAttr & BindingFlags.IgnoreCase) != 0;
            for (int index = 0; index < nestedTypes.Length; index++)
            {
                if (NamesEqual(nestedTypes[index].Name, name, ignoreCase))
                {
                    return nestedTypes[index];
                }
            }

            return null;
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern bool IsAssignableFrom(Type type);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern bool IsInstanceOfType(object obj);

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

        private static bool NamesEqual(string left, string right, bool ignoreCase)
        {
            if (!ignoreCase)
            {
                return left == right;
            }
            if (left == null || right == null || left.Length != right.Length)
            {
                return false;
            }

            for (int index = 0; index < left.Length; index++)
            {
                char leftCharacter = left[index];
                char rightCharacter = right[index];
                if (leftCharacter >= 'A' && leftCharacter <= 'Z')
                {
                    leftCharacter = (char)(leftCharacter + ('a' - 'A'));
                }
                if (rightCharacter >= 'A' && rightCharacter <= 'Z')
                {
                    rightCharacter = (char)(rightCharacter + ('a' - 'A'));
                }
                if (leftCharacter != rightCharacter)
                {
                    return false;
                }
            }

            return true;
        }
    }
}
