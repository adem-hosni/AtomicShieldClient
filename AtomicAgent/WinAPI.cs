using System;
using System.Runtime.InteropServices;

namespace WinAPI
{
	internal static class Memory
	{
		[Flags]
		public enum AllocationType : uint
		{
			Commit = 0x1000,
			Reserve = 0x2000,
			Decommit = 0x4000,
			Release = 0x8000,
			Reset = 0x80000,
			Physical = 0x400000,
			TopDown = 0x100000,
			WriteWatch = 0x200000,
			LargePages = 0x20000000
		}

		[Flags]
		public enum MemoryProtection : uint
		{
			Execute = 0x10,
			ExecuteRead = 0x20,
			ExecuteReadWrite = 0x40,
			ExecuteWriteCopy = 0x80,
			NoAccess = 0x01,
			ReadOnly = 0x02,
			ReadWrite = 0x04,
			WriteCopy = 0x08,
			Guard = 0x100,
			NoCache = 0x200,
			WriteCombine = 0x400
		}
		public enum FreeType : uint
		{
			Decommit = 0x4000,
			Release = 0x8000
		}


		[DllImport("kernel32.dll", SetLastError = true)]
		public static extern IntPtr VirtualAllocEx(
			IntPtr hProcess,
			IntPtr lpAddress,
			uint dwSize,
			AllocationType flAllocationType,
			MemoryProtection flProtect);

		[DllImport("kernel32.dll", SetLastError = true)]
		public static extern bool VirtualProtectEx(
			IntPtr hProcess,
			IntPtr lpAddress,
			uint dwSize,
			MemoryProtection flNewProtect,
			out MemoryProtection lpflOldProtect);

		[DllImport("kernel32.dll", SetLastError = true)]
		public static extern bool WriteProcessMemory(
			IntPtr hProcess,
			IntPtr lpBaseAddress,
			byte[] lpBuffer,
			uint nSize,
			out IntPtr lpNumberOfBytesWritten);


		[DllImport("kernel32.dll")]
		public static extern int GetLastError();


		[DllImport("kernel32.dll", SetLastError = true)]
		[return: MarshalAs(UnmanagedType.Bool)]
		public static extern bool VirtualFreeEx(
	IntPtr hProcess,
	IntPtr lpAddress,
	UIntPtr dwSize,
	FreeType dwFreeType);

	}
}
