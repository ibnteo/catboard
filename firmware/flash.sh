#!/bin/sh
sudo sleep 3
sudo dfu-programmer at90usb162 erase 
sudo dfu-programmer at90usb162 flash catboard.hex
sudo dfu-programmer at90usb162 start
