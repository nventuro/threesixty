# canvas
This is the display part of the project: the rotating board with the LEDs. This board is based on a [TI Stellaris Launchpad](http://www.ti.com/tool/ek-lm4f120xl), which has an LM4F120H5QR microcontroller.

## Dependencies
I loosely followed [TI's guide](http://processors.wiki.ti.com/index.php/Stellaris_Launchpad_with_OpenOCD_and_Linux) to have the Luanchpad work on Linux. OpenOCD was working fine, and I included the required Stellarisware files in the repo, but MentorGraphics no longers offers a free version of the cross-compiler, so I ended up following [this guide](http://gnuarmeclipse.github.io/toolchain/install/) to install `arm-none-eabi-gcc`. The compiler scripts assume the cross-compiler is located in `/usr/local/`.

`install_toolchain.sh` and `install_openocd.sh`, inside `installers`, can be used to perform these installations automatically.

These are the compiler and OpenOCD versions I tested:
```
$ arm-none-eabi-gcc --version
arm-none-eabi-gcc (GNU Tools for ARM Embedded Processors) 5.4.1 20160609 (release) [ARM/embedded-5-branch revision 237715]
$ openocd --version
Open On-Chip Debugger 0.10.0-dev-00329-gf19ac83
```

## Build instructions
First, StellarisWare must be built:

```
cd stellarisware
./compile.sh
````

Then, the display subproject can be built and downloaded to the target board:

```
cd display
./compile.sh
./download.sh
```
