rm -rf build/

CXX=clang++-20 cmake -S . -B build

cmake --build build
