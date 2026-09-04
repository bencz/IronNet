using System.Collections.Generic;

namespace System.Collections.ObjectModel
{
    /// <summary>
    /// Provides a read-only view of a list, reflecting changes made to the underlying list.
    /// </summary>
    public class ReadOnlyCollection<T> : IList<T>, IList, IReadOnlyList<T>
    {
        private readonly IList<T> _list;

        public ReadOnlyCollection(IList<T> list)
        {
            if (list == null)
            {
                throw new ArgumentNullException("list");
            }

            _list = list;
        }

        public int Count => _list.Count;
        public T this[int index] => _list[index];
        protected IList<T> Items => _list;

        public bool Contains(T value)
        {
            return _list.Contains(value);
        }

        public int IndexOf(T value)
        {
            return _list.IndexOf(value);
        }

        public void CopyTo(T[] array, int index)
        {
            _list.CopyTo(array, index);
        }

        public IEnumerator<T> GetEnumerator()
        {
            return _list.GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return ((IEnumerable)_list).GetEnumerator();
        }

        bool ICollection<T>.IsReadOnly => true;
        bool IList.IsReadOnly => true;
        bool IList.IsFixedSize => true;
        bool ICollection.IsSynchronized => false;

        object ICollection.SyncRoot
        {
            get
            {
                ICollection collection = _list as ICollection;
                return collection == null ? this : collection.SyncRoot;
            }
        }

        T IList<T>.this[int index]
        {
            get => _list[index];
            set => throw ReadOnlyError();
        }

        object IList.this[int index]
        {
            get => _list[index];
            set => throw ReadOnlyError();
        }

        void ICollection<T>.Add(T value)
        {
            throw ReadOnlyError();
        }

        void ICollection<T>.Clear()
        {
            throw ReadOnlyError();
        }

        void IList<T>.Insert(int index, T value)
        {
            throw ReadOnlyError();
        }

        bool ICollection<T>.Remove(T value)
        {
            throw ReadOnlyError();
        }

        void IList<T>.RemoveAt(int index)
        {
            throw ReadOnlyError();
        }

        int IList.Add(object value)
        {
            throw ReadOnlyError();
        }

        void IList.Clear()
        {
            throw ReadOnlyError();
        }

        void IList.Insert(int index, object value)
        {
            throw ReadOnlyError();
        }

        void IList.Remove(object value)
        {
            throw ReadOnlyError();
        }

        void IList.RemoveAt(int index)
        {
            throw ReadOnlyError();
        }

        bool IList.Contains(object value)
        {
            return IsCompatible(value) && _list.Contains((T)value);
        }

        int IList.IndexOf(object value)
        {
            return IsCompatible(value) ? _list.IndexOf((T)value) : -1;
        }

        void ICollection.CopyTo(Array array, int index)
        {
            if (array == null)
            {
                throw new ArgumentNullException("array");
            }
            if (array.Rank != 1 || array.GetLowerBound(0) != 0)
            {
                throw new ArgumentException("The destination must be a one-dimensional array with a zero lower bound.");
            }
            if (index < 0)
            {
                throw new ArgumentOutOfRangeException("index");
            }
            if (index > array.Length || Count > array.Length - index)
            {
                throw new ArgumentException("The destination array is not large enough.");
            }

            T[] typedArray = array as T[];
            if (typedArray != null)
            {
                _list.CopyTo(typedArray, index);
                return;
            }

            Type elementType = array.GetType().GetElementType();
            Type sourceType = typeof(T);
            object[] objects = array as object[];
            if (objects == null || (!elementType.IsAssignableFrom(sourceType) && !sourceType.IsAssignableFrom(elementType)))
            {
                throw new ArgumentException("The destination array type is incompatible with this collection.");
            }

            // Reference-array covariance permits narrowing, but every actual value must fit.
            // Validate as we copy so a later failure preserves the already copied prefix.
            int count = _list.Count;
            for (int i = 0; i < count; i++)
            {
                object value = _list[i];
                if (value != null && !elementType.IsInstanceOfType(value))
                {
                    throw new ArgumentException("The destination array type is incompatible with an element.");
                }

                objects[index + i] = value;
            }
        }

        private static bool IsCompatible(object value)
        {
            return value is T || (value == null && (object)default(T) == null);
        }

        private static NotSupportedException ReadOnlyError()
        {
            return new NotSupportedException("Collection is read-only.");
        }
    }
}
