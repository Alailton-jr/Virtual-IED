
PACKAGES = \
    cmake \
    build-essential \
    libssl-dev \
    libboost-all-dev \
    libpthread-stubs0-dev \
    ninja-build \
    nlohmann-json3-dev \
    libfftw3-dev

BUILD_DIR = build
CMAKE_PRESET = Main 

all: install configure build

install:
	@echo "Installing required packages..."
	sudo apt update
	sudo apt install -y $(PACKAGES)

configure:
	@echo "Configuring the CMake project..."
	cmake --preset $(CMAKE_PRESET)

build:
	@echo "Building the project..."
	cmake --build $(BUILD_DIR)

clean:
	@echo "Cleaning up build files..."
	rm -rf $(BUILD_DIR)

.PHONY: install
