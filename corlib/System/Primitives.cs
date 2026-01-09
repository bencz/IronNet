// Fundamental types required by the C# compiler
// These must be defined for -nostdlib compilation

namespace System
{
    public struct Void { }
    
    public struct RuntimeTypeHandle
    {
        internal IntPtr _value;
        public IntPtr Value => _value;
    }
    
    public struct RuntimeMethodHandle
    {
        internal IntPtr _value;
        public IntPtr Value => _value;
    }
    
    public struct RuntimeFieldHandle
    {
        internal IntPtr _value;
        public IntPtr Value => _value;
    }
    
    public sealed class ParamArrayAttribute : Attribute
    {
        public ParamArrayAttribute() { }
    }
}

namespace System.Reflection
{
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Interface)]
    public sealed class DefaultMemberAttribute : Attribute
    {
        public string MemberName { get; }
        
        public DefaultMemberAttribute(string memberName)
        {
            MemberName = memberName;
        }
    }
}

namespace System.Runtime.CompilerServices
{
    public static class RuntimeHelpers
    {
        public static int OffsetToStringData => 0;
        
        public static void InitializeArray(Array array, RuntimeFieldHandle fldHandle)
        {
            // TODO: Implement
        }
    }
    
    [AttributeUsage(AttributeTargets.Assembly, AllowMultiple = true)]
    public sealed class InternalsVisibleToAttribute : Attribute
    {
        public string AssemblyName { get; }
        
        public InternalsVisibleToAttribute(string assemblyName)
        {
            AssemblyName = assemblyName;
        }
    }
    
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Enum | AttributeTargets.Interface | AttributeTargets.Delegate, Inherited = false)]
    public sealed class TypeForwardedFromAttribute : Attribute
    {
        public string AssemblyFullName { get; }
        
        public TypeForwardedFromAttribute(string assemblyFullName)
        {
            AssemblyFullName = assemblyFullName;
        }
    }
    
    public sealed class IsVolatile { }
    
    [AttributeUsage(AttributeTargets.Struct)]
    public sealed class IsByRefLikeAttribute : Attribute { }
    
    [AttributeUsage(AttributeTargets.All, Inherited = false)]
    public sealed class CompilerGeneratedAttribute : Attribute { }
    
    [AttributeUsage(AttributeTargets.Parameter, Inherited = false)]
    public sealed class CallerMemberNameAttribute : Attribute { }
    
    [AttributeUsage(AttributeTargets.Parameter, Inherited = false)]
    public sealed class CallerFilePathAttribute : Attribute { }
    
    [AttributeUsage(AttributeTargets.Parameter, Inherited = false)]
    public sealed class CallerLineNumberAttribute : Attribute { }
    
    public sealed class RuntimeCompatibilityAttribute : Attribute
    {
        public bool WrapNonExceptionThrows { get; set; }
    }
    
    public sealed class CompilationRelaxationsAttribute : Attribute
    {
        public int CompilationRelaxations { get; }
        
        public CompilationRelaxationsAttribute(int relaxations)
        {
            CompilationRelaxations = relaxations;
        }
    }
    
    [AttributeUsage(AttributeTargets.Assembly, AllowMultiple = false)]
    public sealed class ReferenceAssemblyAttribute : Attribute { }
    
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Property | AttributeTargets.Field | AttributeTargets.Event | AttributeTargets.Parameter | AttributeTargets.ReturnValue)]
    public sealed class NullableAttribute : Attribute
    {
        public readonly byte[] NullableFlags;
        
        public NullableAttribute(byte flag)
        {
            NullableFlags = new byte[] { flag };
        }
        
        public NullableAttribute(byte[] flags)
        {
            NullableFlags = flags;
        }
    }
    
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Method | AttributeTargets.Interface | AttributeTargets.Delegate, AllowMultiple = false, Inherited = false)]
    public sealed class NullableContextAttribute : Attribute
    {
        public readonly byte Flag;
        
        public NullableContextAttribute(byte flag)
        {
            Flag = flag;
        }
    }
}
