using System;
using System.Text;

namespace RdfTableEditor.Model;

public static class RdfCrypto
{
    private const string DefaultKey = "KEY_FOR_GAME_DATA_TABLE";

    // Derives the same 8-byte DES key as CDes::DesKeyInit (XOR fold of up to 40 ASCII bytes)
    private static byte[] DeriveDesKey(string key)
    {
        var src = Encoding.ASCII.GetBytes(key);
        var k = new byte[8];
        int lim = Math.Min(src.Length, 40);
        for (int i = 0; i < lim; i++)
            k[i % 8] ^= src[i];
        return k;
    }

    public static bool TryDecrypt(byte[] input, out byte[] decrypted)
    {
        decrypted = Array.Empty<byte>();
        if (input == null || input.Length < 8)
            return false;
        int mLen = (input.Length / 8) * 8;
        if (mLen < 8)
            return false;

        var output = new byte[input.Length];
        try
        {
            using var des = System.Security.Cryptography.DES.Create();
            des.Mode = System.Security.Cryptography.CipherMode.ECB;
            des.Padding = System.Security.Cryptography.PaddingMode.None;
            des.Key = DeriveDesKey(DefaultKey);
            using var dec = des.CreateDecryptor();
            // Decrypt aligned prefix
            dec.TransformBlock(input, 0, mLen, output, 0);
            dec.TransformFinalBlock(Array.Empty<byte>(), 0, 0);
        }
        catch
        {
            // Fallback to compatibility DES if .NET DES rejects the key or provider errors
            var compat = new DesCompat();
            compat.SetKeyFromAscii(DefaultKey);
            Buffer.BlockCopy(input, 0, output, 0, input.Length);
            compat.DecryptEcbInPlace(output);
        }
        // Copy any trailing bytes unchanged
        if (mLen < input.Length)
            Buffer.BlockCopy(input, mLen, output, mLen, input.Length - mLen);

        decrypted = output;
        return true;
    }

    public static bool TryEncrypt(byte[] input, out byte[] encrypted)
    {
        encrypted = Array.Empty<byte>();
        if (input == null || input.Length < 8)
            return false;
        int mLen = (input.Length / 8) * 8;
        if (mLen < 8)
            return false;

        var output = new byte[input.Length];
        try
        {
            using var des = System.Security.Cryptography.DES.Create();
            des.Mode = System.Security.Cryptography.CipherMode.ECB;
            des.Padding = System.Security.Cryptography.PaddingMode.None;
            des.Key = DeriveDesKey(DefaultKey);
            using var enc = des.CreateEncryptor();
            // Encrypt aligned prefix
            enc.TransformBlock(input, 0, mLen, output, 0);
            enc.TransformFinalBlock(Array.Empty<byte>(), 0, 0);
        }
        catch
        {
            // Fallback to compatibility DES
            var compat = new DesCompat();
            compat.SetKeyFromAscii(DefaultKey);
            Buffer.BlockCopy(input, 0, output, 0, input.Length);
            compat.EncryptEcbInPlace(output);
        }
        // Copy any trailing bytes unchanged
        if (mLen < input.Length)
            Buffer.BlockCopy(input, mLen, output, mLen, input.Length - mLen);

        encrypted = output;
        return true;
    }
}
