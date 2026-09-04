using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Text;

namespace System
{
    /// <summary>
    /// Represents multiple failures while preserving each original exception.
    /// </summary>
    public class AggregateException : Exception
    {
        private const string DefaultMessage = "One or more errors occurred.";
        private readonly ReadOnlyCollection<Exception> _innerExceptions;

        public AggregateException() : this(DefaultMessage)
        {
        }

        public AggregateException(string message) : this(message, new List<Exception>())
        {
        }

        public AggregateException(string message, Exception innerException) : this(message, CopySingleException(innerException))
        {
        }

        public AggregateException(params Exception[] innerExceptions) : this(DefaultMessage, CopyExceptions(innerExceptions))
        {
        }

        public AggregateException(IEnumerable<Exception> innerExceptions) : this(DefaultMessage, CopyExceptions(innerExceptions))
        {
        }

        public AggregateException(string message, params Exception[] innerExceptions) : this(message, CopyExceptions(innerExceptions))
        {
        }

        public AggregateException(string message, IEnumerable<Exception> innerExceptions) : this(message, CopyExceptions(innerExceptions))
        {
        }

        private AggregateException(string message, List<Exception> innerExceptions) : base(message, innerExceptions.Count == 0 ? null : innerExceptions[0])
        {
            // Only private, validated lists reach this constructor. No caller can mutate this snapshot.
            _innerExceptions = new ReadOnlyCollection<Exception>(innerExceptions);
        }

        public ReadOnlyCollection<Exception> InnerExceptions => _innerExceptions;

        public override string Message
        {
            get
            {
                if (_innerExceptions.Count == 0)
                {
                    return base.Message;
                }

                StringBuilder message = new StringBuilder(base.Message);
                for (int i = 0; i < _innerExceptions.Count; i++)
                {
                    message.Append(" (");
                    message.Append(_innerExceptions[i].Message);
                    message.Append(')');
                }

                return message.ToString();
            }
        }

        public override Exception GetBaseException()
        {
            Exception exception = this;
            AggregateException aggregate = this;
            while (aggregate != null && aggregate.InnerExceptions.Count == 1)
            {
                exception = aggregate.InnerException;
                aggregate = exception as AggregateException;
            }

            return exception;
        }

        public void Handle(Func<Exception, bool> predicate)
        {
            if (predicate == null)
            {
                throw new ArgumentNullException("predicate");
            }

            List<Exception> unhandled = null;
            for (int i = 0; i < _innerExceptions.Count; i++)
            {
                Exception exception = _innerExceptions[i];
                if (!predicate(exception))
                {
                    if (unhandled == null)
                    {
                        unhandled = new List<Exception>();
                    }

                    unhandled.Add(exception);
                }
            }

            if (unhandled != null)
            {
                throw new AggregateException(Message, unhandled);
            }
        }

        public AggregateException Flatten()
        {
            List<Exception> flattened = new List<Exception>();
            Queue<AggregateException> pending = new Queue<AggregateException>();
            pending.Enqueue(this);

            // Process aggregates breadth-first and iteratively, preserving duplicate leaves.
            while (pending.Count != 0)
            {
                AggregateException aggregate = pending.Dequeue();
                for (int i = 0; i < aggregate._innerExceptions.Count; i++)
                {
                    Exception exception = aggregate._innerExceptions[i];
                    AggregateException nested = exception as AggregateException;
                    if (nested == null)
                    {
                        flattened.Add(exception);
                    }
                    else
                    {
                        pending.Enqueue(nested);
                    }
                }
            }

            string message = GetType() == typeof(AggregateException) ? base.Message : Message;
            return new AggregateException(message, flattened);
        }

        public override string ToString()
        {
            StringBuilder text = new StringBuilder(base.ToString());
            for (int i = 0; i < _innerExceptions.Count; i++)
            {
                Exception exception = _innerExceptions[i];
                if (object.ReferenceEquals(exception, InnerException))
                {
                    continue;
                }

                text.Append(Environment.NewLine);
                text.Append(" ---> (Inner Exception #");
                text.Append(i);
                text.Append(") ");
                text.Append(exception.ToString());
                text.Append("<---");
                text.Append(Environment.NewLine);
            }

            return text.ToString();
        }

        private static List<Exception> CopySingleException(Exception innerException)
        {
            if (innerException == null)
            {
                throw new ArgumentNullException("innerException");
            }

            List<Exception> exceptions = new List<Exception>(1);
            exceptions.Add(innerException);
            return exceptions;
        }

        private static List<Exception> CopyExceptions(IEnumerable<Exception> innerExceptions)
        {
            if (innerExceptions == null)
            {
                throw new ArgumentNullException("innerExceptions");
            }

            List<Exception> exceptions = new List<Exception>(innerExceptions);
            for (int i = 0; i < exceptions.Count; i++)
            {
                if (exceptions[i] == null)
                {
                    throw new ArgumentException("An inner exception cannot be null.");
                }
            }

            return exceptions;
        }
    }
}
