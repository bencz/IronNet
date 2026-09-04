using System;

struct NumericValue
{
    public int Number;
    public double Floating;
}

struct CompositeValue
{
    public NumericValue Numeric;
    public string Text;
    public object Reference;
}

class ValueTypeTest
{
    static void Require(bool condition, string message)
    {
        if (!condition)
            throw new Exception(message);
    }

    static string CreateText()
    {
        return new string(new char[] { 'I', 'r', 'o', 'n' });
    }

    static NumericValue CreateNumeric(int number, double floating)
    {
        NumericValue value = new NumericValue();
        value.Number = number;
        value.Floating = floating;
        return value;
    }

    static CompositeValue CreateComposite(NumericValue numeric, string text, object reference)
    {
        CompositeValue value = new CompositeValue();
        value.Numeric = numeric;
        value.Text = text;
        value.Reference = reference;
        return value;
    }

    static int Main()
    {
        object marker = new object();
        CompositeValue first = CreateComposite(CreateNumeric(17, 25.5), CreateText(), marker);
        CompositeValue second = CreateComposite(CreateNumeric(17, 25.5), CreateText(), marker);

        Require(first.Equals(second), "ValueType.Equals did not compare nested fields structurally.");
        Require(first.GetHashCode() == second.GetHashCode(), "Equal value types produced different hash codes.");

        second.Numeric = CreateNumeric(18, 25.5);
        Require(!first.Equals(second), "ValueType.Equals ignored a changed nested field.");

        NumericValue positiveZero = CreateNumeric(1, 0.0);
        NumericValue negativeZero = CreateNumeric(1, -0.0);
        Require(positiveZero.Equals(negativeZero), "ValueType.Equals must treat positive and negative floating zero as equal.");
        Require(positiveZero.GetHashCode() == negativeZero.GetHashCode(), "Equal floating zero values produced different hash codes.");

        NumericValue firstNaN = CreateNumeric(2, double.NaN);
        NumericValue secondNaN = CreateNumeric(2, 0.0 / 0.0);
        Require(firstNaN.Equals(secondNaN), "ValueType.Equals must treat NaN fields as equal.");
        Require(firstNaN.GetHashCode() == secondNaN.GetHashCode(), "Equal NaN fields produced different hash codes.");

        Console.WriteLine("ValueType test passed");
        return 0;
    }
}
