# Third-party code and tools

- `third_party/littlefs/` is unchanged littlefs v2.11.3, commit
  `6cb4e86540eca0d9ba62500a298385c9d863c8be`, under BSD-3-Clause.
  Its full license remains in that directory. Source URLs and exact hashes:
  [provenance/upstream.json](provenance/upstream.json).
- The authored transaction layer, NOR test double, harness and documentation use
  the root MIT license. No NASA code is included in that authored layer.
- NASA F Prime PrmDb is a cited spacecraft-software context source, not an
  integrated dependency or an implementation whose defect this project proves.
- Optional Zig 0.14.1 is downloaded to ignored `.tools/`. Its archive checksum
  is verified before extraction. The Zig distribution retains its own licenses;
  compiler executables are excluded from the source ZIP.
- Optional Arm GNU Toolchain, Unicorn 2.1.4 and pyelftools 0.32 retain their
  respective distribution licenses. Emulator/compiler binaries are not shipped.

No credentials, hardware authentication token, paid activation or restricted
mission dataset is required for any included experiment.
