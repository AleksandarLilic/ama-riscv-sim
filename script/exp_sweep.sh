#!/bin/bash

#hw="icache" tag=""
#hw="dcache" tag=""
hw="bp"; tag="_all_guided_focused"

today=$(date +%Y-%m-%d)
run_name="sweep_${hw}_${today}${tag}"

# i/dcache runs
#./hw_model_sweep.py -p ./hw_model_sweep_params_caches.json --save_stats --sweep ${hw} --track --work_dir "${run_name}" --add_all_workloads --plot_per_workload --plot_ct_ref none --silent | tee "${run_name}.log"

# branch predictor runs
#./hw_model_sweep.py -p ./hw_model_sweep_params_bp_guided.json --save_stats --sweep bpred --track --save_sim --bp_top_mpki_num 32 --work_dir "${run_name}" --add_all_workloads --plot_per_workload --bp_run_best_for_all_workloads --silent | tee "${run_name}.log"
./hw_model_sweep.py -p ./hw_model_sweep_params_bp_guided_focused.json --save_stats --sweep bpred --track --save_sim --bp_top_mpki_num 48 --work_dir "${run_name}" --add_all_workloads --plot_per_workload --bp_run_best_for_all_workloads --silent | tee "${run_name}.log"
