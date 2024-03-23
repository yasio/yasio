# This script is only for github action ci

$build_script = (Resolve-Path $PSScriptRoot/../build.ps1).Path

. $build_script @args -xc '-DYASIO_ENABLE_NI=ON' -xb '--verbose'

if (!$configOnly) {
    Write-Host "buildResult: $env:buildResult"
    $buildResult = ConvertFrom-Json $env:buildResult

    if ($buildResult.isHostTarget) { # run tests
        $targetOS = $buildResult.targetOS
        $buildDir = $buildResult.buildDir
        # run tests
        $testTable = @{
            'win32' = {
                $buildDir = $args[0]
                if ($buildResult.compilerID -ne 'gcc') {
                    Write-Host "run icmptest on windows ..."
                    & "$buildDir\tests\icmp\Release\icmptest.exe" $env:PING_HOST
                    Write-Host "run mtu on windows ..."
                    & "$buildDir\tests\mtu\Release\mtutest.exe"
                }
            };
            'linux' = {
                $buildDir = $args[0]
                Write-Host "run issue201 on linux..."
                & "$buildDir/tests/issue201/issue201"
                
                Write-Host "run httptest on linux..."
                & "$buildDir/tests/http/httptest"
            
                Write-Host "run ssltest on linux..."
                & "$buildDir/tests/ssl/ssltest"
            
                Write-Host "run icmp test on linux..."
                & "$buildDir/tests/icmp/icmptest" $env:PING_HOST

                Write-Host "run mtu on linux ..."
                & "$buildDir/tests/mtu/mtutest"
            }; 
            'osx' = {
                $buildDir = $args[0]
                if ($buildResult.targetCPU -eq $buildResult.hostCPU) {
                    Write-Host "run test tcptest on osx ..."
                    & "$buildDir/tests/tcp/Release/tcptest"
                    
                    Write-Host "run test issue384 on osx ..."
                    & "$buildDir/tests/issue384/Release/issue384"
            
                    Write-Host "run test icmp on osx ..."
                    & "$buildDir/tests/icmp/Release/icmptest" $env:PING_HOST

                    Write-Host "run mtu on osx ..."
                    & "$buildDir/tests/mtu/Release/mtutest"
                }
            };
        }

        # run test
        $run_test = $testTable[$targetOS]
        if ($run_test) {
            & $run_test $buildDir
        }
    }
}

