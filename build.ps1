#
# This script easy to build win32, linux, winuwp, ios, tvos, osx, android depends on $myRoot/ci/build1k.ps1
# usage: pwsh build.ps1 -p <targetPlatform> -a <arch>
# options
#  -p: build target platform: win32,winuwp,linux,android,osx,ios,tvos,watchos
#      for android: will search ndk in sdk_root which is specified by env:ANDROID_HOME first, 
#      if not found, by default will install ndk-r16b or can be specified by option: -cc 'ndk-r23c'
#  -a: build arch: x86,x64,armv7,arm64; for android can be list by ';', i.e: 'arm64;x64'
#  -cc: toolchain: for win32 you can specific -cc clang to use llvm-clang, please install llvm-clang from https://github.com/llvm/llvm-project/releases
#  -xc: additional cmake options: i.e.  -xc '-Dbuild','-DCMAKE_BUILD_TYPE=Release'
#  -xb: additional cross build options: i.e. -xb '--config','Release'
#  -d: specify project dir to compile, i.e. -d /path/your/project/
# examples:
#   - win32: 
#     - pwsh build.ps1 -p win32
#     - pwsh build.ps1 -p win32 -cc clang
#   - winuwp: pwsh build.ps1 -p winuwp
#   - linux: pwsh build.ps1 -p linux
#   - android: 
#     - pwsh build.ps1 -p android -a arm64
#     - pwsh build.ps1 -p android -a 'arm64;x64'
#   - osx: 
#     - pwsh build.ps1 -p osx -a x64
#     - pwsh build.ps1 -p osx -a arm64
#   - ios: pwsh build.ps1 -p ios -a x64
#   - tvos: pwsh build.ps1 -p tvos -a x64
# build.ps1 without any arguments:
# - pwsh build.ps1
#   on windows: target platform is win32, arch=x64
#   on linux: target platform is linux, arch=x64
#   on macos: target platform is osx, arch=x64
#

$options = @{p = $null; a = 'x64'; d = $null; cc = $null; xc = @(); xb = @(); winsdk = $null }

$optName = $null
foreach ($arg in $args) {
    if (!$optName) {
        if ($arg.StartsWith('-')) { 
            $optName = $arg.SubString(1)
        }
    }
    else {
        if ($options.Contains($optName)) {
            $options[$optName] = $arg
        }
        $optName = $null
    }
}

function add_quote($value) {
    $ret = $null
    $valueType = $value.GetType()
    if ($valueType -eq [string]) {
        $ret = "'$value'"
    }
    elseif ($valueType -eq [object[]]) {
        $ret = ''
        for ($i = 0; $i -lt $value.Count; ++$i) {
            $subVal = $value[$i]
            $ret += "'$subVal'"
            if ($i -ne ($value.Count - 1)) {
                $ret += ','
            }
        }
    }
    return $ret
}

$myRoot = $PSScriptRoot
$workDir = $(Get-Location).Path

$is_ci = $env:GITHUB_ACTIONS -eq 'true'

# start construct full cmd line
$fullCmdLine = @("$((Resolve-Path -Path "$myRoot/1k/build1k.ps1").Path)")

$search_prior_dir = $options.d
$search_paths = if ($search_prior_dir) { @($search_prior_dir, $workDir, $myRoot) } else { @($workDir, $myRoot) }
function search_proj($path, $type) {
    foreach ($search_path in $search_paths) {
        $full_path = Join-Path $search_path $path
        if (Test-Path $full_path -PathType $type) {
            $ret_path = if ($type -eq 'Container') { $full_path } else { $search_path }
            return $ret_path
        }
    }
    return $null
}

$search_rule = @{ path = 'CMakeLists.txt'; type = 'Leaf' }
$proj_dir = search_proj $search_rule.path $search_rule.type

if ($is_ci) {
    $options.xc = [array]$options.xc
    $options.xc += '-DYASIO_ENABLE_KCP=TRUE','-DYASIO_ENABLE_HPERF_IO=1'
}

$bci = $null
# parsing build options
$nopts = $options.xb.Count
for ($i = 0; $i -lt $nopts; ++$i) {
    $optv = $options.xb[$i]
    if($optv -eq '--config') {
        if ($i -lt ($nopts - 1)) {
            $bci = $i + 1
        }
    }
}

if (!$bci) {
    $optimize_flag = @('Debug', 'Release')[$is_ci]
    $options.xb += '--config', $optimize_flag
}

if ($proj_dir) {
    $fullCmdLine += "'-d'", "'$proj_dir'"
}
$prefix = Join-Path $myRoot 'tools/external'
$fullCmdLine += "'-prefix'", "'$prefix'"

# remove arg we don't want forward to
$options.Remove('d')
foreach ($option in $options.GetEnumerator()) {
    if ($option.Value) {
        $fullCmdLine += add_quote "-$($option.Key)"
        $fullCmdLine += add_quote $option.Value
    }
}

$strFullCmdLine = "$fullCmdLine"
Invoke-Expression $strFullCmdLine

$buildResult = ConvertFrom-Json $env:buildResult

if ($is_ci -and !$buildResult.compilerID.StartsWith('mingw')) { # run tests
    $targetOS = $buildResult.targetOS
    $buildDir = $buildResult.buildDir
    & .\ci\test.ps1 -dir $buildDir -target $targetOS
}

. $myRoot/1k/utils.ps1

pauseIfExplorer("build successfully")
