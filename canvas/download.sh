#!/bin/bash

sudo openocd --file /usr/local/share/openocd/scripts/board/ek-lm4f120xl.cfg --command "program out/canvas.axf verify reset exit"
