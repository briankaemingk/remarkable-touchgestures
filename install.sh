#!/bin/sh
#to be run on the device
wget https://github.com/briankaemingk/remarkable-touchgestures/releases/download/0.0.5/touch.tgz -O - | tar xvf -
cp /home/root/touch.service /etc/systemd/system
systemctl enable touch
systemctl start touch
