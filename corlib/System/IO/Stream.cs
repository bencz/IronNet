namespace System.IO
{
    /// <summary>
    /// Provides a generic view of a sequence of bytes
    /// </summary>
    public abstract class Stream : IDisposable
    {
        public static readonly Stream Null = new NullStream();

        public abstract bool CanRead { get; }
        public abstract bool CanSeek { get; }
        public abstract bool CanWrite { get; }
        public abstract long Length { get; }
        public abstract long Position { get; set; }

        public virtual bool CanTimeout => false;
        public virtual int ReadTimeout { get; set; }
        public virtual int WriteTimeout { get; set; }

        public abstract int Read(byte[] buffer, int offset, int count);
        public abstract void Write(byte[] buffer, int offset, int count);
        public abstract long Seek(long offset, SeekOrigin origin);
        public abstract void SetLength(long value);
        public abstract void Flush();

        public virtual int ReadByte()
        {
            byte[] buffer = new byte[1];
            int read = Read(buffer, 0, 1);
            return read == 0 ? -1 : buffer[0];
        }

        public virtual void WriteByte(byte value)
        {
            byte[] buffer = new byte[] { value };
            Write(buffer, 0, 1);
        }

        public void CopyTo(Stream destination)
        {
            CopyTo(destination, 81920);
        }

        public virtual void CopyTo(Stream destination, int bufferSize)
        {
            if (destination == null)
                throw new ArgumentNullException("destination");
            if (bufferSize <= 0)
                throw new ArgumentOutOfRangeException("bufferSize");

            byte[] buffer = new byte[bufferSize];
            int read;
            while ((read = Read(buffer, 0, buffer.Length)) != 0)
            {
                destination.Write(buffer, 0, read);
            }
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
        }

        public virtual void Close()
        {
            Dispose(true);
        }

        private sealed class NullStream : Stream
        {
            public override bool CanRead => true;
            public override bool CanSeek => true;
            public override bool CanWrite => true;
            public override long Length => 0;
            public override long Position { get; set; }

            public override int Read(byte[] buffer, int offset, int count) => 0;
            public override void Write(byte[] buffer, int offset, int count) { }
            public override long Seek(long offset, SeekOrigin origin) => 0;
            public override void SetLength(long value) { }
            public override void Flush() { }
        }
    }

    /// <summary>
    /// Specifies the position in a stream to use for seeking
    /// </summary>
    public enum SeekOrigin
    {
        Begin = 0,
        Current = 1,
        End = 2
    }

    /// <summary>
    /// Creates a stream whose backing store is memory
    /// </summary>
    public class MemoryStream : Stream
    {
        private byte[] _buffer;
        private int _position;
        private int _length;
        private int _capacity;
        private bool _writable;
        private bool _expandable;

        public MemoryStream() : this(0)
        {
        }

        public MemoryStream(int capacity)
        {
            if (capacity < 0)
                throw new ArgumentOutOfRangeException("capacity");
            _buffer = new byte[capacity];
            _capacity = capacity;
            _writable = true;
            _expandable = true;
        }

        public MemoryStream(byte[] buffer) : this(buffer, true)
        {
        }

        public MemoryStream(byte[] buffer, bool writable)
        {
            if (buffer == null)
                throw new ArgumentNullException("buffer");
            _buffer = buffer;
            _length = buffer.Length;
            _capacity = buffer.Length;
            _writable = writable;
            _expandable = false;
        }

        public override bool CanRead => true;
        public override bool CanSeek => true;
        public override bool CanWrite => _writable;
        public override long Length => _length;

        public override long Position
        {
            get => _position;
            set
            {
                if (value < 0)
                    throw new ArgumentOutOfRangeException("value");
                _position = (int)value;
            }
        }

        public virtual int Capacity
        {
            get => _capacity;
            set
            {
                if (value < _length)
                    throw new ArgumentOutOfRangeException("value");
                if (!_expandable && value != _capacity)
                    throw new NotSupportedException();
                if (value != _capacity)
                {
                    byte[] newBuffer = new byte[value];
                    if (_length > 0)
                        Array.Copy(_buffer, newBuffer, _length);
                    _buffer = newBuffer;
                    _capacity = value;
                }
            }
        }

        public override int Read(byte[] buffer, int offset, int count)
        {
            if (buffer == null)
                throw new ArgumentNullException("buffer");
            if (offset < 0)
                throw new ArgumentOutOfRangeException("offset");
            if (count < 0)
                throw new ArgumentOutOfRangeException("count");

            int available = _length - _position;
            if (count > available)
                count = available;
            if (count <= 0)
                return 0;

            Array.Copy(_buffer, _position, buffer, offset, count);
            _position += count;
            return count;
        }

        public override void Write(byte[] buffer, int offset, int count)
        {
            if (buffer == null)
                throw new ArgumentNullException("buffer");
            if (offset < 0)
                throw new ArgumentOutOfRangeException("offset");
            if (count < 0)
                throw new ArgumentOutOfRangeException("count");
            if (!_writable)
                throw new NotSupportedException();

            int newPosition = _position + count;
            if (newPosition > _capacity)
            {
                if (!_expandable)
                    throw new NotSupportedException();
                int newCapacity = _capacity * 2;
                if (newCapacity < newPosition)
                    newCapacity = newPosition;
                Capacity = newCapacity;
            }

            Array.Copy(buffer, offset, _buffer, _position, count);
            _position = newPosition;
            if (newPosition > _length)
                _length = newPosition;
        }

        public override long Seek(long offset, SeekOrigin origin)
        {
            int newPosition;
            switch (origin)
            {
                case SeekOrigin.Begin:
                    newPosition = (int)offset;
                    break;
                case SeekOrigin.Current:
                    newPosition = _position + (int)offset;
                    break;
                case SeekOrigin.End:
                    newPosition = _length + (int)offset;
                    break;
                default:
                    throw new ArgumentException("Invalid seek origin");
            }

            if (newPosition < 0)
                throw new ArgumentOutOfRangeException("offset");

            _position = newPosition;
            return _position;
        }

        public override void SetLength(long value)
        {
            if (value < 0 || value > int.MaxValue)
                throw new ArgumentOutOfRangeException("value");
            if (!_writable)
                throw new NotSupportedException();

            int newLength = (int)value;
            if (newLength > _capacity)
            {
                if (!_expandable)
                    throw new NotSupportedException();
                Capacity = newLength;
            }
            _length = newLength;
            if (_position > _length)
                _position = _length;
        }

        public override void Flush()
        {
        }

        public virtual byte[] ToArray()
        {
            byte[] result = new byte[_length];
            Array.Copy(_buffer, result, _length);
            return result;
        }

        public virtual byte[] GetBuffer()
        {
            return _buffer;
        }
    }
}
