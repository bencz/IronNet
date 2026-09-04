using System.Runtime.CompilerServices;

namespace System
{
    /// <summary>
    /// Provides methods for creating, manipulating, searching, and sorting arrays
    /// </summary>
    public abstract class Array : ICloneable, Collections.IList
    {
        int Collections.ICollection.Count => Length;

        public bool IsReadOnly => false;
        public bool IsFixedSize => true;
        public bool IsSynchronized => false;
        public object SyncRoot => this;

        object Collections.IList.this[int index]
        {
            get => GetValue(index);
            set => SetValue(value, index);
        }

        int Collections.IList.Add(object value)
        {
            throw new NotSupportedException("Collection was of a fixed size.");
        }

        void Collections.IList.Clear()
        {
            Clear(this, GetLowerBound(0), Length);
        }

        bool Collections.IList.Contains(object value)
        {
            return IndexOf(this, value) >= GetLowerBound(0);
        }

        int Collections.IList.IndexOf(object value)
        {
            return IndexOf(this, value);
        }

        void Collections.IList.Insert(int index, object value)
        {
            throw new NotSupportedException("Collection was of a fixed size.");
        }

        void Collections.IList.Remove(object value)
        {
            throw new NotSupportedException("Collection was of a fixed size.");
        }

        void Collections.IList.RemoveAt(int index)
        {
            throw new NotSupportedException("Collection was of a fixed size.");
        }

        public object Clone()
        {
            return MemberwiseClone();
        }

        public void CopyTo(Array array, int index)
        {
            if (array != null && array.Rank != 1)
            {
                throw new ArgumentException("The destination array must be one-dimensional.");
            }

            Copy(this, GetLowerBound(0), array, index, Length);
        }

        public void CopyTo(Array array, long index)
        {
            if (index < int.MinValue || index > int.MaxValue)
            {
                throw new ArgumentOutOfRangeException("index");
            }

            CopyTo(array, (int)index);
        }

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

        public static Collections.ObjectModel.ReadOnlyCollection<T> AsReadOnly<T>(T[] array)
        {
            if (array == null)
            {
                throw new ArgumentNullException("array");
            }

            return new Collections.ObjectModel.ReadOnlyCollection<T>(array);
        }

        /// <summary>
        /// Gets the rank (number of dimensions) of the Array
        /// </summary>
        public extern int Rank
        {
            [MethodImpl(MethodImplOptions.InternalCall)]
            get;
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern int GetLength(int dimension);

        public long GetLongLength(int dimension)
        {
            return GetLength(dimension);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern int GetLowerBound(int dimension);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern int GetUpperBound(int dimension);

        /// <summary>
        /// Gets the value at the specified position in the Array
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern object GetValue(int index);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern object GetValue(int index1, int index2);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern object GetValue(int index1, int index2, int index3);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern object GetValue(params int[] indices);

        /// <summary>
        /// Sets a value to the element at the specified position in the Array
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern void SetValue(object value, int index);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern void SetValue(object value, int index1, int index2);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern void SetValue(object value, int index1, int index2, int index3);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern void SetValue(object value, params int[] indices);

        public static Array CreateInstance(Type elementType, int length)
        {
            return CreateInstance(elementType, new int[] { length });
        }

        public static Array CreateInstance(Type elementType, int length1, int length2)
        {
            return CreateInstance(elementType, new int[] { length1, length2 });
        }

        public static Array CreateInstance(Type elementType, int length1, int length2, int length3)
        {
            return CreateInstance(elementType, new int[] { length1, length2, length3 });
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern Array CreateInstance(Type elementType, int[] lengths);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern Array CreateInstance(Type elementType, int[] lengths, int[] lowerBounds);

        /// <summary>
        /// Copies a range of elements from an Array starting at the first element
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Copy(Array sourceArray, Array destinationArray, int length);

        /// <summary>
        /// Copies a range of elements from an Array starting at the specified source index
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Copy(Array sourceArray, int sourceIndex, Array destinationArray, int destinationIndex, int length);

        /// <summary>
        /// Sets a range of elements in an array to the default value of each element type
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Clear(Array array, int index, int length);

        /// <summary>
        /// Searches for the specified object and returns the index of its first occurrence
        /// </summary>
        public static int IndexOf(Array array, object value)
        {
            if (array == null)
            {
                throw new ArgumentNullException("array");
            }

            return IndexOf(array, value, array.GetLowerBound(0), array.Length);
        }

        public static int IndexOf(Array array, object value, int startIndex)
        {
            if (array == null)
            {
                throw new ArgumentNullException("array");
            }

            int count = unchecked(array.Length - startIndex + array.GetLowerBound(0));
            return IndexOf(array, value, startIndex, count);
        }

        public static int IndexOf(Array array, object value, int startIndex, int count)
        {
            if (array == null)
            {
                throw new ArgumentNullException("array");
            }
            if (array.Rank != 1)
            {
                throw new RankException("Only one-dimensional arrays are supported.");
            }

            int lowerBound = array.GetLowerBound(0);
            long offset = (long)startIndex - lowerBound;
            if (offset < 0 || offset > array.Length)
            {
                throw new ArgumentOutOfRangeException("startIndex");
            }
            if (count < 0 || count > array.Length - offset)
            {
                throw new ArgumentOutOfRangeException("count");
            }

            for (int position = 0; position < count; position++)
            {
                int index = (int)((long)startIndex + position);
                object element = array.GetValue(index);
                if (element == null ? value == null : element.Equals(value))
                {
                    return index;
                }
            }

            return unchecked(lowerBound - 1);
        }

        /// <summary>
        /// Reverses the sequence of the elements in the entire one-dimensional Array
        /// </summary>
        public static void Reverse(Array array)
        {
            if (array == null)
            {
                throw new ArgumentNullException("array");
            }

            Reverse(array, array.GetLowerBound(0), array.Length);
        }

        public static void Reverse(Array array, int index, int length)
        {
            if (array == null)
            {
                throw new ArgumentNullException("array");
            }

            long offset = (long)index - array.GetLowerBound(0);
            if (offset < 0)
            {
                throw new ArgumentOutOfRangeException("index");
            }
            if (length < 0)
            {
                throw new ArgumentOutOfRangeException("length");
            }
            if (offset > array.Length || length > array.Length - offset)
            {
                throw new ArgumentException("The range exceeds the array bounds.");
            }
            if (array.Rank != 1)
            {
                throw new RankException("Only one-dimensional arrays are supported.");
            }

            for (int position = 0; position < length / 2; position++)
            {
                int left = (int)((long)index + position);
                int right = (int)((long)index + length - position - 1);
                object value = array.GetValue(left);
                array.SetValue(array.GetValue(right), left);
                array.SetValue(value, right);
            }
        }

        public Collections.IEnumerator GetEnumerator()
        {
            return new ArrayEnumerator(this);
        }

        private int InternalGetCount<T>()
        {
            return Length;
        }

        private bool InternalGetIsReadOnly<T>()
        {
            return true;
        }

        private T InternalGetItem<T>(int index)
        {
            ValidateVectorIndex(index);
            return (T)GetValue(index);
        }

        private void InternalSetItem<T>(int index, T value)
        {
            ValidateVectorIndex(index);
            SetValue(value, index);
        }

        private bool InternalContains<T>(T item)
        {
            return InternalIndexOf(item) >= 0;
        }

        private int InternalIndexOf<T>(T item)
        {
            Collections.Generic.EqualityComparer<T> comparer = Collections.Generic.EqualityComparer<T>.Default;

            for (int index = 0; index < Length; index++)
            {
                if (comparer.Equals((T)GetValue(index), item))
                {
                    return index;
                }
            }

            return -1;
        }

        private void InternalCopyTo<T>(T[] array, int arrayIndex)
        {
            if (array == null)
            {
                throw new ArgumentNullException("array");
            }
            if (arrayIndex < 0)
            {
                throw new ArgumentOutOfRangeException("arrayIndex");
            }
            if (arrayIndex > array.Length || Length > array.Length - arrayIndex)
            {
                throw new ArgumentException("The destination array is not long enough.");
            }

            Copy(this, 0, array, arrayIndex, Length);
        }

        private void InternalAdd<T>(T item)
        {
            throw new NotSupportedException("Collection was of a fixed size.");
        }

        private void InternalClear<T>()
        {
            throw new NotSupportedException("Collection was of a fixed size.");
        }

        private bool InternalRemove<T>(T item)
        {
            throw new NotSupportedException("Collection was of a fixed size.");
        }

        private void InternalInsert<T>(int index, T item)
        {
            throw new NotSupportedException("Collection was of a fixed size.");
        }

        private void InternalRemoveAt<T>(int index)
        {
            throw new NotSupportedException("Collection was of a fixed size.");
        }

        private void ValidateVectorIndex(int index)
        {
            if (index < 0 || index >= Length)
            {
                throw new ArgumentOutOfRangeException("index");
            }
        }

        private sealed class ArrayEnumerator : Collections.IEnumerator, IDisposable
        {
            private readonly Array _array;
            private readonly int[] _indices;
            private bool _beforeStart;
            private bool _finished;

            internal ArrayEnumerator(Array array)
            {
                _array = array;
                _indices = new int[array.Rank];
                _beforeStart = true;
                _finished = array.Length == 0;
            }

            public object Current
            {
                get
                {
                    if (_beforeStart || _finished)
                    {
                        throw new InvalidOperationException("Enumeration has not started or has already finished.");
                    }

                    return _array.GetValue(_indices);
                }
            }

            public bool MoveNext()
            {
                if (_finished)
                {
                    return false;
                }

                if (_beforeStart)
                {
                    for (int dimension = 0; dimension < _indices.Length; dimension++)
                    {
                        _indices[dimension] = _array.GetLowerBound(dimension);
                    }

                    _beforeStart = false;
                    return true;
                }

                for (int dimension = _indices.Length - 1; dimension >= 0; dimension--)
                {
                    if (_indices[dimension] < _array.GetUpperBound(dimension))
                    {
                        _indices[dimension]++;
                        return true;
                    }

                    _indices[dimension] = _array.GetLowerBound(dimension);
                }

                _finished = true;
                return false;
            }

            public void Reset()
            {
                _beforeStart = true;
                _finished = _array.Length == 0;
            }

            public void Dispose()
            {
            }
        }

    }
}
