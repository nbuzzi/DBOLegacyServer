using System;

namespace RdfTableEditor.Model;

// Minimal DES implementation compatible with project CDes (ECB, no padding), allowing weak keys.
internal sealed class DesCompat
{
    // Tables ported from CDes.cpp (shortened visibility for brevity)
    static readonly byte[] IP = new byte[]
    {58,50,42,34,26,18,10,2,60,52,44,36,28,20,12,4,62,54,46,38,30,22,14,6,64,56,48,40,32,24,16,8,57,49,41,33,25,17,9,1,59,51,43,35,27,19,11,3,61,53,45,37,29,21,13,5,63,55,47,39,31,23,15,7};
    static readonly byte[] FP = new byte[]
    {40,8,48,16,56,24,64,32,39,7,47,15,55,23,63,31,38,6,46,14,54,22,62,30,37,5,45,13,53,21,61,29,36,4,44,12,52,20,60,28,35,3,43,11,51,19,59,27,34,2,42,10,50,18,58,26,33,1,41,9,49,17,57,25};
    static readonly byte[] PCT = new byte[]
    {57,49,41,33,25,17,9,1,58,50,42,34,26,18,10,2,59,51,43,35,27,19,11,3,60,52,44,36,63,55,47,39,31,23,15,7,62,54,46,38,30,22,14,6,61,53,45,37,29,21,13,5,28,20,12,4};
    static readonly byte[] TOTROT = new byte[]{1,2,4,6,8,10,12,14,15,17,19,21,23,25,27,28};
    static readonly byte[] PCK = new byte[]
    {14,17,11,24,1,5,3,28,15,6,21,10,23,19,12,4,26,8,16,7,27,20,13,2,41,52,31,37,47,55,30,40,51,45,33,48,44,49,39,56,34,53,46,42,50,36,29,32};
    static readonly byte[,] SI = new byte[8,64]
    {
        {14,4,13,1,2,15,11,8,3,10,6,12,5,9,0,7,0,15,7,4,14,2,13,1,10,6,12,11,9,5,3,8,4,1,14,8,13,6,2,11,15,12,9,7,3,10,5,0,15,12,8,2,4,9,1,7,5,11,3,14,10,0,6,13},
        {15,1,8,14,6,11,3,4,9,7,2,13,12,0,5,10,3,13,4,7,15,2,8,14,12,0,1,10,6,9,11,5,0,14,7,11,10,4,13,1,5,8,12,6,9,3,2,15,13,8,10,1,3,15,4,2,11,6,7,12,0,5,14,9},
        {10,0,9,14,6,3,15,5,1,13,12,7,11,4,2,8,13,7,0,9,3,4,6,10,2,8,5,14,12,11,15,1,13,6,4,9,8,15,3,0,11,1,2,12,5,10,14,7,1,10,13,0,6,9,8,7,4,15,14,3,11,5,2,12},
        {7,13,14,3,0,6,9,10,1,2,8,5,11,12,4,15,13,8,11,5,6,15,0,3,4,7,2,12,1,10,14,9,10,6,9,0,12,11,7,13,15,1,3,14,5,2,8,4,3,15,0,6,10,1,13,8,9,4,5,11,12,7,2,14},
        {2,12,4,1,7,10,11,6,8,5,3,15,13,0,14,9,14,11,2,12,4,7,13,1,5,0,15,10,3,9,8,6,4,2,1,11,10,13,7,8,15,9,12,5,6,3,0,14,11,8,12,7,1,14,2,13,6,15,0,9,10,4,5,3},
        {12,1,10,15,9,2,6,8,0,13,3,4,14,7,5,11,10,15,4,2,7,12,9,5,6,1,13,14,0,11,3,8,9,14,15,5,2,8,12,3,7,0,4,10,1,13,11,6,4,3,2,12,9,5,15,10,11,14,1,7,6,0,8,13},
        {4,11,2,14,15,0,8,13,3,12,9,7,5,10,6,1,13,0,11,7,4,9,1,10,14,3,5,12,2,15,8,6,1,4,11,13,12,3,7,14,10,15,6,8,0,5,9,2,6,11,13,8,1,4,10,7,9,5,0,15,14,2,3,12},
        {13,2,8,4,6,15,11,1,10,9,3,14,5,0,12,7,1,15,13,8,10,3,7,4,12,5,6,11,0,14,9,2,7,11,4,1,9,12,14,2,0,6,10,13,15,3,5,8,2,1,14,7,4,10,8,13,15,12,9,0,3,5,6,11}
    };
    static readonly byte[] P32 = new byte[]{16,7,20,21,29,12,28,17,1,15,23,26,5,18,31,10,2,8,24,14,32,27,3,9,19,13,30,6,22,11,4,25};

    byte[,,] miperm = new byte[16,16,8];
    byte[,,] mfperm = new byte[16,16,8];
    byte[,,] mp32 = new byte[4,256,4];
    byte[] mpc1m = new byte[56];
    byte[] mpcr = new byte[56];
    byte[,] mkn = new byte[16,6];

    void PermInit(byte[,,] perm, byte[] p)
    {
        Array.Clear(perm, 0, perm.Length);
        for (int i=0;i<16;i++)
            for (int j=0;j<16;j++)
                for (int k=0;k<64;k++)
                {
                    int l = p[k]-1;
                    if ((l>>2)!=i) continue;
                    int[] NIBBLEBIT = {8,4,2,1};
                    if ((j & NIBBLEBIT[l & 3])==0) continue;
                    int m = k & 7;
                    perm[i,j,k>>3] |= (byte)(1 << (7-m));
                }
    }
    void P32Init()
    {
        Array.Clear(mp32, 0, mp32.Length);
        for (int i=0;i<4;i++)
            for (int j=0;j<256;j++)
                for (int k=0;k<32;k++)
                {
                    int l = P32[k]-1;
                    if ((l>>3)!=i) continue;
                    if ((j & (1 << (7-(l & 7))))==0) continue;
                    int m = k & 7;
                    mp32[i,j,k>>3] |= (byte)(1 << (7-m));
                }
    }
    void SInit() { /* precomputed in SI */ }

    void KInit(byte[] key64)
    {
        for (int j=0;j<56;j++)
        {
            int l = PCT[j]-1; int m = l & 7;
            mpc1m[j] = (byte)(((key64[l>>3] & (1 << (7-m)))!=0) ? 1:0);
        }
        for (int i=0;i<16;i++) for(int j=0;j<6;j++) mkn[i,j]=0;
        for (int i=0;i<16;i++)
        {
            for (int j=0;j<56;j++)
            {
                int l = j + TOTROT[i];
                mpcr[j] = mpc1m[ (l < (j<28?28:56)) ? l : (l-28) ];
            }
            for (int j=0;j<48;j++) if (mpcr[PCK[j]-1]!=0)
            {
                int l = j & 7;
                mkn[i, j>>3] |= (byte)(1 << (7-l));
            }
        }
    }

    static void Permute(byte[] inblock, byte[,,] perm, byte[] outblock)
    {
        Array.Clear(outblock, 0, outblock.Length);
        for (int j=0;j<16;j+=2)
        {
            byte ib = inblock[j>>1];
            var p = new byte[8]; var q = new byte[8];
            for(int i=0;i<8;i++){ p[i]=perm[j,(ib>>4)&0x0F,i]; q[i]=perm[j+1,ib&0x0F,i]; }
            for(int i=0;i<8;i++) outblock[i] |= (byte)(p[i] | q[i]);
        }
    }
    void Perm32(byte[] in32, byte[] out32)
    {
        out32[0]=out32[1]=out32[2]=out32[3]=0;
        for (int j=0;j<4;j++)
        {
            byte b = in32[j];
            for (int k=0;k<4;k++) out32[k] |= mp32[j,b,k];
        }
    }
    static void Expand(byte[] right, byte[] bigright)
    {
        // Expand 32 to 48 bits per the original code
        byte r0=right[0], r1=right[1], r2=right[2], r3=right[3];
        bigright[0] = (byte)(((r3 & 0x01)<<7) | ((r0 & 0xF8)>>1) | ((r0 & 0x18)>>3));
        bigright[1] = (byte)(((r0 & 0x07)<<5) | ((r1 & 0x80)>>3) | ((r0 & 0x01)<<3) | ((r1 & 0xE0)>>5));
        bigright[2] = (byte)(((r1 & 0x30)<<3) | ((r1 & 0x3F)<<1) | ((r2 & 0x80)>>7));
        bigright[3] = (byte)(((r1 & 0x01)<<7) | ((r2 & 0xF8)>>1) | ((r2 & 0x18)>>3));
        bigright[4] = (byte)(((r2 & 0x07)<<5) | ((r3 & 0x80)>>3) | ((r2 & 0x01)<<3) | ((r3 & 0xE0)>>5));
        bigright[5] = (byte)(((r3 & 0x30)<<3) | ((r3 & 0x3F)<<1) | ((r0 & 0x80)>>7));
    }
    static void Contract(byte[] in48, byte[] out32)
    {
        int i0=in48[0], i1=in48[1], i2=in48[2], i3=in48[3], i4=in48[4], i5=in48[5];
        out32[0] = (byte)((SI[0, ((i0<<4) | ((i1>>4)&0x0F)) & 0xFFF ]));
        out32[1] = (byte)((SI[1, ((i1<<8) | (i2 & 0xFF)) & 0xFFF ]));
        out32[2] = (byte)((SI[2, ((i3<<4) | ((i4>>4)&0x0F)) & 0xFFF ]));
        out32[3] = (byte)((SI[3, ((i4<<8) | (i5 & 0xFF)) & 0xFFF ]));
    }
    void F(byte[] right, int num, byte[] fret)
    {
        byte[] big = new byte[6]; byte[] res = new byte[6]; byte[] pre = new byte[4];
        Expand(right, big);
        for (int i=0;i<6;i++) res[i] = (byte)(big[i] ^ mkn[num,i]);
        Contract(res, pre);
        Perm32(pre, fret);
    }
    void Iter(int num, byte[] inblock, byte[] outblock)
    {
        byte[] fret = new byte[4];
        byte[] ibR = new byte[]{ inblock[4],inblock[5],inblock[6],inblock[7] };
        F(ibR, num, fret);
        outblock[0]=ibR[0]; outblock[1]=ibR[1]; outblock[2]=ibR[2]; outblock[3]=ibR[3];
        outblock[4]=(byte)(inblock[0]^fret[0]); outblock[5]=(byte)(inblock[1]^fret[1]); outblock[6]=(byte)(inblock[2]^fret[2]); outblock[7]=(byte)(inblock[3]^fret[3]);
    }
    void Endes(byte[] inblock, byte[] outblock)
    {
        byte[] t = new byte[8]; byte[] swap = new byte[8];
        Permute(inblock, miperm, t);
        for (int i=0;i<16;i++) { byte[] next=new byte[8]; Iter(i, t, next); t=next; }
        swap[0]=t[4];swap[1]=t[5];swap[2]=t[6];swap[3]=t[7];swap[4]=t[0];swap[5]=t[1];swap[6]=t[2];swap[7]=t[3];
        Permute(swap, mfperm, outblock);
    }
    void Dedes(byte[] inblock, byte[] outblock)
    {
        byte[] t = new byte[8]; byte[] swap = new byte[8];
        Permute(inblock, miperm, t);
        for (int i=0;i<16;i++) { byte[] next=new byte[8]; Iter(15-i, t, next); t=next; }
        swap[0]=t[4];swap[1]=t[5];swap[2]=t[6];swap[3]=t[7];swap[4]=t[0];swap[5]=t[1];swap[6]=t[2];swap[7]=t[3];
        Permute(swap, mfperm, outblock);
    }

    public void SetKeyFromAscii(string key)
    {
        // Fold ASCII key to 8 bytes via XOR (up to 40 chars), like CDes::DesKeyInit
        var src = System.Text.Encoding.ASCII.GetBytes(key);
        var key64 = new byte[8];
        int lim = Math.Min(src.Length, 40);
        for (int i=0;i<lim;i++) key64[i%8] ^= src[i];
        // Initialize tables and schedule
        PermInit(miperm, IP); PermInit(mfperm, FP); KInit(key64); SInit(); P32Init();
    }

    public void DecryptEcbInPlace(byte[] buffer)
    {
        int mLen = buffer.Length & ~7; // floor to multiple of 8
        var outBlock = new byte[8];
        for (int i=0; i<mLen; i+=8)
        {
            Span<byte> blk = buffer.AsSpan(i,8);
            byte[] inb = blk.ToArray();
            Dedes(inb, outBlock);
            for (int j=0;j<8;j++) buffer[i+j]=outBlock[j];
        }
    }

    public void EncryptEcbInPlace(byte[] buffer)
    {
        int mLen = buffer.Length & ~7; // floor to multiple of 8
        var outBlock = new byte[8];
        for (int i=0; i<mLen; i+=8)
        {
            Span<byte> blk = buffer.AsSpan(i,8);
            byte[] inb = blk.ToArray();
            Endes(inb, outBlock);
            for (int j=0;j<8;j++) buffer[i+j]=outBlock[j];
        }
    }
}
