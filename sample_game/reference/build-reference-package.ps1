param(
    [Parameter(Mandatory=$true)][string]$ReferenceExe,
    [string]$Output = "$PSScriptRoot\package"
)

$ErrorActionPreference = 'Stop'
$packageRoot = [System.IO.Path]::GetFullPath($Output)
$assets = Join-Path $packageRoot 'Assets'
New-Item -ItemType Directory -Force -Path $assets | Out-Null

Copy-Item -Force $ReferenceExe (Join-Path $packageRoot 'ZeroReferenceGame.exe')
Copy-Item -Force (Join-Path $PSScriptRoot 'zero.manifest.json') (Join-Path $packageRoot 'zero.manifest.json')

Add-Type -AssemblyName System.Drawing

function New-ZeroArtwork {
    param(
        [string]$Path,
        [int]$Width,
        [int]$Height,
        [string]$PrimaryText,
        [string]$SecondaryText,
        [int]$TitleSize
    )

    $bmp = New-Object System.Drawing.Bitmap($Width, $Height)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    try {
        $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
        $rect = New-Object System.Drawing.Rectangle(0, 0, $Width, $Height)
        $c1 = [System.Drawing.Color]::FromArgb(18, 18, 18)
        $c2 = [System.Drawing.Color]::FromArgb(80, 72, 62)
        $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush($rect, $c1, $c2, 18.0)
        $g.FillRectangle($brush, $rect)
        $brush.Dispose()

        $linePen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(80,255,255,255), 2)
        $g.DrawLine($linePen, [int]($Width * 0.08), [int]($Height * 0.72), [int]($Width * 0.92), [int]($Height * 0.72))
        $linePen.Dispose()

        $titleFont = New-Object System.Drawing.Font('Segoe UI', $TitleSize, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
        $bodyFont = New-Object System.Drawing.Font('Segoe UI', [Math]::Max(14, [int]($TitleSize * 0.34)), [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
        $white = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)
        $muted = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(210,225,225,225))
        $g.DrawString($PrimaryText, $titleFont, $white, [float]($Width * 0.08), [float]($Height * 0.24))
        if ($SecondaryText) {
            $g.DrawString($SecondaryText, $bodyFont, $muted, [float]($Width * 0.085), [float]($Height * 0.56))
        }
        $titleFont.Dispose(); $bodyFont.Dispose(); $white.Dispose(); $muted.Dispose()
        $bmp.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally {
        $g.Dispose(); $bmp.Dispose()
    }
}

function Write-ZeroIntegrityManifest {
    param([string]$Root)

    $integrityPath = Join-Path $Root 'zero.integrity.sha256'
    if (Test-Path $integrityPath) { Remove-Item -Force $integrityPath }

    $files = @(Get-ChildItem -LiteralPath $Root -Recurse -File |
        Where-Object { $_.Name -ne 'zero.integrity.sha256' } |
        Sort-Object { [System.IO.Path]::GetRelativePath($Root, $_.FullName) })

    [UInt64]$totalBytes = 0
    foreach ($file in $files) { $totalBytes += [UInt64]$file.Length }

    $lines = [System.Collections.Generic.List[string]]::new()
    $lines.Add('# ZERO package integrity v1')
    $lines.Add("# files=$($files.Count) bytes=$totalBytes")
    foreach ($file in $files) {
        $relative = [System.IO.Path]::GetRelativePath($Root, $file.FullName).Replace('\', '/')
        $hash = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        $lines.Add("$hash  $($file.Length)  $relative")
    }
    [System.IO.File]::WriteAllLines($integrityPath, $lines, [System.Text.UTF8Encoding]::new($false))
}

New-ZeroArtwork -Path (Join-Path $assets 'hero.png') -Width 1600 -Height 900 -PrimaryText 'ZERO' -SecondaryText 'REFERENCE EXPERIENCE' -TitleSize 180
New-ZeroArtwork -Path (Join-Path $assets 'icon.png') -Width 512 -Height 512 -PrimaryText 'Z' -SecondaryText '' -TitleSize 250
New-ZeroArtwork -Path (Join-Path $assets 'logo.png') -Width 1200 -Height 360 -PrimaryText 'ZERO' -SecondaryText 'REFERENCE EXPERIENCE' -TitleSize 150
Write-ZeroIntegrityManifest -Root $packageRoot

Write-Host "ZERO reference package created: $packageRoot"
