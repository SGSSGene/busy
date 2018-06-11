#!/bin/bash

apt-get update
apt-get install -y sudo apt-transport-https git g++ gcc ccache $1
