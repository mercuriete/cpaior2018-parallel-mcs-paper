#!/bin/bash

join -1 2 -2 2 <(grep '^[^#]' $1 | sort -k2 ) <(grep '[^#]' $2 | sort -k2 ) | cut -d' ' -f1,2,4 | sort -n -k3,3
