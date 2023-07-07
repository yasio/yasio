function pauseIfExplorer($msg) {
    if ($IsWindows -or ("$env:OS" -eq 'Windows_NT')) {
        $myProcess = [System.Diagnostics.Process]::GetCurrentProcess()
        $parentProcess = $myProcess.Parent
        if (!$parentProcess) {
            $myPID = $myProcess.Id
            $instance = Get-WmiObject Win32_Process -Filter "ProcessId = $myPID"
            $parentProcess = Get-Process -Id $instance.ParentProcessID
        }
        $parentProcessName = $parentProcess.ProcessName
        if ($parentProcessName -like "explorer") {
            Write-Host "$msg, press any key to continue ..." -NoNewline
            cmd /c pause 1>$null
            return
        }
    }
    Write-Host "build1k: ${msg}."
}
