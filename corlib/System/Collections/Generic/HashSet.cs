namespace System.Collections.Generic
{
    /// <summary>
    /// Represents a set of values.
    /// </summary>
    public class HashSet<T> : ISet<T>
    {
        private struct Slot
        {
            public int HashCode;
            public int Next;
            public T Value;
        }

        private int[] _buckets;
        private Slot[] _slots;
        private int _count;
        private int _lastIndex;
        private int _freeList;
        private int _version;
        private readonly IEqualityComparer<T> _comparer;

        public HashSet() : this(EqualityComparer<T>.Default)
        {
        }

        public HashSet(IEqualityComparer<T> comparer)
        {
            _comparer = comparer ?? EqualityComparer<T>.Default;
            _freeList = -1;
        }

        public HashSet(IEnumerable<T> collection) : this(collection, EqualityComparer<T>.Default)
        {
        }

        public HashSet(IEnumerable<T> collection, IEqualityComparer<T> comparer) : this(comparer)
        {
            if (collection == null)
            {
                throw new ArgumentNullException("collection");
            }

            UnionWith(collection);
        }

        public int Count => _count;
        public IEqualityComparer<T> Comparer => _comparer;
        bool ICollection<T>.IsReadOnly => false;

        public bool Add(T item)
        {
            return AddIfNotPresent(item);
        }

        void ICollection<T>.Add(T item)
        {
            AddIfNotPresent(item);
        }

        public void Clear()
        {
            if (_lastIndex == 0)
            {
                return;
            }

            Array.Clear(_slots, 0, _lastIndex);
            for (int i = 0; i < _buckets.Length; i++)
            {
                _buckets[i] = -1;
            }

            _count = 0;
            _lastIndex = 0;
            _freeList = -1;
            _version++;
        }

        public bool Contains(T item)
        {
            return FindItemIndex(item) >= 0;
        }

        public void CopyTo(T[] array)
        {
            CopyTo(array, 0, _count);
        }

        public void CopyTo(T[] array, int arrayIndex)
        {
            CopyTo(array, arrayIndex, _count);
        }

        public void CopyTo(T[] array, int arrayIndex, int count)
        {
            if (array == null)
            {
                throw new ArgumentNullException("array");
            }

            if (arrayIndex < 0)
            {
                throw new ArgumentOutOfRangeException("arrayIndex");
            }

            if (count < 0 || count > _count)
            {
                throw new ArgumentOutOfRangeException("count");
            }

            if (arrayIndex > array.Length || array.Length - arrayIndex < count)
            {
                throw new ArgumentException("The destination array is not large enough.");
            }

            int copied = 0;
            for (int i = 0; i < _lastIndex && copied < count; i++)
            {
                if (_slots[i].HashCode >= 0)
                {
                    array[arrayIndex + copied] = _slots[i].Value;
                    copied++;
                }
            }
        }

        public bool Remove(T item)
        {
            if (_buckets == null)
            {
                return false;
            }

            int hashCode = InternalGetHashCode(item);
            int bucket = hashCode % _buckets.Length;
            int previous = -1;
            int collisions = 0;

            for (int i = _buckets[bucket]; i >= 0; i = _slots[i].Next)
            {
                if (_slots[i].HashCode == hashCode && _comparer.Equals(_slots[i].Value, item))
                {
                    if (previous < 0)
                    {
                        _buckets[bucket] = _slots[i].Next;
                    }
                    else
                    {
                        _slots[previous].Next = _slots[i].Next;
                    }

                    _slots[i].HashCode = -1;
                    _slots[i].Next = _freeList;
                    _slots[i].Value = default(T);
                    _freeList = i;
                    _count--;
                    _version++;

                    if (_count == 0)
                    {
                        _lastIndex = 0;
                        _freeList = -1;
                    }

                    return true;
                }

                previous = i;
                collisions++;
                EnsureValidChain(collisions);
            }

            return false;
        }

        public void UnionWith(IEnumerable<T> other)
        {
            ValidateOther(other);
            foreach (T item in other)
            {
                AddIfNotPresent(item);
            }
        }

        public void IntersectWith(IEnumerable<T> other)
        {
            ValidateOther(other);
            if (_count == 0)
            {
                return;
            }

            HashSet<T> otherSet = CreateCompatibleSet(other);
            List<T> toRemove = new List<T>();
            foreach (T item in this)
            {
                if (!otherSet.Contains(item))
                {
                    toRemove.Add(item);
                }
            }

            for (int i = 0; i < toRemove.Count; i++)
            {
                Remove(toRemove[i]);
            }
        }

        public void ExceptWith(IEnumerable<T> other)
        {
            ValidateOther(other);
            if (object.ReferenceEquals(this, other))
            {
                Clear();
                return;
            }

            foreach (T item in other)
            {
                Remove(item);
            }
        }

        public void SymmetricExceptWith(IEnumerable<T> other)
        {
            ValidateOther(other);
            if (object.ReferenceEquals(this, other))
            {
                Clear();
                return;
            }

            HashSet<T> uniqueOther = CreateCompatibleSet(other);
            foreach (T item in uniqueOther)
            {
                if (!Remove(item))
                {
                    AddIfNotPresent(item);
                }
            }
        }

        public bool IsSubsetOf(IEnumerable<T> other)
        {
            ValidateOther(other);
            HashSet<T> otherSet = CreateCompatibleSet(other);
            if (_count > otherSet.Count)
            {
                return false;
            }

            foreach (T item in this)
            {
                if (!otherSet.Contains(item))
                {
                    return false;
                }
            }

            return true;
        }

        public bool IsProperSubsetOf(IEnumerable<T> other)
        {
            ValidateOther(other);
            HashSet<T> otherSet = CreateCompatibleSet(other);
            return _count < otherSet.Count && IsSubsetOf(otherSet);
        }

        public bool IsSupersetOf(IEnumerable<T> other)
        {
            ValidateOther(other);
            foreach (T item in other)
            {
                if (!Contains(item))
                {
                    return false;
                }
            }

            return true;
        }

        public bool IsProperSupersetOf(IEnumerable<T> other)
        {
            ValidateOther(other);
            HashSet<T> otherSet = CreateCompatibleSet(other);
            return _count > otherSet.Count && IsSupersetOf(otherSet);
        }

        public bool Overlaps(IEnumerable<T> other)
        {
            ValidateOther(other);
            foreach (T item in other)
            {
                if (Contains(item))
                {
                    return true;
                }
            }

            return false;
        }

        public bool SetEquals(IEnumerable<T> other)
        {
            ValidateOther(other);
            HashSet<T> otherSet = CreateCompatibleSet(other);
            return _count == otherSet.Count && IsSubsetOf(otherSet);
        }

        public void TrimExcess()
        {
            if (_count == 0)
            {
                _buckets = null;
                _slots = null;
                _lastIndex = 0;
                _freeList = -1;
                _version++;
                return;
            }

            int newSize = HashHelpers.GetPrime(_count);
            int[] newBuckets = CreateBuckets(newSize);
            Slot[] newSlots = new Slot[newSize];
            int newIndex = 0;

            for (int i = 0; i < _lastIndex; i++)
            {
                if (_slots[i].HashCode < 0)
                {
                    continue;
                }

                int bucket = _slots[i].HashCode % newSize;
                newSlots[newIndex].HashCode = _slots[i].HashCode;
                newSlots[newIndex].Value = _slots[i].Value;
                newSlots[newIndex].Next = newBuckets[bucket];
                newBuckets[bucket] = newIndex;
                newIndex++;
            }

            _buckets = newBuckets;
            _slots = newSlots;
            _lastIndex = newIndex;
            _freeList = -1;
            _version++;
        }

        public IEnumerator<T> GetEnumerator()
        {
            return new Enumerator(this);
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        private bool AddIfNotPresent(T value)
        {
            if (_buckets == null)
            {
                Initialize(0);
            }

            int hashCode = InternalGetHashCode(value);
            int bucket = hashCode % _buckets.Length;
            int collisions = 0;
            for (int i = _buckets[bucket]; i >= 0; i = _slots[i].Next)
            {
                if (_slots[i].HashCode == hashCode && _comparer.Equals(_slots[i].Value, value))
                {
                    return false;
                }

                collisions++;
                EnsureValidChain(collisions);
            }

            int index;
            if (_freeList >= 0)
            {
                index = _freeList;
                _freeList = _slots[index].Next;
            }
            else
            {
                if (_lastIndex == _slots.Length)
                {
                    IncreaseCapacity();
                    bucket = hashCode % _buckets.Length;
                }

                index = _lastIndex;
                _lastIndex++;
            }

            _slots[index].HashCode = hashCode;
            _slots[index].Value = value;
            _slots[index].Next = _buckets[bucket];
            _buckets[bucket] = index;
            _count++;
            _version++;
            return true;
        }

        private int FindItemIndex(T item)
        {
            if (_buckets == null)
            {
                return -1;
            }

            int hashCode = InternalGetHashCode(item);
            int collisions = 0;
            for (int i = _buckets[hashCode % _buckets.Length]; i >= 0; i = _slots[i].Next)
            {
                if (_slots[i].HashCode == hashCode && _comparer.Equals(_slots[i].Value, item))
                {
                    return i;
                }

                collisions++;
                EnsureValidChain(collisions);
            }

            return -1;
        }

        private int InternalGetHashCode(T item)
        {
            if (item == null)
            {
                return 0;
            }

            return _comparer.GetHashCode(item) & 0x7FFFFFFF;
        }

        private void Initialize(int capacity)
        {
            int size = HashHelpers.GetPrime(capacity);
            _buckets = CreateBuckets(size);
            _slots = new Slot[size];
            _freeList = -1;
        }

        private void IncreaseCapacity()
        {
            int newSize = HashHelpers.ExpandPrime(_count);
            int[] newBuckets = CreateBuckets(newSize);
            Slot[] newSlots = new Slot[newSize];
            Array.Copy(_slots, 0, newSlots, 0, _lastIndex);

            for (int i = 0; i < _lastIndex; i++)
            {
                int bucket = newSlots[i].HashCode % newSize;
                newSlots[i].Next = newBuckets[bucket];
                newBuckets[bucket] = i;
            }

            _buckets = newBuckets;
            _slots = newSlots;
        }

        private static int[] CreateBuckets(int size)
        {
            int[] buckets = new int[size];
            for (int i = 0; i < buckets.Length; i++)
            {
                buckets[i] = -1;
            }

            return buckets;
        }

        private HashSet<T> CreateCompatibleSet(IEnumerable<T> source)
        {
            HashSet<T> sourceSet = source as HashSet<T>;
            if (sourceSet != null && object.ReferenceEquals(sourceSet.Comparer, _comparer))
            {
                return sourceSet;
            }

            return new HashSet<T>(source, _comparer);
        }

        private void ValidateOther(IEnumerable<T> other)
        {
            if (other == null)
            {
                throw new ArgumentNullException("other");
            }
        }

        private void EnsureValidChain(int collisions)
        {
            if (collisions > _lastIndex)
            {
                throw new InvalidOperationException("The set's internal state is corrupted.");
            }
        }

        private struct Enumerator : IEnumerator<T>
        {
            private readonly HashSet<T> _set;
            private readonly int _version;
            private int _index;
            private T _current;

            internal Enumerator(HashSet<T> set)
            {
                _set = set;
                _version = set._version;
                _index = 0;
                _current = default(T);
            }

            public T Current => _current;
            object IEnumerator.Current => Current;

            public bool MoveNext()
            {
                EnsureUnmodified();
                while (_index < _set._lastIndex)
                {
                    if (_set._slots[_index].HashCode >= 0)
                    {
                        _current = _set._slots[_index].Value;
                        _index++;
                        return true;
                    }

                    _index++;
                }

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
                if (_version != _set._version)
                {
                    throw new InvalidOperationException("Collection was modified during enumeration.");
                }
            }
        }
    }
}
