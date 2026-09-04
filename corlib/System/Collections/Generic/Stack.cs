namespace System.Collections.Generic
{
    /// <summary>
    /// Represents a last-in, first-out collection of objects.
    /// </summary>
    public class Stack<T> : IEnumerable<T>, ICollection
    {
        private const int DefaultCapacity = 4;

        private T[] _array;
        private int _size;
        private int _version;

        public Stack()
        {
            _array = new T[0];
        }

        public Stack(int capacity)
        {
            if (capacity < 0)
            {
                throw new ArgumentOutOfRangeException("capacity");
            }

            _array = new T[capacity];
        }

        public Stack(IEnumerable<T> collection)
        {
            if (collection == null)
            {
                throw new ArgumentNullException("collection");
            }

            _array = new T[0];
            foreach (T item in collection)
            {
                Push(item);
            }
        }

        public int Count => _size;
        bool ICollection.IsSynchronized => false;
        object ICollection.SyncRoot => this;

        public void Clear()
        {
            if (_size == 0)
            {
                return;
            }

            Array.Clear(_array, 0, _size);
            _size = 0;
            _version++;
        }

        public bool Contains(T item)
        {
            EqualityComparer<T> comparer = EqualityComparer<T>.Default;
            for (int i = _size - 1; i >= 0; i--)
            {
                if (comparer.Equals(_array[i], item))
                {
                    return true;
                }
            }

            return false;
        }

        public void CopyTo(T[] array, int arrayIndex)
        {
            ValidateCopyTo(array, arrayIndex);
            for (int i = 0; i < _size; i++)
            {
                array[arrayIndex + i] = _array[_size - i - 1];
            }
        }

        void ICollection.CopyTo(Array array, int index)
        {
            if (array == null)
            {
                throw new ArgumentNullException("array");
            }

            if (index < 0 || index > array.Length || array.Length - index < _size)
            {
                throw new ArgumentException("The destination array is not large enough.");
            }

            for (int i = 0; i < _size; i++)
            {
                array.SetValue(_array[_size - i - 1], index + i);
            }
        }

        public T Peek()
        {
            if (_size == 0)
            {
                throw new InvalidOperationException("Stack empty.");
            }

            return _array[_size - 1];
        }

        public T Pop()
        {
            if (_size == 0)
            {
                throw new InvalidOperationException("Stack empty.");
            }

            _size--;
            T removed = _array[_size];
            _array[_size] = default(T);
            _version++;
            return removed;
        }

        public void Push(T item)
        {
            if (_size == _array.Length)
            {
                int capacity = _array.Length == 0 ? DefaultCapacity : _array.Length * 2;
                T[] newArray = new T[capacity];
                Array.Copy(_array, 0, newArray, 0, _size);
                _array = newArray;
            }

            _array[_size] = item;
            _size++;
            _version++;
        }

        public bool TryPeek(out T result)
        {
            if (_size == 0)
            {
                result = default(T);
                return false;
            }

            result = _array[_size - 1];
            return true;
        }

        public bool TryPop(out T result)
        {
            if (_size == 0)
            {
                result = default(T);
                return false;
            }

            result = Pop();
            return true;
        }

        public T[] ToArray()
        {
            T[] result = new T[_size];
            CopyTo(result, 0);
            return result;
        }

        public void TrimExcess()
        {
            int threshold = (int)(_array.Length * 0.9);
            if (_size < threshold)
            {
                T[] newArray = new T[_size];
                Array.Copy(_array, 0, newArray, 0, _size);
                _array = newArray;
                _version++;
            }
        }

        public IEnumerator<T> GetEnumerator()
        {
            return new Enumerator(this);
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        private void ValidateCopyTo(T[] array, int arrayIndex)
        {
            if (array == null)
            {
                throw new ArgumentNullException("array");
            }

            if (arrayIndex < 0)
            {
                throw new ArgumentOutOfRangeException("arrayIndex");
            }

            if (arrayIndex > array.Length || array.Length - arrayIndex < _size)
            {
                throw new ArgumentException("The destination array is not large enough.");
            }
        }

        private struct Enumerator : IEnumerator<T>
        {
            private readonly Stack<T> _stack;
            private readonly int _version;
            private int _index;
            private T _current;

            internal Enumerator(Stack<T> stack)
            {
                _stack = stack;
                _version = stack._version;
                _index = -2;
                _current = default(T);
            }

            public T Current
            {
                get
                {
                    if (_index == -2 || _index == -1)
                    {
                        throw new InvalidOperationException("Enumeration has not started or has already finished.");
                    }

                    return _current;
                }
            }

            object IEnumerator.Current => Current;

            public bool MoveNext()
            {
                EnsureUnmodified();
                if (_index == -2)
                {
                    _index = _stack._size - 1;
                }
                else
                {
                    _index--;
                }

                if (_index >= 0)
                {
                    _current = _stack._array[_index];
                    return true;
                }

                _current = default(T);
                return false;
            }

            public void Reset()
            {
                EnsureUnmodified();
                _index = -2;
                _current = default(T);
            }

            public void Dispose()
            {
            }

            private void EnsureUnmodified()
            {
                if (_version != _stack._version)
                {
                    throw new InvalidOperationException("Collection was modified during enumeration.");
                }
            }
        }
    }
}
