import nimcrypto
import base64
import random
import sugar
import os
import strformat
import strutils

func toByteSeq*(str: string): seq[byte] {.inline.} =
  @(str.toOpenArrayByte(0, str.high))

proc bytesToHexSeq*(src: seq[byte]): string =
  var res: string = "["
  for b in src:
    res = res & "0x" & toLowerAscii(toHex(b)) & ", "
  res = res[0..^3] & "]"

  return res

var fileName: string

if paramCount() >= 1:
  fileName = paramStr(0)
else:
  echo "Usage: aes.nim <shellcode.bin>"
  quit(0)

# Create random PSK
let chars = {'a'..'z','A'..'Z'}
randomize()
var envkey = collect(newSeq, (for i in 0..<32: chars.sample)).join

# Read file content
let blob = readFile("calc.bin")

var
    data: seq[byte] = toByteSeq(blob)
    ectx: CTR[aes256]
    key: array[aes256.sizeKey, byte]
    iv: array[aes256.sizeBlock, byte]
    plaintext = newSeq[byte](len(data))
    enctext = newSeq[byte](len(data))

# Create random IV
discard randomBytes(addr iv[0], 16)

copyMem(addr plaintext[0], addr data[0], len(data))

# Expand key to 32 bytes using SHA256 as the KDF
var expandedkey = sha256.digest(envkey)
copyMem(addr key[0], addr expandedkey.data[0], len(expandedkey.data))

# Encrypt bytes
ectx.init(key, iv)
ectx.encrypt(plaintext, enctext)
ectx.clear()

#echo "RAW SHELLCODE"
#echo "var shellcode: seq[byte] = @" & bytesToHexSeq(data) & "\n"

#echo "\nENCRYPTED SHELLCODE"
echo "var enc_bytes: seq[byte] = @" & bytesToHexSeq(enctext) & "\n"

#echo "\nPSK (Pre-Shared Key)"
echo "var psk: string = \"" & envkey & "\"\n"

#echo "\nIV (Initialization Vector)"
echo "var iv: array[aes256.sizeBlock, byte] = " & bytesToHexSeq(@iv)


