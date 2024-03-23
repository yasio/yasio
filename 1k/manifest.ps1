# Default manifest, refer in 1k/build.ps1

if ($IsWin) {
    $manifest['nasm'] = '2.16.01+'
} else {
    $manifest['nasm'] = '2.15.05+'
}
