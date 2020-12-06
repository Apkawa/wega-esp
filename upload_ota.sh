#!/bin/bash

ENV=$1
HOST=${2:-$ENV}
pio -c clion run --target upload -e $ENV --upload-port $HOST.local