# gen docs, requires python3.x and install dependencies:
#  pip install -r docs/requirements.tx
# usage: ./scripts/docs.ps1 '3.39.11:v3.39.11,3.39.12:3.39.x'
$rel_str = $args[0]

python -m mkdocs --version
python -m mike --version
python -m mike delete --all

Write-Host "Active versions: $rel_str"

if ("$rel_str" -eq "") {
    Write-Error "docs versions list can't be empty, should be 'docs_ver[:docs_tag],...'"
    exit 1
}

$rel_arr = ($rel_str -split ',')

$docs_ver = $null
foreach($rel in $rel_arr) {
    # ver:tag
    $info = ($rel -split ':')
    $docs_ver = $info[0]
    $docs_tag = if ($info.Count -ge 2) { $info[1] } else { "v$($info[0])" }
    git checkout "$docs_tag"
    python -m mike deploy $docs_ver
}

git checkout dev
python -m mike deploy latest
python -m mike list
python -m mike set-default $docs_ver --push
