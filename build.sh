#!/bin/bash
gcc server.c -o server -pthread
gcc client.c -o client -pthread