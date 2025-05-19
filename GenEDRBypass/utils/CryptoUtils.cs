using System;
using System.Security.Cryptography;

namespace GenEDRBypass.Utils
{
    public static class CryptoUtils
    {
        public static byte[] DecryptShellcode(byte[] encryptedData, byte[] key, byte[] iv)
        {
            using (Aes aesAlg = Aes.Create())
            {
                aesAlg.Key = key;
                aesAlg.IV = iv;
                
                ICryptoTransform decryptor = aesAlg.CreateDecryptor(aesAlg.Key, aesAlg.IV);

                return PerformCryptography(encryptedData, decryptor);
            }
        }

        private static byte[] PerformCryptography(byte[] data, ICryptoTransform cryptoTransform)
        {
            byte[] result = cryptoTransform.TransformFinalBlock(data, 0, data.Length);
            return result;
        }
    }
}
