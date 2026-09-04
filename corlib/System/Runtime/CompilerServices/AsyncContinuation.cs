using System.Threading;

namespace System.Runtime.CompilerServices
{
    internal sealed class AsyncContinuation
    {
        private readonly Action _continuation;
        private readonly SynchronizationContext _context;

        internal AsyncContinuation(Action continuation, bool continueOnCapturedContext)
        {
            if (continuation == null)
            {
                throw new ArgumentNullException("continuation");
            }

            _continuation = continuation;
            SynchronizationContext context = continueOnCapturedContext ? SynchronizationContext.Current : null;
            if (context != null && context.GetType() != typeof(SynchronizationContext))
            {
                _context = context;
            }
        }

        internal void Schedule()
        {
            if (_context != null)
            {
                try
                {
                    _context.Post(state => ((AsyncContinuation)state).Invoke(), this);
                }
                catch (Exception exception)
                {
                    ThrowAsync(exception, null);
                }
            }
            else
            {
                Thread thread = new Thread(Invoke);
                thread.IsBackground = true;
                thread.Start();
            }
        }

        private void Invoke()
        {
            SynchronizationContext previous = SynchronizationContext.Current;
            try
            {
                SynchronizationContext.SetSynchronizationContext(_context);
                _continuation();
            }
            finally
            {
                SynchronizationContext.SetSynchronizationContext(previous);
            }
        }

        internal static void ThrowAsync(Exception exception, SynchronizationContext context)
        {
            if (context != null)
            {
                try
                {
                    context.Post(state => { throw (Exception)state; }, exception);
                    return;
                }
                catch (Exception dispatchException)
                {
                    exception = new AggregateException(new Exception[] { exception, dispatchException });
                }
            }

            Exception failure = exception;
            Thread thread = new Thread(() => { throw failure; });
            thread.IsBackground = true;
            thread.Start();
        }

        internal static void Start<TStateMachine>(ref TStateMachine stateMachine) where TStateMachine : IAsyncStateMachine
        {
            if (stateMachine == null)
            {
                throw new ArgumentNullException("stateMachine");
            }

            SynchronizationContext previous = SynchronizationContext.Current;
            try
            {
                stateMachine.MoveNext();
            }
            finally
            {
                SynchronizationContext.SetSynchronizationContext(previous);
            }
        }
    }
}
