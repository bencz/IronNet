namespace System.Runtime.InteropServices
{
    /// <summary>
    /// Indicates that the attributed method is exposed by an unmanaged DLL as a static entry point
    /// </summary>
    [AttributeUsage(AttributeTargets.Method, Inherited = false)]
    public sealed class DllImportAttribute : Attribute
    {
        public string Value { get; }
        public string EntryPoint;
        public CharSet CharSet;
        public bool SetLastError;
        public bool ExactSpelling;
        public CallingConvention CallingConvention;
        public bool BestFitMapping;
        public bool ThrowOnUnmappableChar;
        public bool PreserveSig;

        public DllImportAttribute(string dllName)
        {
            Value = dllName;
            PreserveSig = true;
        }
    }

    /// <summary>
    /// Specifies the character set to be used
    /// </summary>
    public enum CharSet
    {
        None = 1,
        Ansi = 2,
        Unicode = 3,
        Auto = 4
    }

    /// <summary>
    /// Specifies the calling convention required to call methods implemented in unmanaged code
    /// </summary>
    public enum CallingConvention
    {
        Winapi = 1,
        Cdecl = 2,
        StdCall = 3,
        ThisCall = 4,
        FastCall = 5
    }

    /// <summary>
    /// Controls how a managed object is laid out in memory
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct, Inherited = false)]
    public sealed class StructLayoutAttribute : Attribute
    {
        public LayoutKind Value { get; }
        public int Pack;
        public int Size;
        public CharSet CharSet;

        public StructLayoutAttribute(LayoutKind layoutKind)
        {
            Value = layoutKind;
        }
    }

    /// <summary>
    /// Controls the layout of an object when exported to unmanaged code
    /// </summary>
    public enum LayoutKind
    {
        Sequential = 0,
        Explicit = 2,
        Auto = 3
    }

    /// <summary>
    /// Indicates the physical position of fields within the unmanaged representation
    /// </summary>
    [AttributeUsage(AttributeTargets.Field, Inherited = false)]
    public sealed class FieldOffsetAttribute : Attribute
    {
        public int Value { get; }

        public FieldOffsetAttribute(int offset)
        {
            Value = offset;
        }
    }

    /// <summary>
    /// Indicates how to marshal the data between managed and unmanaged code
    /// </summary>
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Parameter | AttributeTargets.ReturnValue, Inherited = false)]
    public sealed class MarshalAsAttribute : Attribute
    {
        public UnmanagedType Value { get; }
        public UnmanagedType ArraySubType;
        public int SizeConst;
        public short SizeParamIndex;

        public MarshalAsAttribute(UnmanagedType unmanagedType)
        {
            Value = unmanagedType;
        }

        public MarshalAsAttribute(short unmanagedType)
        {
            Value = (UnmanagedType)unmanagedType;
        }
    }

    /// <summary>
    /// Identifies how to marshal data to and from unmanaged code
    /// </summary>
    public enum UnmanagedType
    {
        Bool = 0x02,
        I1 = 0x03,
        U1 = 0x04,
        I2 = 0x05,
        U2 = 0x06,
        I4 = 0x07,
        U4 = 0x08,
        I8 = 0x09,
        U8 = 0x0A,
        R4 = 0x0B,
        R8 = 0x0C,
        Currency = 0x0F,
        BStr = 0x13,
        LPStr = 0x14,
        LPWStr = 0x15,
        LPTStr = 0x16,
        ByValTStr = 0x17,
        IUnknown = 0x19,
        IDispatch = 0x1A,
        Struct = 0x1B,
        Interface = 0x1C,
        SafeArray = 0x1D,
        ByValArray = 0x1E,
        SysInt = 0x1F,
        SysUInt = 0x20,
        VBByRefStr = 0x22,
        AnsiBStr = 0x23,
        TBStr = 0x24,
        VariantBool = 0x25,
        FunctionPtr = 0x26,
        AsAny = 0x28,
        LPArray = 0x2A,
        LPStruct = 0x2B,
        CustomMarshaler = 0x2C,
        Error = 0x2D,
        IInspectable = 0x2E,
        HString = 0x2F,
        LPUTF8Str = 0x30
    }

    /// <summary>
    /// Indicates that a method's unmanaged signature expects a locale identifier parameter
    /// </summary>
    [AttributeUsage(AttributeTargets.Parameter, Inherited = false)]
    public sealed class InAttribute : Attribute
    {
    }

    /// <summary>
    /// Indicates that data should be marshaled from callee back to caller
    /// </summary>
    [AttributeUsage(AttributeTargets.Parameter, Inherited = false)]
    public sealed class OutAttribute : Attribute
    {
    }

    /// <summary>
    /// Wraps a managed object holding a handle to a resource
    /// </summary>
    public abstract class SafeHandle : IDisposable
    {
        protected IntPtr handle;
        private bool _ownsHandle;
        private bool _disposed;

        protected SafeHandle(IntPtr invalidHandleValue, bool ownsHandle)
        {
            handle = invalidHandleValue;
            _ownsHandle = ownsHandle;
        }

        public bool IsClosed => _disposed;
        public abstract bool IsInvalid { get; }

        public IntPtr DangerousGetHandle()
        {
            return handle;
        }

        public void DangerousAddRef(ref bool success)
        {
            success = true;
        }

        public void DangerousRelease()
        {
        }

        public void SetHandleAsInvalid()
        {
            _disposed = true;
        }

        protected void SetHandle(IntPtr handle)
        {
            this.handle = handle;
        }

        protected abstract bool ReleaseHandle();

        public void Close()
        {
            Dispose(true);
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (!_disposed && _ownsHandle && !IsInvalid)
            {
                ReleaseHandle();
            }
            _disposed = true;
        }

        ~SafeHandle()
        {
            Dispose(false);
        }
    }

    /// <summary>
    /// Provides a controlled memory buffer for use with unmanaged code
    /// </summary>
    public static class Marshal
    {
        public static int SizeOf(Type t)
        {
            // TODO: Implement
            return 0;
        }

        public static int SizeOf<T>()
        {
            return SizeOf(typeof(T));
        }

        public static IntPtr AllocHGlobal(int cb)
        {
            // TODO: Implement
            return IntPtr.Zero;
        }

        public static void FreeHGlobal(IntPtr hglobal)
        {
            // TODO: Implement
        }

        public static void Copy(byte[] source, int startIndex, IntPtr destination, int length)
        {
            // TODO: Implement
        }

        public static void Copy(IntPtr source, byte[] destination, int startIndex, int length)
        {
            // TODO: Implement
        }

        public static string PtrToStringAnsi(IntPtr ptr)
        {
            // TODO: Implement
            return null;
        }

        public static string PtrToStringUni(IntPtr ptr)
        {
            // TODO: Implement
            return null;
        }

        public static IntPtr StringToHGlobalAnsi(string s)
        {
            // TODO: Implement
            return IntPtr.Zero;
        }

        public static IntPtr StringToHGlobalUni(string s)
        {
            // TODO: Implement
            return IntPtr.Zero;
        }

        public static int GetLastWin32Error()
        {
            return 0;
        }
    }
}
