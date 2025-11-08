using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using EngineLoader.PE;
using Shared.Utils;

namespace EngineLoader.Loader
{
    internal static class Native
    {
        public const uint MEM_COMMIT = 0x1000;
        public const uint MEM_RESERVE = 0x2000;
        public const uint PAGE_READWRITE = 0x04;
        public const uint PAGE_EXECUTE_READWRITE = 0x40;
        public const uint MEM_RELEASE = 0x8000;

        [DllImport("kernel32", SetLastError = true, ExactSpelling = true)]
        public static extern IntPtr VirtualAlloc(IntPtr lpAddress, UIntPtr dwSize, uint flAllocationType, uint flProtect);

        [DllImport("kernel32", SetLastError = true, ExactSpelling = true)]
        public static extern bool VirtualFree(IntPtr lpAddress, UIntPtr dwSize, uint dwFreeType);

        [DllImport("kernel32", SetLastError = true)]
        public static extern bool VirtualProtect(IntPtr lpAddress, UIntPtr dwSize, uint flNewProtect, out uint lpflOldProtect);

        [DllImport("kernel32", CharSet = CharSet.Ansi, SetLastError = true)]
        public static extern IntPtr LoadLibraryA(string lpFileName);

        [DllImport("kernel32", EntryPoint = "GetProcAddress", CharSet = CharSet.Ansi, SetLastError = true)]
        public static extern IntPtr GetProcAddress(IntPtr hModule, string procName);

        [DllImport("kernel32", EntryPoint = "GetProcAddress", SetLastError = true)]
        public static extern IntPtr GetProcAddressOrdinal(IntPtr hModule, IntPtr procName);

        [DllImport("kernel32.dll")]
        public static extern IntPtr GetModuleHandle(string lpModuleName);

        [DllImport("ntdll.dll", SetLastError = true)]
        public static extern bool RtlAddFunctionTable(IntPtr functionTable, uint entryCount, ulong baseAddress);

        [DllImport("ntdll.dll", SetLastError = true)]
        public static extern bool RtlDeleteFunctionTable(IntPtr functionTable);
    }

    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    internal delegate bool DllEntry(IntPtr hInstance, uint reason, IntPtr reserved);

    [StructLayout(LayoutKind.Sequential)]
    internal struct IMAGE_TLS_DIRECTORY64
    {
        public ulong StartAddressOfRawData;
        public ulong EndAddressOfRawData;
        public ulong AddressOfIndex;
        public ulong AddressOfCallBacks;
        public uint SizeOfZeroFill;
        public uint Characteristics;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct IMAGE_TLS_DIRECTORY32
    {
        public uint StartAddressOfRawData;
        public uint EndAddressOfRawData;
        public uint AddressOfIndex;
        public uint AddressOfCallBacks;
        public uint SizeOfZeroFill;
        public uint Characteristics;
    }


    [StructLayout(LayoutKind.Sequential)]
    internal struct RUNTIME_FUNCTION
    {
        public uint BeginAddress;
        public uint EndAddress;
        public uint UnwindInfoAddress;
    }

    public class InProcessManualMapper : IDisposable
    {
        private IntPtr _allocatedBase = IntPtr.Zero;
        private ulong _sizeOfImage = 0;
        private bool _disposed = false;
        private readonly int _pointerSize = IntPtr.Size;

        public void Dispose()
        {
            if (_disposed) return;
            if (_allocatedBase != IntPtr.Zero)
            {
                Native.VirtualFree(_allocatedBase, UIntPtr.Zero, Native.MEM_RELEASE);
                _allocatedBase = IntPtr.Zero;
            }
            _disposed = true;
        }

        public IntPtr Map(byte[] dllBytes, bool callEntry = true)
        {
            Logger.AddDebugLog("Starting in-process manual map...");

            PEImage pe;
            try { pe = new PEImage(dllBytes); }
            catch (Exception ex)
            {
                Logger.AddDebugLog("Invalid PE image: {0}", ex.Message);
                return IntPtr.Zero;
            }

            if (pe.Is32Bit && _pointerSize != 4)
            { Logger.AddDebugLog("32-bit DLL in 64-bit process not supported."); return IntPtr.Zero; }
            if (!pe.Is32Bit && _pointerSize != 8)
            { Logger.AddDebugLog("64-bit DLL in 32-bit process not supported."); return IntPtr.Zero; }

            _sizeOfImage = pe.SizeOfImage;
            if (_sizeOfImage == 0) { Logger.AddDebugLog("PE SizeOfImage is zero."); return IntPtr.Zero; }

            _allocatedBase = Native.VirtualAlloc(IntPtr.Zero, new UIntPtr(_sizeOfImage), Native.MEM_RESERVE | Native.MEM_COMMIT, Native.PAGE_EXECUTE_READWRITE);
            if (_allocatedBase == IntPtr.Zero)
            { Logger.AddDebugLog("VirtualAlloc failed, err={0}", Marshal.GetLastWin32Error()); return IntPtr.Zero; }

            Logger.AddDebugLog("Allocated image base at 0x{0:X}", _allocatedBase.ToInt64());

            try
            {
                // 1. Copy headers
                Marshal.Copy(PEHelpers.GetBytes(dllBytes, 0, pe.SizeOfHeaders), 0, _allocatedBase, (int)pe.SizeOfHeaders);

                // 2. Copy sections
                foreach (var s in pe.Sections)
                {
                    if (s.SizeOfRawData == 0) continue;
                    IntPtr dest = _allocatedBase + (int)s.VirtualAddress;
                    byte[] secData = PEHelpers.GetBytes(dllBytes, s.PointerToRawData, s.SizeOfRawData);
                    Marshal.Copy(secData, 0, dest, secData.Length);
                }
                Logger.AddDebugLog("Headers and sections copied.");

                // 3. Apply relocations
                long delta = (long)_allocatedBase.ToInt64() - (long)pe.ImageBase;
                if (delta != 0) ApplyRelocations(pe, dllBytes, _allocatedBase, delta);
                else Logger.AddDebugLog("No relocations needed.");

                // 5. Register x64 exception table
                RegisterExceptionTable(pe, dllBytes, _allocatedBase);

                // 6. Resolve imports
                if (!ResolveImports(pe, dllBytes, _allocatedBase))
                { Logger.AddDebugLog("Import resolution failed."); Native.VirtualFree(_allocatedBase, UIntPtr.Zero, Native.MEM_RELEASE); return IntPtr.Zero; }

                Logger.AddDebugLog("Image mapped successfully at 0x{0:X}", _allocatedBase.ToInt64());
                
                // 4. Set section protections
                SetSectionProtections(pe, _allocatedBase);

                // 7. Run TLS callbacks
                RunTlsCallbacks(pe, dllBytes, _allocatedBase);

                // 8. Call DllMain / entry point
                if (callEntry && pe.AddressOfEntryPoint != 0)
                {
                    IntPtr entryAddr = _allocatedBase + (int)pe.AddressOfEntryPoint;
                    Logger.AddDebugLog("Calling entry point at 0x{0:X}", entryAddr.ToInt64());
                    var entry = Marshal.GetDelegateForFunctionPointer<DllEntry>(entryAddr);
                    entry(_allocatedBase, 1, IntPtr.Zero);
                }

                return _allocatedBase;
            }
            catch (Exception ex)
            {
                Logger.AddDebugLog("Mapper failed: {0}", ex.Message);
                if (_allocatedBase != IntPtr.Zero) Native.VirtualFree(_allocatedBase, UIntPtr.Zero, Native.MEM_RELEASE);
                _allocatedBase = IntPtr.Zero;
                return IntPtr.Zero;
            }
        }


        private int RvaToFileOffset(PEImage pe, byte[] raw, uint rva)
        {
            if (rva == 0) return 0;
            foreach (var sec in pe.Sections)
            {
                uint start = sec.VirtualAddress;
                uint size = Math.Max(sec.VirtualSize, sec.SizeOfRawData);
                if (rva >= start && rva < start + size)
                {
                    uint delta = rva - start;
                    uint fileOffset = sec.PointerToRawData + delta;
                    if (fileOffset >= raw.Length) return 0;
                    return (int)fileOffset;
                }
            }

            if (rva < pe.SizeOfHeaders && rva < raw.Length) return (int)rva;
            return 0;
        }

        private void ApplyRelocations(PEImage pe, byte[] raw, IntPtr baseAddr, long delta)
        {
            var dirs = pe.DataDirectories;
            var relocDir = dirs[PEConstants.IMAGE_DIRECTORY_ENTRY_BASERELOC];
            if (relocDir.Size == 0 || relocDir.VirtualAddress == 0) return;

            int relOff = RvaToFileOffset(pe, raw, relocDir.VirtualAddress);
            if (relOff == 0) return;

            int pos = relOff;
            int end = relOff + (int)relocDir.Size;

            while (pos < end)
            {
                if (pos + 8 > raw.Length) break;
                uint pageRVA = BitConverter.ToUInt32(raw, pos);
                uint blockSize = BitConverter.ToUInt32(raw, pos + 4);
                if (blockSize < 8) break;
                int entryCount = (int)((blockSize - 8) / 2);
                int entryPos = pos + 8;

                for (int i = 0; i < entryCount; i++)
                {
                    ushort entry = BitConverter.ToUInt16(raw, entryPos + i * 2);
                    ushort type = (ushort)(entry >> 12);
                    ushort offset = (ushort)(entry & 0x0FFF);

                    const ushort IMAGE_REL_BASED_HIGHLOW = 3;
                    const ushort IMAGE_REL_BASED_DIR64 = 10;

                    if (type == IMAGE_REL_BASED_HIGHLOW && _pointerSize == 4)
                    {
                        IntPtr patchAddr = baseAddr + (int)(pageRVA + offset);
                        int original = Marshal.ReadInt32(patchAddr);
                        int patched = (int)(original + delta);
                        Marshal.WriteInt32(patchAddr, patched);
                    }
                    else if (type == IMAGE_REL_BASED_DIR64 && _pointerSize == 8)
                    {
                        IntPtr patchAddr = baseAddr + (int)(pageRVA + offset);
                        long original = Marshal.ReadInt64(patchAddr);
                        long patched = original + delta;
                        Marshal.WriteInt64(patchAddr, patched);
                    }
                }

                pos += (int)blockSize;
            }
        }

        private void SetSectionProtections(PEImage pe, IntPtr baseAddr)
        {
            foreach (var s in pe.Sections)
            {
                if (s.VirtualAddress == 0) continue;
                IntPtr addr = baseAddr + (int)s.VirtualAddress;
                UIntPtr size = new UIntPtr(Math.Max(1u, s.VirtualSize));

                uint protect = MapSectionCharacteristicsToProtect(s.Characteristics);
                if (!Native.VirtualProtect(addr, size, protect, out uint old))
                {
                    Logger.AddDebugLog("VirtualProtect failed for section {0}, err={1}", s.Name.Trim('\0'), Marshal.GetLastWin32Error());
                }
                else
                {
                    Logger.AddDebugLog("Set protection 0x{0:X} for section {1} (old 0x{2:X})", protect, s.Name.Trim('\0'), old);
                }
            }
        }

        private static uint MapSectionCharacteristicsToProtect(uint characteristics)
        {
            const uint IMAGE_SCN_MEM_EXECUTE = 0x20000000;
            const uint IMAGE_SCN_MEM_READ = 0x40000000;
            const uint IMAGE_SCN_MEM_WRITE = 0x80000000;

            bool exec = (characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
            bool read = (characteristics & IMAGE_SCN_MEM_READ) != 0;
            bool write = (characteristics & IMAGE_SCN_MEM_WRITE) != 0;

            if (exec)
            {
                if (read)
                {
                    if (write) return 0x40; // PAGE_EXECUTE_READWRITE
                    return 0x20; // PAGE_EXECUTE_READ
                }
                return 0x10; // PAGE_EXECUTE
            }
            else
            {
                if (read)
                {
                    if (write) return 0x04; // PAGE_READWRITE
                    return 0x02; // PAGE_READONLY
                }
                return 0x01; // PAGE_NOACCESS
            }
        }

        private void RegisterExceptionTable(PEImage pe, byte[] raw, IntPtr baseAddr)
        {
            var dirs = pe.DataDirectories;
            var exDir = dirs[PEConstants.IMAGE_DIRECTORY_ENTRY_EXCEPTION];
            if (exDir.VirtualAddress == 0 || exDir.Size == 0) return;

            // the exception table is already copied into the mapped image, so compute in-memory pointer
            IntPtr functionTablePtr = baseAddr + (int)exDir.VirtualAddress;
            uint entrySize = (uint)Marshal.SizeOf<RUNTIME_FUNCTION>();
            uint entryCount = exDir.Size / entrySize;
            if (entryCount == 0) return;

            bool ok = Native.RtlAddFunctionTable(functionTablePtr, entryCount, (ulong)baseAddr.ToInt64());
            if (!ok)
                Logger.AddDebugLog("RtlAddFunctionTable failed (err={0})", Marshal.GetLastWin32Error());
            else
                Logger.AddDebugLog("Registered {0} runtime function entries for unwind.", entryCount);
        }

        private bool ResolveImports(PEImage pe, byte[] raw, IntPtr baseAddr)
        {
            var dirs = pe.DataDirectories;
            var importDir = dirs[PEConstants.IMAGE_DIRECTORY_ENTRY_IMPORT];
            if (importDir.Size == 0 || importDir.VirtualAddress == 0)
            {
                Logger.AddDebugLog("No import table present.");
                return true;
            }

            int pointerSize = _pointerSize;
            long mappedBase = baseAddr.ToInt64();
            ulong mappedEnd = (ulong)(mappedBase + (long)_sizeOfImage);

            bool InMappedRange(nint addr, int size)
            {
                if (addr == 0) return false;
                ulong a = (ulong)addr;
                if (a < (ulong)mappedBase) return false;
                if (a + (ulong)size > mappedEnd) return false;
                return true;
            }

            try
            {
                IntPtr descPtr = baseAddr + (int)importDir.VirtualAddress;
                int descSize = Marshal.SizeOf<IMAGE_IMPORT_DESCRIPTOR>();

                while (true)
                {
                    if (!InMappedRange((nint)descPtr, descSize))
                    {
                        Logger.AddDebugLog("Import descriptor pointer out of mapped range.");
                        return false;
                    }

                    var desc = Marshal.PtrToStructure<IMAGE_IMPORT_DESCRIPTOR>(descPtr);
                    if (desc.Name == 0 && desc.FirstThunk == 0 && desc.OriginalFirstThunk == 0) break;

                    IntPtr dllNamePtr = baseAddr + (int)desc.Name;
                    if (!InMappedRange((nint)dllNamePtr, 1))
                    {
                        Logger.AddDebugLog("DLL name pointer out of range (RVA=0x{0:X})", desc.Name);
                        return false;
                    }

                    string dllName = ReadAsciiString(dllNamePtr);
                    if (string.IsNullOrEmpty(dllName))
                    {
                        Logger.AddDebugLog("Empty import DLL name at RVA 0x{0:X}", desc.Name);
                        return false;
                    }
                    Logger.AddDebugLog("Import DLL: {0}", dllName);

                    IntPtr moduleHandle = Native.LoadLibraryA(dllName);
                    if (moduleHandle == IntPtr.Zero)
                    {
                        moduleHandle = Native.GetModuleHandle(dllName);
                        if (moduleHandle == IntPtr.Zero)
                        {
                            Logger.AddDebugLog("Could not load import library: {0}", dllName);
                            return false;
                        }
                    }

                    uint oftRva = desc.OriginalFirstThunk != 0 ? desc.OriginalFirstThunk : desc.FirstThunk;
                    uint iatRva = desc.FirstThunk;
                    int index = 0;

                    for (; ; index++)
                    {
                        IntPtr oftEntryAddr = baseAddr + (nint)(oftRva + (uint)(index * pointerSize));
                        IntPtr iatEntryAddr = baseAddr + (nint)(iatRva + (uint)(index * pointerSize));

                        if (!InMappedRange((nint)oftEntryAddr, pointerSize) || !InMappedRange((nint)iatEntryAddr, pointerSize))
                        {
                            Logger.AddDebugLog("Thunk entry out of mapped range for {0} (index={1}).", dllName, index);
                            return false;
                        }

                        // Read the thunk (pointer-sized) from the mapped image
                        IntPtr thunkPtr = Marshal.ReadIntPtr(oftEntryAddr);
                        if (thunkPtr == IntPtr.Zero) break; // end of list

                        ulong thunkVal = (pointerSize == 8) ? (ulong)thunkPtr.ToInt64() : (uint)thunkPtr.ToInt32();

                        IntPtr resolved = IntPtr.Zero;
                        bool byOrdinal = false;

                        if (pointerSize == 8)
                        {
                            const ulong ORDINAL_FLAG64 = 0x8000000000000000UL;
                            if ((thunkVal & ORDINAL_FLAG64) != 0)
                            {
                                ushort ord = (ushort)(thunkVal & 0xFFFF);
                                resolved = Native.GetProcAddressOrdinal(moduleHandle, (IntPtr)ord);
                                byOrdinal = true;
                            }
                            else
                            {
                                uint hintNameRva = (uint)(thunkVal & 0x7FFFFFFF);
                                IntPtr hintNameAddr = baseAddr + (nint)hintNameRva + 2; // skip hint
                                if (!InMappedRange((nint)hintNameAddr, 1))
                                {
                                    Logger.AddDebugLog("Import name pointer out of range (RVA=0x{0:X})", hintNameRva);
                                    return false;
                                }
                                string importName = ReadAsciiString(hintNameAddr);
                                resolved = Native.GetProcAddress(moduleHandle, importName);
                            }
                        }
                        else // 32-bit
                        {
                            const uint ORDINAL_FLAG32 = 0x80000000U;
                            if ((thunkVal & ORDINAL_FLAG32) != 0)
                            {
                                ushort ord = (ushort)(thunkVal & 0xFFFF);
                                resolved = Native.GetProcAddressOrdinal(moduleHandle, (IntPtr)ord);
                                byOrdinal = true;
                            }
                            else
                            {
                                uint hintNameRva = (uint)thunkVal;
                                IntPtr hintNameAddr = baseAddr + (nint)hintNameRva + 2;
                                if (!InMappedRange((nint)hintNameAddr, 1))
                                {
                                    Logger.AddDebugLog("Import name pointer out of range (RVA=0x{0:X})", hintNameRva);
                                    return false;
                                }
                                string importName = ReadAsciiString(hintNameAddr);
                                resolved = Native.GetProcAddress(moduleHandle, importName);
                            }
                        }

                        if (resolved == IntPtr.Zero)
                        {
                            Logger.AddDebugLog("Failed to resolve import (dll={0}, index={1}, byOrdinal={2})", dllName, index, byOrdinal);
                            return false;
                        }

                        // DEBUG: log the addresses and pointer before writing
                        Logger.AddDebugLog("Writing IAT entry: dll={0} index={1} iatAddr=0x{2:X} resolved=0x{3:X}",
                            dllName, index, iatEntryAddr.ToInt64(), resolved.ToInt64());

                        // Write resolved pointer into IAT (use WriteIntPtr so runtime chooses correct overload)
                        try
                        {
                            Marshal.WriteIntPtr(iatEntryAddr, resolved);
                        }
                        catch (Exception wex)
                        {
                            Logger.AddDebugLog("Exception while writing IAT: {0} -- iatAddr=0x{1:X} resolved=0x{2:X}",
                                wex.Message, iatEntryAddr.ToInt64(), resolved.ToInt64());
                            return false;
                        }
                    }

                    descPtr += descSize; // next descriptor
                }
            }
            catch (Exception ex)
            {
                Logger.AddDebugLog("ResolveImports exception: {0}", ex.Message);
                return false;
            }

            return true;
        }

        // Read null-terminated ASCII string from memory pointer
        private static string ReadAsciiString(IntPtr addr)
        {
            var sb = new StringBuilder();
            int offset = 0;
            while (true)
            {
                byte b = Marshal.ReadByte(addr, offset);
                if (b == 0) break;
                sb.Append((char)b);
                offset++;
            }
            return sb.ToString();
        }

        private int RVAtoFileOffset(PEImage pe, int rva)
        {
            foreach (var s in pe.Sections)
            {
                int secStart = (int)s.VirtualAddress;
                int secEnd = secStart + (int)s.VirtualSize;
                if (rva >= secStart && rva < secEnd)
                {
                    return (int)(rva - secStart + s.PointerToRawData);
                }
            }
            return rva; // fallback
        }

        private void RunTlsCallbacks(PEImage pe, byte[] raw, IntPtr baseAddr)
        {
            var tlsDir = pe.DataDirectories[PEConstants.IMAGE_DIRECTORY_ENTRY_TLS];
            if (tlsDir.VirtualAddress == 0 || tlsDir.Size == 0) return;

            int tlsOffset = RvaToFileOffset(pe, raw, tlsDir.VirtualAddress);
            if (tlsOffset == 0) return;

            // Mapped image range for bounds checking
            long mappedBase = baseAddr.ToInt64();
            ulong mappedEnd = (ulong)(mappedBase + (long)pe.SizeOfImage);

            bool InMappedRange(nint addr, int size)
            {
                if (addr == 0) return false;
                ulong a = (ulong)addr;
                if (a < (ulong)mappedBase) return false;
                if (a + (ulong)size > mappedEnd) return false;
                return true;
            }

            if (pe.Is32Bit)
            {
                var tls = PEHelpers.FromBytes<IMAGE_TLS_DIRECTORY32>(raw, tlsOffset);
                if (tls.AddressOfCallBacks == 0) return;

                uint callbacksRva = tls.AddressOfCallBacks;
                // Usually RVA → VA by adding base; but sometimes value may already be a VA.
                nint callbacksPtr = baseAddr + (int)callbacksRva;
                if (!InMappedRange(callbacksPtr, 1))
                {
                    // try treating AddressOfCallBacks as an absolute VA instead
                    callbacksPtr = (nint)callbacksRva;
                    if (!InMappedRange(callbacksPtr, 1))
                    {
                        Logger.AddDebugLog("TLS callbacks pointer out of mapped range (rva=0x{0:X}). Skipping.", callbacksRva);
                        return;
                    }
                }

                int idx = 0;
                while (true)
                {
                    nint entryAddr = callbacksPtr + idx * 4;
                    if (!InMappedRange(entryAddr, 4)) break; // reached end or invalid

                    int cbPtrVal;
                    try { cbPtrVal = Marshal.ReadInt32((IntPtr)entryAddr); }
                    catch (Exception ex) { Logger.AddDebugLog("ReadInt32 failed at 0x{0:X}: {1}", entryAddr, ex.Message); break; }

                    if (cbPtrVal == 0) break;
                    IntPtr cbAddr = new IntPtr(cbPtrVal);

                    try
                    {
                        var cb = Marshal.GetDelegateForFunctionPointer<DllEntry>(cbAddr);
                        Logger.AddDebugLog("Invoking TLS callback #{0} at 0x{1:X}", idx, cbAddr.ToInt64());
                        cb(baseAddr, 1u, IntPtr.Zero);
                    }
                    catch (Exception ex)
                    {
                        Logger.AddDebugLog("TLS callback #{0} threw: {1}", idx, ex.Message);
                    }

                    idx++;
                }
            }
            else // x64
            {
                var tls = PEHelpers.FromBytes<IMAGE_TLS_DIRECTORY64>(raw, tlsOffset);
                if (tls.AddressOfCallBacks == 0) return;

                ulong callbacksRva = tls.AddressOfCallBacks;
                nint callbacksPtr = baseAddr + (nint)callbacksRva;
                if (!InMappedRange(callbacksPtr, 1))
                {
                    // fallback: maybe AddressOfCallBacks is already VA
                    callbacksPtr = (nint)callbacksRva;
                    if (!InMappedRange(callbacksPtr, 1))
                    {
                        Logger.AddDebugLog("TLS callbacks pointer out of mapped range (rva/va=0x{0:X}). Skipping.", callbacksRva);
                        return;
                    }
                }

                int idx = 0;
                while (true)
                {
                    nint entryAddr = callbacksPtr + idx * 8;
                    if (!InMappedRange(entryAddr, 8)) break;

                    IntPtr cbAddr;
                    try { cbAddr = Marshal.ReadIntPtr((IntPtr)entryAddr); }
                    catch (Exception ex) { Logger.AddDebugLog("ReadIntPtr failed at 0x{0:X}: {1}", entryAddr, ex.Message); break; }

                    if (cbAddr == IntPtr.Zero) break;

                    try
                    {
                        var cb = Marshal.GetDelegateForFunctionPointer<DllEntry>(cbAddr);
                        Logger.AddDebugLog("Invoking TLS callback #{0} at 0x{1:X}", idx, cbAddr.ToInt64());
                        cb(baseAddr, 1u, IntPtr.Zero);
                    }
                    catch (Exception ex)
                    {
                        Logger.AddDebugLog("TLS callback #{0} threw: {1}", idx, ex.Message);
                    }

                    idx++;
                }
            }
        }

        private static string ReadAsciiString(byte[] raw, int offset)
        {
            if (offset <= 0 || offset >= raw.Length) return string.Empty;
            int i = offset;
            var sb = new StringBuilder();
            while (i < raw.Length && raw[i] != 0) { sb.Append((char)raw[i]); i++; }
            return sb.ToString();
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct IMAGE_IMPORT_DESCRIPTOR
        {
            public uint OriginalFirstThunk;
            public uint TimeDateStamp;
            public uint ForwarderChain;
            public uint Name;
            public uint FirstThunk;
        }
    }
}
