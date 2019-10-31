## Building for Windows

On Windows, assuming Visual Studio 2019 is installed, it can be built in the
following way:
```bash
# Clone it down.
git clone https://github.com/target/libdart.git
cd libdart

# Create the cmake build directory and prepare a build
# with tests enabled
mkdir build
cd build

# Windows doesn't have standardized directories for storing
# system headers like Linux/macOS, and so you need to provide
# the path to the GSL installation to use.
# If performing this step in Visual Studio, it can be configured
# graphically.
cmake .. -DCMAKE_INCLUDE_PATH="C:\Path\To\Guidelines\Support\Library\include"
cmake --build . --config Release
ctest -C Release

# We've tested things, now install the library itself.
# The install location can be customized using
# -DCMAKE_INSTALL_PREFIX
cmake --install . --config Release
```
