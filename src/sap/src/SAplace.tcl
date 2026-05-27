namespace eval sap {
    sta::define_cmd_args "saplace_simulated_annealing" \
        { [-iterations_per_T iterations] [-initial_T temperature] [-alpha alpha] [-halo_size halo_size]}

    proc ::saplace_simulated_annealing {args} {
        sta::parse_key_args "saplace_simulated_annealing" args keys \
            {-iterations_per_T -initial_T -alpha -halo_size }

        set iterations_per_T 400
        if { [info exists keys(-iterations_per_T)] } {
            set iterations_per_T $keys(-iterations_per_T)
            sta::check_positive_integer "-iterations_per_T" $iterations_per_T
        }

        set initial_T 10000000
        if { [info exists keys(-initial_T)] } {
            set initial_T $keys(-initial_T)
            sta::check_positive_float "-initial_T" $initial_T
        }

        set alpha 0.90
        if { [info exists keys(-alpha)] } {
            set alpha $keys(-alpha)
            if { $alpha <= 0.0 || $alpha >= 1.0 } {
                utl::error sap 4 "alpha must be >0 and <1."
            }
        }

        set halo_size 20
        if { [info exists keys(-halo_size)] } {
            set halo_size $keys(-halo_size)
            sta::check_positive_integer "-halo_size" $halo_size
        }

        sap::saplace_simulated_annealing_simple_cmd $iterations_per_T $initial_T $alpha $halo_size
    }
}
