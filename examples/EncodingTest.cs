using System;
using System.Text;

class EncodingTest
{
    static void Main()
    {
        string source = "Aé€😀";
        byte[] encoded = Encoding.UTF8.GetBytes(source);
        string decoded = Encoding.UTF8.GetString(encoded);

        Console.WriteLine(encoded.Length);
        Console.WriteLine(Encoding.UTF8.GetCharCount(encoded));
        Console.WriteLine(decoded == source);

        byte[] malformed = new byte[] { 0xE2, 0x28, 0xA1 };
        Console.WriteLine(Encoding.UTF8.GetCharCount(malformed));
        Console.WriteLine(Encoding.UTF8.GetString(malformed));
    }
}
