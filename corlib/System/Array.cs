using System.Runtime.CompilerServices;

namespace System
{
    /// <summary>
    /// Provides methods for creating, manipulating, searching, and sorting arrays
    /// </summary>
    public abstract class Array
    {
        /// <summary>
        /// Gets the total number of elements in all the dimensions of the Array
        /// </summary>
        public extern int Length
        {
            [MethodImpl(MethodImplOptions.InternalCall)]
            get;
        }

        /// <summary>
        /// Gets a 64-bit integer that represents the total number of elements
        /// </summary>
        public long LongLength => Length;

        /// <summary>
        /// Gets the rank (number of dimensions) of the Array
        /// </summary>
        public virtual int Rank => 1;

        /// <summary>
        /// Gets the value at the specified position in the one-dimensional Array
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern object GetValue(int index);

        /// <summary>
        /// Sets a value to the element at the specified position in the one-dimensional Array
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern void SetValue(object value, int index);

        /// <summary>
        /// Copies a range of elements from an Array starting at the first element
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Copy(Array sourceArray, Array destinationArray, int length);

        /// <summary>
        /// Copies a range of elements from an Array starting at the specified source index
        /// </summary>
        public static void Copy(Array sourceArray, int sourceIndex,
                                Array destinationArray, int destinationIndex, int length)
        {
            if (sourceArray == null)
                throw new ArgumentNullException("sourceArray");
            if (destinationArray == null)
                throw new ArgumentNullException("destinationArray");
            if (sourceIndex < 0 || destinationIndex < 0 || length < 0)
                throw new ArgumentOutOfRangeException();
            if (sourceIndex + length > sourceArray.Length)
                throw new ArgumentException("Source array too small");
            if (destinationIndex + length > destinationArray.Length)
                throw new ArgumentException("Destination array too small");

            /* Copy element by element using GetValue/SetValue.
             * Handle overlapping regions by choosing copy direction. */
            if (sourceArray == destinationArray && sourceIndex < destinationIndex)
            {
                /* Copy backward to avoid overwriting source elements */
                for (int i = length - 1; i >= 0; i--)
                    destinationArray.SetValue(sourceArray.GetValue(sourceIndex + i), destinationIndex + i);
            }
            else
            {
                for (int i = 0; i < length; i++)
                    destinationArray.SetValue(sourceArray.GetValue(sourceIndex + i), destinationIndex + i);
            }
        }

        /// <summary>
        /// Sets a range of elements in an array to the default value of each element type
        /// </summary>
        public static void Clear(Array array, int index, int length)
        {
            if (array == null)
                throw new ArgumentNullException("array");
            for (int i = index; i < index + length && i < array.Length; i++)
            {
                array.SetValue(null, i);
            }
        }

        /// <summary>
        /// Searches for the specified object and returns the index of its first occurrence
        /// </summary>
        public static int IndexOf(Array array, object value)
        {
            if (array == null)
                throw new ArgumentNullException("array");
            for (int i = 0; i < array.Length; i++)
            {
                object element = array.GetValue(i);
                if (element == null && value == null)
                    return i;
                if (element != null && element.Equals(value))
                    return i;
            }
            return -1;
        }

        /// <summary>
        /// Reverses the sequence of the elements in the entire one-dimensional Array
        /// </summary>
        public static void Reverse(Array array)
        {
            if (array == null)
                throw new ArgumentNullException("array");
            int i = 0;
            int j = array.Length - 1;
            while (i < j)
            {
                object temp = array.GetValue(i);
                array.SetValue(array.GetValue(j), i);
                array.SetValue(temp, j);
                i++;
                j--;
            }
        }
    }
}
