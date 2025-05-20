# Use some test cases as CI tests
script_dir=$(cd $(dirname $0);pwd)
cd $script_dir/

check_devel_include() {
    local error_found=0
    cd $script_dir/../
    inc_dir=$(pwd)/include/oeaware
    cmake_file=$(pwd)/CMakeLists.txt

    if [ ! -f "$cmake_file" ]; then
            echo "Error: CMakeLists.txt not found at $cmake_file"
            return 1
    fi

    cmake_content=$(sed 's/#.*//' "$cmake_file") # Remove comments
    cd "$inc_dir" || { echo "Error: inc_dir not found"; return 1; }
    while read -r header; do
        rel_path="${header#./}"
        if echo "$cmake_content" | grep -qF "$rel_path"; then
            echo "[OK] $rel_path is referenced in CMakeLists.txt"
        else
            echo "[ERROR] $rel_path is NOT referenced in CMakeLists.txt"
            error_found=1
        fi
    done < <(find . -type f -name "*.h")

    cd $script_dir/
    return $error_found
}

echo "Checking header files..."
# check oeware header files
if ! check_devel_include; then
    echo "Some header files are not referenced in CMakeLists.txt"
    exit -1
fi
echo "All header files are properly referenced."

# todo: add more test cases
exit 0