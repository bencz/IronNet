using System;
using System.IO;

public static class StreamTest
{
    private static void Assert(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception(message);
        }
    }

    private static void TestReadWriteAndGrowth()
    {
        MemoryStream stream = new MemoryStream(2);
        byte[] source = new byte[] { 10, 20, 30, 40 };

        stream.Write(source, 0, source.Length);
        Assert(stream.Length == 4, "Write did not update Length.");
        Assert(stream.Capacity >= 4, "Write did not grow Capacity.");

        stream.Position = 1;
        byte[] destination = new byte[2];
        int read = stream.Read(destination, 0, destination.Length);
        Assert(read == 2 && destination[0] == 20 && destination[1] == 30, "Read returned incorrect data.");

        stream.Position = 6;
        stream.WriteByte(99);
        byte[] contents = stream.ToArray();
        Assert(contents.Length == 7, "Sparse write produced the wrong length.");
        Assert(contents[4] == 0 && contents[5] == 0 && contents[6] == 99, "Sparse write did not clear the gap.");

        stream.SetLength(9);
        contents = stream.ToArray();
        Assert(contents[7] == 0 && contents[8] == 0, "SetLength did not clear the extended range.");
    }

    private static void TestSeekAndCopy()
    {
        MemoryStream source = new MemoryStream(new byte[] { 1, 2, 3, 4 });
        MemoryStream destination = new MemoryStream();

        Assert(source.Seek(-2, SeekOrigin.End) == 2, "Seek from end returned the wrong position.");
        source.CopyTo(destination, 2);

        byte[] copied = destination.ToArray();
        Assert(copied.Length == 2 && copied[0] == 3 && copied[1] == 4, "CopyTo copied the wrong range.");

        bool threw = false;
        try
        {
            source.Seek(-5, SeekOrigin.Begin);
        }
        catch (IOException)
        {
            threw = true;
        }

        Assert(threw, "Seeking before the beginning did not throw IOException.");
    }

    private static void TestValidationAndAccess()
    {
        MemoryStream stream = new MemoryStream();
        byte[] buffer = new byte[4];
        bool threw = false;

        try
        {
            stream.Read(buffer, 3, 2);
        }
        catch (ArgumentException)
        {
            threw = true;
        }

        Assert(threw, "Read accepted an invalid offset/count range.");

        threw = false;
        try
        {
            stream.Position = (long)int.MaxValue + 1;
        }
        catch (ArgumentOutOfRangeException)
        {
            threw = true;
        }

        Assert(threw, "Position accepted a value larger than Int32.MaxValue.");

        stream.Position = int.MaxValue;
        threw = false;
        try
        {
            stream.WriteByte(1);
        }
        catch (IOException)
        {
            threw = true;
        }

        Assert(threw, "Writing beyond Int32.MaxValue did not throw IOException.");

        MemoryStream wrapped = new MemoryStream(buffer);
        threw = false;
        try
        {
            wrapped.GetBuffer();
        }
        catch (UnauthorizedAccessException)
        {
            threw = true;
        }

        Assert(threw, "GetBuffer exposed a user-provided buffer.");

        MemoryStream readOnly = new MemoryStream(buffer, false);
        threw = false;
        try
        {
            readOnly.WriteByte(1);
        }
        catch (NotSupportedException)
        {
            threw = true;
        }

        Assert(threw, "A read-only MemoryStream accepted a write.");
    }

    private static void TestDisposalAndNullStream()
    {
        MemoryStream stream = new MemoryStream();
        stream.WriteByte(7);
        stream.Dispose();

        Assert(!stream.CanRead && !stream.CanSeek && !stream.CanWrite, "Disposed capability flags are incorrect.");
        Assert(stream.ToArray()[0] == 7, "ToArray must remain available after disposal.");

        bool threw = false;
        try
        {
            long ignored = stream.Length;
            Console.WriteLine(ignored);
        }
        catch (ObjectDisposedException)
        {
            threw = true;
        }

        Assert(threw, "Length did not detect a disposed stream.");

        byte[] buffer = new byte[1];
        Assert(Stream.Null.Read(buffer, 0, 1) == 0, "Stream.Null returned data.");
        Stream.Null.Write(buffer, 0, 1);

        threw = false;
        try
        {
            Stream.Null.Read(null, 0, 0);
        }
        catch (ArgumentNullException)
        {
            threw = true;
        }

        Assert(threw, "Stream.Null did not validate its buffer.");
    }

    public static void Main()
    {
        TestReadWriteAndGrowth();
        TestSeekAndCopy();
        TestValidationAndAccess();
        TestDisposalAndNullStream();
        Console.WriteLine("StreamTest: PASS");
    }
}
