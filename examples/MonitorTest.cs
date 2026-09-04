using System;
using System.Threading;

class MonitorTest
{
    private static readonly object _gate = new object();
    private static int _waiting;
    private static int _value;
    private static int _observed;
    private static bool _timedAcquire;

    static void Require(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception(message);
        }
    }

    static void WaitingWorker()
    {
        Monitor.Enter(_gate);
        _waiting = 1;
        while (_value == 0)
        {
            Monitor.Wait(_gate);
        }

        _observed = _value;
        Monitor.Exit(_gate);
    }

    static void TimeoutWorker()
    {
        _timedAcquire = Monitor.TryEnter(_gate, 20);
        if (_timedAcquire)
        {
            Monitor.Exit(_gate);
        }
    }

    static int Main()
    {
        Monitor.Enter(_gate);
        Monitor.Enter(_gate);
        Monitor.Exit(_gate);
        Require(Monitor.TryEnter(_gate, 0), "A monitor was not reentrant for its owning thread.");
        Monitor.Exit(_gate);
        Require(!Monitor.Wait(_gate, 0), "Monitor.Wait should report a timeout when it is not pulsed.");
        Monitor.Exit(_gate);

        Thread waiter = new Thread(WaitingWorker);
        waiter.Start();
        while (_waiting == 0)
        {
            Thread.Sleep(1);
        }

        Monitor.Enter(_gate);
        _value = 73;
        Monitor.Pulse(_gate);
        Monitor.Exit(_gate);
        waiter.Join();
        Require(_observed == 73, "Monitor.Wait did not resume after Monitor.Pulse.");

        Monitor.Enter(_gate);
        Thread timeout = new Thread(TimeoutWorker);
        timeout.Start();
        timeout.Join();
        Require(!_timedAcquire, "Monitor.TryEnter ignored its timeout while another thread owned the monitor.");
        Monitor.Exit(_gate);

        Console.WriteLine("Monitor test passed");
        return 0;
    }
}
