## Build instructions

1. Firstly install `conan`: https://conan.io/
```
python3 -m venv venv
pip install conan
```
2. When building for the first time, run `make prepare-release`
3. Run `make release`
4. Tests could be ran using `make release-test`

See `Makefile` for more info.
