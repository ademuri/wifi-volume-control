#!/bin/bash

set -euo pipefail

constants_created=

if [ ! -f "include/constants.h" ]; then
  cp include/constants.sample.h include/constants.h
  constants_created=true
fi

platformio run
if [ "$constants_created" = true ]; then
  rm include/constants.h
fi
