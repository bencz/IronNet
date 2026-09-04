using System;
using System.Threading;

class ThreadTest
{
    private static int _counter;
    private static int _workerThreadId;
    private static int _parameterValue;

    static void Require(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception(message);
        }
    }

    static void Worker()
    {
        Thread current = Thread.CurrentThread;

        _workerThreadId = current.ManagedThreadId;
        Thread.Sleep(25);
        Interlocked.Increment(ref _counter);
    }

    static void ParameterWorker(object value)
    {
        _parameterValue = (int)value;
        Interlocked.Add(ref _counter, 10);
    }

    static int Main()
    {
        Thread main = Thread.CurrentThread;
        Require(main != null, "Thread.CurrentThread returned null on the entry thread.");
        Require(main.ManagedThreadId == 1, "The entry thread has an invalid managed ID.");
        Require(main.IsAlive, "The entry thread must be alive while Main is running.");

        Thread worker = new Thread(Worker);
        Require(worker.ManagedThreadId != main.ManagedThreadId, "A worker reused the entry thread's managed ID.");
        Require(!worker.IsAlive, "An unstarted thread reported itself as alive.");

        worker.Start();
        Require(worker.IsAlive, "A started worker did not report itself as alive.");
        Require(!worker.Join(0), "A zero-timeout join unexpectedly completed a sleeping worker.");
        worker.Join();
        Require(!worker.IsAlive, "A joined worker still reported itself as alive.");
        Require(_counter == 1, "The ThreadStart delegate did not run exactly once.");
        Require(_workerThreadId == worker.ManagedThreadId, "Thread.CurrentThread did not return the running worker.");

        Thread parameterWorker = new Thread(ParameterWorker);
        parameterWorker.Start(42);
        Require(parameterWorker.Join(1000), "The parameterized worker did not complete before its timeout.");
        Require(_parameterValue == 42, "ParameterizedThreadStart received the wrong object.");
        Require(_counter == 11, "Interlocked updates from the worker threads were lost.");

        Console.WriteLine("Thread test passed");
        return 0;
    }
}
