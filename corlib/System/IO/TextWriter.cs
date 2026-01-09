namespace System.IO
{
    /// <summary>
    /// Represents a writer that can write a sequential series of characters
    /// </summary>
    public abstract class TextWriter : IDisposable
    {
        protected char[] CoreNewLine = new char[] { '\n' };

        public virtual string NewLine
        {
            get => new string(CoreNewLine);
            set => CoreNewLine = value?.ToCharArray() ?? new char[] { '\n' };
        }

        public abstract void Write(char value);

        public virtual void Write(char[] buffer)
        {
            if (buffer != null)
            {
                Write(buffer, 0, buffer.Length);
            }
        }

        public virtual void Write(char[] buffer, int index, int count)
        {
            if (buffer == null)
                throw new ArgumentNullException("buffer");
            for (int i = 0; i < count; i++)
            {
                Write(buffer[index + i]);
            }
        }

        public virtual void Write(string value)
        {
            if (value != null)
            {
                for (int i = 0; i < value.Length; i++)
                {
                    Write(value[i]);
                }
            }
        }

        public virtual void Write(bool value)
        {
            Write(value ? "True" : "False");
        }

        public virtual void Write(int value)
        {
            Write(value.ToString());
        }

        public virtual void Write(long value)
        {
            Write(value.ToString());
        }

        public virtual void Write(double value)
        {
            Write(value.ToString());
        }

        public virtual void Write(object value)
        {
            if (value != null)
            {
                Write(value.ToString());
            }
        }

        public virtual void WriteLine()
        {
            Write(CoreNewLine);
        }

        public virtual void WriteLine(char value)
        {
            Write(value);
            WriteLine();
        }

        public virtual void WriteLine(char[] buffer)
        {
            Write(buffer);
            WriteLine();
        }

        public virtual void WriteLine(string value)
        {
            Write(value);
            WriteLine();
        }

        public virtual void WriteLine(bool value)
        {
            Write(value);
            WriteLine();
        }

        public virtual void WriteLine(int value)
        {
            Write(value);
            WriteLine();
        }

        public virtual void WriteLine(long value)
        {
            Write(value);
            WriteLine();
        }

        public virtual void WriteLine(double value)
        {
            Write(value);
            WriteLine();
        }

        public virtual void WriteLine(object value)
        {
            Write(value);
            WriteLine();
        }

        public virtual void Flush()
        {
        }

        public virtual void Close()
        {
            Dispose(true);
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
        }
    }

    /// <summary>
    /// Represents a reader that can read a sequential series of characters
    /// </summary>
    public abstract class TextReader : IDisposable
    {
        public virtual int Peek()
        {
            return -1;
        }

        public virtual int Read()
        {
            return -1;
        }

        public virtual int Read(char[] buffer, int index, int count)
        {
            if (buffer == null)
                throw new ArgumentNullException("buffer");
            int n = 0;
            for (int i = 0; i < count; i++)
            {
                int ch = Read();
                if (ch == -1)
                    break;
                buffer[index + i] = (char)ch;
                n++;
            }
            return n;
        }

        public virtual string ReadLine()
        {
            // Simple implementation
            char[] buffer = new char[256];
            int pos = 0;
            int ch;
            while ((ch = Read()) != -1)
            {
                if (ch == '\r' || ch == '\n')
                {
                    if (ch == '\r' && Peek() == '\n')
                        Read();
                    break;
                }
                if (pos >= buffer.Length)
                {
                    char[] newBuffer = new char[buffer.Length * 2];
                    Array.Copy(buffer, newBuffer, pos);
                    buffer = newBuffer;
                }
                buffer[pos++] = (char)ch;
            }
            if (pos == 0 && ch == -1)
                return null;
            return new string(buffer, 0, pos);
        }

        public virtual string ReadToEnd()
        {
            char[] buffer = new char[4096];
            int pos = 0;
            int ch;
            while ((ch = Read()) != -1)
            {
                if (pos >= buffer.Length)
                {
                    char[] newBuffer = new char[buffer.Length * 2];
                    Array.Copy(buffer, newBuffer, pos);
                    buffer = newBuffer;
                }
                buffer[pos++] = (char)ch;
            }
            return new string(buffer, 0, pos);
        }

        public virtual void Close()
        {
            Dispose(true);
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
        }
    }

    /// <summary>
    /// Implements a TextWriter that writes information to a string
    /// </summary>
    public class StringWriter : TextWriter
    {
        private char[] _buffer;
        private int _position;

        public StringWriter()
        {
            _buffer = new char[256];
            _position = 0;
        }

        public override void Write(char value)
        {
            if (_position >= _buffer.Length)
            {
                char[] newBuffer = new char[_buffer.Length * 2];
                Array.Copy(_buffer, newBuffer, _position);
                _buffer = newBuffer;
            }
            _buffer[_position++] = value;
        }

        public override string ToString()
        {
            return new string(_buffer, 0, _position);
        }
    }

    /// <summary>
    /// Implements a TextReader that reads from a string
    /// </summary>
    public class StringReader : TextReader
    {
        private string _s;
        private int _pos;

        public StringReader(string s)
        {
            _s = s ?? throw new ArgumentNullException("s");
            _pos = 0;
        }

        public override int Peek()
        {
            if (_pos >= _s.Length)
                return -1;
            return _s[_pos];
        }

        public override int Read()
        {
            if (_pos >= _s.Length)
                return -1;
            return _s[_pos++];
        }
    }
}
