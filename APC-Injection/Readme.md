# APC Queue Injection Rust

Check my blog post out about [this technique here](https://fluxsec.red/apc-queue-injection-rust)! You can find the [proof of concept video on my YouTube](https://www.youtube.com/watch?v=H68IAfeWxaM).

This project performs APC Queue Injection in Rust, an EDR Evasion and EDR Bypass technique! 

We enumerate through all of the threads active in the target remote process, then queue an APC
callback routine which is a pointer to the shellcode (remote virtual address).

APC's (Asynchronous Procedure Calls) allow a thread to execute a specific function asynchronously at some point in the future. A function can be queued to the APC queue for a specific thread for it to be executed, when that thread becomes 'alertable'.

APC's are a strange thing, in that I have never seen them used in a project I have worked on, or developed myself. However, to provide some examples of why developers may wish to use APC's:

 - Networking: In network applications, APCs allow for non-blocking socket operations. This means a thread can continue processing other tasks while waiting for data to be sent or received, improving the application's responsiveness.
 - File System: When performing file read/write operations, APCs can be used to queue completion routines, allowing the thread to handle other tasks while waiting for I/O operations to complete.
 - Signaling: APCs facilitate communication between threads by allowing one thread to queue a function for execution by another thread. This is useful for signaling and coordinating complex multi-threaded operations.
