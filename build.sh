mkdir build

cd build

cmake ..

cd ..
#pour enlever les erreurs dans vscode avec l'extension clangd
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B build
