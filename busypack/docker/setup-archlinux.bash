#!/bin/bash

which pacman
pacman -Suy --noconfirm
pacman -Sy --noconfirm sudo git gcc ccache pacman
