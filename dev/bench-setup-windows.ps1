# Symta benchmark-environment setup (Windows)
#
# Configures the host so benchmark / drift / sweep timings reflect
# the Symta code under test, not the OS-imposed CPU mitigations and
# AV/IDS scan latency that dominate cold runs on a default Win11
# install.
#
# Run as Administrator:
#   PowerShell -ExecutionPolicy Bypass -File dev\bench-setup-windows.ps1
#   PowerShell -ExecutionPolicy Bypass -File dev\bench-setup-windows.ps1 -Revert
#
# What it touches:
#
# 1. Spectre / Meltdown / MDS mitigations (registry).
#    Sets FeatureSettingsOverride=3 and FeatureSettingsOverrideMask=3
#    under HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\
#    Memory Management.  Disables the CPU-microcode workarounds that
#    Intel/AMD ship for the various 2018+ side-channel CVEs.  Each
#    of those costs 5-10% on indirect-branch-heavy code (which
#    every bytecode interpreter is).  Reverts to default by setting
#    both keys to 0.
#
# 2. Windows Defender real-time-scan exclusion for symta.exe and
#    its built test/benchmark go.exe binaries.  Real-time scan
#    triggers on every executable load and many file writes; on a
#    test sweep that compiles dozens of .sbc files it adds tens of
#    seconds and high variance.  -Revert removes the exclusions.
#
# What it ONLY reports (manual steps follow):
#
# 3. VBS / Memory Integrity.  Per-process effects on indirect-call
#    latency from Virtualization-Based Security; requires manual
#    toggle in Windows Security -> Device security -> Core isolation
#    -> Memory integrity, plus a reboot.  This script reports
#    current state but doesn't flip it.
#
# 4. Hyper-V stack.  Even when no VM is running, an installed
#    Hyper-V exposes the root OS to nested-paging / hypercall
#    latency on certain code paths.  Disable via
#      bcdedit /set hypervisorlaunchtype off
#    or via "Turn Windows features on or off" -> uncheck Hyper-V.
#    Reboot required.  This script reports the current launch
#    type but doesn't flip it.
#
# All four items influence benchmark numbers and need to be at a
# known state before any cross-revision comparison.

param(
    [switch]$Revert
)

$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$symtaRoot = Join-Path $repoRoot 'symta'

function Require-Admin {
    $id = [Security.Principal.WindowsIdentity]::GetCurrent()
    $p  = New-Object Security.Principal.WindowsPrincipal($id)
    if (-not $p.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        Write-Error "Run as Administrator.  (right-click PowerShell -> Run as administrator)"
        exit 1
    }
}

function Set-MitigationsRegistry {
    param([int]$Value)  # 3 = disabled, 0 = default

    $key = 'HKLM:\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management'
    Set-ItemProperty -Path $key -Name 'FeatureSettingsOverride'     -Type DWord -Value $Value
    Set-ItemProperty -Path $key -Name 'FeatureSettingsOverrideMask' -Type DWord -Value $Value

    if ($Value -eq 3) {
        Write-Host "[OK]   Spectre/Meltdown/MDS mitigations DISABLED (effective after reboot)"
    } else {
        Write-Host "[OK]   Spectre/Meltdown/MDS mitigations restored to OS default (effective after reboot)"
    }
}

function Set-DefenderExclusions {
    param([switch]$Remove)

    $bins = @(
        (Join-Path $symtaRoot 'symta.exe'),
        (Join-Path $symtaRoot 'benchmark\rt\go.exe'),
        (Join-Path $symtaRoot 'benchmark\am\go.exe')
    )

    foreach ($b in $bins) {
        if ($Remove) {
            try {
                Remove-MpPreference -ExclusionProcess $b -ErrorAction Stop
                Write-Host "[OK]   Defender exclusion REMOVED: $b"
            } catch {
                Write-Host "[skip] Defender exclusion not present for: $b"
            }
        } else {
            Add-MpPreference -ExclusionProcess $b
            Write-Host "[OK]   Defender exclusion ADDED: $b"
        }
    }
}

function Report-VBSMemoryIntegrity {
    Write-Host ""
    Write-Host "Memory Integrity (HVCI) -- manual:"
    try {
        $devGuard = Get-CimInstance -ClassName Win32_DeviceGuard `
            -Namespace 'root\Microsoft\Windows\DeviceGuard' -ErrorAction Stop
        $running = $devGuard.SecurityServicesRunning
        $enabled = $devGuard.SecurityServicesConfigured

        $hvciRunning = $running -contains 2  # 2 = HypervisorEnforcedCodeIntegrity
        $hvciConfigured = $enabled -contains 2
        $cflowRunning = $running -contains 3  # 3 = SystemGuard secure launch (rare)
        $vbsStatus = $devGuard.VirtualizationBasedSecurityStatus  # 0=off 1=configured 2=running

        Write-Host "  VBS status:               $vbsStatus  (0=off, 1=configured, 2=running)"
        Write-Host "  HVCI running:             $hvciRunning"
        Write-Host "  HVCI configured:          $hvciConfigured"

        if ($hvciRunning) {
            Write-Host "  -> Turn OFF in: Windows Security -> Device security ->"
            Write-Host "     Core isolation -> Memory integrity.  Reboot required."
        }
    } catch {
        Write-Host "  (could not query Win32_DeviceGuard: $($_.Exception.Message))"
    }
}

function Report-HyperV {
    Write-Host ""
    Write-Host "Hyper-V hypervisor -- manual:"
    try {
        $bcd = bcdedit /enum '{current}' 2>$null
        $line = $bcd | Select-String -Pattern 'hypervisorlaunchtype' | Select-Object -First 1
        if ($line) {
            Write-Host "  bcdedit: $($line.Line.Trim())"
        } else {
            Write-Host "  bcdedit: hypervisorlaunchtype not explicit (default = Auto)"
        }
        Write-Host "  -> Disable with:  bcdedit /set hypervisorlaunchtype off"
        Write-Host "     (or 'Turn Windows features on or off' -> uncheck Hyper-V).  Reboot required."
    } catch {
        Write-Host "  (could not query bcdedit: $($_.Exception.Message))"
    }
}

function Report-CurrentRegistry {
    Write-Host ""
    Write-Host "Spectre/Meltdown/MDS mitigations -- current registry state:"
    $key = 'HKLM:\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management'
    try {
        $v  = (Get-ItemProperty -Path $key -Name 'FeatureSettingsOverride' -ErrorAction Stop).FeatureSettingsOverride
        $vm = (Get-ItemProperty -Path $key -Name 'FeatureSettingsOverrideMask' -ErrorAction Stop).FeatureSettingsOverrideMask
        Write-Host "  FeatureSettingsOverride:     $v  ($('disabled (3)','partial','partial','disabled (3)' [$v]))"
        Write-Host "  FeatureSettingsOverrideMask: $vm"
        Write-Host "  (effective on reboot)"
    } catch {
        Write-Host "  (keys not set -- using Windows defaults; mitigations are ON)"
    }
}


# --- main ---------------------------------------------------------

Require-Admin

if ($Revert) {
    Write-Host "Reverting benchmark-environment changes..."
    Set-MitigationsRegistry -Value 0
    Set-DefenderExclusions -Remove
} else {
    Write-Host "Applying benchmark-environment changes..."
    Set-MitigationsRegistry -Value 3
    Set-DefenderExclusions
}

Report-CurrentRegistry
Report-VBSMemoryIntegrity
Report-HyperV

Write-Host ""
Write-Host "Done.  Reboot for the registry / VBS / Hyper-V changes to fully take effect."
Write-Host "After reboot, verify benchmark stability with:"
Write-Host "  bash benchmark/rt/run.sh"
Write-Host "  bash benchmark/am/run.sh"
