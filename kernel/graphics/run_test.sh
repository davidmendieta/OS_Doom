#!/bin/bash

# Set the correct compiler path
export PATH=~gheith/public/cs439/bin:$PATH
export LD_LIBRARY_PATH=~gheith/public/cs439/lib64:$LD_LIBRARY_PATH

# Clean and build
make clean
make

# Run the test
make run 