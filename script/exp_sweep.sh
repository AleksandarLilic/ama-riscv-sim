#!/bin/bash

today=$(date +%Y-%m-%d)
run_name="sweep_bp_${today}_all_guided"

#./hw_model_sweep.py -p ./hw_model_sweep_params_bp_guided.json --save_stats --sweep bpred --track --save_sim --bp_top_mpki_num 32 --work_dir "${run_name}" --add_all_workloads --plot_per_workload --bp_run_best_for_all_workloads --silent | tee "${run_name}_2.log"

./hw_model_sweep.py -p ./hw_model_sweep_params_bp_guided_focused.json --save_stats --sweep bpred --track --save_sim --bp_top_mpki_num 48 --work_dir "${run_name}" --add_all_workloads --plot_per_workload --bp_run_best_for_all_workloads --silent | tee "${run_name}_focused.log"
