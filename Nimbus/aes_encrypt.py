import os
import sys
import string
import random
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import padding


def generate_random_bytes(length: int) -> bytes:
    return os.urandom(length)

def encrypt_file(file_path: str):
    # Read the file's raw bytes
    file = open(file_path, 'rb')
    raw_bytes = file.read()

    # Generate a random PSK (AES-256 requires a 32-byte key) (using UTF-8 characters for better readability)
    psk = ''.join(random.choices(string.ascii_letters, k=32))

    # Generate a random IV (AES block size is 16 bytes)
    iv = generate_random_bytes(16)

    # Create AES cipher in CBC mode with the random key and IV
    backend = default_backend()
    cipher = Cipher(algorithms.AES(bytearray(psk, encoding='utf-8')), modes.CBC(iv), backend=backend)

    # Initialize the encryptor
    encryptor = cipher.encryptor()

    # Add padding (PKCS7) to the plaintext so its length is a multiple of the block size (16 bytes for AES)
    padder = padding.PKCS7(algorithms.AES.block_size).padder()
    padded_data = padder.update(raw_bytes) + padder.finalize()

    # Encrypt the padded data
    encrypted_bytes = encryptor.update(padded_data) + encryptor.finalize()

    result = encrypted_bytes

    encrypted_hex = ', '.join(f'0x{b:02x}' for b in result)
    print("var enc_bytes: seq[byte] = @[" + encrypted_hex + "]\n")

    print("var psk: string = \"" + str(psk) + "\"\n")

    iv_hex = ', '.join(f'0x{b:02x}' for b in iv)
    print("var iv: array[aes.sizeBlock, byte] = [" + iv_hex + "]")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python3 aes_encrypt_file.py <shellcode.bin>")
        sys.exit(1)

    file_path = sys.argv[1]
    encrypt_file(file_path)

