# rm -rf build/

CXX=clang++-20 cmake -S . -B build  -DCMAKE_BUILD_TYPE=Release

cmake --build build
