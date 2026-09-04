using System.Runtime.CompilerServices;

namespace System
{
    /// <summary>
    /// Provides information about, and means to manipulate, the current environment and platform
    /// </summary>
    public static class Environment
    {
        /// <summary>
        /// Gets the newline string defined for this environment
        /// </summary>
        public static string NewLine => "\n";

        /// <summary>
        /// Gets the number of milliseconds elapsed since the system started
        /// </summary>
        public extern static int TickCount
        {
            [MethodImpl(MethodImplOptions.InternalCall)]
            get;
        }

        /// <summary>
        /// Terminates this process and returns an exit code to the operating system
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Exit(int exitCode);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern long GetUtcNowTicks();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern long GetLocalNowTicks();

        /// <summary>
        /// Gets the fully qualified path of the current working directory
        /// </summary>
        public extern static string CurrentDirectory
        {
            [MethodImpl(MethodImplOptions.InternalCall)]
            get;
        }

        /// <summary>
        /// Gets the command line for this process
        /// </summary>
        public static string CommandLine => "";

        /// <summary>
        /// Gets or sets the exit code of the process
        /// </summary>
        public static int ExitCode { get; set; }

        /// <summary>
        /// Gets the number of processors on the current machine
        /// </summary>
        public static int ProcessorCount => 1;

        /// <summary>
        /// Gets the NetBIOS name of this local computer
        /// </summary>
        public static string MachineName => "IronNet";

        /// <summary>
        /// Gets the user name of the person who is currently logged on
        /// </summary>
        public static string UserName => "user";

        /// <summary>
        /// Gets a value that indicates whether the current process is a 64-bit process
        /// </summary>
        public static bool Is64BitProcess => IntPtr.Size == 8;

        /// <summary>
        /// Gets a value that indicates whether the current operating system is a 64-bit operating system
        /// </summary>
        public static bool Is64BitOperatingSystem => Is64BitProcess;

        /// <summary>
        /// Gets current stack trace information
        /// </summary>
        public static string StackTrace => "";

        /// <summary>
        /// Gets the version of the common language runtime
        /// </summary>
        public static Version Version => new Version(0, 1, 0, 0);
    }

    /// <summary>
    /// Represents the version number of an assembly, operating system, or the common language runtime
    /// </summary>
    public sealed class Version
    {
        public int Major { get; }
        public int Minor { get; }
        public int Build { get; }
        public int Revision { get; }

        public Version(int major, int minor)
        {
            Major = major;
            Minor = minor;
            Build = -1;
            Revision = -1;
        }

        public Version(int major, int minor, int build)
        {
            Major = major;
            Minor = minor;
            Build = build;
            Revision = -1;
        }

        public Version(int major, int minor, int build, int revision)
        {
            Major = major;
            Minor = minor;
            Build = build;
            Revision = revision;
        }

        public override string ToString()
        {
            if (Build < 0)
                return Major + "." + Minor;
            if (Revision < 0)
                return Major + "." + Minor + "." + Build;
            return Major + "." + Minor + "." + Build + "." + Revision;
        }
    }
}
