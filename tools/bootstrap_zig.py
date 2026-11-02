"""Optional pinned Windows C compiler; public download, no system installation."""
from pathlib import Path
import hashlib
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
URL = 'https://ziglang.org/download/0.14.1/zig-x86_64-windows-0.14.1.zip'
SHA256 = '554f5378228923ffd558eac35e21af020c73789d87afeabf4bfd16f2e6feed2c'

def bootstrap():
    target = ROOT / '.tools/zig-x86_64-windows-0.14.1/zig.exe'
    if target.exists():
        return target
    cache = ROOT / '.tools'
    cache.mkdir(exist_ok=True)
    archive = cache / 'zig.zip'
    if not archive.exists():
        urllib.request.urlretrieve(URL, archive)
    if hashlib.sha256(archive.read_bytes()).hexdigest() != SHA256:
        raise RuntimeError('Zig archive checksum mismatch')
    with zipfile.ZipFile(archive) as z:
        for member in z.infolist():
            if not (cache / member.filename).resolve().is_relative_to(cache.resolve()):
                raise RuntimeError('Unsafe archive member')
        z.extractall(cache)
    return target

if __name__ == '__main__':
    print(bootstrap())
