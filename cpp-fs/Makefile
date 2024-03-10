prepare:
	CONAN_HOME=$(shell pwd)/build/ conan config install ./settings_user.yml

prepare-debug: prepare
	CONAN_HOME=$(shell pwd)/build/ conan install . --output-folder=build/debug --build=missing \
		--profile:build=./profiles/debug --profile:host=./profiles/debug

prepare-release: prepare
	CONAN_HOME=$(shell pwd)/build/ conan install . --output-folder=build/release --build=missing \
		--profile:build=./profiles/release --profile:host=./profiles/release

prepare-asan: prepare
	CONAN_HOME=$(shell pwd)/build/ conan install . --output-folder=build/asan --build=missing \
		--profile:build=./profiles/asan --profile:host=./profiles/asan

debug:
	cmake -S . -B ./build/debug -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake \
		-DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build ./build/debug

release:
	cmake -S . -B ./build/release -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
	cmake --build ./build/release

asan:
	cmake -S . -B ./build/asan -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=ASAN
	cmake --build ./build/asan

debug-test:
	ctest --test-dir ./build/debug

release-test:
	ctest --test-dir ./build/release

asan-test:
	ctest --test-dir ./build/asan

