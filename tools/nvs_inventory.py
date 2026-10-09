"""Read-only inventory of NVS entries from an esptool flash backup.

Reports blob sizes and hashes without printing pet names or other value data.
No device access, writes, erases, or reconstruction of NVS pages.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

def inventory(data):
    namespaces = {}
    records = []
    pages = []
    for offset in range(0, len(data), 4096):
        page = data[offset:offset + 4096]
        if len(page) != 4096 or page[:4] == b'\xff'*4:
            continue
        state, seq = struct.unpack_from('<II', page)
        if state not in (0xfffffffe, 0xfffffffc, 0xfffffff8):
            raise ValueError(f'Unexpected NVS page state {state:#x} at {offset:#x}')
        pages.append((seq, offset, page))
    used = 0
    for seq, offset, page in sorted(pages):
        i = 0
        while i < 126:
            entry_state = (page[32 + i//4] >> ((i%4)*2)) & 3
            if entry_state != 2:
                i += 1
                continue
            entry = page[64+i*32:96+i*32]
            ns, kind, span, chunk = entry[:4]
            if not span or i + span > 126:
                raise ValueError(f'Invalid entry span at page {offset:#x}, entry {i}')
            key = entry[8:24].split(b'\0')[0].decode('ascii')
            raw = entry[24:32]
            used += span
            if ns == 0 and kind == 1:
                namespaces[raw[0]] = key
            else:
                if kind in (0x41, 0x42, 0x21):
                    length = struct.unpack_from('<H', raw)[0]
                    value = page[96+i*32:64+(i+span)*32][:length]
                    if len(value) != length:
                        raise ValueError('Incomplete NVS data entry')
                else:
                    value = raw
                records.append((ns,key,kind,chunk,value))
            i += span
    result = {}
    for ns,key,kind,chunk,value in records:
        name = namespaces.get(ns, f'UNKNOWN-{ns}')
        if kind == 0x48:
            length, count, start = struct.unpack_from('<IBB',value)
            chunks = {(n,k,c):v for n,k,t,c,v in records if t==0x42}
            value = b''.join(chunks[(ns,key,(start+j)&255)] for j in range(count))
            if len(value) != length:
                raise ValueError(f'Blob size mismatch: {name}/{key}')
        elif kind == 0x42:
            continue
        elif kind not in (0x41,0x21):
            value = value[:{1:1,0x11:1,2:2,0x12:2,4:4,0x14:4,8:8,0x18:8}.get(kind,8)]
        result.setdefault(name,{})[key] = {'type':kind,'bytes':len(value),'sha256':hashlib.sha256(value).hexdigest()}
    return {'used_entries':used,'namespaces':result}

if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('backup',type=Path)
    parser.add_argument('--offset',type=lambda x:int(x,0),default=0)
    parser.add_argument('--size',type=lambda x:int(x,0),default=0x5000)
    parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    data=args.backup.read_bytes()[args.offset:args.offset+args.size]
    if len(data)!=args.size:raise ValueError('Backup does not contain the requested partition')
    result=inventory(data)
    text=json.dumps(result,indent=2,sort_keys=True)
    if args.output:args.output.write_text(text+'\n',encoding='utf-8')
    for ns,keys in sorted(result['namespaces'].items()):
        print(ns,', '.join(f'{key}={value["bytes"]}B' for key,value in sorted(keys.items())))
    print('Active entries:',result['used_entries'])
