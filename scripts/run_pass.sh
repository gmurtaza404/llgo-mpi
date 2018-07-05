cd ../build
make
cd ../examples
llgo -emit-llvm -c hw.go -o hw.bc
cd ../scripts
opt -load ../build/src/libSkeletonPass.so -skeleton < ../examples/hw.bc > /dev/null
rm ../examples/hw.bc
