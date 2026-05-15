source helpers.tcl
read_lef src/gpl/test/nangate45.lef
read_lef src/gpl/test/bp_be_top_macro.lef
read_def src/gpl/test/macro01.def

saplace_simulated_annealing
