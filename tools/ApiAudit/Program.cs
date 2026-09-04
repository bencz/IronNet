using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

namespace IronNet.ApiAudit;

internal static class Program
{
    private sealed record TypeGap(string Name, string Namespace, bool TypePresent, int ReferenceMembers, int MatchingMembers, string[] MissingMembers);

    private static int Main(string[] args)
    {
        if (args.Length != 3)
        {
            Console.Error.WriteLine("Usage: ApiAudit <reference-assembly> <implementation-assembly> <report-directory>");
            return 2;
        }

        try
        {
            WriteReport(args[0], args[1], args[2]);
            return 0;
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine(exception.Message);
            return 1;
        }
    }

    private static void WriteReport(string referencePath, string implementationPath, string directory)
    {
        Dictionary<string, ApiType> reference = ApiSurface.Read(referencePath);
        Dictionary<string, ApiType> implementation = ApiSurface.Read(implementationPath);
        List<TypeGap> gaps = new List<TypeGap>();

        foreach (ApiType type in reference.Values.OrderBy(type => type.Name, StringComparer.Ordinal))
        {
            bool present = implementation.TryGetValue(type.Name, out ApiType? actual);
            string[] missing = type.Members.Where(member => actual == null || !actual.Members.Contains(member)).Order(StringComparer.Ordinal).ToArray();
            gaps.Add(new TypeGap(type.Name, type.Namespace, present, type.Members.Count, type.Members.Count - missing.Length, missing));
        }

        int presentTypes = gaps.Count(gap => gap.TypePresent);
        int referenceMembers = gaps.Sum(gap => gap.ReferenceMembers);
        int matchingMembers = gaps.Sum(gap => gap.MatchingMembers);
        string scope = "Declared public/protected types and member signatures, including property/event accessors. Presence is not behavioral or binary conformance.";
        var report = new
        {
            Scope = scope,
            Reference = Describe(referencePath),
            Implementation = Describe(implementationPath),
            ReferenceTypes = reference.Count,
            PresentTypes = presentTypes,
            MissingTypes = reference.Count - presentTypes,
            ReferenceMembers = referenceMembers,
            MatchingMembers = matchingMembers,
            MissingMembers = referenceMembers - matchingMembers,
            Types = gaps
        };

        Directory.CreateDirectory(directory);
        File.WriteAllText(Path.Combine(directory, "api-gaps.json"), JsonSerializer.Serialize(report, new JsonSerializerOptions { WriteIndented = true }));
        StringBuilder markdown = new StringBuilder();
        markdown.AppendLine("# Declared API inventory");
        markdown.AppendLine();
        markdown.AppendLine(scope);
        markdown.AppendLine();
        markdown.AppendLine($"Types present: {presentTypes}/{reference.Count}. Member signatures present: {matchingMembers}/{referenceMembers}.");
        markdown.AppendLine();
        markdown.AppendLine("| Namespace | Types present | Types required | Signatures present | Signatures required |");
        markdown.AppendLine("| --- | ---: | ---: | ---: | ---: |");
        foreach (IGrouping<string, TypeGap> group in gaps.GroupBy(gap => gap.Namespace).OrderBy(group => group.Key, StringComparer.Ordinal))
        {
            markdown.AppendLine($"| {group.Key} | {group.Count(gap => gap.TypePresent)} | {group.Count()} | {group.Sum(gap => gap.MatchingMembers)} | {group.Sum(gap => gap.ReferenceMembers)} |");
        }

        File.WriteAllText(Path.Combine(directory, "summary.md"), markdown.ToString());
        Console.WriteLine($"Types present: {presentTypes}/{reference.Count}; declared signatures present: {matchingMembers}/{referenceMembers}.");
        Console.WriteLine("Reports: " + Path.GetFullPath(directory));
    }

    private static object Describe(string path)
    {
        using FileStream stream = File.OpenRead(path);
        return new { File = Path.GetFullPath(path), Sha256 = Convert.ToHexString(SHA256.HashData(stream)) };
    }
}
