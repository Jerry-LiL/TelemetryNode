.PHONY: all test clean

all:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
	cmake --build build -j

test: all
	ctest --test-dir build --output-on-failure

clean:
	rm -rf build
