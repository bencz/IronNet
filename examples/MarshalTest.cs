using System;
using System.Runtime.InteropServices;

struct MarshalPair
{
    public int Number;
    public long WideNumber;
}

class MarshalTest
{
    static void Require(bool condition, string message)
    {
        if (!condition)
            throw new Exception(message);
    }

    static int Main()
    {
        MarshalPair pair = new MarshalPair();
        pair.Number = 1;
        pair.WideNumber = 2;

        Require(pair.Number == 1 && pair.WideNumber == 2, "Sequential structure field access failed.");
        Require(Marshal.SizeOf<MarshalPair>() == 16, "Marshal.SizeOf returned an invalid sequential structure size.");
        Require(Marshal.SizeOf(typeof(IntPtr)) == IntPtr.Size, "Marshal.SizeOf did not honor the native pointer size.");

        byte[] source = new byte[] { 10, 20, 30, 40, 50 };
        byte[] destination = new byte[] { 1, 2, 3, 4, 5, 6 };
        IntPtr buffer = Marshal.AllocHGlobal(3);

        Marshal.Copy(source, 1, buffer, 3);
        Marshal.Copy(buffer, destination, 2, 3);
        Marshal.FreeHGlobal(buffer);

        Require(destination[0] == 1 && destination[1] == 2, "Marshal.Copy changed bytes outside the destination range.");
        Require(destination[2] == 20 && destination[3] == 30 && destination[4] == 40, "Marshal.Copy did not preserve native byte contents.");
        Require(destination[5] == 6, "Marshal.Copy changed the trailing destination byte.");

        string ansiText = "IronNet á中";
        IntPtr ansi = Marshal.StringToHGlobalAnsi(ansiText);
        string ansiRoundTrip = Marshal.PtrToStringAnsi(ansi);
        Marshal.FreeHGlobal(ansi);
        Require(ansiRoundTrip == ansiText, "ANSI string round-trip failed.");

        string unicodeText = "UTF-16 Ω 😀";
        IntPtr unicode = Marshal.StringToHGlobalUni(unicodeText);
        string unicodeRoundTrip = Marshal.PtrToStringUni(unicode);
        Marshal.FreeHGlobal(unicode);
        Require(unicodeRoundTrip == unicodeText, "UTF-16 string round-trip failed.");

        Require(Marshal.StringToHGlobalAnsi(null) == IntPtr.Zero, "A null ANSI string must produce a null pointer.");
        Require(Marshal.StringToHGlobalUni(null) == IntPtr.Zero, "A null UTF-16 string must produce a null pointer.");
        Require(Marshal.PtrToStringAnsi(IntPtr.Zero) == null, "A null ANSI pointer must produce a null string.");
        Require(Marshal.PtrToStringUni(IntPtr.Zero) == null, "A null UTF-16 pointer must produce a null string.");

        Marshal.FreeHGlobal(IntPtr.Zero);
        Console.WriteLine("Marshal test passed");
        return 0;
    }
}
