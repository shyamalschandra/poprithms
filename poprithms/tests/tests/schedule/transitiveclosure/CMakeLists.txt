add_poprithms_unit_test(schedule_transitiveclosure_correctness_0 
                        correctness_0.cpp)

add_poprithms_unit_test(schedule_transitiveclosure_remove_redundant_0 
                        remove_redundant_0.cpp)

add_poprithms_unit_test(schedule_transitiveclosure_get_unconstrained_0 
                        get_unconstrained_0.cpp)


add_poprithms_unit_test(schedule_transitiveclosure_relative_positions_0 
                        relative_positions_0.cpp)

add_poprithms_unit_test(schedule_transitive_closure_post_0 
                        post_0.cpp)

add_poprithms_unit_test(schedule_transitiveclosure_performance_0 
                        performance_0.cpp
                        N 600 
                        E 5 
                        D 20)

add_poprithms_unit_test(schedule_transitiveclosure_bitset_performance_0 
                        bitset_performance_0.cpp
                        repeat 7
                        )
