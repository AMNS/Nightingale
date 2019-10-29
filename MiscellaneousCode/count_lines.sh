#!/bin/bash
# counts the number of lines in the project
# created by chirgwin on Mon Apr 30 18:28:06 PDT 2012

find src/* -type f -not -name ".*" | xargs wc -l

