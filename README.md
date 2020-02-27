## ghoul

👻

### Getting started

#### Dependencies

* `posix`
* `cmake` >= 3.5
* `LLVM`
* `g++`
* A version of `clang` or `g++` with support for `C++17` (or greater)

#### Build

```sh
git clone https://github.com/atemmel/ghoul.git
cd ghoul
mkdir build && cd build

# Development version
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
```

### Examples

#### Hello world

```cpp
// hello.gh

extern fn printf(char*) int   // Link with an extern function

fn main() {                   // void as a return type is implicit if no return type is specified
	printf("Hello, World!\n")   // Newlines replace ';'
}
```
