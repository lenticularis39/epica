#!/bin/sh
./epica tests/fact_iter.epica > tests/fact_iter.ll
llc tests/fact_iter.ll
gcc -o tests/fact_iter.test tests/fact_iter.s tests/fact_iter_main.c
