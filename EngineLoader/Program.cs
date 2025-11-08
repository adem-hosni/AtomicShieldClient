using System;
using System.IO;
using EngineLoader.Loader;

namespace EngineLoader
{
    class Program
    {
        static void Main(string[] args)
        {
            var bytes = File.ReadAllBytes("C:\\AtomicShield\\AtomicShieldClient\\Build\\Atomic Engine.dll");
            using var mapper = new InProcessManualMapper();
            var basePtr = mapper.Map(bytes, true);

            if (basePtr != IntPtr.Zero)
            {
                Console.WriteLine($"[+] Successfully mapped Atomic Engine.dll at address: 0x{basePtr.ToString("X")}");
            }
            else
            {
                Console.WriteLine("[-] Failed to map Atomic Engine.dll");
            }

            Console.ReadKey();
        }
    }
}