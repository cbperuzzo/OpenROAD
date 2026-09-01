namespace eval sap {
    sta::define_cmd_args "saplace_simulated_annealing" \
        {
            [-iterations_per_T iterations] [-initial_T temperature] [-alpha alpha]
            [-halo_width halo_width] [-halo_height halo_height]
            [-worst_hpwl]
        }

    proc ::saplace_simulated_annealing {args} {
        sta::parse_key_args "saplace_simulated_annealing" args keys \
            {-iterations_per_T -initial_T -alpha -halo_width -halo_height} \
            flags {-worst_hpwl}

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

        set halo_width 0
        if { [info exists keys(-halo_width)] } {
            set halo_width $keys(-halo_width)
            sta::check_positive_integer "-halo_size" $halo_width
        }

        set halo_height 0
        if { [info exists keys(-halo_height)] } {
            set halo_height $keys(-halo_height)
            sta::check_positive_integer "-halo_size" $halo_height
        }

        set worst_hpwl [info exists flags(-worst_hpwl)]

        sap::saplace_simulated_annealing_simple_cmd $iterations_per_T $initial_T $alpha \
            $halo_width $halo_height $worst_hpwl
    }
}
