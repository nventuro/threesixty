#!/bin/bash

sudo openocd --file /usr/local/share/openocd/scripts/board/ek-lm4f120xl.cfg --command "program gcc/threesixty-display.axf verify reset exit"
