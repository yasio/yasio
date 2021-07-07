mkdocs --version
mike --version
mike delete --all

rel_str=$ACTIVE_VERSIONS
echo Active versions: $rel_str
rel_arr=(${rel_str//,/ })
rel_count=${#rel_arr[@]}
for (( i=0; i<${rel_count}; ++i )); do
  git checkout ${rel_arr[$i]}
  mike deploy ${rel_arr[$i]}
done

git checkout main
mike deploy latest
mike set-default ${rel_arr[$rel_count-1]} --push
