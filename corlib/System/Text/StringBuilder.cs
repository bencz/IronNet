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

            if (newValue == null)
                newValue = "";

            int oldLen = oldValue.Length;
            int newLen = newValue.Length;
            int i = 0;

            while (i <= _length - oldLen)
            {
                bool match = true;
                for (int j = 0; j < oldLen; j++)
                {
                    if (_buffer[i + j] != oldValue[j])
                    {
                        match = false;
                        break;
                    }
                }

                if (match)
                {
                    if (newLen != oldLen)
                    {
                        int delta = newLen - oldLen;
                        if (delta > 0)
                            EnsureCapacity(_length + delta);

                        /* Shift characters after the match */
                        if (delta > 0)
                        {
                            for (int k = _length - 1; k >= i + oldLen; k--)
                                _buffer[k + delta] = _buffer[k];
                        }
                        else if (delta < 0)
                        {
                            for (int k = i + oldLen; k < _length; k++)
                                _buffer[k + delta] = _buffer[k];
                        }
                        _length += delta;
                    }

                    /* Copy replacement string */
                    for (int k = 0; k < newLen; k++)
                        _buffer[i + k] = newValue[k];

                    i += newLen;
                }
                else
                {
                    i++;
                }
            }

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
        private const int ReplacementCharacter = 0xFFFD;

        private static bool IsContinuation(byte value)
        {
            return (value & 0xC0) == 0x80;
        }

        private static int DecodeCodePoint(byte[] bytes, ref int index, int end)
        {
            int first = bytes[index];

            if (first < 0x80)
            {
                index++;
                return first;
            }

            if (first >= 0xC2 && first <= 0xDF && index + 1 < end && IsContinuation(bytes[index + 1]))
            {
                int codePoint = ((first & 0x1F) << 6) | (bytes[index + 1] & 0x3F);
                index += 2;
                return codePoint;
            }

            if (first >= 0xE0 && first <= 0xEF && index + 2 < end && IsContinuation(bytes[index + 1]) && IsContinuation(bytes[index + 2]))
            {
                int second = bytes[index + 1];
                if ((first != 0xE0 || second >= 0xA0) && (first != 0xED || second < 0xA0))
                {
                    int codePoint = ((first & 0x0F) << 12) | ((second & 0x3F) << 6) | (bytes[index + 2] & 0x3F);
                    index += 3;
                    return codePoint;
                }
            }

            if (first >= 0xF0 && first <= 0xF4 && index + 3 < end && IsContinuation(bytes[index + 1]) && IsContinuation(bytes[index + 2]) && IsContinuation(bytes[index + 3]))
            {
                int second = bytes[index + 1];
                if ((first != 0xF0 || second >= 0x90) && (first != 0xF4 || second <= 0x8F))
                {
                    int codePoint = ((first & 0x07) << 18) | ((second & 0x3F) << 12) | ((bytes[index + 2] & 0x3F) << 6) | (bytes[index + 3] & 0x3F);
                    index += 4;
                    return codePoint;
                }
            }

            index++;
            return ReplacementCharacter;
        }

        private static int ReadCodePoint(string value, ref int index)
        {
            int first = value[index];
            if (first >= 0xD800 && first <= 0xDBFF && index + 1 < value.Length)
            {
                int second = value[index + 1];
                if (second >= 0xDC00 && second <= 0xDFFF)
                {
                    index++;
                    return 0x10000 + ((first - 0xD800) << 10) + second - 0xDC00;
                }
            }

            if (first >= 0xD800 && first <= 0xDFFF)
                return ReplacementCharacter;

            return first;
        }

        private static int EncodedSize(int codePoint)
        {
            if (codePoint < 0x80)
                return 1;
            if (codePoint < 0x800)
                return 2;
            if (codePoint < 0x10000)
                return 3;
            return 4;
        }

        public override byte[] GetBytes(string s)
        {
            if (s == null)
                throw new ArgumentNullException("s");

            byte[] result = new byte[GetByteCount(s)];
            int output = 0;
            for (int i = 0; i < s.Length; i++)
            {
                int codePoint = ReadCodePoint(s, ref i);
                if (codePoint < 0x80)
                {
                    result[output++] = (byte)codePoint;
                }
                else if (codePoint < 0x800)
                {
                    result[output++] = (byte)(0xC0 | (codePoint >> 6));
                    result[output++] = (byte)(0x80 | (codePoint & 0x3F));
                }
                else if (codePoint < 0x10000)
                {
                    result[output++] = (byte)(0xE0 | (codePoint >> 12));
                    result[output++] = (byte)(0x80 | ((codePoint >> 6) & 0x3F));
                    result[output++] = (byte)(0x80 | (codePoint & 0x3F));
                }
                else
                {
                    result[output++] = (byte)(0xF0 | (codePoint >> 18));
                    result[output++] = (byte)(0x80 | ((codePoint >> 12) & 0x3F));
                    result[output++] = (byte)(0x80 | ((codePoint >> 6) & 0x3F));
                    result[output++] = (byte)(0x80 | (codePoint & 0x3F));
                }
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
            if (index < 0)
                throw new ArgumentOutOfRangeException("index");
            if (count < 0)
                throw new ArgumentOutOfRangeException("count");
            if (index > bytes.Length - count)
                throw new ArgumentOutOfRangeException("count");

            int end = index + count;
            int scan = index;
            int characterCount = 0;
            while (scan < end)
            {
                int codePoint = DecodeCodePoint(bytes, ref scan, end);
                characterCount += codePoint < 0x10000 ? 1 : 2;
            }

            char[] chars = new char[characterCount];
            int output = 0;
            while (index < end)
            {
                int codePoint = DecodeCodePoint(bytes, ref index, end);
                if (codePoint < 0x10000)
                {
                    chars[output++] = (char)codePoint;
                }
                else
                {
                    codePoint -= 0x10000;
                    chars[output++] = (char)(0xD800 + (codePoint >> 10));
                    chars[output++] = (char)(0xDC00 + (codePoint & 0x3FF));
                }
            }
            return new string(chars);
        }

        public override int GetByteCount(string s)
        {
            if (s == null)
                throw new ArgumentNullException("s");

            int byteCount = 0;
            for (int i = 0; i < s.Length; i++)
            {
                int codePoint = ReadCodePoint(s, ref i);
                byteCount += EncodedSize(codePoint);
            }
            return byteCount;
        }

        public override int GetCharCount(byte[] bytes)
        {
            if (bytes == null)
                throw new ArgumentNullException("bytes");

            int index = 0;
            int characterCount = 0;
            while (index < bytes.Length)
            {
                int codePoint = DecodeCodePoint(bytes, ref index, bytes.Length);
                characterCount += codePoint < 0x10000 ? 1 : 2;
            }
            return characterCount;
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
            if (index < 0)
                throw new ArgumentOutOfRangeException("index");
            if (count < 0)
                throw new ArgumentOutOfRangeException("count");
            if (index > bytes.Length - count)
                throw new ArgumentOutOfRangeException("count");
            
            char[] chars = new char[count];
            for (int i = 0; i < count; i++)
            {
                chars[i] = (char)bytes[index + i];
            }
            return new string(chars);
        }

        public override int GetByteCount(string s)
        {
            if (s == null)
                throw new ArgumentNullException("s");
            return s.Length;
        }

        public override int GetCharCount(byte[] bytes)
        {
            if (bytes == null)
                throw new ArgumentNullException("bytes");
            return bytes.Length;
        }
    }
}
