#!/bin/bash

today=$(date +%Y-%m-%d)
#hw="icache"
#hw="dcache"
hw="bp"
tag="_all_guided_focused"
run_name="sweep_${hw}_${today}_${tag}"

# icache runs
#./hw_model_sweep.py -p ./hw_model_sweep_params_caches_dev.json --save_stats --sweep icache --track --work_dir "${run_name}" --add_all_workloads --plot_per_workload --silent | tee "${run_name}.log"

# branch predictor runs
#./hw_model_sweep.py -p ./hw_model_sweep_params_bp_guided.json --save_stats --sweep bpred --track --save_sim --bp_top_mpki_num 32 --work_dir "${run_name}" --add_all_workloads --plot_per_workload --bp_run_best_for_all_workloads --silent | tee "${run_name}.log"

./hw_model_sweep.py -p ./hw_model_sweep_params_bp_guided_focused.json --save_stats --sweep bpred --track --save_sim --bp_top_mpki_num 48 --work_dir "${run_name}" --add_all_workloads --plot_per_workload --bp_run_best_for_all_workloads --silent | tee "${run_name}.log"
