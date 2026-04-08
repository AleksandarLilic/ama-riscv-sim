/* Common main.c for the benchmarks

   Copyright (C) 2014 Embecosm Limited and University of Bristol
   Copyright (C) 2018-2019 Embecosm Limited

   Contributor: James Pallister <james.pallister@bristol.ac.uk>
   Contributor: Jeremy Bennett <jeremy.bennett@embecosm.com>

   This file is part of Embench and was formerly part of the Bristol/Embecosm
   Embedded Benchmark Suite.

   SPDX-License-Identifier: GPL-3.0-or-later */

#include "support.h"

uint32_t start_time, end_time, clks, time_diff;

void initialise_board (void) {};

#ifdef MHPM
tda_cnt_t pe = {0ul};
#endif

void start_trigger (void) {
  printf("Running test...");

  #ifdef MHPM
  init_tda_counters();
  #else
  set_cpu_cycles(0u);
  #endif

  start_time = get_cpu_time();
  //PROF_START;
}

void stop_trigger (void) {
   //PROF_STOP;
   uint32_t end_time = get_cpu_time();

   #ifdef MHPM
   save_tda_counters(&pe);
   print_tda_counters_json(&pe);
   uint32_t clks = pe.cycles;
   #else
   uint32_t clks = get_cpu_cycles();
   #endif

   uint32_t time_diff = (end_time - start_time) / 1000;
   printf("Ran for %d cycles and %d ms\n", clks, time_diff);
}

int __attribute__ ((used))
main (int argc __attribute__ ((unused)),
      char *argv[] __attribute__ ((unused)))
{
  int i;
  volatile int result;
  int correct;

  initialise_board ();
  initialise_benchmark ();
  warm_caches (WARMUP_HEAT);

  start_trigger ();
  result = benchmark ();
  stop_trigger ();

  /* bmarks that use arrays will check a global array rather than int result */

  correct = verify_benchmark (result);

  if (correct) {
    pass();
  } else {
    // correct value unavailable here from verify_benchmark
    write_mismatch(result, 0, 1);
    fail();
  }

}				/* main () */


/*
   Local Variables:
   mode: C
   c-file-style: "gnu"
   End:
*/
