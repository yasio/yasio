$rel_str = $args[0]

mkdocs --version
mike --version
mike delete --all

Write-Host "Active versions: $rel_str"

$rel_arr = ($rel_str -split ',')
if ($rel_arr.Count -eq 0) {
    Write-Error "docs versions list can't be empty, should be 'docs_ver[:docs_tag],...'
    exit 1
}

$docs_ver = $null
foreach($rel in $rel_arr) {
    # ver:tag
    $info = ($rel -split ':')
    $docs_ver = $info[0]
    $docs_tag = if ($info.Count -ge 2) { $info[1] } else {$info[0]}
    git checkout "$docs_tag"
    mike deploy $docs_ver
}

git checkout dev
mike deploy latest
mike list
# mike set-default $docs_ver --push
