
PACKAGES = \
    cmake \
    build-essential \
    libssl-dev \
    libboost-all-dev \
    libpthread-stubs0-dev \
    ninja-build \
    nlohmann-json3-dev \
    libfftw3-dev

all: install

install:

	sudo apt update
	sudo apt install -y $(PACKAGES)

.PHONY: install
