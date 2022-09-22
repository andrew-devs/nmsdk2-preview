# Northern Mechatronics Software Development Kit 2.0 (NMSDK2)

## Overview

The NMSDK2 is a platform library for the Northern Mechatronics NM180100 LoRa BLE module. It provides support for LoRa direct, LoRa real-time, LoRaWAN, and BLE wireless connectivity and a FreeRTOS framework for rapid application development across a wide range of use cases and environments. 

## Prerequisites

Northern Mechatronics recommends you complete the following steps when using the NMSDK2:

* [Install Microsoft Visual Studio Code](#install-microsoft-visual-studio-code)
* [Set up GitHub (Optional)](#set-up-github-optional)
* [Download NMSDK2 source code](#download-nmsdk2-source-code)
* [Install recommended VS Code extensions](#install-recommended-vs-code-extensions)
* [Install and Configure MSYS2](#install-and-configure-msys2)
* [Download and Install GNU Arm Toolchain Compiler](#download-and-install-gnu-arm-toolchain-compiler)
* [Download and Configure Python 3](#download-and-configure-python-3)

We recommend installing the following tools to debug your application:

* [Download the J-Link Software and Documentation pack](#download-the-j-link-software-and-documentation-pack)
* [Connect the J-Link Debug Probe to your NM180310 or NM180100EVB](#connect-the-j-link-debug-probe-to-your-nm180310-or-nm180100ev)


## Install Microsoft Visual Studio Code

Microsoft Visual Studio Code (VS Code) is a source-code editor made by Microsoft for Windows, Linux, and macOS. We recommend using it as your integrated development environment (IDE) of choice due to its ease of use. 

To install VS Code:

1. Download the latest version of VS Code from the [Download page](https://code.visualstudio.com/Download/). Choose the correct version for your operating system.
2. On the **Select Additional Tasks** screen of the installation wizard, enable the **Add to PATH (requires shell restart)** checkbox. 

![VS Code Installation with PATH checked](doc/VSCodePath.png)


3. Click the **Install** button. 

![VS Code installation confirmation](doc/VSCodeInstall.png)

4. VS Code should now be installed.


## Set up GitHub (Optional)

For your source control, we recommend using GitHub. It works seamlessly with the command line interface (CLI) offered through VS Code. It also has a desktop version available for download, offering a feature-rich user interface that simplifies version control for those less familiar with git commands.

Get the latest version of GitHub Desktop from their [Downloads page](https://desktop.github.com/).


### VS Code and GitHub Integration
Once GitHub is downloaded, perform these steps to enable it in VS Code:

1. Make sure you have already created your account on [GitHub](https://github.com/join).
2. Open VS Code.
3. Click on **Settings** or the gear icon.
4. Type “Git: Enabled” in the search bar.
5. Ensure the **Git: Enabled** checkbox is enabled: 

![Git Enabled](doc/VSCodePath.png)

## Download NMSDK2 source code
Once you have your preferred IDE and source control in place, it’s time to access the NMSDK2 source code.

1. Navigate to the main page of the [NMSDK2 repository](https://github.com/NorthernMechatronics/nmsdk2-preview).
2. Above the list of files, click **Code**.

![Clone Repository button](doc/CloneRepo.png)

3. Copy the URL for the repository.

![GitHub Copy Repository URL](doc/CloneRepo2.png)

4. Open VS Code.
5. Open the command palette with the key combination of Ctrl + Shift + P.
6. At the command palette prompt, enter `gitcl`, select the Git: Clone command, and press Enter.
7. When prompted for the **Repository URL**, select clone from GitHub, then press Enter.
8. If you are asked to sign into GitHub, complete the sign-in process.
9. Enter the copied URL for the repository in the **Repository URL** field.
10. Select (or create) the local directory into which you want to clone the project.
11. When you receive the notification asking if you want to open the cloned repository, select Open.


## Install Git
After opening the repository folder in VS Code for the first time, you may be prompted to install Git.

![Dowload Git Prompt](doc/DowloadGitPrompt.png)

To download and install Git, follow these steps:

1. Click the **Download Git** button, or go directly to the [Git Downloads](https://git-scm.com/downloads) page.
2. Download the 62-bt or 32-bit version of Git depending on your operating system.
3. Select VS Code as the Git’s default editor

![Git VS Code Selected](doc/GitVSCodeSelected.png)

4. Leave all other settings as default for the rest of the install options.

## Install recommended VS Code extensions
VS Code extensions let you add languages, debuggers, and tools to your installation to support your development workflow. We recommend installing several extensions. To view the recommended VS Code extensions, follow these steps:

1. Open VS Code.
2. Click on **File** in the menu bar, then **Open Folder**.

![File menu with Open Folder option](doc/RecommendedExtensions0.png)

3. Open the folder that contains your local version of the project.

![Open Folder](doc/RecommendedOpenFolder.png)

4. When the project is opened, you should see the following pop-up message:

![Recommended extension popup](doc/RecommendedPopup.png)

If no pop-up appears, follow these instructions to install our recommended extensions:
1. Open the downloaded [NMSDK2 source code](#download-nmsdk2-source-code) repository folder in VS Code.
2. Click the **Gear icon** in the bottom-left of VS Code.
3. Select **Extensions**.
4. In the search bar that shows the prompt **Search Extensions in Marketplace**, enter the text “@recommended”. 

![Recommended Extensions](doc/RecommendedExtensions.png)

5. Download all of the extensions that appear under the **Workplace Recommendations** heading. 

![Recommended Extensions List](doc/RecommendedExtensions2.png)

If you cannot view these items listed in your workspace, manually install the following extensions:
* C/C++
* C/C++ Extensions
* C/C++ Themes
* Makefile Tools
* Cortex-Debug
* LinkerScript

## Install and Configure MSYS2

MSYS2 is a collection of tools and libraries providing you with an easy-to-use environment for building, installing, and running native Windows software.
Install MSYS2 on your machine by following the [MSYS2 Getting Started guide](https://www.msys2.org/).
Verify the installation was successful by executing the following command in your terminal:

```
$ gcc --version
```

### Adding MSYS2 to your PATH variable

To make the MYSYS2 commands available everywhere, you need to add them to the “path” variable. The following instructions are for Windows 10 machines. Add the MYSYS2 executable to your PATH:

1. In the Start menu search box, type the text “edit the system environment variables”.
2. Click the **Environment Variables…** button.
3. Click the **New…** button.
4. Click Browse Directory and add the following location of the `bin` folder under the `usr `directory to your path. The location will depend on where you chose to [install and configure MSYS2](#install-and-configure-msys2). 
    1. The default location for Windows 64-bit systems is: `C:\msys64\usr\bin`. 
    2. The default location for Windows 32-bit systems is: `C:\msys32\usr\bin`.

![MSYS2 Path highlighted](doc/MSYS2%20Path.png)

## Download and Install GNU Arm Toolchain Compiler

The GNU Arm Embedded Toolchain is a ready-to-use, open-source suite of tools for C, C++, and assembly programming. We recommend downloading the compiler for use with our SDK.

Download and install GNU Arm Toolchain version 11.3.Rel1:

1. Navigate to the [Arm Downloads page](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) and download version 11.3.Rel1.
2. Download the following version: arm-gnu-toolchain-11.3.rel1-mingw-w64-i686-arm-none-eabi.exe
3. Add the compiler to your PATH using the **Add path to environment variable** checkbox.

![Arm Compiler Path v11](doc/ArmCompilerPathv11.png)

## Download and Configure Python 3

Python is an interpreted, object-oriented, high-level programming language with dynamic semantics. Python is required for the support scripts that come with the SDK.

To install Python:

1. Download the latest version of Python 3 from the [Downloads page](https://www.python.org/downloads/windows/). 
2. User the Installation Wizard to add Python 3 to your PATH. 

![Python Installer](doc/PythonInstaller.png)

3. Verify that the installation was successful by running the following command in your terminal:

```
	python --version
```

## Complete first-time build of the NMSDK2
With the build [prerequisites](#prerequisites)in place, it is time to perform a build of the SDK.

To build the SDK for the first time:

1. Open VS Code.
2. Click on **Terminal** in the menu bar.
3. Click on **Run Build Task…**
4. Select **build debug**. 

![Run Build Debug](doc/RunBuildDebug.png)

5. The output of this operation should resemble the following image:

![Run Build DebugOutput](doc/RunBuildDebugOutput.png)

## Debugging

SEGGER J-Links are the most widely used line of debug probes on the market. These Debuggers can communicate at high speed with a large number of supported target CPU cores. 

To successfully debug your application using the SEGGER J-Link tools, follow these steps:

1. [Download the J-Link Software and Documentation pack](#download-the-j-link-software-and-documentation-pack).
2. [Connect the J-Link debug probe to your NM180310 or NM180100EVB](#connect-the-j-link-debug-probe-to-your-nm180310-or-nm180100ev).
3. [Debug in VS Code](#debug-in-vs-code).

### Download the J-Link Software and Documentation pack
Download the official J-Link Software and Documentation pack. The exact version will depend on your operating system. Download the version that applies to you from the [J-Links Download page](https://www.segger.com/downloads/jlink/).

### Connect the J-Link Debug Probe to your NM180310 or NM180100EV
When you have acquired the J-Link Debug Probe, and downloaded the associated J-Link debugging software, it is time to connect the J-Link debugger and your NM180310 or NM180100EV.

![JLink Device Manager connected](doc/JLinkDeviceManager.png)

### Debug in VS Code
To start debugging in VS Code:

1. Select **Run** in the menu bar.
2. Click **Start Debugging**. 

![Run start debugging](doc/StartDebugging.png)

3. When debugging has started, your screen should look like the following screenshot. 

![DebuggingStarted](doc/DebuggingStarted.png)

## Troubleshooting
This section contains a list of common errors and issues our users face:

* [Device not connected](#device-not-connected)
* [JLinkGDBServerCL not found](#jlinkgdbservercl-not-found)
* [Python not found](#python-not-found)
* [GNU GCC not found](#gnu-gcc-not-found)
* [Make not found](#make-not-found)

### Device not connected
If you see the following error message of “undefined GDB Server Quit Unexpectedly”, it is because the J-Link debug device is not being detected.

![Device Not Detected](doc/DeviceNotDetected.png)

Follow these steps to resolve the issue:

1. Disconnect the device fully.
2. Wait for at least 10 seconds.
3. Reconnect the device
4. Check the connection has been established in the Device Manager: 

![JLink Device Manager](doc/JLinkDeviceManager.png)

### JLinkGDBServerCL not found

If you see the following error: “Failed to launch undefined GDB Server: Error: spawn C:\Program Files(x86)\SEGGER\JLink\JLinkGDBServerCL ENOENT”, it’s because the JLinkGDBServerCL.exe executable cannot be found.

![JLink GDB Sever Error](doc/JLINKGDBSeverError.png)

To solve this issue, check the JLinkGDBServerCL.exe was successfully installed under the SEGGER > JLink folder:

![JLink GDB Solution](doc/JLINKGDBSolution.png)


### Python not found
You may encounter an error message in your console containing the text “Python was not found”:

![Python Not Found error message](doc/PythonNotFound.png)

Complete the following steps to verify the installation of Python was successful.

1. Check that [Python has been added to your PATH](#download-and-configure-python-3). 

![PythonPath](doc/PythonPath.png)

2. Check the installation folder for the Python executable: 

![Python Installation Folder](doc/PythonInstallationFolder.png)

### GNU GCC not found
You may see this message in the Output tab of your terminal:

![GNU GCC Not Found Output](doc/GNUGCCNotFound1.png)

You may see a similar error in the Terminal tab itself:

![GNU GCC Not Found Terminal](doc/GNUGCCNotFound2.png)

Perform the following checks to rectify the issue:

1. Check the GNU Arm Toolchain Compiler has been added to your Environment Variable path:

![GNU Path](doc/GNUPath.png)

2. Check the bin folder contains the arm-none-eabi-gcc.exe executable: 

![GNU Bin Folder](doc/GNUBinFolder.png)

### Make not found
If you see an error in the terminal containing the text “The term 'make' is not recognized as the name of a cmdlet, function, script file, or operable program”, there may be an issue with your [MSYS2 installation](#install-and-configure-msys2). 

![Make Not Found error](doc/MakeNotFound.png)

Check the following aspects of the installation:

1. Check the bin folder for [MYSY2 has been added to your path](#adding-msys2-to-your-path-variable): 

![MSYS Path Highlighted](doc/MSYSPathHighlighted.png)

2. Check the installation folder contains the make.exe executable: 

![Make Executable](doc/MakeExecutable.png)
