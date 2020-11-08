mkdir release
pushd release
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
popd