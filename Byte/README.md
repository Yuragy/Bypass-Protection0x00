# ByteBomber

ByteBomber is a tool for createing a ZIP bombs. A ZIP bomb is a highly compressed ZIP file that massively expands in size when extracted. ByteBomber is designed to demonstrate how compression algorithms (specifically ZIP's DEFLATE) can be used to exhaust system resources (disk space, RAM, or CPU), potentially crashing systems or causing instability.

## What ByteBomber Does

1. Takes input for how big the uncompressed bomb should be.
2. Takes input for how large each individual payload file should be.
3. Generates a file filled with null bytes (`\x00`) of that size.
4. Creates a ZIP archive containing that file duplicated many times.
5. Applies DEFLATE compression to exploit redundancy.

Since every payload file is identical and filled with zeroes, compression is extremely effective—producing a small ZIP file that expands drastically when extracted.

## CLI

When you run the script, you'll be prompted for the following:

`Bomb decompressed size:`

- This is the total uncompressed size you want the final ZIP bomb to expand to.
- Default is 500 GB.

`Payload file size:`

- Size of the individual file inside the ZIP archive.
- The smaller this is, the more files the ZIP bomb will contain.
- Default is 1 MB.

`Output zip name:`

- Name of the final ZIP file to be created.
- Default is `bomb.zip`.

> [!NOTE]
> Use the format `<number> <unit>` when entering values for decompressed size and payload size (e.g., `500 GB`, `1 TB`).\
> Supported units: B, KB, MB, GB, TB, PB

Once input is provided, a summary of the configuration is shown:

```
Creating ZIP bomb:

    Payload size:         1048576 bytes
    Total uncompressed:   536870912000 bytes
    File count:           512000
    Output:               bomb.zip
```

- Payload size: Size of the file being copied inside the ZIP.
- Total uncompressed: Target final size when the ZIP is extracted.
- File count: How many copies of the payload file are added.
- Output: Filename of the ZIP bomb.

It will then show live progress as files are added to the ZIP.

## What's in the ZIP

Inside the ZIP there are tens of thousands to millions of identical files like:

- 0.txt
- 1.txt
- 2.txt
- ...

All filled with null bytes. The compression algorithm detects repetition and compresses it heavily.

> [!WARNING]
> This tool is for educational purposes only. Do not deploy ZIP bombs on systems you do not own or have permission to test. Misuse can result in data loss or system damage.
