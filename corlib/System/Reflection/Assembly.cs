using System.Runtime.CompilerServices;

namespace System.Reflection
{
    /// <summary>
    /// Represents an assembly, which is a reusable, versionable, and self-describing building block
    /// </summary>
    public abstract class Assembly
    {
        public abstract string FullName { get; }
        public abstract string Location { get; }

        public virtual string CodeBase => Location;

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern Assembly GetExecutingAssembly();

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern Assembly GetCallingAssembly();

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern Assembly GetEntryAssembly();

        public abstract Type[] GetTypes();

        public virtual Type GetType(string name)
        {
            return GetType(name, false, false);
        }

        public virtual Type GetType(string name, bool throwOnError)
        {
            return GetType(name, throwOnError, false);
        }

        public virtual Type GetType(string name, bool throwOnError, bool ignoreCase)
        {
            if (name == null)
            {
                throw new ArgumentNullException("name");
            }

            Type type = GetTypeCore(name, ignoreCase);
            if (type != null)
            {
                return type;
            }
            if (throwOnError)
            {
                throw new TypeLoadException("Type not found: " + name);
            }

            return null;
        }

        protected abstract Type GetTypeCore(string name, bool ignoreCase);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern Assembly GetAssembly(Type type);

        public virtual AssemblyName GetName()
        {
            return new AssemblyName(FullName);
        }

        public override string ToString()
        {
            return FullName ?? "Assembly";
        }
    }

    internal sealed class RuntimeAssembly : Assembly
    {
        private IntPtr _handle;

        internal RuntimeAssembly(IntPtr handle)
        {
            _handle = handle;
        }

        public override extern string FullName
        {
            [MethodImpl(MethodImplOptions.InternalCall)]
            get;
        }

        public override extern string Location
        {
            [MethodImpl(MethodImplOptions.InternalCall)]
            get;
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public override extern Type[] GetTypes();

        [MethodImpl(MethodImplOptions.InternalCall)]
        protected override extern Type GetTypeCore(string name, bool ignoreCase);
    }

    /// <summary>
    /// Describes an assembly's unique identity in full
    /// </summary>
    public sealed class AssemblyName
    {
        private string _publicKeyToken;

        public string Name { get; set; }
        public Version Version { get; set; }
        public string CultureName { get; set; }

        public AssemblyName()
        {
        }

        public AssemblyName(string assemblyName)
        {
            if (assemblyName == null)
            {
                throw new ArgumentNullException("assemblyName");
            }

            int commaIndex = assemblyName.IndexOf(',');
            Name = commaIndex >= 0 ? assemblyName.Substring(0, commaIndex) : assemblyName;
            int position = commaIndex >= 0 ? commaIndex + 1 : assemblyName.Length;
            while (position < assemblyName.Length)
            {
                while (position < assemblyName.Length && assemblyName[position] == ' ')
                {
                    position++;
                }

                int end = FindCharacter(assemblyName, ',', position);
                if (end < 0)
                {
                    end = assemblyName.Length;
                }

                if (MatchesPrefix(assemblyName, position, "Version="))
                {
                    Version = ParseVersion(assemblyName.Substring(position + 8, end - position - 8));
                }
                else if (MatchesPrefix(assemblyName, position, "Culture="))
                {
                    CultureName = assemblyName.Substring(position + 8, end - position - 8);
                }
                else if (MatchesPrefix(assemblyName, position, "PublicKeyToken="))
                {
                    _publicKeyToken = assemblyName.Substring(position + 15, end - position - 15);
                }

                position = end + 1;
            }
        }

        public string FullName
        {
            get
            {
                if (Name == null)
                {
                    return null;
                }

                string fullName = Name;
                if (Version != null)
                {
                    fullName += ", Version=" + Version;
                }
                if (CultureName != null)
                {
                    fullName += ", Culture=" + CultureName;
                }
                if (_publicKeyToken != null)
                {
                    fullName += ", PublicKeyToken=" + _publicKeyToken;
                }

                return fullName;
            }
        }

        private static int FindCharacter(string value, char character, int startIndex)
        {
            for (int index = startIndex; index < value.Length; index++)
            {
                if (value[index] == character)
                {
                    return index;
                }
            }

            return -1;
        }

        private static bool MatchesPrefix(string value, int startIndex, string prefix)
        {
            if (startIndex > value.Length - prefix.Length)
            {
                return false;
            }

            for (int index = 0; index < prefix.Length; index++)
            {
                if (value[startIndex + index] != prefix[index])
                {
                    return false;
                }
            }

            return true;
        }

        private static Version ParseVersion(string value)
        {
            int[] components = new int[4];
            int componentIndex = 0;
            int componentStart = 0;

            while (componentIndex < components.Length)
            {
                int separator = FindCharacter(value, '.', componentStart);
                int componentEnd = separator < 0 ? value.Length : separator;
                components[componentIndex++] = int.Parse(value.Substring(componentStart, componentEnd - componentStart));
                if (separator < 0)
                {
                    break;
                }

                componentStart = separator + 1;
            }

            if (componentIndex < 2)
            {
                throw new FormatException("An assembly version requires at least major and minor components.");
            }

            return new Version(components[0], components[1], componentIndex > 2 ? components[2] : 0, componentIndex > 3 ? components[3] : 0);
        }

        public override string ToString()
        {
            return FullName;
        }
    }

    /// <summary>
    /// The exception that is thrown when type-loading failures occur
    /// </summary>
    public class TypeLoadException : SystemException
    {
        public TypeLoadException() : base("Could not load type.") { }
        public TypeLoadException(string message) : base(message) { }
    }

    public class AmbiguousMatchException : SystemException
    {
        public AmbiguousMatchException() : base("Ambiguous match found.") { }
        public AmbiguousMatchException(string message) : base(message) { }
    }

    /// <summary>
    /// Discovers the attributes of a member and provides access to member metadata
    /// </summary>
    public abstract class MemberInfo
    {
        private IntPtr _handle;

        protected MemberInfo()
        {
        }

        internal MemberInfo(IntPtr handle)
        {
            _handle = handle;
        }

        protected void SetHandle(IntPtr handle)
        {
            _handle = handle;
        }

        public abstract string Name { get; }
        public abstract Type DeclaringType { get; }
        public abstract MemberTypes MemberType { get; }

        public virtual Type ReflectedType => DeclaringType;

        public override string ToString()
        {
            return Name ?? "MemberInfo";
        }

        public override bool Equals(object obj)
        {
            MemberInfo other = obj as MemberInfo;
            return other != null && GetType() == other.GetType() && _handle == other._handle;
        }

        public override int GetHashCode()
        {
            return _handle.GetHashCode();
        }
    }

    /// <summary>
    /// Marks each type of member that is defined as a derived class of MemberInfo
    /// </summary>
    [Flags]
    public enum MemberTypes
    {
        Constructor = 1,
        Event = 2,
        Field = 4,
        Method = 8,
        Property = 16,
        TypeInfo = 32,
        Custom = 64,
        NestedType = 128,
        All = Constructor | Event | Field | Method | Property | TypeInfo | NestedType
    }

    /// <summary>
    /// Discovers the attributes of a method and provides access to method metadata
    /// </summary>
    public abstract class MethodBase : MemberInfo
    {
        public abstract MethodAttributes Attributes { get; }
        public bool IsPublic => (Attributes & MethodAttributes.Public) != 0;
        public bool IsPrivate => (Attributes & MethodAttributes.Private) != 0;
        public bool IsStatic => (Attributes & MethodAttributes.Static) != 0;
        public bool IsVirtual => (Attributes & MethodAttributes.Virtual) != 0;
        public bool IsAbstract => (Attributes & MethodAttributes.Abstract) != 0;
        public abstract bool ContainsGenericParameters { get; }

        public abstract ParameterInfo[] GetParameters();

        public abstract object Invoke(object obj, object[] parameters);
    }

    /// <summary>
    /// Discovers the attributes of a method and provides access to method metadata
    /// </summary>
    public abstract class MethodInfo : MethodBase
    {
        public override MemberTypes MemberType => MemberTypes.Method;
        public abstract Type ReturnType { get; }
        public abstract bool IsGenericMethod { get; }
        public abstract bool IsGenericMethodDefinition { get; }
        public abstract Type[] GetGenericArguments();
        public abstract MethodInfo GetGenericMethodDefinition();
        public abstract MethodInfo MakeGenericMethod(Type[] typeArguments);
    }

    /// <summary>
    /// Discovers the attributes of a class constructor and provides access to constructor metadata
    /// </summary>
    public abstract class ConstructorInfo : MethodBase
    {
        public override MemberTypes MemberType => MemberTypes.Constructor;

        public object Invoke(object[] parameters)
        {
            return Invoke(null, parameters);
        }
    }

    /// <summary>
    /// Discovers the attributes of a field and provides access to field metadata
    /// </summary>
    public abstract class FieldInfo : MemberInfo
    {
        public override MemberTypes MemberType => MemberTypes.Field;
        public abstract Type FieldType { get; }
        public abstract FieldAttributes Attributes { get; }

        public bool IsPublic => (Attributes & FieldAttributes.Public) != 0;
        public bool IsPrivate => (Attributes & FieldAttributes.Private) != 0;
        public bool IsStatic => (Attributes & FieldAttributes.Static) != 0;

        public abstract object GetValue(object obj);
        public abstract void SetValue(object obj, object value);
    }

    /// <summary>
    /// Discovers the attributes of a property and provides access to property metadata
    /// </summary>
    public abstract class PropertyInfo : MemberInfo
    {
        public override MemberTypes MemberType => MemberTypes.Property;
        public abstract Type PropertyType { get; }
        public abstract bool CanRead { get; }
        public abstract bool CanWrite { get; }

        public virtual object GetValue(object obj)
        {
            return GetValue(obj, null);
        }

        public abstract object GetValue(object obj, object[] index);

        public virtual void SetValue(object obj, object value)
        {
            SetValue(obj, value, null);
        }

        public abstract void SetValue(object obj, object value, object[] index);
        public abstract MethodInfo GetGetMethod();
        public abstract MethodInfo GetSetMethod();
        public abstract ParameterInfo[] GetIndexParameters();
    }

    /// <summary>
    /// Discovers event metadata and provides access to its accessor methods
    /// </summary>
    public abstract class EventInfo : MemberInfo
    {
        public override MemberTypes MemberType => MemberTypes.Event;
        public abstract EventAttributes Attributes { get; }
        public abstract Type EventHandlerType { get; }

        public abstract MethodInfo GetAddMethod();
        public abstract MethodInfo GetRemoveMethod();
        public abstract MethodInfo GetRaiseMethod();

        public virtual void AddEventHandler(object target, Delegate handler)
        {
            if (handler == null)
            {
                throw new ArgumentNullException("handler");
            }

            MethodInfo addMethod = GetAddMethod();
            if (addMethod == null)
            {
                throw new InvalidOperationException("Event does not have an add accessor.");
            }
            addMethod.Invoke(target, new object[] { handler });
        }

        public virtual void RemoveEventHandler(object target, Delegate handler)
        {
            if (handler == null)
            {
                throw new ArgumentNullException("handler");
            }

            MethodInfo removeMethod = GetRemoveMethod();
            if (removeMethod == null)
            {
                throw new InvalidOperationException("Event does not have a remove accessor.");
            }
            removeMethod.Invoke(target, new object[] { handler });
        }
    }

    /// <summary>
    /// Discovers the attributes of a parameter and provides access to parameter metadata
    /// </summary>
    public class ParameterInfo
    {
        private IntPtr _handle;

        protected ParameterInfo()
        {
        }

        internal ParameterInfo(IntPtr handle)
        {
            _handle = handle;
        }

        public virtual string Name { get; }
        public virtual Type ParameterType { get; }
        public virtual int Position { get; }
        public virtual bool IsOptional { get; }
        public virtual object DefaultValue { get; }

        public override bool Equals(object obj)
        {
            ParameterInfo other = obj as ParameterInfo;
            return other != null && GetType() == other.GetType() && _handle == other._handle;
        }

        public override int GetHashCode()
        {
            return _handle.GetHashCode();
        }
    }

    internal sealed class RuntimeMethodInfo : MethodInfo
    {
        internal RuntimeMethodInfo(IntPtr handle)
        {
            SetHandle(handle);
        }

        public override extern string Name { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public override extern Type DeclaringType { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public override extern MethodAttributes Attributes { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public override extern Type ReturnType { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public override extern bool ContainsGenericParameters { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public override extern bool IsGenericMethod { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public override extern bool IsGenericMethodDefinition { [MethodImpl(MethodImplOptions.InternalCall)] get; }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public override extern ParameterInfo[] GetParameters();

        [MethodImpl(MethodImplOptions.InternalCall)]
        public override extern Type[] GetGenericArguments();

        [MethodImpl(MethodImplOptions.InternalCall)]
        public override extern MethodInfo GetGenericMethodDefinition();

        [MethodImpl(MethodImplOptions.InternalCall)]
        public override extern MethodInfo MakeGenericMethod(Type[] typeArguments);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public override extern object Invoke(object obj, object[] parameters);
    }

    internal sealed class RuntimeConstructorInfo : ConstructorInfo
    {
        internal RuntimeConstructorInfo(IntPtr handle)
        {
            SetHandle(handle);
        }

        public override extern string Name { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public override extern Type DeclaringType { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public override extern MethodAttributes Attributes { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public override extern bool ContainsGenericParameters { [MethodImpl(MethodImplOptions.InternalCall)] get; }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public override extern ParameterInfo[] GetParameters();

        [MethodImpl(MethodImplOptions.InternalCall)]
        public override extern object Invoke(object obj, object[] parameters);
    }

    internal sealed class RuntimeFieldInfo : FieldInfo
    {
        internal RuntimeFieldInfo(IntPtr handle)
        {
            SetHandle(handle);
        }

        public override extern string Name { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public override extern Type DeclaringType { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public override extern Type FieldType { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public override extern FieldAttributes Attributes { [MethodImpl(MethodImplOptions.InternalCall)] get; }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public override extern object GetValue(object obj);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public override extern void SetValue(object obj, object value);
    }

    internal sealed class RuntimeParameterInfo : ParameterInfo
    {
        internal RuntimeParameterInfo(IntPtr handle) : base(handle) { }

        public override extern string Name { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public override extern Type ParameterType { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public override extern int Position { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public override extern bool IsOptional { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public override extern object DefaultValue { [MethodImpl(MethodImplOptions.InternalCall)] get; }
    }

    internal sealed class RuntimePropertyInfo : PropertyInfo
    {
        internal RuntimePropertyInfo(IntPtr handle)
        {
            SetHandle(handle);
        }

        public override extern string Name { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public override extern Type DeclaringType { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public override extern Type PropertyType { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public override extern bool CanRead { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public override extern bool CanWrite { [MethodImpl(MethodImplOptions.InternalCall)] get; }

        public override object GetValue(object obj, object[] index)
        {
            MethodInfo getter = GetGetMethod();
            if (getter == null)
            {
                throw new InvalidOperationException("Property does not have a getter.");
            }

            return getter.Invoke(obj, index);
        }

        public override void SetValue(object obj, object value, object[] index)
        {
            MethodInfo setter = GetSetMethod();
            if (setter == null)
            {
                throw new InvalidOperationException("Property does not have a setter.");
            }

            int indexCount = index == null ? 0 : index.Length;
            object[] parameters = new object[indexCount + 1];
            for (int parameterIndex = 0; parameterIndex < indexCount; parameterIndex++)
            {
                parameters[parameterIndex] = index[parameterIndex];
            }
            parameters[indexCount] = value;
            setter.Invoke(obj, parameters);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public override extern MethodInfo GetGetMethod();

        [MethodImpl(MethodImplOptions.InternalCall)]
        public override extern MethodInfo GetSetMethod();

        [MethodImpl(MethodImplOptions.InternalCall)]
        public override extern ParameterInfo[] GetIndexParameters();
    }

    internal sealed class RuntimeEventInfo : EventInfo
    {
        internal RuntimeEventInfo(IntPtr handle)
        {
            SetHandle(handle);
        }

        public override extern string Name { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public override extern Type DeclaringType { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public override extern EventAttributes Attributes { [MethodImpl(MethodImplOptions.InternalCall)] get; }
        public override extern Type EventHandlerType { [MethodImpl(MethodImplOptions.InternalCall)] get; }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public override extern MethodInfo GetAddMethod();

        [MethodImpl(MethodImplOptions.InternalCall)]
        public override extern MethodInfo GetRemoveMethod();

        [MethodImpl(MethodImplOptions.InternalCall)]
        public override extern MethodInfo GetRaiseMethod();
    }

    /// <summary>
    /// Specifies flags for method attributes
    /// </summary>
    [Flags]
    public enum MethodAttributes
    {
        MemberAccessMask = 0x0007,
        PrivateScope = 0x0000,
        Private = 0x0001,
        FamANDAssem = 0x0002,
        Assembly = 0x0003,
        Family = 0x0004,
        FamORAssem = 0x0005,
        Public = 0x0006,
        Static = 0x0010,
        Final = 0x0020,
        Virtual = 0x0040,
        HideBySig = 0x0080,
        VtableLayoutMask = 0x0100,
        ReuseSlot = 0x0000,
        NewSlot = 0x0100,
        CheckAccessOnOverride = 0x0200,
        Abstract = 0x0400,
        SpecialName = 0x0800,
        PinvokeImpl = 0x2000,
        UnmanagedExport = 0x0008,
        RTSpecialName = 0x1000,
        HasSecurity = 0x4000,
        RequireSecObject = 0x8000
    }

    [Flags]
    public enum GenericParameterAttributes
    {
        None = 0,
        Covariant = 0x0001,
        Contravariant = 0x0002,
        VarianceMask = 0x0003,
        ReferenceTypeConstraint = 0x0004,
        NotNullableValueTypeConstraint = 0x0008,
        DefaultConstructorConstraint = 0x0010,
        SpecialConstraintMask = 0x001C
    }

    /// <summary>
    /// Specifies flags for field attributes
    /// </summary>
    [Flags]
    public enum FieldAttributes
    {
        FieldAccessMask = 0x0007,
        PrivateScope = 0x0000,
        Private = 0x0001,
        FamANDAssem = 0x0002,
        Assembly = 0x0003,
        Family = 0x0004,
        FamORAssem = 0x0005,
        Public = 0x0006,
        Static = 0x0010,
        InitOnly = 0x0020,
        Literal = 0x0040,
        NotSerialized = 0x0080,
        SpecialName = 0x0200,
        PinvokeImpl = 0x2000,
        RTSpecialName = 0x0400,
        HasFieldMarshal = 0x1000,
        HasDefault = 0x8000,
        HasFieldRVA = 0x0100
    }

    /// <summary>
    /// Specifies event metadata attributes
    /// </summary>
    [Flags]
    public enum EventAttributes
    {
        None = 0x0000,
        SpecialName = 0x0200,
        RTSpecialName = 0x0400
    }

    /// <summary>
    /// Specifies type metadata attributes
    /// </summary>
    [Flags]
    public enum TypeAttributes
    {
        VisibilityMask = 0x00000007,
        NotPublic = 0x00000000,
        Public = 0x00000001,
        NestedPublic = 0x00000002,
        NestedPrivate = 0x00000003,
        NestedFamily = 0x00000004,
        NestedAssembly = 0x00000005,
        NestedFamANDAssem = 0x00000006,
        NestedFamORAssem = 0x00000007,
        LayoutMask = 0x00000018,
        AutoLayout = 0x00000000,
        SequentialLayout = 0x00000008,
        ExplicitLayout = 0x00000010,
        ClassSemanticsMask = 0x00000020,
        Class = 0x00000000,
        Interface = 0x00000020,
        Abstract = 0x00000080,
        Sealed = 0x00000100,
        SpecialName = 0x00000400,
        RTSpecialName = 0x00000800,
        Import = 0x00001000,
        Serializable = 0x00002000,
        HasSecurity = 0x00040000,
        BeforeFieldInit = 0x00100000
    }

    /// <summary>
    /// Specifies flags that control binding and invocation
    /// </summary>
    [Flags]
    public enum BindingFlags
    {
        Default = 0x00,
        IgnoreCase = 0x01,
        DeclaredOnly = 0x02,
        Instance = 0x04,
        Static = 0x08,
        Public = 0x10,
        NonPublic = 0x20,
        FlattenHierarchy = 0x40,
        InvokeMethod = 0x0100,
        CreateInstance = 0x0200,
        GetField = 0x0400,
        SetField = 0x0800,
        GetProperty = 0x1000,
        SetProperty = 0x2000,
        PutDispProperty = 0x4000,
        PutRefDispProperty = 0x8000,
        ExactBinding = 0x010000,
        SuppressChangeType = 0x020000,
        OptionalParamBinding = 0x040000,
        IgnoreReturn = 0x01000000
    }
}
