using System.Runtime.CompilerServices;

namespace System
{
    /// <summary>
    /// Represents the standard input, output, and error streams for console applications
    /// </summary>
    public static class Console
    {
        /// <summary>
        /// Writes the specified string value to the standard output stream
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Write(string value);

        /// <summary>
        /// Writes the text representation of the specified 32-bit signed integer value
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Write(int value);

        /// <summary>
        /// Writes the current line terminator to the standard output stream
        /// </summary>
        public static void WriteLine()
        {
            WriteLine("");
        }

        /// <summary>
        /// Writes the specified string value, followed by the current line terminator
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void WriteLine(string value);

        /// <summary>
        /// Writes the text representation of the specified 32-bit signed integer value
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void WriteLine(int value);

        /// <summary>
        /// Writes the text representation of the specified object
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void WriteLine(object value);

        /// <summary>
        /// Writes the text representation of the specified Boolean value
        /// </summary>
        public static void WriteLine(bool value)
        {
            WriteLine(value ? "True" : "False");
        }

        /// <summary>
        /// Writes the specified Unicode character
        /// </summary>
        public static void WriteLine(char value)
        {
            WriteLine(value.ToString());
        }

        /// <summary>
        /// Reads the next line of characters from the standard input stream
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern string ReadLine();

        /// <summary>
        /// Reads the next character from the standard input stream
        /// </summary>
        public static int Read()
        {
            string line = ReadLine();
            if (line == null || line.Length == 0)
                return -1;
            return line[0];
        }
    }
}
