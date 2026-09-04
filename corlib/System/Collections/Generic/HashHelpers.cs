namespace System.Collections.Generic
{
    internal static class HashHelpers
    {
        private static readonly int[] Primes = {
            3, 7, 11, 17, 23, 29, 37, 47, 59, 71, 89, 107, 131, 163, 197, 239, 293, 353, 431, 521, 631, 761, 919, 1103, 1327, 1597, 1931, 2333,
            2801, 3371, 4049, 4861, 5839, 7013, 8419, 10103, 12143, 14591, 17519, 21023, 25229, 30293, 36353, 43627, 52361, 62851, 75431, 90523,
            108631, 130363, 156437, 187751, 225307, 270371, 324449, 389357, 467237, 560689, 672827, 807403, 968897, 1162687, 1395263, 1674319,
            2009191, 2411033, 2893249, 3471899, 4166287, 4999559, 5999471, 7199369
        };

        internal static int GetPrime(int minimum)
        {
            if (minimum < 0)
            {
                throw new ArgumentOutOfRangeException("minimum");
            }

            for (int i = 0; i < Primes.Length; i++)
            {
                if (Primes[i] >= minimum)
                {
                    return Primes[i];
                }
            }

            int candidate = minimum | 1;
            while (candidate > 0)
            {
                if (IsPrime(candidate))
                {
                    return candidate;
                }

                candidate += 2;
            }

            return minimum;
        }

        internal static int ExpandPrime(int oldSize)
        {
            if (oldSize > 1073741823)
            {
                return Int32.MaxValue;
            }

            return GetPrime(oldSize * 2);
        }

        private static bool IsPrime(int candidate)
        {
            if ((candidate & 1) == 0)
            {
                return candidate == 2;
            }

            int divisor = 3;
            while ((long)divisor * divisor <= candidate)
            {
                if (candidate % divisor == 0)
                {
                    return false;
                }

                divisor += 2;
            }

            return candidate != 1;
        }
    }
}
