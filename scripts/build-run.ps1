$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")

if (-not $env:VCPKG_ROOT) {
    throw "VCPKG_ROOT is not set. Install vcpkg and set VCPKG_ROOT to its directory."
}

Push-Location $Root
try {
    cmake --preset vs2022-release
    cmake --build --preset build-release

    $Executable = Join-Path $Root "build\vs2022-release\bin\Release\ArcGame.exe"
    if (-not (Test-Path -LiteralPath $Executable)) {
        throw "ArcGame.exe was not found at $Executable"
    }

    & $Executable
}
finally {
    Pop-Location
}
