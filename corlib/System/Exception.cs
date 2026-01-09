namespace System
{
    /// <summary>
    /// Represents errors that occur during application execution
    /// </summary>
    public class Exception
    {
        private string _message;
        private Exception _innerException;
        private string _stackTrace;

        public Exception()
        {
            _message = "Exception of type '" + GetType().FullName + "' was thrown.";
        }

        public Exception(string message)
        {
            _message = message;
        }

        public Exception(string message, Exception innerException)
        {
            _message = message;
            _innerException = innerException;
        }

        public virtual string Message => _message ?? "An error occurred.";

        public Exception InnerException => _innerException;

        public virtual string StackTrace => _stackTrace;

        public override string ToString()
        {
            string result = GetType().FullName + ": " + Message;
            if (_innerException != null)
                result += " ---> " + _innerException.ToString();
            if (_stackTrace != null)
                result += "\n" + _stackTrace;
            return result;
        }
    }

    public class SystemException : Exception
    {
        public SystemException() : base() { }
        public SystemException(string message) : base(message) { }
        public SystemException(string message, Exception innerException) : base(message, innerException) { }
    }

    public class ArgumentException : SystemException
    {
        public ArgumentException() : base("Value does not fall within the expected range.") { }
        public ArgumentException(string message) : base(message) { }
        public ArgumentException(string message, Exception innerException) : base(message, innerException) { }
    }

    public class ArgumentNullException : ArgumentException
    {
        public ArgumentNullException() : base("Value cannot be null.") { }
        public ArgumentNullException(string paramName) : base("Value cannot be null. Parameter name: " + paramName) { }
    }

    public class ArgumentOutOfRangeException : ArgumentException
    {
        public ArgumentOutOfRangeException() : base("Specified argument was out of the range of valid values.") { }
        public ArgumentOutOfRangeException(string paramName) : base("Specified argument was out of the range of valid values. Parameter name: " + paramName) { }
    }

    public class InvalidOperationException : SystemException
    {
        public InvalidOperationException() : base("Operation is not valid due to the current state of the object.") { }
        public InvalidOperationException(string message) : base(message) { }
    }

    public class NotSupportedException : SystemException
    {
        public NotSupportedException() : base("Specified method is not supported.") { }
        public NotSupportedException(string message) : base(message) { }
    }

    public class NotImplementedException : SystemException
    {
        public NotImplementedException() : base("The method or operation is not implemented.") { }
        public NotImplementedException(string message) : base(message) { }
    }

    public class NullReferenceException : SystemException
    {
        public NullReferenceException() : base("Object reference not set to an instance of an object.") { }
        public NullReferenceException(string message) : base(message) { }
    }

    public class IndexOutOfRangeException : SystemException
    {
        public IndexOutOfRangeException() : base("Index was outside the bounds of the array.") { }
        public IndexOutOfRangeException(string message) : base(message) { }
    }

    public class InvalidCastException : SystemException
    {
        public InvalidCastException() : base("Specified cast is not valid.") { }
        public InvalidCastException(string message) : base(message) { }
    }

    public class ArithmeticException : SystemException
    {
        public ArithmeticException() : base("Overflow or underflow in the arithmetic operation.") { }
        public ArithmeticException(string message) : base(message) { }
    }

    public class OverflowException : ArithmeticException
    {
        public OverflowException() : base("Arithmetic operation resulted in an overflow.") { }
        public OverflowException(string message) : base(message) { }
    }

    public class DivideByZeroException : ArithmeticException
    {
        public DivideByZeroException() : base("Attempted to divide by zero.") { }
        public DivideByZeroException(string message) : base(message) { }
    }

    public class FormatException : SystemException
    {
        public FormatException() : base("Input string was not in a correct format.") { }
        public FormatException(string message) : base(message) { }
    }

    public class OutOfMemoryException : SystemException
    {
        public OutOfMemoryException() : base("Insufficient memory to continue the execution of the program.") { }
        public OutOfMemoryException(string message) : base(message) { }
    }

    public class StackOverflowException : SystemException
    {
        public StackOverflowException() : base("Operation caused a stack overflow.") { }
    }
}
