#!/usr/bin/env python3
"""Decrypt a Wii WAD's contents properly.
TMD content record: cid(4) index(2) type(2) size(8) sha1(20) = 36 bytes.
Hashed contents (type & 0x8000) have a 0x20-byte SHA-256 header prepended
in the WAD data section. IV = index (u16 BE) + 14 zero bytes.
Verifies every decrypted content's SHA1 against the TMD record."""
import struct, pathlib, subprocess, hashlib

WAD = pathlib.Path('/home/joshua/Desktop/Forecast channel/Forecast Channel (Wii) (USA) (WiiLink).wad')
OUTDIR = pathlib.Path('/home/joshua/Desktop/Forecast channel/extracted/decrypted')
OUTDIR.mkdir(parents=True, exist_ok=True)

COMMON_KEY = bytes.fromhex('ebe42a225e8593e4ead7c4a54c9b8a58')

def aes_cbc_dec(key, iv, blob):
    open('/tmp/wad_c_in.bin','wb').write(blob)
    subprocess.run(['openssl','enc','-d','-aes-128-cbc','-nopad',
                    '-K', key.hex(), '-iv', iv.hex(),
                    '-in','/tmp/wad_c_in.bin','-out','/tmp/wad_c_out.bin'], check=True)
    return open('/tmp/wad_c_out.bin','rb').read()

data = WAD.read_bytes()
hdr_size = struct.unpack('>I', data[0:4])[0]
cert_size = struct.unpack('>I', data[8:12])[0]
tik_size  = struct.unpack('>I', data[16:20])[0]
tmd_size  = struct.unpack('>I', data[20:24])[0]
align64 = lambda x: (x + 0x3F) & ~0x3F
tik_ofs = align64(hdr_size + cert_size)
tmd_ofs = align64(tik_ofs + tik_size)
dat_ofs = align64(tmd_ofs + tmd_size)

tik = data[tik_ofs:tik_ofs+tik_size]
title_id = struct.unpack('>Q', tik[0x1DC:0x1E4])[0]
title_key = aes_cbc_dec(COMMON_KEY, struct.pack('>Q', title_id) + b'\x00'*8,
                        tik[0x1BF:0x1BF+16])[:16]
print(f"title id {title_id:016x}, key {title_key.hex()}")

tmd = data[tmd_ofs:tmd_ofs+tmd_size]
n_contents = struct.unpack('>H', tmd[0x1DE:0x1E0])[0]
print(f"{n_contents} contents, data @0x{dat_ofs:x}")

cur = dat_ofs
ok = 0
for i in range(n_contents):
    rec = tmd[0x1E4 + i*36: 0x1E4 + (i+1)*36]
    cid, index, typ = struct.unpack('>IHH', rec[:8])
    sz = struct.unpack('>Q', rec[8:16])[0]
    sha1_tmd = rec[16:36].hex()
    hashed = bool(typ & 0x8000)
    blob_sz = sz + 0x20 if hashed else sz
    blob = data[cur:cur+blob_sz]
    cur = align64(cur + blob_sz)
    payload = blob[0x20:] if hashed else blob
    iv = struct.pack('>H', index) + b'\x00'*14
    dec = aes_cbc_dec(title_key, iv, payload)
    sha = hashlib.sha1(dec[:sz]).hexdigest()
    match = sha == sha1_tmd
    ok += match
    (OUTDIR / f"{cid:08x}.app").write_bytes(dec[:sz])
    print(f"  {cid:08x}.app idx={index} type={typ:#06x} size={sz} {'SHA1 MATCH' if match else 'MISMATCH ' + sha[:12]}")

print(f"\n{ok}/{n_contents} SHA1-verified")
