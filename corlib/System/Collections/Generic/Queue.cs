namespace System.Collections.Generic
{
    /// <summary>
    /// Represents a first-in, first-out collection of objects.
    /// </summary>
    public class Queue<T> : IEnumerable<T>, ICollection
    {
        private const int DefaultCapacity = 4;

        private T[] _array;
        private int _head;
        private int _tail;
        private int _size;
        private int _version;

        public Queue()
        {
            _array = new T[0];
        }

        public Queue(int capacity)
        {
            if (capacity < 0)
            {
                throw new ArgumentOutOfRangeException("capacity");
            }

            _array = new T[capacity];
        }

        public Queue(IEnumerable<T> collection)
        {
            if (collection == null)
            {
                throw new ArgumentNullException("collection");
            }

            _array = new T[0];
            foreach (T item in collection)
            {
                Enqueue(item);
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

            if (_head < _tail)
            {
                Array.Clear(_array, _head, _size);
            }
            else
            {
                Array.Clear(_array, _head, _array.Length - _head);
                Array.Clear(_array, 0, _tail);
            }

            _head = 0;
            _tail = 0;
            _size = 0;
            _version++;
        }

        public bool Contains(T item)
        {
            EqualityComparer<T> comparer = EqualityComparer<T>.Default;
            int index = _head;
            for (int i = 0; i < _size; i++)
            {
                if (comparer.Equals(_array[index], item))
                {
                    return true;
                }

                index = MoveNext(index);
            }

            return false;
        }

        public void CopyTo(T[] array, int arrayIndex)
        {
            ValidateCopyTo(array, arrayIndex);
            if (_size == 0)
            {
                return;
            }

            int firstPart = _array.Length - _head;
            if (firstPart > _size)
            {
                firstPart = _size;
            }

            Array.Copy(_array, _head, array, arrayIndex, firstPart);
            int secondPart = _size - firstPart;
            if (secondPart > 0)
            {
                Array.Copy(_array, 0, array, arrayIndex + firstPart, secondPart);
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

            int sourceIndex = _head;
            for (int i = 0; i < _size; i++)
            {
                array.SetValue(_array[sourceIndex], index + i);
                sourceIndex = MoveNext(sourceIndex);
            }
        }

        public T Dequeue()
        {
            if (_size == 0)
            {
                throw new InvalidOperationException("Queue empty.");
            }

            T removed = _array[_head];
            _array[_head] = default(T);
            _head = MoveNext(_head);
            _size--;
            _version++;
            return removed;
        }

        public void Enqueue(T item)
        {
            if (_size == _array.Length)
            {
                int newCapacity = _array.Length == 0 ? DefaultCapacity : _array.Length * 2;
                SetCapacity(newCapacity);
            }

            _array[_tail] = item;
            _tail = MoveNext(_tail);
            _size++;
            _version++;
        }

        public T Peek()
        {
            if (_size == 0)
            {
                throw new InvalidOperationException("Queue empty.");
            }

            return _array[_head];
        }

        public bool TryDequeue(out T result)
        {
            if (_size == 0)
            {
                result = default(T);
                return false;
            }

            result = Dequeue();
            return true;
        }

        public bool TryPeek(out T result)
        {
            if (_size == 0)
            {
                result = default(T);
                return false;
            }

            result = _array[_head];
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
                SetCapacity(_size);
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

        private int MoveNext(int index)
        {
            index++;
            if (index == _array.Length)
            {
                return 0;
            }

            return index;
        }

        private void SetCapacity(int capacity)
        {
            T[] newArray = new T[capacity];
            if (_size > 0)
            {
                CopyTo(newArray, 0);
            }

            _array = newArray;
            _head = 0;
            _tail = _size == capacity ? 0 : _size;
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
            private readonly Queue<T> _queue;
            private readonly int _version;
            private int _index;
            private T _current;

            internal Enumerator(Queue<T> queue)
            {
                _queue = queue;
                _version = queue._version;
                _index = -1;
                _current = default(T);
            }

            public T Current
            {
                get
                {
                    if (_index < 0 || _index >= _queue._size)
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
                if (_index + 1 >= _queue._size)
                {
                    _index = _queue._size;
                    _current = default(T);
                    return false;
                }

                _index++;
                int arrayIndex = _queue._head + _index;
                if (arrayIndex >= _queue._array.Length)
                {
                    arrayIndex -= _queue._array.Length;
                }

                _current = _queue._array[arrayIndex];
                return true;
            }

            public void Reset()
            {
                EnsureUnmodified();
                _index = -1;
                _current = default(T);
            }

            public void Dispose()
            {
            }

            private void EnsureUnmodified()
            {
                if (_version != _queue._version)
                {
                    throw new InvalidOperationException("Collection was modified during enumeration.");
                }
            }
        }
    }
}
