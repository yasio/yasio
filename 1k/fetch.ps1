# fetch pkg by url or manifest.json path
param(
    $uri, # the pkg uri
    $prefix, # the prefix to store
    $manifest_file = $null,
    $name = $null,
    $version = $null, # version hint
    $revision = $null # revision hint
)

# content of _1kiss with yaml format
# ver: 1.0
# branch: 1.x
# commits: 2802
# rev: 29b0b28

Set-Alias println Write-Host

if (!$uri -or !$prefix) {
    throw 'fetch.ps1: missing parameters'
}

function download_file($uri, $out) {
    if (Test-Path $out -PathType Leaf) { return }
    println "Downloading $uri to $out ..."
    Invoke-WebRequest -Uri $uri -OutFile $out
}

function mkdirs($path) {
    if (!(Test-Path $path -PathType Container)) {
        New-Item $path -ItemType Directory 1>$null
    }
}

function fetch_repo($url, $out) {
    if (!$url.EndsWith('.git')) {
        download_file $url $out
        if ($out.EndsWith('.zip')) {
            Expand-Archive -Path $out -DestinationPath $prefix
        }
        elseif ($out.EndsWith('.tar.gz')) {
            tar xf "$out" -C $prefix
        }
    }
    else {
        git clone --progress $url $lib_src | Out-Host
        if (!(Test-Path $(Join-Path $lib_src '.git')) -and (Test-Path $lib_src -PathType Container)) {
            Remove-Item $lib_src -Recurse -Force 
        }
    }
}

$cache_dir = Join-Path (Resolve-Path $PSScriptRoot/..).Path 'cache'

if (!(Test-Path $cache_dir -PathType Container)) {
    mkdirs $cache_dir
}

# parse url from $uri
$uriInfo = [array]$uri.Split('#')
$uri = $uriInfo[0]
if (!$version) {
    $version = $uriInfo[1]
}

$url = $null
if ($uri -match '^([a-z]+://|git@)') {
    $url = $uri
}
elseif ($uri.StartsWith('gh:')) {
    $url = "https://github.com/$($uri.substring(3))"
    if (!$url.EndsWith('.git')) { $url += '.git' }
}
elseif ($uri.StartsWith('gl:')) {
    $url = "https://gitlab.com/$($uri.substring(3))"
    if (!$url.EndsWith('.git')) { $url += '.git' }
}
else {
    $name = $uri
}

# simple match url/ssh schema
if ($url) {
    # fetch by url directly
    if ($PSVersionTable.PSVersion.major -gt 5) {
        $folder_name = (Split-Path $url -LeafBase)
    } else {
        $folder_name = (Split-Path $url -Leaf)
        $first_dot = $folder_name.IndexOf('.')
        if ($first_dot -ne -1) { $folder_name = $folder_name.Substring(0, $first_dot) }
    }
    if ($folder_name.EndsWith('.tar')) {
        $folder_name = $folder_name.Substring(0, $folder_name.length - 4)
    }

    if (!$name) { $name = $folder_name }

    $lib_src = Join-Path $prefix $folder_name
}
else {
    # fetch package from manifest config
    $lib_src = Join-Path $prefix $name
    $mirror = if (!(Test-Path (Join-Path $PSScriptRoot '.gitee') -PathType Leaf)) { 'github' } else { 'gitee' }
    $url_base = @{'github' = 'https://github.com/'; 'gitee' = 'https://gitee.com/' }[$mirror]

    $manifest_map = ConvertFrom-Json (Get-Content $manifest_file -raw)

    if (!$version) {
        $version_map = $manifest_map.versions
        $version = $version_map.PSObject.Properties[$name].Value
    }
    if ($version) {
        $url_path = $manifest_map.mirrors.PSObject.Properties[$mirror].Value.PSObject.Properties[$name].Value
        if ($url_path) {
            $url = "$url_base/$url_path"
            if (!$url.EndsWith('.git')) { $url += '.git' }
        }
    }
}

if (!$url) {
    throw "fetch.ps1: can't determine package version of '$name'"
}

if (!$version) {
    throw "fetch.ps1: can't determine package url of '$name'"
}

Set-Variable -Name "${name}_src" -Value $lib_src -Scope global

$is_git_repo = $url.EndsWith('.git')
$sentry = Join-Path $lib_src '_1kiss'

$is_rev_modified = $false
# if sentry file missing, re-clone
if (!(Test-Path $sentry -PathType Leaf)) {
    if (Test-Path $lib_src -PathType Container) {
        Remove-Item $lib_src -Recurse -Force
    }

    if ($url.EndsWith('.tar.gz')) {
        $out_file = Join-Path $cache_dir "${folder_name}.tar.gz"
    }
    elseif ($url.EndsWith('.zip')) {
        $out_file = Join-Path $cache_dir "${folder_name}.zip"
    }
    else {
        $out_file = $null
    }

    fetch_repo -url $url -out $out_file
    
    if (Test-Path $lib_src -PathType Container) {
        New-Item $sentry -ItemType File 1>$null
        $is_rev_modified = $true
    }
    else {
        throw "fetch.ps1: fetch content from $url failed"
    }
}

# checkout revision for git repo
if (!$revision) {
    $ver_pair = [array]$version.Split('-')
    $use_hash = $ver_pair.Count -gt 1
    $revision = $ver_pair[$use_hash].Trim()
    $version = $ver_pair[0]
}
if ($is_git_repo) {
    $old_rev_hash = $(git -C $lib_src rev-parse HEAD)

    $tag_info = git -C $lib_src tag | Select-String $revision
    if ($tag_info) {
        $revision = ([array]$tag_info.Line)[0]
    }

    $cur_rev_hash = $(git -C $lib_src rev-parse --verify --quiet "$revision^{}")

    if (!$cur_rev_hash) {
        git -C $lib_src fetch
        $cur_rev_hash = $(git -C $lib_src rev-parse --verify --quiet "$revision^{}")
        if (!$cur_rev_hash) {
            throw "fetch.ps1: Could not found commit hash of $revision"
        }
    }

    if ($old_rev_hash -ne $cur_rev_hash) {
        git -C $lib_src checkout $revision 1>$null 2>$null

        $new_rev_hash = $(git -C $lib_src rev-parse HEAD)

        println "fetch.ps1: Checked out to $revision@$new_rev_hash"
        
        if (!$is_rev_modified) {
            $is_rev_modified = $old_rev_hash -ne $new_rev_hash
        }
    }
    else {
        println "fetch.ps1: HEAD is now at $revision@$cur_rev_hash"
    }
}

if ($is_rev_modified) {
    $sentry_content = "ver: $version"

    if ($is_git_repo) {
        $branch_name = $(git -C $lib_src branch --show-current)
        if ($branch_name) {
            # track branch
            git -C $lib_src pull
            $commits = $(git -C $lib_src rev-list --count HEAD)
            $sentry_content += "`nbranch: $branch_name"
            $sentry_content += "`ncommits: $commits"
            $revision = $(git -C $lib_src rev-parse --short=7 HEAD)
            $sentry_content += "`nrev: $revision"
        }
    }

    [System.IO.File]::WriteAllText($sentry, $sentry_content)

    git -C $lib_src add '_1kiss'
}

# google gclient spec
if (Test-Path (Join-Path $lib_src '.gn') -PathType Leaf) {
    # the repo use google gn build system manage deps and build
    Push-Location $lib_src
    # angle (A GLES native implementation by google)
    if (Test-Path 'scripts/bootstrap.py' -PathType Leaf) {
        python scripts/bootstrap.py
    }
    # darwin (A WebGPU native implementation by google)
    if (Test-Path 'scripts/standalone.gclient' -PathType Leaf) {
        Copy-Item scripts/standalone.gclient .gclient -Force
    }
    gclient sync -D
    Pop-Location
}
