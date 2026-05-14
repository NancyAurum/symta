# Benchmark-environment setup (Windows)

Symta benchmarks (`benchmark/am/`, `benchmark/rt/`) need a host
state that's free of the four common Windows-imposed sources of
variance that swamp the actual code under test.  This doc
covers what they are, why they matter, how to disable them,
and how to revert.

The companion script `bench-setup-windows.ps1` automates items
1-2.  Items 3-4 need manual Windows-features changes + a
reboot.

The four factors (in rough order of typical impact):

| #   | Factor                  | Typical hit | Auto? |
|-----|-------------------------|-------------|-------|
| 1   | Spectre / Meltdown / MDS mitigations | 5-10 % on indirect-branch-heavy code | yes |
| 2   | Defender real-time scan | tens of seconds + high variance on test sweeps | yes |
| 3   | VBS / HVCI / Memory Integrity        | ~2-5 % on indirect calls + memory access | no |
| 4   | Hyper-V hypervisor (root partition)  | small, but quirky on certain code paths | no |

Compile-time companion (already in `Makefile.w64`):

- `-fcf-protection=none` — disable Intel CET endbr64 emission.
  We never JIT executable code and the bytecode interpreter is
  pure C, so the indirect-branch tracking CET adds is overhead.
  Microsoft-flavour `/GUARD:NO` and `/CETCOMPAT:NO` linker
  flags would be no-ops on mingw output (the mingw linker
  doesn't add CFG instrumentation or the CETCOMPAT PE-header
  bit), so they're not in the link line.

---

## 1. Spectre / Meltdown / MDS mitigations

CPU-microcode workarounds for the various 2018+ side-channel
CVEs.  Every indirect branch and certain memory accesses pay a
microcode tax.  Bytecode interpreters are all indirect branches
(switch jump table, computed gotos, method dispatch); a real
6-8 % gap between mitigations-on and mitigations-off is normal.

**Disable:**

```cmd
reg add "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management" /v FeatureSettingsOverride     /t REG_DWORD /d 3 /f
reg add "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management" /v FeatureSettingsOverrideMask /t REG_DWORD /d 3 /f
```

The setup script wraps these.  Effect after reboot.

**Verify after reboot:**

```powershell
Get-SpeculationControlSettings
```

(install with `Install-Module SpeculationControl` if missing).

**Re-enable (back to OS default):**

```cmd
reg add "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management" /v FeatureSettingsOverride     /t REG_DWORD /d 0 /f
reg add "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management" /v FeatureSettingsOverrideMask /t REG_DWORD /d 0 /f
```

Or `bench-setup-windows.ps1 -Revert`.

---

## 2. Windows Defender real-time scan

Real-time scan triggers on every executable load and many file
writes.  On a `make test-all` sweep that compiles dozens of
`.sbc` files and spawns dozens of `go.exe` invocations, this
adds tens of seconds and high run-to-run variance (Defender
caches partially, so the second run is much faster than the
first).

**Exclude `symta.exe` + benchmark binaries:**

```powershell
Add-MpPreference -ExclusionProcess "$PWD\symta.exe"
Add-MpPreference -ExclusionProcess "$PWD\benchmark\rt\go.exe"
Add-MpPreference -ExclusionProcess "$PWD\benchmark\am\go.exe"
```

The setup script does this with absolute paths resolved to the
repo root.

**Verify:**

```powershell
Get-MpPreference | Select-Object -ExpandProperty ExclusionProcess
```

**Remove exclusions:**

```powershell
Remove-MpPreference -ExclusionProcess "$PWD\symta.exe"
# ...etc for each path
```

Or `bench-setup-windows.ps1 -Revert`.

---

## 3. VBS / HVCI / Memory Integrity (manual)

Virtualization-Based Security runs the Windows kernel under
Hyper-V's hypervisor and enforces Hypervisor-Enforced Code
Integrity (HVCI) on driver loads.  Both add small but
measurable per-process latency on indirect calls and certain
memory access patterns.

**Disable:**

1. Open *Windows Security*.
2. *Device security* → *Core isolation details*.
3. Toggle *Memory integrity* **OFF**.
4. Reboot.

**Verify after reboot:**

```powershell
Get-CimInstance -ClassName Win32_DeviceGuard `
    -Namespace 'root\Microsoft\Windows\DeviceGuard' |
  Select-Object VirtualizationBasedSecurityStatus, SecurityServicesRunning
```

`VirtualizationBasedSecurityStatus = 0` means VBS is off.
`SecurityServicesRunning` empty (or no `2`) means HVCI is off.

The setup script reports current state under the
"Memory Integrity" heading; flipping it requires the manual
toggle above.

**Re-enable:** same UI path, toggle ON, reboot.

---

## 4. Hyper-V hypervisor (manual)

Even when no VM is running, an installed Hyper-V exposes the
root OS to nested-paging / hypercall latency on certain code
paths.  Less impact than VBS but worth disabling for the
cleanest baseline.

**Disable:**

```cmd
bcdedit /set hypervisorlaunchtype off
```

Reboot.

Optionally also uncheck "Hyper-V" under *Turn Windows features
on or off* to remove the binaries entirely.

**Verify after reboot:**

```cmd
bcdedit /enum {current}
```

Look for `hypervisorlaunchtype off`.

**Re-enable:**

```cmd
bcdedit /set hypervisorlaunchtype auto
```

Reboot.

The setup script reports the current bcdedit state under the
"Hyper-V" heading; flipping it requires the manual commands
above.

---

## Combined workflow

For a clean benchmark session:

1. Run as Administrator:
   ```
   PowerShell -ExecutionPolicy Bypass -File dev\bench-setup-windows.ps1
   ```
   (handles items 1-2 + reports items 3-4)

2. Flip items 3-4 manually if the script's status lines say
   they're on.

3. Reboot.

4. Run the benchmark suite normally:
   ```
   bash benchmark/am/run.sh --save am-after.txt
   bash benchmark/rt/run.sh --save rt-after.txt
   ```

5. When done benchmarking, revert:
   ```
   PowerShell -ExecutionPolicy Bypass -File dev\bench-setup-windows.ps1 -Revert
   ```
   (handles items 1-2; flip items 3-4 back manually).

   Reboot.

---

## What about the build flags?

The `-fcf-protection=none` flag in `Makefile.w64` is already
applied to every Symta build; it's not part of the bench-only
setup.  Reasoning:

- We never JIT executable memory (sffi is statically compiled
  per ABI, see `runtime/sffi/ARCHITECTURE.md`).
- The bytecode interpreter is pure C; there's no user-supplied
  control flow that could benefit from CET's shadow stack +
  endbr landing pads.
- Emitting `endbr64` at every function entry on x86-64 inflates
  hot-path code by ~4 bytes per indirect-callable function and
  trips the OS-level CET mitigation on Windows 11 if the
  CETCOMPAT PE-header bit is set.

`-fcf-protection=none` is benign on systems without CET (the
flag is a no-op when the CPU doesn't support it), so it's safe
to leave in for production builds, not just benchmarks.

Microsoft-flavour linker flags are absent for a reason:
mingw's `ld` doesn't add CFG instrumentation or the CETCOMPAT
PE-header bit in the first place, so `/GUARD:NO` and
`/CETCOMPAT:NO` are no-ops for our build.  If we ever switch
to the MSVC toolchain we'd need to add them.
