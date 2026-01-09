namespace System
{
    /// <summary>
    /// Represents a value type that can be assigned null
    /// </summary>
    public struct Nullable<T> where T : struct
    {
        private bool _hasValue;
        private T _value;

        public Nullable(T value)
        {
            _hasValue = true;
            _value = value;
        }

        public bool HasValue => _hasValue;

        public T Value
        {
            get
            {
                if (!_hasValue)
                    throw new InvalidOperationException("Nullable object must have a value.");
                return _value;
            }
        }

        public T GetValueOrDefault()
        {
            return _value;
        }

        public T GetValueOrDefault(T defaultValue)
        {
            return _hasValue ? _value : defaultValue;
        }

        public override bool Equals(object other)
        {
            if (!_hasValue)
                return other == null;
            if (other == null)
                return false;
            return _value.Equals(other);
        }

        public override int GetHashCode()
        {
            return _hasValue ? _value.GetHashCode() : 0;
        }

        public override string ToString()
        {
            return _hasValue ? _value.ToString() : "";
        }

        public static implicit operator Nullable<T>(T value)
        {
            return new Nullable<T>(value);
        }

        public static explicit operator T(Nullable<T> value)
        {
            return value.Value;
        }
    }
}
