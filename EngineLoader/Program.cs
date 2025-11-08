using System;
using System.IO;
using EngineLoader.Loader;
using Shared.Utils;

namespace EngineLoader
{
    class Program
    {
        static void Main(string[] args)
        {
            Logger.SetFileName("EngineLoader.log");
            PipeServer.Launch().GetAwaiter().GetResult();
        }
    }
}