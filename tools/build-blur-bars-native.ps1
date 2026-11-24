param(
    [string]$Sdk,
    [string]$Glslang,
    [int]$Jobs = 2
)
$ErrorActionPreference = 'Stop'
$repository = Split-Path -Parent $PSScriptRoot
if (-not $Sdk) {
    $line = Get-Content -LiteralPath (Join-Path $repository 'local.properties') | Where-Object { $_ -like 'sdk.dir=*' } | Select-Object -First 1
    if (-not $line) { throw 'Pass -Sdk or set sdk.dir in local.properties' }
    $Sdk = $line.Substring(8).Replace('\\','\').Replace('\:',':')
}
$ndk = Join-Path $Sdk 'ndk/27.3.13750724'
$cmake = Join-Path $Sdk 'cmake/3.22.1/bin/cmake.exe'
$ninja = Join-Path $Sdk 'cmake/3.22.1/bin/ninja.exe'
$nativeBuild = Join-Path $repository 'app/build/blur-bars-native'
$argsConfigure = @('-S', (Join-Path $repository 'app/src/main/cpp'), '-B', $nativeBuild, '-G', 'Ninja', "-DCMAKE_MAKE_PROGRAM=$ninja", "-DCMAKE_TOOLCHAIN_FILE=$ndk/build/cmake/android.toolchain.cmake", '-DANDROID_ABI=arm64-v8a', '-DANDROID_PLATFORM=android-29', '-DCMAKE_BUILD_TYPE=Release', '-DGAMENATIVE_VULKAN_ONLY=ON')
if ($Glslang) { $argsConfigure += "-DGLSLANG_VALIDATOR=$Glslang" }
& $cmake @argsConfigure
if ($LASTEXITCODE -ne 0) { throw 'Vulkan configuration failed' }
& $cmake --build $nativeBuild --target vulkan_renderer --parallel $Jobs
if ($LASTEXITCODE -ne 0) { throw 'Vulkan build failed' }
foreach ($flavor in @('modern','legacy')) {
    $output = Join-Path $repository "app/src/$flavor/jniLibs/arm64-v8a/libvulkan_renderer.so"
    Copy-Item -LiteralPath (Join-Path $nativeBuild 'libvulkan_renderer.so') -Destination $output -Force
    Get-FileHash -LiteralPath $output -Algorithm SHA256
}
