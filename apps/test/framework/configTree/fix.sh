#!/bin/bash

ls /opt/legato/configTree -l

sudo rm /opt/legato/configTree/*
sudo cp system.backup /opt/legato/configTree/system.paper

ls /opt/legato/configTree -l

