namespace System.Collections.Generic
{
    /// <summary>
    /// Represents a strongly typed list of objects that can be accessed by index.
    /// </summary>
    public class List<T> : IList<T>, IList
    {
        private const int DefaultCapacity = 4;

        private T[] _items;
        private int _size;
        private int _version;

        public List()
        {
            _items = new T[0];
        }

        public List(int capacity)
        {
            if (capacity < 0)
            {
                throw new ArgumentOutOfRangeException("capacity");
            }

            _items = new T[capacity];
        }

        public List(IEnumerable<T> collection)
        {
            if (collection == null)
            {
                throw new ArgumentNullException("collection");
            }

            T[] sourceArray = collection as T[];
            if (sourceArray != null)
            {
                _items = new T[sourceArray.Length];
                Array.Copy(sourceArray, 0, _items, 0, sourceArray.Length);
                _size = sourceArray.Length;
                return;
            }

            ICollection<T> sourceCollection = collection as ICollection<T>;
            if (sourceCollection != null)
            {
                _items = new T[sourceCollection.Count];
                sourceCollection.CopyTo(_items, 0);
                _size = sourceCollection.Count;
                return;
            }

            _items = new T[0];
            foreach (T item in collection)
            {
                Add(item);
            }
        }

        public int Count => _size;

        public int Capacity
        {
            get => _items.Length;
            set
            {
                if (value < _size)
                {
                    throw new ArgumentOutOfRangeException("value");
                }

                if (value == _items.Length)
                {
                    return;
                }

                T[] newItems = new T[value];
                if (_size != 0)
                {
                    Array.Copy(_items, 0, newItems, 0, _size);
                }

                _items = newItems;
            }
        }

        public T this[int index]
        {
            get
            {
                ValidateIndex(index);
                return _items[index];
            }
            set
            {
                ValidateIndex(index);
                _items[index] = value;
                _version++;
            }
        }

        public void Add(T item)
        {
            if (_size == _items.Length)
            {
                EnsureCapacity(_size + 1);
            }

            _items[_size] = item;
            _size++;
            _version++;
        }

        public void AddRange(IEnumerable<T> collection)
        {
            InsertRange(_size, collection);
        }

        public void Clear()
        {
            if (_size == 0)
            {
                return;
            }

            Array.Clear(_items, 0, _size);
            _size = 0;
            _version++;
        }

        public bool Contains(T item)
        {
            return IndexOf(item) >= 0;
        }

        public void CopyTo(T[] array)
        {
            CopyTo(array, 0);
        }

        public void CopyTo(T[] array, int arrayIndex)
        {
            ValidateCopyTo(array, arrayIndex);
            Array.Copy(_items, 0, array, arrayIndex, _size);
        }

        public int IndexOf(T item)
        {
            EqualityComparer<T> comparer = EqualityComparer<T>.Default;
            for (int i = 0; i < _size; i++)
            {
                if (comparer.Equals(_items[i], item))
                {
                    return i;
                }
            }

            return -1;
        }

        public int LastIndexOf(T item)
        {
            EqualityComparer<T> comparer = EqualityComparer<T>.Default;
            for (int i = _size - 1; i >= 0; i--)
            {
                if (comparer.Equals(_items[i], item))
                {
                    return i;
                }
            }

            return -1;
        }

        public void Insert(int index, T item)
        {
            ValidateInsertIndex(index);
            if (_size == _items.Length)
            {
                EnsureCapacity(_size + 1);
            }

            if (index < _size)
            {
                Array.Copy(_items, index, _items, index + 1, _size - index);
            }

            _items[index] = item;
            _size++;
            _version++;
        }

        public void InsertRange(int index, IEnumerable<T> collection)
        {
            ValidateInsertIndex(index);
            if (collection == null)
            {
                throw new ArgumentNullException("collection");
            }

            T[] sourceArray = collection as T[];
            if (sourceArray != null)
            {
                int arrayCount = sourceArray.Length;
                if (arrayCount == 0)
                {
                    return;
                }

                EnsureCapacity(_size + arrayCount);
                if (index < _size)
                {
                    Array.Copy(_items, index, _items, index + arrayCount, _size - index);
                }

                Array.Copy(sourceArray, 0, _items, index, arrayCount);
                _size += arrayCount;
                _version++;
                return;
            }

            ICollection<T> sourceCollection = collection as ICollection<T>;
            if (sourceCollection == null)
            {
                foreach (T item in collection)
                {
                    Insert(index, item);
                    index++;
                }

                return;
            }

            int count = sourceCollection.Count;
            if (count == 0)
            {
                return;
            }

            EnsureCapacity(_size + count);
            if (index < _size)
            {
                Array.Copy(_items, index, _items, index + count, _size - index);
            }

            if (object.ReferenceEquals(this, sourceCollection))
            {
                Array.Copy(_items, 0, _items, index, index);
                Array.Copy(_items, index + count, _items, index * 2, _size - index);
            }
            else
            {
                sourceCollection.CopyTo(_items, index);
            }

            _size += count;
            _version++;
        }

        public bool Remove(T item)
        {
            int index = IndexOf(item);
            if (index < 0)
            {
                return false;
            }

            RemoveAt(index);
            return true;
        }

        public int RemoveAll(Predicate<T> match)
        {
            if (match == null)
            {
                throw new ArgumentNullException("match");
            }

            int freeIndex = 0;
            while (freeIndex < _size && !match(_items[freeIndex]))
            {
                freeIndex++;
            }

            if (freeIndex >= _size)
            {
                return 0;
            }

            int current = freeIndex + 1;
            while (current < _size)
            {
                while (current < _size && match(_items[current]))
                {
                    current++;
                }

                if (current < _size)
                {
                    _items[freeIndex] = _items[current];
                    freeIndex++;
                    current++;
                }
            }

            int removed = _size - freeIndex;
            Array.Clear(_items, freeIndex, removed);
            _size = freeIndex;
            _version++;
            return removed;
        }

        public void RemoveAt(int index)
        {
            ValidateIndex(index);
            _size--;
            if (index < _size)
            {
                Array.Copy(_items, index + 1, _items, index, _size - index);
            }

            _items[_size] = default(T);
            _version++;
        }

        public void RemoveRange(int index, int count)
        {
            ValidateRange(index, count);
            if (count == 0)
            {
                return;
            }

            int oldSize = _size;
            _size -= count;
            if (index < _size)
            {
                Array.Copy(_items, index + count, _items, index, _size - index);
            }

            Array.Clear(_items, _size, oldSize - _size);
            _version++;
        }

        public void Reverse()
        {
            Reverse(0, _size);
        }

        public void Reverse(int index, int count)
        {
            ValidateRange(index, count);

            int left = index;
            int right = index + count - 1;
            while (left < right)
            {
                T temporary = _items[left];
                _items[left] = _items[right];
                _items[right] = temporary;
                left++;
                right--;
            }

            if (count > 1)
            {
                _version++;
            }
        }

        public T[] ToArray()
        {
            T[] array = new T[_size];
            Array.Copy(_items, 0, array, 0, _size);
            return array;
        }

        public void TrimExcess()
        {
            int threshold = (int)(_items.Length * 0.9);
            if (_size < threshold)
            {
                Capacity = _size;
            }
        }

        public int FindIndex(Predicate<T> match)
        {
            return FindIndex(0, _size, match);
        }

        public int FindIndex(int startIndex, Predicate<T> match)
        {
            if (startIndex < 0 || startIndex > _size)
            {
                throw new ArgumentOutOfRangeException("startIndex");
            }

            return FindIndex(startIndex, _size - startIndex, match);
        }

        public int FindIndex(int startIndex, int count, Predicate<T> match)
        {
            ValidateRange(startIndex, count);
            if (match == null)
            {
                throw new ArgumentNullException("match");
            }

            int endIndex = startIndex + count;
            for (int i = startIndex; i < endIndex; i++)
            {
                if (match(_items[i]))
                {
                    return i;
                }
            }

            return -1;
        }

        public T Find(Predicate<T> match)
        {
            int index = FindIndex(match);
            if (index >= 0)
            {
                return _items[index];
            }

            return default(T);
        }

        public T FindLast(Predicate<T> match)
        {
            if (match == null)
            {
                throw new ArgumentNullException("match");
            }

            for (int i = _size - 1; i >= 0; i--)
            {
                if (match(_items[i]))
                {
                    return _items[i];
                }
            }

            return default(T);
        }

        public List<T> FindAll(Predicate<T> match)
        {
            if (match == null)
            {
                throw new ArgumentNullException("match");
            }

            List<T> result = new List<T>();
            for (int i = 0; i < _size; i++)
            {
                if (match(_items[i]))
                {
                    result.Add(_items[i]);
                }
            }

            return result;
        }

        public bool Exists(Predicate<T> match)
        {
            return FindIndex(match) >= 0;
        }

        public bool TrueForAll(Predicate<T> match)
        {
            if (match == null)
            {
                throw new ArgumentNullException("match");
            }

            for (int i = 0; i < _size; i++)
            {
                if (!match(_items[i]))
                {
                    return false;
                }
            }

            return true;
        }

        public void ForEach(Action<T> action)
        {
            if (action == null)
            {
                throw new ArgumentNullException("action");
            }

            int version = _version;
            for (int i = 0; i < _size; i++)
            {
                action(_items[i]);
                if (version != _version)
                {
                    throw new InvalidOperationException("Collection was modified during enumeration.");
                }
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

        bool ICollection<T>.IsReadOnly => false;
        bool IList.IsFixedSize => false;
        bool IList.IsReadOnly => false;
        bool ICollection.IsSynchronized => false;
        object ICollection.SyncRoot => this;

        object IList.this[int index]
        {
            get => this[index];
            set => this[index] = (T)value;
        }

        int IList.Add(object value)
        {
            Add((T)value);
            return _size - 1;
        }

        bool IList.Contains(object value)
        {
            return Contains((T)value);
        }

        int IList.IndexOf(object value)
        {
            return IndexOf((T)value);
        }

        void IList.Insert(int index, object value)
        {
            Insert(index, (T)value);
        }

        void IList.Remove(object value)
        {
            Remove((T)value);
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

            Array.Copy(_items, 0, array, index, _size);
        }

        private void EnsureCapacity(int minimum)
        {
            if (_items.Length >= minimum)
            {
                return;
            }

            int newCapacity = _items.Length == 0 ? DefaultCapacity : _items.Length * 2;
            if (newCapacity < minimum)
            {
                newCapacity = minimum;
            }

            Capacity = newCapacity;
        }

        private void ValidateIndex(int index)
        {
            if (index < 0 || index >= _size)
            {
                throw new ArgumentOutOfRangeException("index");
            }
        }

        private void ValidateInsertIndex(int index)
        {
            if (index < 0 || index > _size)
            {
                throw new ArgumentOutOfRangeException("index");
            }
        }

        private void ValidateRange(int index, int count)
        {
            if (index < 0)
            {
                throw new ArgumentOutOfRangeException("index");
            }

            if (count < 0)
            {
                throw new ArgumentOutOfRangeException("count");
            }

            if (index > _size - count)
            {
                throw new ArgumentException("The requested range is outside the list.");
            }
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
            private readonly List<T> _list;
            private readonly int _version;
            private int _index;
            private T _current;

            internal Enumerator(List<T> list)
            {
                _list = list;
                _version = list._version;
                _index = 0;
                _current = default(T);
            }

            public T Current
            {
                get
                {
                    if (_index == 0 || _index == _list._size + 1)
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
                if (_index < _list._size)
                {
                    _current = _list._items[_index];
                    _index++;
                    return true;
                }

                _index = _list._size + 1;
                _current = default(T);
                return false;
            }

            public void Reset()
            {
                EnsureUnmodified();
                _index = 0;
                _current = default(T);
            }

            public void Dispose()
            {
            }

            private void EnsureUnmodified()
            {
                if (_version != _list._version)
                {
                    throw new InvalidOperationException("Collection was modified during enumeration.");
                }
            }
        }
    }
}
