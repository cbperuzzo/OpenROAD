set script_dir [file dirname [file normalize [info script]]]
cd $script_dir

source "helpers.tcl"

set test_name test_sap_01

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/boundary_push1.def"

set_thread_count 1

saplace_simulated_annealing \
    -iterations_per_T 200 \
    -initial_T 1500 \
    -alpha 0.85 \

set def_file [make_result_file boundary_push1_saplace.def]
write_def $def_file

puts "SAplace completed. Output written to: $def_file"
