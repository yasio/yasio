$rel_str = $args[0]

mkdocs --version
mike --version
mike delete --all

Write-Host "Active versions: $rel_str"

$rel_arr = ($rel_str -split ',')

foreach($rel in $rel_arr) {
    git checkout "v$rel"
    mike deploy $rel
}

git checkout dev
mike deploy latest
mike list
mike set-default $rel_arr[$rel_arr.Count - 1] --push
