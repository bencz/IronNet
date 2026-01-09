namespace System
{
    /// <summary>
    /// Provides a mechanism for releasing unmanaged resources
    /// </summary>
    public interface IDisposable
    {
        void Dispose();
    }

    /// <summary>
    /// Defines a generalized type-specific comparison method
    /// </summary>
    public interface IComparable
    {
        int CompareTo(object obj);
    }

    /// <summary>
    /// Defines a generalized comparison method
    /// </summary>
    public interface IComparable<in T>
    {
        int CompareTo(T other);
    }

    /// <summary>
    /// Defines a method that a type implements to compare two objects
    /// </summary>
    public interface IEquatable<T>
    {
        bool Equals(T other);
    }

    /// <summary>
    /// Provides a mechanism for retrieving an object to control formatting
    /// </summary>
    public interface IFormatProvider
    {
        object GetFormat(Type formatType);
    }

    /// <summary>
    /// Provides functionality to format the value of an object into a string representation
    /// </summary>
    public interface IFormattable
    {
        string ToString(string format, IFormatProvider formatProvider);
    }

    /// <summary>
    /// Supports cloning, which creates a new instance of a class with the same value
    /// </summary>
    public interface ICloneable
    {
        object Clone();
    }
}
