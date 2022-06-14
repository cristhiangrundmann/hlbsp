#!/bin/bash

cd "maps"
for entry in *.bsp
do
  ../hlbsp.out "$entry" "../cols/${entry%.*}.col"
done