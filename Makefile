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

all: install build

install:
	@echo "Installing required packages..."
	sudo apt update
	sudo apt install -y $(PACKAGES)

build:
	@if [ -d build ]; then echo "Removing existing build folder"; rm -rf build; fi
	@echo "Building the project..."
	cmake -DCMAKE_INSTALL_PREFIX=$(BUILD_DIR)/install/Main -S. -B$(BUILD_DIR) -G Ninja
	cmake --build $(BUILD_DIR) --target vIED

clean:
	@echo "Cleaning up build files..."
	rm -rf $(BUILD_DIR)

.PHONY: install build clean
