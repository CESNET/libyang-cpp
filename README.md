# C++ bindings for libyang

![License](https://img.shields.io/github/license/CESNET/libyang-cpp)
[![Gerrit](https://img.shields.io/badge/patches-via%20Gerrit-blue)](https://gerrit.cesnet.cz/q/project:CzechLight/libyang-cpp)
[![Zuul CI](https://img.shields.io/badge/zuul-checked-blue)](https://zuul.gerrit.cesnet.cz/t/public/buildsets?project=CzechLight/libyang-cpp)

*libyang-cpp* implements object-oriented bindings of the [`libyang`](https://github.com/CESNET/libyang) library in modern C++.
Object lifetimes are managed automatically via RAII.

## Dependencies
- [libyang](https://github.com/CESNET/libyang) - the `devel` branch (even for the `master` branch of *libyang-cpp*)
- C++20 compiler (e.g., GCC 10.x+, clang 10+)
- CMake 3.19+

## Building
*libyang-cpp* uses *CMake* for building.
One way of building *libyang-cpp* looks like this:
```
mkdir build
cd build
cmake ..
make
make install
```

## Usage

Check the [test suite in `tests/`](tests/) for usage examples.

## Contributing
The development is being done on Gerrit [here](https://gerrit.cesnet.cz/q/project:CzechLight/libyang-cpp).
Instructions on how to submit patches can be found [here](https://gerrit.cesnet.cz/Documentation/intro-gerrit-walkthrough-github.html).
GitHub Pull Requests are not used.
