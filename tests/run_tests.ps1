# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (C) 2026 Xiaoguo
#
# Part of bfc. Licensed under the GNU Affero General Public License,
# version 3 or later. See LICENSE.
# Additional permission for compiler output: see LICENSE-EXCEPTION.md.

$ErrorActionPreference = "Stop"
$here = $PSScriptRoot
$root = Split-Path -Parent $here
$bfc  = Join-Path $root "bfc.exe"
$bdir = Join-Path $here "bf"
$adir = Join-Path $here "asm"
$bin  = Join-Path $here "bin"
New-Item -ItemType Directory -Force -Path $adir, $bin | Out-Null

# name -> expected stdout as hex (verified against an independent interpreter)
$cases = @(
  @{ n = "A";            exp = "41" },
  @{ n = "move";         exp = "41" },
  @{ n = "hello";        exp = "48 65 6C 6C 6F 20 57 6F 72 6C 64 21 0A" },
  @{ n = "run_merge";    exp = "03" },
  @{ n = "multiply";     exp = "0C" },
  @{ n = "copy";         exp = "05 05" },
  @{ n = "move_b";       exp = "41" },
  @{ n = "move_c";       exp = "41" },
  @{ n = "move_d";       exp = "41" },
  @{ n = "scan_right";   exp = "01" },
  @{ n = "scan_left";    exp = "01" },
  @{ n = "scan_guard";   exp = "01" },
  @{ n = "clear_plus";   exp = "05" },
  @{ n = "cancel_add";   exp = "02" },
  @{ n = "cancel_move";  exp = "01" },
  @{ n = "dots";         exp = "01 01 02 02" },
  @{ n = "nested_fold";  exp = "40" },
  @{ n = "nested_kept";  exp = "10" },
  @{ n = "nested_guard"; exp = "07" },
  @{ n = "nested_reset"; exp = "00 00 00 00 00 08 00" },
  @{ n = "nested_deep";  exp = "00 00 00 00 00 00 18" },
  @{ n = "nested_v0";    exp = "00 00 00 00 07 05 00" },
  @{ n = "wrap255";      exp = "FF" },
  @{ n = "wrap300";      exp = "2C" },
  @{ n = "comment";      exp = "41" },
  @{ n = "echo";         exp = "5A" }
)

$pass = 0; $fail = 0
Push-Location $bdir
foreach ($c in $cases) {
  & $bfc ($c.n + ".bf") -o ($bin + "\" + $c.n + ".exe") 2>$null | Out-Null
  if ($LASTEXITCODE -ne 0) { Write-Host ("FAIL {0}: compile" -f $c.n); $fail++; continue }
  $s = Join-Path $bdir ($c.n + ".s")
  if (Test-Path $s) { Move-Item -Force $s (Join-Path $adir ($c.n + ".s")) }
  $exe = $bin + "\" + $c.n + ".exe"
  $out = $bin + "\" + $c.n + ".out"
  if ($c.n -eq "echo") { cmd /c ('"' + $exe + '" < "' + $bdir + '\echo_in.txt" > "' + $out + '"') }
  else                 { cmd /c ('"' + $exe + '" > "' + $out + '"') }
  $b   = [IO.File]::ReadAllBytes($out)
  $got = (($b | Where-Object { $_ -ne 13 }) | ForEach-Object { $_.ToString("X2") }) -join " "
  $got = $got.Trim()
  if ($got -eq $c.exp) { $pass++ } else { Write-Host ("FAIL {0}: exp=[{1}] got=[{2}]" -f $c.n, $c.exp, $got); $fail++ }
}
Pop-Location

# bracket errors must be rejected (non-zero exit, no executable)
$saved = $ErrorActionPreference
$ErrorActionPreference = "Continue"
foreach ($n in @("err_open", "err_close")) {
  $src = Join-Path $bdir ($n + ".bf")
  $exe = $bin + "\" + $n + ".exe"
  cmd /c ('"' + $bfc + '" "' + $src + '" -o "' + $exe + '" 2>nul') | Out-Null
  if ($LASTEXITCODE -eq 0) { Write-Host ("FAIL {0}: should have been rejected" -f $n); $fail++ } else { $pass++ }
}
$ErrorActionPreference = $saved

Remove-Item -Recurse -Force $bin -ErrorAction SilentlyContinue
Write-Host ""
Write-Host ("{0} passed, {1} failed" -f $pass, $fail)
if ($fail -gt 0) { exit 1 }
exit 0
