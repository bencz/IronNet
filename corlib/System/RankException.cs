namespace System
{
    public class RankException : SystemException
    {
        public RankException() : base("Attempted to operate on an array with an incorrect number of dimensions.")
        {
        }

        public RankException(string message) : base(message)
        {
        }

        public RankException(string message, Exception innerException) : base(message, innerException)
        {
        }
    }
}
