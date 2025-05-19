using System;
using GenEDRBypass.Tests;

class TestRunner
{
    static void Main(string[] args)
    {
        TestMemoryManager.RunTests();
        TestEvasionTechniques.RunTests();
        TestShellInjector.RunTests();
        
        Console.WriteLine("Все тесты выполнены.");
    }
}
