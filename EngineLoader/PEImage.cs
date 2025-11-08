using System;
using System.Runtime.InteropServices;

namespace EngineLoader.PE
{
    [StructLayout(LayoutKind.Sequential)]
    public struct IMAGE_DOS_HEADER
    {
        public ushort e_magic;
        public ushort e_cblp;
        public ushort e_cp;
        public ushort e_crlc;
        public ushort e_cparhdr;
        public ushort e_minalloc;
        public ushort e_maxalloc;
        public ushort e_ss;
        public ushort e_sp;
        public ushort e_csum;
        public ushort e_ip;
        public ushort e_cs;
        public ushort e_lfarlc;
        public ushort e_ovno;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
        public ushort[] e_res1;
        public ushort e_oemid;
        public ushort e_oeminfo;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 10)]
        public ushort[] e_res2;
        public int e_lfanew;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct IMAGE_FILE_HEADER
    {
        public ushort Machine;
        public ushort NumberOfSections;
        public uint TimeDateStamp;
        public uint PointerToSymbolTable;
        public uint NumberOfSymbols;
        public ushort SizeOfOptionalHeader;
        public ushort Characteristics;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct IMAGE_DATA_DIRECTORY
    {
        public uint VirtualAddress;
        public uint Size;
    }

    public static class PEConstants
    {
        public const ushort IMAGE_DOS_SIGNATURE = 0x5A4D; // MZ
        public const uint IMAGE_NT_SIGNATURE = 0x00004550; // PE\0\0
        public const int IMAGE_NUMBEROF_DIRECTORY_ENTRIES = 16;
        public const int IMAGE_DIRECTORY_ENTRY_EXPORT = 0;
        public const int IMAGE_DIRECTORY_ENTRY_IMPORT = 1;
        public const int IMAGE_DIRECTORY_ENTRY_RESOURCE = 2;
        public const int IMAGE_DIRECTORY_ENTRY_EXCEPTION = 3;
        public const int IMAGE_DIRECTORY_ENTRY_SECURITY = 4;
        public const int IMAGE_DIRECTORY_ENTRY_BASERELOC = 5;
        public const int IMAGE_DIRECTORY_ENTRY_DEBUG = 6;
        public const int IMAGE_DIRECTORY_ENTRY_ARCHITECTURE = 7;
        public const int IMAGE_DIRECTORY_ENTRY_GLOBALPTR = 8;
        public const int IMAGE_DIRECTORY_ENTRY_TLS = 9;
        public const int IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG = 10;
        public const int IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT = 11;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct IMAGE_OPTIONAL_HEADER32
    {
        public ushort Magic;
        public byte MajorLinkerVersion;
        public byte MinorLinkerVersion;
        public uint SizeOfCode;
        public uint SizeOfInitializedData;
        public uint SizeOfUninitializedData;
        public uint AddressOfEntryPoint;
        public uint BaseOfCode;
        public uint BaseOfData;
        public uint ImageBase;
        public uint SectionAlignment;
        public uint FileAlignment;
        public ushort MajorOperatingSystemVersion;
        public ushort MinorOperatingSystemVersion;
        public ushort MajorImageVersion;
        public ushort MinorImageVersion;
        public ushort MajorSubsystemVersion;
        public ushort MinorSubsystemVersion;
        public uint Win32VersionValue;
        public uint SizeOfImage;
        public uint SizeOfHeaders;
        public uint CheckSum;
        public ushort Subsystem;
        public ushort DllCharacteristics;
        public uint SizeOfStackReserve;
        public uint SizeOfStackCommit;
        public uint SizeOfHeapReserve;
        public uint SizeOfHeapCommit;
        public uint LoaderFlags;
        public uint NumberOfRvaAndSizes;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = PEConstants.IMAGE_NUMBEROF_DIRECTORY_ENTRIES)]
        public IMAGE_DATA_DIRECTORY[] DataDirectory;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct IMAGE_OPTIONAL_HEADER64
    {
        public ushort Magic;
        public byte MajorLinkerVersion;
        public byte MinorLinkerVersion;
        public uint SizeOfCode;
        public uint SizeOfInitializedData;
        public uint SizeOfUninitializedData;
        public uint AddressOfEntryPoint;
        public uint BaseOfCode;
        public ulong ImageBase;
        public uint SectionAlignment;
        public uint FileAlignment;
        public ushort MajorOperatingSystemVersion;
        public ushort MinorOperatingSystemVersion;
        public ushort MajorImageVersion;
        public ushort MinorImageVersion;
        public ushort MajorSubsystemVersion;
        public ushort MinorSubsystemVersion;
        public uint Win32VersionValue;
        public uint SizeOfImage;
        public uint SizeOfHeaders;
        public uint CheckSum;
        public ushort Subsystem;
        public ushort DllCharacteristics;
        public ulong SizeOfStackReserve;
        public ulong SizeOfStackCommit;
        public ulong SizeOfHeapReserve;
        public ulong SizeOfHeapCommit;
        public uint LoaderFlags;
        public uint NumberOfRvaAndSizes;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = PEConstants.IMAGE_NUMBEROF_DIRECTORY_ENTRIES)]
        public IMAGE_DATA_DIRECTORY[] DataDirectory;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct IMAGE_NT_HEADERS32
    {
        public uint Signature;
        public IMAGE_FILE_HEADER FileHeader;
        public IMAGE_OPTIONAL_HEADER32 OptionalHeader;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct IMAGE_NT_HEADERS64
    {
        public uint Signature;
        public IMAGE_FILE_HEADER FileHeader;
        public IMAGE_OPTIONAL_HEADER64 OptionalHeader;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct IMAGE_SECTION_HEADER
    {
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 8)]
        public string Name;
        public uint VirtualSize;
        public uint VirtualAddress;
        public uint SizeOfRawData;
        public uint PointerToRawData;
        public uint PointerToRelocations;
        public uint PointerToLinenumbers;
        public ushort NumberOfRelocations;
        public ushort NumberOfLinenumbers;
        public uint Characteristics;
    }

    public static class PEHelpers
    {
        public static T FromBytes<T>(byte[] bytes, int offset) where T : struct
        {
            int size = Marshal.SizeOf<T>();
            if (offset + size > bytes.Length) throw new ArgumentException("Buffer too small to read structure.");
            IntPtr ptr = Marshal.AllocHGlobal(size);
            try
            {
                Marshal.Copy(bytes, offset, ptr, size);
                return Marshal.PtrToStructure<T>(ptr)!;
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
            }
        }

        public static byte[] GetBytes(byte[] src, uint offset, uint length)
        {
            if (offset + length > src.Length) throw new ArgumentOutOfRangeException();
            byte[] dst = new byte[length];
            Buffer.BlockCopy(src, (int)offset, dst, 0, (int)length);
            return dst;
        }
    }

    public class PEImage
    {
        public IMAGE_DOS_HEADER DosHeader { get; }
        public bool Is32Bit { get; }
        public IMAGE_NT_HEADERS32 Nt32 { get; }
        public IMAGE_NT_HEADERS64 Nt64 { get; }
        public IMAGE_SECTION_HEADER[] Sections { get; }

        public ulong ImageBase => Is32Bit ? Nt32.OptionalHeader.ImageBase : Nt64.OptionalHeader.ImageBase;
        public uint SizeOfImage => Is32Bit ? Nt32.OptionalHeader.SizeOfImage : Nt64.OptionalHeader.SizeOfImage;
        public uint SizeOfHeaders => Is32Bit ? Nt32.OptionalHeader.SizeOfHeaders : Nt64.OptionalHeader.SizeOfHeaders;
        public uint AddressOfEntryPoint => Is32Bit ? Nt32.OptionalHeader.AddressOfEntryPoint : Nt64.OptionalHeader.AddressOfEntryPoint;
        public IMAGE_DATA_DIRECTORY[] DataDirectories => Is32Bit ? Nt32.OptionalHeader.DataDirectory : Nt64.OptionalHeader.DataDirectory;

        public PEImage(byte[] raw)
        {
            if (raw == null) throw new ArgumentNullException(nameof(raw));
            if (raw.Length < 0x100) throw new ArgumentException("PE buffer too small.");

            DosHeader = PEHelpers.FromBytes<IMAGE_DOS_HEADER>(raw, 0);
            if (DosHeader.e_magic != PEConstants.IMAGE_DOS_SIGNATURE) throw new BadImageFormatException("DOS header magic mismatch.");

            uint ntOffset = (uint)DosHeader.e_lfanew;
            uint sig = BitConverter.ToUInt32(raw, (int)ntOffset);
            if (sig != PEConstants.IMAGE_NT_SIGNATURE) throw new BadImageFormatException("NT signature mismatch.");

            ushort magic = BitConverter.ToUInt16(raw, (int)(ntOffset + 24));
            Is32Bit = magic == 0x10B;

            if (Is32Bit)
                Nt32 = PEHelpers.FromBytes<IMAGE_NT_HEADERS32>(raw, (int)ntOffset);
            else
                Nt64 = PEHelpers.FromBytes<IMAGE_NT_HEADERS64>(raw, (int)ntOffset);

            uint sectionOffset = ntOffset + (Is32Bit ? (uint)Marshal.SizeOf<IMAGE_NT_HEADERS32>() : (uint)Marshal.SizeOf<IMAGE_NT_HEADERS64>());
            ushort numSections = Is32Bit ? Nt32.FileHeader.NumberOfSections : Nt64.FileHeader.NumberOfSections;
            Sections = new IMAGE_SECTION_HEADER[numSections];
            for (int i = 0; i < numSections; i++)
            {
                Sections[i] = PEHelpers.FromBytes<IMAGE_SECTION_HEADER>(raw, (int)(sectionOffset + i * Marshal.SizeOf<IMAGE_SECTION_HEADER>()));
            }
        }
    }
}
