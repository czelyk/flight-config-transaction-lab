# Build paths

## Portable runner

`python tools/run_tests.py` uses GCC/Clang from PATH; `--cc` selects an executable.
Windows without a native compiler uses the checksum-pinned local Zig distribution.
Use an ordinary short project path. No user or machine-wide environment changes
are performed. `python tools/analyze.py` selects the newest completed native run.

## CMake

With GCC/Clang, use the README's normal configure/build/CTest commands. The project
exports a `flight_config` static library linked with pinned littlefs; the modeled
NOR implementation is only linked into the test executables.

For the verified local Windows/Zig route, install the optional build driver:

```powershell
python -m venv .venv
.venv/Scripts/python -m pip install ninja==1.13.0
python tools/bootstrap_zig.py
$repo = (Get-Location).Path.Replace('\','/')
cmake -S . -B build-cmake -G Ninja "-DCMAKE_MAKE_PROGRAM=$repo/.venv/Scripts/ninja.exe" "-DCMAKE_TOOLCHAIN_FILE=cmake/zig-windows.cmake" "-DZIG_EXECUTABLE=$repo/.tools/zig-x86_64-windows-0.14.1/zig.exe" -DCMAKE_BUILD_TYPE=Debug
cmake --build build-cmake
ctest --test-dir build-cmake --output-on-failure
```

Own C sources use -Wall -Wextra -Werror. The unchanged third-party code's warnings
are retained separately. An initial all-sources -Werror build exposed an unused
upstream helper under the compiler's assertion configuration; no upstream source
was changed to hide it.

## ARM

`tools/run_arm.py` requires requirements-arm.txt and Arm GNU Toolchain. It links
an emulator fixture at declared addresses from cmake/arm-emulation.ld. That linker
script and RAM-backed medium are not an STM32 board port. Runs save compiler
commands, section sizes, stack-usage files and allocator-symbol checks. Stack
usage files are evidence only; no whole-program peak-stack bound is claimed.
