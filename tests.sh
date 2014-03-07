#!/bin/bash
cmp <(./processDT.exe 512 512 8 16 <input.raw) <(./build/Release/HPC5.exe 512 512 8 16 <input.raw)