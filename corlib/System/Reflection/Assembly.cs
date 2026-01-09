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

        public static Assembly GetExecutingAssembly()
        {
            // TODO: Implement
            return null;
        }

        public static Assembly GetCallingAssembly()
        {
            // TODO: Implement
            return null;
        }

        public static Assembly GetEntryAssembly()
        {
            // TODO: Implement
            return null;
        }

        public virtual Type[] GetTypes()
        {
            return new Type[0];
        }

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
            // TODO: Implement
            if (throwOnError)
                throw new TypeLoadException("Type not found: " + name);
            return null;
        }

        public virtual AssemblyName GetName()
        {
            return new AssemblyName(FullName);
        }

        public override string ToString()
        {
            return FullName ?? "Assembly";
        }
    }

    /// <summary>
    /// Describes an assembly's unique identity in full
    /// </summary>
    public sealed class AssemblyName
    {
        public string Name { get; set; }
        public Version Version { get; set; }
        public string CultureName { get; set; }

        public AssemblyName()
        {
        }

        public AssemblyName(string assemblyName)
        {
            if (assemblyName == null)
                throw new ArgumentNullException("assemblyName");
            
            // Simple parsing - just get the name before comma
            int commaIndex = assemblyName.IndexOf(',');
            Name = commaIndex >= 0 ? assemblyName.Substring(0, commaIndex) : assemblyName;
        }

        public string FullName => Name;

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

    /// <summary>
    /// Discovers the attributes of a member and provides access to member metadata
    /// </summary>
    public abstract class MemberInfo
    {
        public abstract string Name { get; }
        public abstract Type DeclaringType { get; }
        public abstract MemberTypes MemberType { get; }

        public virtual Type ReflectedType => DeclaringType;

        public override string ToString()
        {
            return Name ?? "MemberInfo";
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

        public abstract ParameterInfo[] GetParameters();

        public virtual object Invoke(object obj, object[] parameters)
        {
            throw new NotImplementedException();
        }
    }

    /// <summary>
    /// Discovers the attributes of a method and provides access to method metadata
    /// </summary>
    public abstract class MethodInfo : MethodBase
    {
        public override MemberTypes MemberType => MemberTypes.Method;
        public abstract Type ReturnType { get; }
    }

    /// <summary>
    /// Discovers the attributes of a class constructor and provides access to constructor metadata
    /// </summary>
    public abstract class ConstructorInfo : MethodBase
    {
        public override MemberTypes MemberType => MemberTypes.Constructor;
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

        public abstract object GetValue(object obj);
        public abstract void SetValue(object obj, object value);
        public abstract MethodInfo GetGetMethod();
        public abstract MethodInfo GetSetMethod();
    }

    /// <summary>
    /// Discovers the attributes of a parameter and provides access to parameter metadata
    /// </summary>
    public class ParameterInfo
    {
        public virtual string Name { get; }
        public virtual Type ParameterType { get; }
        public virtual int Position { get; }
        public virtual bool IsOptional { get; }
        public virtual object DefaultValue { get; }
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
