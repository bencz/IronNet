using System.Runtime.CompilerServices;

namespace System
{
    /// <summary>
    /// Controls the system garbage collector
    /// </summary>
    public static class GC
    {
        /// <summary>
        /// Forces an immediate garbage collection of all generations
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Collect();

        /// <summary>
        /// Forces an immediate garbage collection from generation 0 through a specified generation
        /// </summary>
        public static void Collect(int generation)
        {
            Collect();
        }

        /// <summary>
        /// Retrieves the number of bytes currently thought to be allocated
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern long GetTotalMemory(bool forceFullCollection);

        /// <summary>
        /// Requests that the system not call the finalizer for the specified object
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SuppressFinalize(object obj);

        /// <summary>
        /// Requests that the system call the finalizer for the specified object
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void ReRegisterForFinalize(object obj);

        /// <summary>
        /// References the specified object, preventing garbage collection until after this method returns
        /// </summary>
        public static void KeepAlive(object obj)
        {
            // This method exists to prevent the JIT from optimizing away the reference
        }

        /// <summary>
        /// Returns the current generation number of the specified object
        /// </summary>
        public static int GetGeneration(object obj)
        {
            return 0; // Simple GC has no generations
        }

        /// <summary>
        /// Returns the maximum number of generations the system currently supports
        /// </summary>
        public static int MaxGeneration => 0;
    }
}
