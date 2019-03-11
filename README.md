# jetson-gadget snap

This repository contains the building recipe for a gadget snap for the
Jetson TX1/TX2 devices.

## Build instructions

The recommended build environment is Ubuntu 18.04. To build, first
install snapcraft:

`snap install snapcraft`

Then, to build the snap, if in the target machine:

`SNAPCRAFT_BUILD_ENVIRONMENT=host snapcraft`

Otherwise:

`SNAPCRAFT_BUILD_ENVIRONMENT=host snapcraft --target-arch arm64`
