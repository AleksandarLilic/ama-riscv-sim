#!/usr/bin/env python3

import os
import sys
import numpy as np
import matplotlib.pyplot as plt

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from codegen_common import *

if len(sys.argv) != 2:
    print(f"Usage: python3 codegen.py <ARR_LEN>")
    sys.exit(1)

ARR_LEN = int(sys.argv[1])
OUT = f"test_arrays.h"

random.seed(0)

code = []
code.append("#pragma once\n")
code.append("#include <stdint.h>\n")
code.append(f"#define ARR_LEN {ARR_LEN}")

# temp: deterministic cycle through buckets (<25,<50,>=50)
# repeats: 20C for a while, 35C for a while, 60C for a while
temp_cycle = np.array([20, 35, 60], dtype=np.int16)
temp_c = np.repeat(temp_cycle, (ARR_LEN + 2)//3)[:ARR_LEN]

# flags: mostly valid + deterministic invalid bursts
flags = np.ones(ARR_LEN, dtype=np.uint8)
burst_every = 96
burst_len = 16
for s in range(200, ARR_LEN, burst_every):
    flags[s:min(ARR_LEN, s + burst_len)] = 0

value = NUM["int16_t"]

# "common-ish" ranges in raw LSB
# (kept conservative so squares don't instantly clip)
# accel ~ +/- 1g worth of LSB (for +/-2g full-scale, 1g ~ 16384)
# gyro  ~ +/- 2000 LSB arbitrary (fits common quiet-ish rates)
value['ax'] = rnd_gen_1d_arr(-16000, 16000, ARR_LEN, value["nf"])
value['ay'] = rnd_gen_1d_arr(-13000, 13000, ARR_LEN, value["nf"])
value['az'] = rnd_gen_1d_arr(-16000, 16000, ARR_LEN, value["nf"])
value['gx'] = rnd_gen_1d_arr(-2000, 2000, ARR_LEN, value["nf"])
value['gy'] = rnd_gen_1d_arr(-2000, 2000, ARR_LEN, value["nf"])
value['gz'] = rnd_gen_1d_arr(-1000, 2000, ARR_LEN, value["nf"])

# make invalid samples obvious, just easier to spot
bad = (flags == 0)
value['ax'][bad] = value['ay'][bad] = value['az'][bad] = 0
value['gx'][bad] = value['gy'][bad] = value['gz'][bad] = 0

ctype = np2c_type(value["nf"])
code.append("#define NF " + ctype)

code.append(np2c_1d_arr('ax', value['ax']))
code.append(np2c_1d_arr('ay', value['ay']))
code.append(np2c_1d_arr('az', value['az']))
code.append(np2c_1d_arr('gx', value['gx']))
code.append(np2c_1d_arr('gy', value['gy']))
code.append(np2c_1d_arr('gz', value['gz']))
code.append(np2c_1d_arr('temp', temp_c))
code.append(np2c_1d_arr('flags', flags, nf="int8_t"))

finish_gen(code, OUT, add_assert=False)

# save png for reference
fig, axs = plt.subplots(4, figsize=(16, 10))

axs[0].set_title("Temperature")
axs[0].plot(temp_c)

axs[1].set_title("Flags")
axs[1].plot(flags)

axs[2].set_title("Accelerometer")
axs[2].plot(value['ax'], linewidth=0.3)
axs[2].plot(value['ay'], linewidth=0.3)
axs[2].plot(value['az'], linewidth=0.3)

axs[3].set_title("Gyro")
axs[3].plot(value['gx'], linewidth=0.3)
axs[3].plot(value['gy'], linewidth=0.3)
axs[3].plot(value['gz'], linewidth=0.3)

for a in axs:
    a.grid()
    a.margins(x=.01)

out = "data.png"
fig.savefig(out, dpi=100)
print(f"Saved chart to: '{out}'")
