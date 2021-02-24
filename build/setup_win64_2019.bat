@echo off

mkdir ga2-win64
cd ga2-win64
cmake -G "Visual Studio 16 2019" ../../src/engine
cd ..
