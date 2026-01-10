namespace System.Runtime.CompilerServices
{
    /// <summary>
    /// Indicates that a method is an extension method
    /// </summary>
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Class | AttributeTargets.Assembly)]
    public sealed class ExtensionAttribute : Attribute
    {
        public ExtensionAttribute() { }
    }
    
    /// <summary>
    /// Specifies the details of how a method is implemented
    /// </summary>
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor, Inherited = false)]
    public sealed class MethodImplAttribute : Attribute
    {
        public MethodImplOptions Value { get; }

        public MethodImplAttribute(MethodImplOptions methodImplOptions)
        {
            Value = methodImplOptions;
        }

        public MethodImplAttribute(short value)
        {
            Value = (MethodImplOptions)value;
        }

        public MethodImplAttribute()
        {
            Value = MethodImplOptions.IL;
        }
    }

    /// <summary>
    /// Defines the details of how a method is implemented
    /// </summary>
    [Flags]
    public enum MethodImplOptions
    {
        IL = 0x0000,
        Managed = 0x0000,
        Native = 0x0001,
        OPTIL = 0x0002,
        Runtime = 0x0003,
        Unmanaged = 0x0004,
        NoInlining = 0x0008,
        ForwardRef = 0x0010,
        Synchronized = 0x0020,
        NoOptimization = 0x0040,
        PreserveSig = 0x0080,
        AggressiveInlining = 0x0100,
        AggressiveOptimization = 0x0200,
        InternalCall = 0x1000
    }
}
