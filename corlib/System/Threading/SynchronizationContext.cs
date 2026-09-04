namespace System.Threading
{
    public delegate void SendOrPostCallback(object state);

    public class SynchronizationContext
    {
        public static SynchronizationContext Current => Thread.CurrentThread.SynchronizationContext;

        public static void SetSynchronizationContext(SynchronizationContext syncContext)
        {
            Thread.CurrentThread.SynchronizationContext = syncContext;
        }

        public virtual void Send(SendOrPostCallback callback, object state)
        {
            if (callback == null)
            {
                throw new ArgumentNullException("callback");
            }

            callback(state);
        }

        public virtual void Post(SendOrPostCallback callback, object state)
        {
            if (callback == null)
            {
                throw new ArgumentNullException("callback");
            }

            Thread thread = new Thread(() => callback(state));
            thread.IsBackground = true;
            thread.Start();
        }

        public virtual SynchronizationContext CreateCopy()
        {
            return new SynchronizationContext();
        }

        public virtual void OperationStarted()
        {
            // The base context does not track operations; derived dispatchers can override this notification.
        }

        public virtual void OperationCompleted()
        {
            // The base context does not track operations; derived dispatchers can override this notification.
        }
    }
}
