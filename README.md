# Pseudo Character Driver for Multiple Devices

This repository contains a Linux kernel module that implements a pseudo character device driver for managing multiple devices. The driver supports up to four devices, each with different read and write permissions. The devices can be read from and written to, with different access control depending on the device configuration. The driver provides basic file operations like `read`, `write`, `open`, and `close`.

## Features

- Supports up to **4 devices** with different read-write permissions:
  - `PCDEV1`: Read-only
  - `PCDEV2`: Write-only
  - `PCDEV3` and `PCDEV4`: Read-write
- Implements basic file operations for each device:
  - **Read** from the device buffer
  - **Write** to the device buffer
  - **Seek** (to change file offset)
  - **Open** and **Release** device file handles
- Provides customizable memory sizes for each device (1 KB each in this implementation)
- **Error handling** for invalid read/write access based on device permissions
- **CI Test Integration**: Continuous Integration testing is set up to validate the kernel module and its functionality (to be added).

## Getting Started

### Prerequisites

1. Linux system with kernel development tools installed.
2. Kernel source code or headers matching your running kernel version.
3. Basic understanding of Linux kernel modules.

### Installation

1. Clone the repository to your local machine:
   ```bash
   git clone https://github.com/alessandro-scalambrino/multiple-dn-driver.git
   cd multiple-dn-driver
   ```

2. Build the kernel module:
   ```bash
   make all
   ```

3. Load the kernel module:
   ```bash
   sudo insmod multiple-dn-driver.ko
   ```

### Verify the Devices are Created

The devices will be available under `/dev/pcdev-*`. You can list them using:

```bash
ls /dev/pcdev-*
```

### To Unload the Module

```bash
sudo rmmod pcd_driver
```

## Usage

You can interact with the devices using standard Linux file operations. For example:

- **Read from a device:**

```bash
cat /dev/pcdev-0
```

- **Write to a device:**

```bash
echo "Test Data" > /dev/pcdev-1
```

## Permissions

- **PCDEV1**: Read-only access (`RDONLY`).
- **PCDEV2**: Write-only access (`WRONLY`).
- **PCDEV3** and **PCDEV4**: Read-write access (`RDWR`).

Accessing the devices with incorrect permissions will result in a failure message.

## Kernel Driver Details

The kernel module defines multiple operations and behaviors for handling multiple devices:

### File Operations:

- **open**: Initializes the device file and validates permissions.
- **read**: Copies data from the device buffer to user space.
- **write**: Copies data from user space to the device buffer.
- **llseek**: Changes the file offset.
- **release**: Releases the device file when done.

### Permission Checks:

Each device has different permissions, and accessing a device in an unsupported mode (e.g., read from a write-only device) results in an error.

### Memory Management:

Each device has a buffer with a defined size that limits read and write operations.

## Kernel Functions

- **pcd_llseek**: Implements seeking to a new file offset.
- **pcd_read**: Handles reading data from the device buffer.
- **pcd_write**: Handles writing data to the device buffer.
- **check_permission**: Ensures that access modes match the device's permissions.

## Testing

### Continuous Integration (CI)

CI testing will be added to ensure the kernel module behaves correctly and passes necessary tests, such as:

- Validating correct memory access (read/write).
- Ensuring permission enforcement (read-only, write-only, and read-write).
- Testing the device creation and cleanup processes.

This project was developed by Alessandro Scalambrino.
