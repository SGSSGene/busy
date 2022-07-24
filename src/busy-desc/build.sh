#!/usr/bin/bash

g++ -std=c++20 -O0 -ggdb \
    main.cpp -o busy \
    -lyaml-cpp
