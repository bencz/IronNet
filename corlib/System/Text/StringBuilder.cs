namespace System.Text
{
    /// <summary>
    /// Represents a mutable string of characters
    /// </summary>
    public sealed class StringBuilder
    {
        private char[] _buffer;
        private int _length;

        public StringBuilder() : this(16)
        {
        }

        public StringBuilder(int capacity)
        {
            if (capacity < 0)
                throw new ArgumentOutOfRangeException("capacity");
            _buffer = new char[capacity];
            _length = 0;
        }

        public StringBuilder(string value) : this(value, 16)
        {
        }

        public StringBuilder(string value, int capacity)
        {
            if (capacity < 0)
                throw new ArgumentOutOfRangeException("capacity");
            int len = value?.Length ?? 0;
            int cap = capacity > len ? capacity : len;
            _buffer = new char[cap];
            if (value != null)
            {
                for (int i = 0; i < value.Length; i++)
                    _buffer[i] = value[i];
                _length = value.Length;
            }
        }

        public int Length
        {
            get => _length;
            set
            {
                if (value < 0)
                    throw new ArgumentOutOfRangeException("value");
                if (value > _buffer.Length)
                    EnsureCapacity(value);
                _length = value;
            }
        }

        public int Capacity
        {
            get => _buffer.Length;
            set
            {
                if (value < _length)
                    throw new ArgumentOutOfRangeException("value");
                if (value != _buffer.Length)
                {
                    char[] newBuffer = new char[value];
                    Array.Copy(_buffer, newBuffer, _length);
                    _buffer = newBuffer;
                }
            }
        }

        public char this[int index]
        {
            get
            {
                if (index < 0 || index >= _length)
                    throw new IndexOutOfRangeException();
                return _buffer[index];
            }
            set
            {
                if (index < 0 || index >= _length)
                    throw new IndexOutOfRangeException();
                _buffer[index] = value;
            }
        }

        public int EnsureCapacity(int capacity)
        {
            if (capacity > _buffer.Length)
            {
                int newCapacity = _buffer.Length * 2;
                if (newCapacity < capacity)
                    newCapacity = capacity;
                Capacity = newCapacity;
            }
            return _buffer.Length;
        }

        public StringBuilder Append(char value)
        {
            if (_length >= _buffer.Length)
                EnsureCapacity(_length + 1);
            _buffer[_length++] = value;
            return this;
        }

        public StringBuilder Append(char value, int repeatCount)
        {
            if (repeatCount < 0)
                throw new ArgumentOutOfRangeException("repeatCount");
            EnsureCapacity(_length + repeatCount);
            for (int i = 0; i < repeatCount; i++)
                _buffer[_length++] = value;
            return this;
        }

        public StringBuilder Append(string value)
        {
            if (value == null || value.Length == 0)
                return this;
            EnsureCapacity(_length + value.Length);
            for (int i = 0; i < value.Length; i++)
                _buffer[_length++] = value[i];
            return this;
        }

        public StringBuilder Append(object value)
        {
            if (value == null)
                return this;
            return Append(value.ToString());
        }

        public StringBuilder Append(int value)
        {
            return Append(value.ToString());
        }

        public StringBuilder Append(long value)
        {
            return Append(value.ToString());
        }

        public StringBuilder Append(bool value)
        {
            return Append(value ? "True" : "False");
        }

        public StringBuilder AppendLine()
        {
            return Append(Environment.NewLine);
        }

        public StringBuilder AppendLine(string value)
        {
            Append(value);
            return AppendLine();
        }

        public StringBuilder Insert(int index, char value)
        {
            if (index < 0 || index > _length)
                throw new ArgumentOutOfRangeException("index");
            EnsureCapacity(_length + 1);
            for (int i = _length; i > index; i--)
                _buffer[i] = _buffer[i - 1];
            _buffer[index] = value;
            _length++;
            return this;
        }

        public StringBuilder Insert(int index, string value)
        {
            if (index < 0 || index > _length)
                throw new ArgumentOutOfRangeException("index");
            if (value == null || value.Length == 0)
                return this;
            EnsureCapacity(_length + value.Length);
            for (int i = _length - 1; i >= index; i--)
                _buffer[i + value.Length] = _buffer[i];
            for (int i = 0; i < value.Length; i++)
                _buffer[index + i] = value[i];
            _length += value.Length;
            return this;
        }

        public StringBuilder Remove(int startIndex, int length)
        {
            if (startIndex < 0)
                throw new ArgumentOutOfRangeException("startIndex");
            if (length < 0)
                throw new ArgumentOutOfRangeException("length");
            if (startIndex + length > _length)
                throw new ArgumentOutOfRangeException("length");

            for (int i = startIndex; i < _length - length; i++)
                _buffer[i] = _buffer[i + length];
            _length -= length;
            return this;
        }

        public StringBuilder Replace(char oldChar, char newChar)
        {
            for (int i = 0; i < _length; i++)
            {
                if (_buffer[i] == oldChar)
                    _buffer[i] = newChar;
            }
            return this;
        }

        public StringBuilder Replace(string oldValue, string newValue)
        {
            if (oldValue == null)
                throw new ArgumentNullException("oldValue");
            if (oldValue.Length == 0)
                throw new ArgumentException("oldValue cannot be empty");

            // Simple implementation - not optimized
            string current = ToString();
            // TODO: Implement proper string replacement
            return this;
        }

        public StringBuilder Clear()
        {
            _length = 0;
            return this;
        }

        public override string ToString()
        {
            return new string(_buffer, 0, _length);
        }

        public string ToString(int startIndex, int length)
        {
            if (startIndex < 0)
                throw new ArgumentOutOfRangeException("startIndex");
            if (length < 0)
                throw new ArgumentOutOfRangeException("length");
            if (startIndex + length > _length)
                throw new ArgumentOutOfRangeException("length");

            return new string(_buffer, startIndex, length);
        }
    }

    /// <summary>
    /// Represents a character encoding
    /// </summary>
    public abstract class Encoding
    {
        private static Encoding _utf8;
        private static Encoding _ascii;

        public static Encoding UTF8 => _utf8 ?? (_utf8 = new UTF8Encoding());
        public static Encoding ASCII => _ascii ?? (_ascii = new ASCIIEncoding());
        public static Encoding Default => UTF8;

        public abstract byte[] GetBytes(string s);
        public abstract string GetString(byte[] bytes);
        public abstract string GetString(byte[] bytes, int index, int count);
        public abstract int GetByteCount(string s);
        public abstract int GetCharCount(byte[] bytes);
    }

    /// <summary>
    /// Represents a UTF-8 encoding of Unicode characters
    /// </summary>
    public class UTF8Encoding : Encoding
    {
        public override byte[] GetBytes(string s)
        {
            if (s == null)
                throw new ArgumentNullException("s");
            
            // Simple ASCII-only implementation for now
            byte[] result = new byte[s.Length];
            for (int i = 0; i < s.Length; i++)
            {
                char c = s[i];
                result[i] = c < 128 ? (byte)c : (byte)'?';
            }
            return result;
        }

        public override string GetString(byte[] bytes)
        {
            if (bytes == null)
                throw new ArgumentNullException("bytes");
            return GetString(bytes, 0, bytes.Length);
        }

        public override string GetString(byte[] bytes, int index, int count)
        {
            if (bytes == null)
                throw new ArgumentNullException("bytes");
            
            char[] chars = new char[count];
            for (int i = 0; i < count; i++)
            {
                chars[i] = (char)bytes[index + i];
            }
            return new string(chars);
        }

        public override int GetByteCount(string s)
        {
            return s?.Length ?? 0;
        }

        public override int GetCharCount(byte[] bytes)
        {
            return bytes?.Length ?? 0;
        }
    }

    /// <summary>
    /// Represents an ASCII character encoding
    /// </summary>
    public class ASCIIEncoding : Encoding
    {
        public override byte[] GetBytes(string s)
        {
            if (s == null)
                throw new ArgumentNullException("s");
            
            byte[] result = new byte[s.Length];
            for (int i = 0; i < s.Length; i++)
            {
                char c = s[i];
                result[i] = c < 128 ? (byte)c : (byte)'?';
            }
            return result;
        }

        public override string GetString(byte[] bytes)
        {
            if (bytes == null)
                throw new ArgumentNullException("bytes");
            return GetString(bytes, 0, bytes.Length);
        }

        public override string GetString(byte[] bytes, int index, int count)
        {
            if (bytes == null)
                throw new ArgumentNullException("bytes");
            
            char[] chars = new char[count];
            for (int i = 0; i < count; i++)
            {
                chars[i] = (char)bytes[index + i];
            }
            return new string(chars);
        }

        public override int GetByteCount(string s)
        {
            return s?.Length ?? 0;
        }

        public override int GetCharCount(byte[] bytes)
        {
            return bytes?.Length ?? 0;
        }
    }
}
