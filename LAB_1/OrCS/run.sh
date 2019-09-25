#!/bin/bash

./orcs -t disposable/astar/astar.CINT.PP200M > saida/saida_astar
./orcs -t disposable/calculix/calculix.CFP.PP200M > saida/saida_calculix
./orcs -t disposable/dealII/dealII.CFP.PP200M > saida/saida_dealII
./orcs -t disposable/gromacs/gromacs.CFP.PP200M > saida/saida_gromacs
./orcs -t disposable/libquantum/libquantum.CINT.PP200M > saida/saida_libquantum
./orcs -t disposable/namd/namd.CFP.PP200M > saida/saida_namd