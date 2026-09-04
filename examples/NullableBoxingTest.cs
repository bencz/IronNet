using System;
using System.Reflection;

public static class NullableBoxingTest
{
    public enum Code : long
    {
        Value = 0x123456789ABC
    }

    public struct Payload
    {
        public long Number;
        public object Reference;
    }

    public sealed class Holder
    {
        public int? Value;

        public int? Echo(int? value)
        {
            Value = value;
            return value;
        }
    }

    private static void Require(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception(message);
        }
    }

    private static object Box<T>(T value)
    {
        return value;
    }

    private static T Unbox<T>(object value)
    {
        return (T)value;
    }

    private static void Check<T>(T value) where T : struct
    {
        object empty = Box<T?>(null);
        Require(empty == null && !Unbox<T?>(empty).HasValue, "Empty nullable boxing/unboxing failed.");
        object boxed = Box<T?>(value);
        Require(boxed != null && boxed.GetType() == typeof(T), "Nullable boxed as its wrapper type.");
        T? restored = Unbox<T?>(boxed);
        Require(restored.HasValue && restored.Value.Equals(value), "Nullable unboxing lost the underlying value.");
        Require(boxed is T? && typeof(T?).IsInstanceOfType(boxed), "Nullable type testing rejected its boxed underlying value.");
    }

    private static void TestArraysAndReflection()
    {
        int?[] values = new int?[] { null, 42 };
        Array array = values;
        Require(array.GetValue(0) == null && (int)array.GetValue(1) == 42, "Array.GetValue did not apply nullable boxing.");
        array.SetValue(7, 0);
        array.SetValue(null, 1);
        Require(values[0].Value == 7 && !values[1].HasValue, "Array.SetValue did not apply nullable unboxing.");

        Holder holder = new Holder();
        FieldInfo field = typeof(Holder).GetField("Value");
        Require(field.GetValue(holder) == null, "Reflection treated an empty nullable as allocation failure.");
        field.SetValue(holder, 12);
        Require(holder.Value.Value == 12 && (int)field.GetValue(holder) == 12, "Nullable field reflection failed.");
        field.SetValue(holder, null);
        Require(!holder.Value.HasValue, "Reflection did not clear a nullable field.");
        MethodInfo method = typeof(Holder).GetMethod("Echo");
        Require((int)method.Invoke(holder, new object[] { 17 }) == 17, "Nullable reflection argument or return failed.");
        Require(method.Invoke(holder, new object[] { null }) == null && !holder.Value.HasValue, "Empty nullable reflection invocation failed.");
    }

    public static int Main()
    {
        Check<int>(123);
        Check<long>(0x123456789ABC);
        Check<double>(123.5);
        Check<Code>(Code.Value);
        object reference = new object();
        Payload payload = new Payload { Number = 9876543210, Reference = reference };
        object boxed = Box<Payload?>(payload);
        GC.Collect();
        Payload? copy = Unbox<Payload?>(boxed);
        Require(copy.HasValue && copy.Value.Number == payload.Number && object.ReferenceEquals(copy.Value.Reference, reference), "Nullable struct boxing lost data or references.");

        bool invalidRejected = false;
        try
        {
            Unbox<int?>((object)123L);
        }
        catch (InvalidCastException)
        {
            invalidRejected = true;
        }

        Require(invalidRejected, "Nullable unboxing accepted the wrong boxed numeric type.");
        TestArraysAndReflection();
        Console.WriteLine("NullableBoxingTest passed");
        return 0;
    }
}
