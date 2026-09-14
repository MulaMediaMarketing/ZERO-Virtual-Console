param(
  [Parameter(Mandatory=$true)][string[]]$Paths,
  [string]$ReportPath = "./build/SigningReports/authenticode.json"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$records = New-Object System.Collections.Generic.List[object]
foreach ($path in $Paths) {
  if (-not (Test-Path $path -PathType Leaf)) { throw "Missing signing target: $path" }
  $sig = Get-AuthenticodeSignature -FilePath $path
  $ok = $sig.Status -eq [System.Management.Automation.SignatureStatus]::Valid -and $null -ne $sig.SignerCertificate
  $records.Add([ordered]@{
    path = $path
    status = [string]$sig.Status
    status_message = $sig.StatusMessage
    signer_subject = if ($sig.SignerCertificate) { $sig.SignerCertificate.Subject } else { $null }
    signer_thumbprint = if ($sig.SignerCertificate) { $sig.SignerCertificate.Thumbprint } else { $null }
    timestamp_subject = if ($sig.TimeStamperCertificate) { $sig.TimeStamperCertificate.Subject } else { $null }
    valid = $ok
  })
  if (-not $ok) { throw "Authenticode validation failed for $path: $($sig.Status) $($sig.StatusMessage)" }
}

$dir = Split-Path -Parent $ReportPath
New-Item -ItemType Directory -Force -Path $dir | Out-Null
[ordered]@{
  schema = 1
  result = "PASS"
  generated_at_utc = [DateTime]::UtcNow.ToString("o")
  files = $records.ToArray()
} | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 -Path $ReportPath
Write-Host "ZERO Authenticode verification: PASS"
