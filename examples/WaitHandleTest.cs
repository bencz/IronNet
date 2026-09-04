using System;
using System.Threading;

class WaitHandleTest
{
    private static ManualResetEvent _manual;
    private static Semaphore _semaphore;
    private static Mutex _mutex;
    private static bool _mutexTimedOut;

    static void Require(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception(message);
        }
    }

    static void SignalManual()
    {
        Thread.Sleep(20);
        _manual.Set();
    }

    static void ReleaseSemaphore()
    {
        Thread.Sleep(20);
        _semaphore.Release();
    }

    static void ContendMutex()
    {
        _mutexTimedOut = !_mutex.WaitOne(20);
        if (!_mutexTimedOut)
        {
            _mutex.ReleaseMutex();
        }
    }

    static int Main()
    {
        _manual = new ManualResetEvent(false);
        Require(!_manual.WaitOne(0), "An unsignaled ManualResetEvent completed immediately.");
        Thread manualWorker = new Thread(SignalManual);
        manualWorker.Start();
        Require(_manual.WaitOne(1000), "ManualResetEvent did not observe Set.");
        manualWorker.Join();
        Require(_manual.WaitOne(0), "ManualResetEvent did not remain signaled.");
        _manual.Reset();
        Require(!_manual.WaitOne(0), "ManualResetEvent.Reset did not clear the signal.");

        AutoResetEvent automatic = new AutoResetEvent(false);
        automatic.Set();
        Require(automatic.WaitOne(0), "AutoResetEvent did not consume its signal.");
        Require(!automatic.WaitOne(0), "AutoResetEvent failed to reset after one waiter.");

        _semaphore = new Semaphore(0, 1);
        Thread semaphoreWorker = new Thread(ReleaseSemaphore);
        semaphoreWorker.Start();
        Require(_semaphore.WaitOne(1000), "Semaphore did not observe Release.");
        semaphoreWorker.Join();
        Require(!_semaphore.WaitOne(0), "Semaphore allowed more acquisitions than releases.");

        _mutex = new Mutex(true);
        Thread mutexWorker = new Thread(ContendMutex);
        mutexWorker.Start();
        mutexWorker.Join();
        Require(_mutexTimedOut, "Mutex timeout was ignored while another thread owned it.");
        _mutex.ReleaseMutex();
        Require(_mutex.WaitOne(0), "Mutex could not be acquired after release.");
        _mutex.ReleaseMutex();

        Console.WriteLine("Wait handle test passed");
        return 0;
    }
}
