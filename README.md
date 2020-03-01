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

import "io"

fn main() {
	printf("Hello, World!")
}

```

#### Factorial

```cpp
// factorial.gh

import "io"

fn factorial(int n) int {
	if n <= 1 {
		return 1
	}

	return n * factorial(n - 1)
}

fn main() {
	int n = 5
	printf("Factorial of %d is %d\n", n, factorial(n) )
}

```
