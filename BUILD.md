# Building Aurorae with Separated Dependencies

This project uses a two-stage build process: first build dependencies, then build the application.

## Build Dependencies (One-Time)

Important: AuroraeDependencies is a single STATIC library that exports all individual dependency libraries through a CMake facade. The actual dependency libraries are linked transitively via the facade's target_link_libraries declaration.

Build and install dependencies into the `install/` folder:

```bash
cmake -S dependencies -B build-deps -A x64
cmake --build build-deps --config Release
cmake --install build-deps --prefix install --config Release
```

For Debug build:
```bash
cmake -S dependencies -B build-deps -A x64
cmake --build build-deps --config Debug
cmake --install build-deps --prefix install --config Debug
```

The `install/` folder now contains:
- `lib/AuroraeDependencies.lib` - CMake facade library (small, ~1KB)
- `lib/*.lib` - All individual dependency libraries (glfw3, spdlog, assimp, etc.)
- `include/` - All dependency headers
- `lib/cmake/AuroraeDependencies/` - Config files for find_package (handles transitive linking)

## Build Application

Once dependencies are installed, build the application:

```bash
cmake -S . -B build-app -A x64
cmake --build build-app --config Debug
```

The CMakeLists.txt will automatically find dependencies in the `install/` folder via `find_package(AuroraeDependencies)`.

## Advantages of This Approach

- ✅ Dependencies built once, cached indefinitely
- ✅ Application rebuilds without touching dependencies
- ✅ Clean separation of concerns
- ✅ Can regenerate CMake cache without rebuilding deps
- ✅ Faster iteration on application code
