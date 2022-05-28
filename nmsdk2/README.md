# NMSDK2
License : [![License](https://img.shields.io/badge/license-BSD_3-blue.svg)](http://gitlab.northernmechatronics.com:50250/nmi/software/nmsdk/blob/master/LICENSE)

Platform Details: [![Hardware](https://img.shields.io/badge/hardware-wiki-green.svg)](https://www.northernmechatronics.com/nm180100)


## Introduction
The NMSDK2 is a software development kit for the Northern Mechatronics communication
modules.  It provides libraries for LoRa direct, LoRaWAN, and BLE wireless connectivity.

## Build Instructions
### Pre-requisites
* git
* make
* sed
* python3

### Build
* clone the [NMSDK2](https://github.com/NorthernMechatronics/nmsdk2):
    ```
    git clone https://github.com/NorthernMechatronics/nmsdk2
    ```
* change to the target platform of interest (ex: targets/nm180100):
    ```
    cd nmsdk2
    cd targets/nm180100
    ```
* then enter the following command to build:
    ```
    make
    ```
### Installation
* Once the SDK is built, install the libraries with:
    ```
    make install
    ```
  This will copy all the libraries to the lib directory under the target directory.  In this example, it is under `/targets/nm180100/lib`

## Architecture


## Folder Structure

