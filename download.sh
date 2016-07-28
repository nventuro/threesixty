#!/bin/bash

sudo openocd --file /usr/local/share/openocd/scripts/board/ek-lm4f120xl.cfg --command "program stellarisware/boards/ek-lm4f120xl/project0/gcc/project0.axf verify reset exit"
