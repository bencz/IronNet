namespace System.IO
{
    public class IOException : SystemException
    {
        public IOException() : base("I/O error occurred.") { }
        public IOException(string message) : base(message) { }
        public IOException(string message, Exception innerException) : base(message, innerException) { }
    }

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
        public virtual int ReadTimeout
        {
            get
            {
                throw new InvalidOperationException("Timeouts are not supported on this stream.");
            }
            set
            {
                throw new InvalidOperationException("Timeouts are not supported on this stream.");
            }
        }

        public virtual int WriteTimeout
        {
            get
            {
                throw new InvalidOperationException("Timeouts are not supported on this stream.");
            }
            set
            {
                throw new InvalidOperationException("Timeouts are not supported on this stream.");
            }
        }

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
            {
                throw new ArgumentNullException("destination");
            }
            if (bufferSize <= 0)
            {
                throw new ArgumentOutOfRangeException("bufferSize");
            }
            if (!CanRead)
            {
                throw new NotSupportedException("The source stream does not support reading.");
            }
            if (!destination.CanWrite)
            {
                throw new NotSupportedException("The destination stream does not support writing.");
            }

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
            Dispose();
        }

        protected static void ValidateBufferArguments(byte[] buffer, int offset, int count)
        {
            if (buffer == null)
            {
                throw new ArgumentNullException("buffer");
            }
            if (offset < 0)
            {
                throw new ArgumentOutOfRangeException("offset");
            }
            if (count < 0)
            {
                throw new ArgumentOutOfRangeException("count");
            }
            if (offset > buffer.Length || count > buffer.Length - offset)
            {
                throw new ArgumentException("Offset and length were out of bounds for the array.");
            }
        }

        private sealed class NullStream : Stream
        {
            public override bool CanRead => true;
            public override bool CanSeek => true;
            public override bool CanWrite => true;
            public override long Length => 0;
            public override long Position
            {
                get => 0;
                set
                {
                }
            }

            public override int Read(byte[] buffer, int offset, int count)
            {
                ValidateBufferArguments(buffer, offset, count);
                return 0;
            }

            public override void Write(byte[] buffer, int offset, int count)
            {
                ValidateBufferArguments(buffer, offset, count);
            }
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
        private bool _exposable;
        private bool _isOpen;

        public MemoryStream() : this(0)
        {
        }

        public MemoryStream(int capacity)
        {
            if (capacity < 0)
            {
                throw new ArgumentOutOfRangeException("capacity");
            }

            _buffer = new byte[capacity];
            _capacity = capacity;
            _writable = true;
            _expandable = true;
            _exposable = true;
            _isOpen = true;
        }

        public MemoryStream(byte[] buffer) : this(buffer, true)
        {
        }

        public MemoryStream(byte[] buffer, bool writable)
        {
            if (buffer == null)
            {
                throw new ArgumentNullException("buffer");
            }

            _buffer = buffer;
            _length = buffer.Length;
            _capacity = buffer.Length;
            _writable = writable;
            _expandable = false;
            _exposable = false;
            _isOpen = true;
        }

        public override bool CanRead => _isOpen;
        public override bool CanSeek => _isOpen;
        public override bool CanWrite => _writable;

        public override long Length
        {
            get
            {
                EnsureOpen();
                return _length;
            }
        }

        public override long Position
        {
            get
            {
                EnsureOpen();
                return _position;
            }
            set
            {
                if (value < 0 || value > int.MaxValue)
                {
                    throw new ArgumentOutOfRangeException("value");
                }

                EnsureOpen();
                _position = (int)value;
            }
        }

        public virtual int Capacity
        {
            get
            {
                EnsureOpen();
                return _capacity;
            }
            set
            {
                EnsureOpen();

                if (value < _length)
                {
                    throw new ArgumentOutOfRangeException("value");
                }
                if (!_expandable && value != _capacity)
                {
                    throw new NotSupportedException();
                }
                if (value != _capacity)
                {
                    byte[] newBuffer = new byte[value];
                    if (_length > 0)
                    {
                        Array.Copy(_buffer, newBuffer, _length);
                    }

                    _buffer = newBuffer;
                    _capacity = value;
                }
            }
        }

        public override int Read(byte[] buffer, int offset, int count)
        {
            ValidateBufferArguments(buffer, offset, count);
            EnsureOpen();

            int available = _length - _position;
            if (count > available)
            {
                count = available;
            }
            if (count <= 0)
            {
                return 0;
            }

            Array.Copy(_buffer, _position, buffer, offset, count);
            _position += count;
            return count;
        }

        public override void Write(byte[] buffer, int offset, int count)
        {
            ValidateBufferArguments(buffer, offset, count);
            EnsureOpen();
            EnsureWritable();

            long requiredLength = (long)_position + count;
            if (requiredLength > int.MaxValue)
            {
                throw new IOException("Stream was too long.");
            }

            int newPosition = (int)requiredLength;
            if (newPosition > _capacity)
            {
                EnsureCapacity(newPosition);
            }

            if (_position > _length)
            {
                Array.Clear(_buffer, _length, _position - _length);
            }

            Array.Copy(buffer, offset, _buffer, _position, count);
            _position = newPosition;
            if (newPosition > _length)
                _length = newPosition;
        }

        public override long Seek(long offset, SeekOrigin origin)
        {
            EnsureOpen();

            long newPosition;
            switch (origin)
            {
                case SeekOrigin.Begin:
                    newPosition = offset;
                    break;
                case SeekOrigin.Current:
                    newPosition = (long)_position + offset;
                    break;
                case SeekOrigin.End:
                    newPosition = (long)_length + offset;
                    break;
                default:
                    throw new ArgumentException("Invalid seek origin");
            }

            if (newPosition < 0)
            {
                throw new IOException("An attempt was made to move the position before the beginning of the stream.");
            }
            if (newPosition > int.MaxValue)
            {
                throw new ArgumentOutOfRangeException("offset");
            }

            _position = (int)newPosition;
            return _position;
        }

        public override void SetLength(long value)
        {
            if (value < 0 || value > int.MaxValue)
            {
                throw new ArgumentOutOfRangeException("value");
            }

            EnsureOpen();
            EnsureWritable();

            int newLength = (int)value;
            if (newLength > _capacity)
            {
                EnsureCapacity(newLength);
            }
            else if (newLength > _length)
            {
                Array.Clear(_buffer, _length, newLength - _length);
            }

            _length = newLength;
            if (_position > _length)
            {
                _position = _length;
            }
        }

        public override void Flush()
        {
        }

        public virtual byte[] ToArray()
        {
            byte[] result = new byte[_length];
            if (_length > 0)
            {
                Array.Copy(_buffer, result, _length);
            }

            return result;
        }

        public virtual byte[] GetBuffer()
        {
            if (!_exposable)
            {
                throw new UnauthorizedAccessException("MemoryStream's internal buffer cannot be accessed.");
            }

            return _buffer;
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                _isOpen = false;
                _writable = false;
                _expandable = false;
            }
        }

        private void EnsureOpen()
        {
            if (!_isOpen)
            {
                throw new ObjectDisposedException("MemoryStream");
            }
        }

        private void EnsureWritable()
        {
            if (!_writable)
            {
                throw new NotSupportedException("Stream does not support writing.");
            }
        }

        private void EnsureCapacity(int value)
        {
            if (value <= _capacity)
            {
                return;
            }
            if (!_expandable)
            {
                throw new NotSupportedException("Memory stream is not expandable.");
            }

            int newCapacity = value;
            if (newCapacity < 256)
            {
                newCapacity = 256;
            }

            if (_capacity <= int.MaxValue / 2)
            {
                int doubledCapacity = _capacity * 2;
                if (newCapacity < doubledCapacity)
                {
                    newCapacity = doubledCapacity;
                }
            }

            Capacity = newCapacity;
        }
    }
}
