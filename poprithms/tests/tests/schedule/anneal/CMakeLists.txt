add_poprithms_unit_test(schedule_anneal_bifurcate bifurcate.cpp D 5)
add_poprithms_unit_test(schedule_anneal_grid grid.cpp N 5)
add_poprithms_unit_test(schedule_anneal_test_random test_random.cpp)
add_poprithms_unit_test(schedule_anneal_recompute recompute.cpp N 50 type sqrt)
add_poprithms_unit_test(schedule_anneal_random random.cpp N 100 E 5 D 10 graphSeed 1011)
add_poprithms_unit_test(schedule_anneal_attractions attractions.cpp)
add_poprithms_unit_test(schedule_anneal_tight_pairs tight_pairs.cpp)
add_poprithms_unit_test(schedule_anneal_op_0 op_0.cpp)
add_poprithms_unit_test(schedule_anneal_empty_0 empty_0.cpp)
add_poprithms_unit_test(schedule_anneal_graph_basics_0 graph_basics_0.cpp)
add_poprithms_unit_test(schedule_anneal_inputs inputs.cpp)
add_poprithms_unit_test(schedule_anneal_allocweight_0 allocweight_0.cpp)
add_poprithms_unit_test(schedule_anneal_links_0 links_0.cpp)
add_poprithms_unit_test(schedule_anneal_links_1 links_1.cpp)
add_poprithms_unit_test(schedule_anneal_links_2 links_2.cpp)
add_poprithms_unit_test(schedule_anneal_unisort unisort.cpp)
add_poprithms_unit_test(schedule_anneal_get_merged_0 get_merged_0.cpp)
add_poprithms_unit_test(schedule_anneal_is_schedulable schedulable.cpp)
add_poprithms_unit_test(schedule_anneal_is_search_limits searchlimits.cpp)
add_poprithms_unit_test(schedule_anneal_diamond_0 diamond_0.cpp N 19)

add_poprithms_unit_test(schedule_anneal_pmo_link_close_tight_pairs_0
                                        pmo_link_close_tight_pairs_0.cpp)
add_poprithms_unit_test(schedule_anneal_pmo_link_tight_drops_0
                                        pmo_link_tight_drops_0.cpp)
add_poprithms_unit_test(schedule_anneal_pmo_link_tight_drops_1
                                        pmo_link_tight_drops_1.cpp)
add_poprithms_unit_test(schedule_anneal_pmo_constrain_weight_separated_0
                                        pmo_constrain_weight_separated_0.cpp)
add_poprithms_unit_test(schedule_anneal_pmo_constrain_weight_separated_1
                                        pmo_constrain_weight_separated_1.cpp)
add_poprithms_unit_test(schedule_anneal_pmo_constrain_weight_separated_2
                                        pmo_constrain_weight_separated_2.cpp)
add_poprithms_unit_test(schedule_anneal_pmo_constrain_weight_separated_3
                                        pmo_constrain_weight_separated_3.cpp)
add_poprithms_unit_test(schedule_anneal_pmo_constrain_parallel_chains_0
                                        pmo_constrain_parallel_chains_0.cpp)