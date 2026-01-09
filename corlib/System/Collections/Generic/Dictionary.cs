namespace System.Collections.Generic
{
    /// <summary>
    /// Represents a collection of keys and values
    /// </summary>
    public class Dictionary<TKey, TValue> : IDictionary<TKey, TValue>, IDictionary
    {
        private struct Entry
        {
            public int hashCode;
            public int next;
            public TKey key;
            public TValue value;
        }

        private int[] _buckets;
        private Entry[] _entries;
        private int _count;
        private int _freeList;
        private int _freeCount;
        private IEqualityComparer<TKey> _comparer;
        private KeyCollection _keys;
        private ValueCollection _values;

        private const int InitialSize = 3;

        public Dictionary() : this(0, null)
        {
        }

        public Dictionary(int capacity) : this(capacity, null)
        {
        }

        public Dictionary(IEqualityComparer<TKey> comparer) : this(0, comparer)
        {
        }

        public Dictionary(int capacity, IEqualityComparer<TKey> comparer)
        {
            if (capacity < 0)
                throw new ArgumentOutOfRangeException("capacity");
            if (capacity > 0)
                Initialize(capacity);
            _comparer = comparer ?? EqualityComparer<TKey>.Default;
        }

        public int Count => _count - _freeCount;

        public TValue this[TKey key]
        {
            get
            {
                int i = FindEntry(key);
                if (i >= 0)
                    return _entries[i].value;
                throw new KeyNotFoundException();
            }
            set
            {
                Insert(key, value, false);
            }
        }

        public KeyCollection Keys => _keys ?? (_keys = new KeyCollection(this));
        public ValueCollection Values => _values ?? (_values = new ValueCollection(this));

        ICollection<TKey> IDictionary<TKey, TValue>.Keys => Keys;
        ICollection<TValue> IDictionary<TKey, TValue>.Values => Values;

        public void Add(TKey key, TValue value)
        {
            Insert(key, value, true);
        }

        public void Clear()
        {
            if (_count > 0)
            {
                for (int i = 0; i < _buckets.Length; i++)
                    _buckets[i] = -1;
                Array.Clear(_entries, 0, _count);
                _freeList = -1;
                _count = 0;
                _freeCount = 0;
            }
        }

        public bool ContainsKey(TKey key)
        {
            return FindEntry(key) >= 0;
        }

        public bool ContainsValue(TValue value)
        {
            for (int i = 0; i < _count; i++)
            {
                if (_entries[i].hashCode >= 0 && 
                    object.Equals(_entries[i].value, value))
                    return true;
            }
            return false;
        }

        public bool Remove(TKey key)
        {
            if (key == null)
                throw new ArgumentNullException("key");

            if (_buckets != null)
            {
                int hashCode = _comparer.GetHashCode(key) & 0x7FFFFFFF;
                int bucket = hashCode % _buckets.Length;
                int last = -1;
                for (int i = _buckets[bucket]; i >= 0; last = i, i = _entries[i].next)
                {
                    if (_entries[i].hashCode == hashCode && _comparer.Equals(_entries[i].key, key))
                    {
                        if (last < 0)
                            _buckets[bucket] = _entries[i].next;
                        else
                            _entries[last].next = _entries[i].next;
                        _entries[i].hashCode = -1;
                        _entries[i].next = _freeList;
                        _entries[i].key = default(TKey);
                        _entries[i].value = default(TValue);
                        _freeList = i;
                        _freeCount++;
                        return true;
                    }
                }
            }
            return false;
        }

        public bool TryGetValue(TKey key, out TValue value)
        {
            int i = FindEntry(key);
            if (i >= 0)
            {
                value = _entries[i].value;
                return true;
            }
            value = default(TValue);
            return false;
        }

        private void Initialize(int capacity)
        {
            int size = GetPrime(capacity);
            _buckets = new int[size];
            for (int i = 0; i < _buckets.Length; i++)
                _buckets[i] = -1;
            _entries = new Entry[size];
            _freeList = -1;
        }

        private int FindEntry(TKey key)
        {
            if (key == null)
                throw new ArgumentNullException("key");

            if (_buckets != null)
            {
                int hashCode = _comparer.GetHashCode(key) & 0x7FFFFFFF;
                for (int i = _buckets[hashCode % _buckets.Length]; i >= 0; i = _entries[i].next)
                {
                    if (_entries[i].hashCode == hashCode && _comparer.Equals(_entries[i].key, key))
                        return i;
                }
            }
            return -1;
        }

        private void Insert(TKey key, TValue value, bool add)
        {
            if (key == null)
                throw new ArgumentNullException("key");

            if (_buckets == null)
                Initialize(0);

            int hashCode = _comparer.GetHashCode(key) & 0x7FFFFFFF;
            int targetBucket = hashCode % _buckets.Length;

            for (int i = _buckets[targetBucket]; i >= 0; i = _entries[i].next)
            {
                if (_entries[i].hashCode == hashCode && _comparer.Equals(_entries[i].key, key))
                {
                    if (add)
                        throw new ArgumentException("Key already exists");
                    _entries[i].value = value;
                    return;
                }
            }

            int index;
            if (_freeCount > 0)
            {
                index = _freeList;
                _freeList = _entries[index].next;
                _freeCount--;
            }
            else
            {
                if (_count == _entries.Length)
                {
                    Resize();
                    targetBucket = hashCode % _buckets.Length;
                }
                index = _count;
                _count++;
            }

            _entries[index].hashCode = hashCode;
            _entries[index].next = _buckets[targetBucket];
            _entries[index].key = key;
            _entries[index].value = value;
            _buckets[targetBucket] = index;
        }

        private void Resize()
        {
            int newSize = GetPrime(_count * 2);
            int[] newBuckets = new int[newSize];
            for (int i = 0; i < newBuckets.Length; i++)
                newBuckets[i] = -1;
            Entry[] newEntries = new Entry[newSize];
            Array.Copy(_entries, newEntries, _count);
            for (int i = 0; i < _count; i++)
            {
                if (newEntries[i].hashCode >= 0)
                {
                    int bucket = newEntries[i].hashCode % newSize;
                    newEntries[i].next = newBuckets[bucket];
                    newBuckets[bucket] = i;
                }
            }
            _buckets = newBuckets;
            _entries = newEntries;
        }

        private static int GetPrime(int min)
        {
            int[] primes = { 3, 7, 11, 17, 23, 29, 37, 47, 59, 71, 89, 107, 131, 163, 197, 239, 293, 353, 431, 521, 631, 761, 919, 1103, 1327, 1597, 1931, 2333, 2801, 3371, 4049, 4861, 5839, 7013, 8419, 10103, 12143, 14591, 17519, 21023, 25229, 30293, 36353, 43627, 52361, 62851, 75431, 90523, 108631, 130363, 156437, 187751, 225307, 270371, 324449, 389357, 467237, 560689, 672827, 807403, 968897, 1162687, 1395263, 1674319, 2009191, 2411033, 2893249, 3471899, 4166287, 4999559, 5999471, 7199369 };
            for (int i = 0; i < primes.Length; i++)
            {
                if (primes[i] >= min)
                    return primes[i];
            }
            return min;
        }

        public IEnumerator<KeyValuePair<TKey, TValue>> GetEnumerator()
        {
            return new Enumerator(this);
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        // ICollection<KeyValuePair> implementation
        bool ICollection<KeyValuePair<TKey, TValue>>.IsReadOnly => false;

        void ICollection<KeyValuePair<TKey, TValue>>.Add(KeyValuePair<TKey, TValue> item)
        {
            Add(item.Key, item.Value);
        }

        bool ICollection<KeyValuePair<TKey, TValue>>.Contains(KeyValuePair<TKey, TValue> item)
        {
            int i = FindEntry(item.Key);
            return i >= 0 && object.Equals(_entries[i].value, item.Value);
        }

        void ICollection<KeyValuePair<TKey, TValue>>.CopyTo(KeyValuePair<TKey, TValue>[] array, int arrayIndex)
        {
            foreach (var kvp in this)
                array[arrayIndex++] = kvp;
        }

        bool ICollection<KeyValuePair<TKey, TValue>>.Remove(KeyValuePair<TKey, TValue> item)
        {
            int i = FindEntry(item.Key);
            if (i >= 0 && object.Equals(_entries[i].value, item.Value))
            {
                Remove(item.Key);
                return true;
            }
            return false;
        }

        // IDictionary implementation
        bool IDictionary.IsFixedSize => false;
        bool IDictionary.IsReadOnly => false;
        ICollection IDictionary.Keys => Keys;
        ICollection IDictionary.Values => Values;
        bool ICollection.IsSynchronized => false;
        object ICollection.SyncRoot => this;

        object IDictionary.this[object key]
        {
            get => this[(TKey)key];
            set => this[(TKey)key] = (TValue)value;
        }

        void IDictionary.Add(object key, object value)
        {
            Add((TKey)key, (TValue)value);
        }

        bool IDictionary.Contains(object key)
        {
            return ContainsKey((TKey)key);
        }

        IDictionaryEnumerator IDictionary.GetEnumerator()
        {
            return new DictionaryEnumerator(this);
        }

        void IDictionary.Remove(object key)
        {
            Remove((TKey)key);
        }

        void ICollection.CopyTo(Array array, int index)
        {
            foreach (var kvp in this)
                array.SetValue(kvp, index++);
        }

        public sealed class KeyCollection : ICollection<TKey>, ICollection
        {
            private Dictionary<TKey, TValue> _dictionary;

            public KeyCollection(Dictionary<TKey, TValue> dictionary)
            {
                _dictionary = dictionary ?? throw new ArgumentNullException("dictionary");
            }

            public int Count => _dictionary.Count;
            bool ICollection<TKey>.IsReadOnly => true;
            bool ICollection.IsSynchronized => false;
            object ICollection.SyncRoot => _dictionary;

            public IEnumerator<TKey> GetEnumerator()
            {
                foreach (var kvp in _dictionary)
                    yield return kvp.Key;
            }

            IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();

            void ICollection<TKey>.Add(TKey item) => throw new NotSupportedException();
            void ICollection<TKey>.Clear() => throw new NotSupportedException();
            bool ICollection<TKey>.Contains(TKey item) => _dictionary.ContainsKey(item);
            void ICollection<TKey>.CopyTo(TKey[] array, int arrayIndex)
            {
                foreach (var key in this)
                    array[arrayIndex++] = key;
            }
            bool ICollection<TKey>.Remove(TKey item) => throw new NotSupportedException();
            void ICollection.CopyTo(Array array, int index)
            {
                foreach (var key in this)
                    array.SetValue(key, index++);
            }
        }

        public sealed class ValueCollection : ICollection<TValue>, ICollection
        {
            private Dictionary<TKey, TValue> _dictionary;

            public ValueCollection(Dictionary<TKey, TValue> dictionary)
            {
                _dictionary = dictionary ?? throw new ArgumentNullException("dictionary");
            }

            public int Count => _dictionary.Count;
            bool ICollection<TValue>.IsReadOnly => true;
            bool ICollection.IsSynchronized => false;
            object ICollection.SyncRoot => _dictionary;

            public IEnumerator<TValue> GetEnumerator()
            {
                foreach (var kvp in _dictionary)
                    yield return kvp.Value;
            }

            IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();

            void ICollection<TValue>.Add(TValue item) => throw new NotSupportedException();
            void ICollection<TValue>.Clear() => throw new NotSupportedException();
            bool ICollection<TValue>.Contains(TValue item) => _dictionary.ContainsValue(item);
            void ICollection<TValue>.CopyTo(TValue[] array, int arrayIndex)
            {
                foreach (var value in this)
                    array[arrayIndex++] = value;
            }
            bool ICollection<TValue>.Remove(TValue item) => throw new NotSupportedException();
            void ICollection.CopyTo(Array array, int index)
            {
                foreach (var value in this)
                    array.SetValue(value, index++);
            }
        }

        private struct Enumerator : IEnumerator<KeyValuePair<TKey, TValue>>
        {
            private Dictionary<TKey, TValue> _dictionary;
            private int _index;
            private KeyValuePair<TKey, TValue> _current;

            internal Enumerator(Dictionary<TKey, TValue> dictionary)
            {
                _dictionary = dictionary;
                _index = 0;
                _current = default;
            }

            public KeyValuePair<TKey, TValue> Current => _current;
            object IEnumerator.Current => Current;

            public bool MoveNext()
            {
                while (_index < _dictionary._count)
                {
                    if (_dictionary._entries[_index].hashCode >= 0)
                    {
                        _current = new KeyValuePair<TKey, TValue>(
                            _dictionary._entries[_index].key,
                            _dictionary._entries[_index].value);
                        _index++;
                        return true;
                    }
                    _index++;
                }
                _current = default;
                return false;
            }

            public void Reset()
            {
                _index = 0;
                _current = default;
            }

            public void Dispose() { }
        }

        private struct DictionaryEnumerator : IDictionaryEnumerator
        {
            private Enumerator _enumerator;

            internal DictionaryEnumerator(Dictionary<TKey, TValue> dictionary)
            {
                _enumerator = new Enumerator(dictionary);
            }

            public DictionaryEntry Entry => new DictionaryEntry(_enumerator.Current.Key, _enumerator.Current.Value);
            public object Key => _enumerator.Current.Key;
            public object Value => _enumerator.Current.Value;
            public object Current => Entry;
            public bool MoveNext() => _enumerator.MoveNext();
            public void Reset() => _enumerator.Reset();
        }
    }

    /// <summary>
    /// The exception that is thrown when the key specified for accessing an element does not match any key
    /// </summary>
    public class KeyNotFoundException : SystemException
    {
        public KeyNotFoundException() : base("The given key was not present in the dictionary.") { }
        public KeyNotFoundException(string message) : base(message) { }
    }

    /// <summary>
    /// Provides a base class for implementations of the IEqualityComparer generic interface
    /// </summary>
    public abstract class EqualityComparer<T> : IEqualityComparer<T>, IEqualityComparer
    {
        private static EqualityComparer<T> _default;

        public static EqualityComparer<T> Default => _default ?? (_default = new DefaultEqualityComparer<T>());

        public abstract bool Equals(T x, T y);
        public abstract int GetHashCode(T obj);

        bool IEqualityComparer.Equals(object x, object y)
        {
            return Equals((T)x, (T)y);
        }

        int IEqualityComparer.GetHashCode(object obj)
        {
            return GetHashCode((T)obj);
        }
    }

    internal class DefaultEqualityComparer<T> : EqualityComparer<T>
    {
        public override bool Equals(T x, T y)
        {
            if (x == null)
                return y == null;
            return x.Equals(y);
        }

        public override int GetHashCode(T obj)
        {
            return obj?.GetHashCode() ?? 0;
        }
    }

}
