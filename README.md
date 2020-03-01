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

import "io"			// printf

fn main() {			// void is implicit return type unless otherwise specified
	printf("Hello, World!")	// newlines replace semicolons
}

```

#### Factorial

```cpp
// factorial.gh

import "io"

fn main() {
	int n = 5
	printf("Factorial of %d is %d\n", n, factorial(n) )
}

fn factorial(int n) int {	// Order of function declarations is resolved at compile-time
	if n <= 1 {		// Branches do not need parantheses to contain their expressions
		return 1
	}

	return n * factorial(n - 1)
}

```
