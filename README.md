# aurorae
yet another renderer project

# Build

* Install Vulkan SDK, at least for version 1.3:

https://vulkan.lunarg.com/

* Get repo

```
git clone https://github.com/vug/aurorae.git
git submodule update --init --recursive
```

* Build dependencies via following:

```
cd superbuild\
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build
```

* Build app

```
cd src\
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```