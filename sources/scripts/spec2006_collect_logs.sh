#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# source: https://stackoverflow.com/a/7126780/128240
dir_resolve() {
  cd "$1" 2>/dev/null || return $? # cd to desired directory; if fail, quell any error messages but return exit status
  echo "`pwd -P`" # output full, link-resolved path
}

root_dir=`dir_resolve "$DIR/../.."`
spec_dir="$root_dir/vendor/spec2006"
base_dir="$spec_dir/benchspec/CPU2006"

result_dir="$root_dir/spec2006_logs"
if [ -d "$result_dir" ]; then
  echo "Results directory '$result_dir' already exists, cannot continue."
  exit 1
fi

mkdir "$result_dir"

for file_path in `find "$base_dir" -type f -wholename "*/run_base_ref_infra-dangless-malloc.0000/*" -print0 | xargs -0 grep -I -l "\[setup\-report\] begin"`; do
  if [[ $file_path =~ \/CPU2006\/(.*?)\/run\/ ]]; then
    benchmark_name="${BASH_REMATCH[1]}"
    file_name=`basename "$file_path"`

    mkdir -p "$result_dir/$benchmark_name"
    cp "$file_path" "$result_dir/$benchmark_name/$file_name"
  else
    echo "!!!! NO MATCH: $file_path"
  fi
done

#zip -r "$root_dir/spec2006_logs.zip" "$result_dir/*"
echo "Results are in $result_dir"
