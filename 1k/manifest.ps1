# overwrite $manifest in main script build1k.ps1
$manifest = @{
    msvc         = '143+';
    xcode        = '13.0.0~14.2.0'; # range
    clang        = '15.0.0+';
    gcc          = '9.0.0+';
    cmake        = '3.26.4+';
    ninja        = '1.11.1+';
    ndk          = 'r16b+';
}

[void]$manifest
