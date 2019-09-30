#!/bin/bash

./orcs -t ../tests/astar.CINT.PP200M > saida/saida_astar
./orcs -t ../tests/calculix.CFP.PP200M > saida/saida_calculix
./orcs -t ../tests/dealII.CFP.PP200M > saida/saida_dealII
./orcs -t ../tests/gromacs.CFP.PP200M > saida/saida_gromacs
./orcs -t ../tests/libquantum.CINT.PP200M > saida/saida_libquantum
./orcs -t ../tests/namd.CFP.PP200M > saida/saida_namd
