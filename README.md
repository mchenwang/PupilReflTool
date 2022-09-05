# Pupil Reflection Tool

Reflection code generator for Pupil Reflection.

## Feature

- Parse syntax tree to get the meta information.
- Automatically generate corresponding reflection code (for Pupil Reflection).
- Custom annotation for UI or other purposes:
  - meta: Identify the fields that need to be reflected.
  - range: Determine the variable range of the field.
  - info: Brief description.
  - step: Step size of variable increase or decrease.
  - ...

## Tested Environment

- Windows 10
- Visual Studio 2019
- MSVC 19.29.30146.0
- CMake 3.22.0
- C++ 17

## Build

1. clone LLVM：`git clone --config core.autocrlf=false https://github.com/mchenwang/llvm-project.git --recursive`
2. checkout branch：`git checkout PupilReflTool`
3. generate：
   - turn to project root directory
   - `mkdir build`
   - `cd build`
   - `cmake -DLLVM_ENABLE_PROJECTS=clang -G "Visual Studio 16 2019" -A x64 -Thost=x64 ..\llvm`
4. build:
   - open LLVM.sln
   - set `PupilReflTool` as startup project(under `Clang executables` directory)
   - make sure that `PupilReflTool` project has enabled Run-Time Type Information(/GR)
     - because `dynamic_cast` is used in the project
   - run build

## Usage

An executable file `PupilReflTool.exe` will be generated after building successfully.

Just copy `PupilReflTool.exe` to your project and run command `PupilReflTool.exe target\path\file.h` before compiling to generate the reflection code for target file.

Take CMake as an example, add the following code to the `CMakeLists.txt`:

```cmake
add_custom_target(PRE ALL
COMMAND ${CMAKE_COMMAND} -E echo "=============== [Precompile] BEGIN "
COMMAND ${PROJECT_ROOT}/tool/PupilReflTool.exe ${REFLECT_FILE}
COMMAND ${CMAKE_COMMAND} -E echo "=============== [Precompile] FINISHED"
)
```



More information about Pupil Reflection: https://github.com/mchenwang/PupilReflect
