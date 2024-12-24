import sys
import os
import numpy as np
import torch
import torch.nn as nn

from brevitas.nn import QuantLinear, QuantReLU
from brevitas.quant import Int8ActPerTensorFloat, Int8WeightPerTensorFloat

if len(sys.argv) < 2:
    print("Usage: python convert_model.py <model.pth>")
    sys.exit(1)

pth_input = sys.argv[1]
if (not os.path.isfile(pth_input)):
    print(f"Error: {pth_input} not found")
    sys.exit(1)

header_out = pth_input.replace(".pth", ".h")
print(f"Converting {pth_input} to {header_out}")

bitwidth = pth_input.split("_")[1].split(".")[0]

BW_W = int(bitwidth[1])
SUPPORTED_BW_W = [4, 8]
if BW_W not in SUPPORTED_BW_W:
    print(f"ERROR: BW_W is {BW_W}. Supported values are {SUPPORTED_BW_W}")
    sys.exit(1)

BW_A = int(bitwidth[3])
SUPPORTED_BW_A = 8
if BW_A != SUPPORTED_BW_A:
    print(f"ERROR: BW_A is {BW_A}. Supported values are {SUPPORTED_BW_A}")
    sys.exit(1)

WQ = Int8WeightPerTensorFloat
AQ = Int8ActPerTensorFloat

NIN = 16 # x-dim of the input, square image assumed
FCDIM = 64
BIAS = False

dims = pth_input.split("_")[2].split(".")[0]
dims = dims.split("-")
dims = [int(x) for x in dims]
SUPPORTED_DIM = [FCDIM, FCDIM, FCDIM, 10] # only support this for now
if not dims == SUPPORTED_DIM:
    print(f"ERROR: Unsupported dimensions {dims}. " + \
          f"Supported dimensions are {SUPPORTED_DIM}")
    sys.exit(1)

class QuantizedMLP(nn.Module):
    def __init__(self):
        super(QuantizedMLP, self).__init__()
        self.flatten = nn.Flatten()
        self.fc1 = QuantLinear(NIN*NIN, FCDIM, weight_bit_width=BW_W, bias=BIAS, weight_quant=WQ)
        self.relu1 = QuantReLU(bit_width=BW_A, act_quant=AQ)
        self.fc2 = QuantLinear(FCDIM, FCDIM, weight_bit_width=BW_W, bias=BIAS, weight_quant=WQ)
        self.relu2 = QuantReLU(bit_width=BW_A, act_quant=AQ)
        self.fc3 = QuantLinear(FCDIM, FCDIM, weight_bit_width=BW_W, bias=BIAS, weight_quant=WQ)
        self.relu3 = QuantReLU(bit_width=BW_A, act_quant=AQ)
        self.fc_last = QuantLinear(FCDIM, 10, weight_bit_width=BW_W, bias=BIAS, weight_quant=WQ)

    def forward(self, x):
        x = self.flatten(x)
        x = self.fc1(x)
        x = self.relu1(x)
        x = self.fc2(x)
        x = self.relu2(x)
        x = self.fc3(x)
        x = self.relu3(x)
        x = self.fc_last(x)
        return x

loaded_model = QuantizedMLP()
loaded_model.load_state_dict(torch.load(pth_input), strict=False)

nn_size_bytes = 0
SCALE_FACTOR = 2**(BW_W-1) - 1
with open(f"{header_out}", "w") as f:
    f.write("#ifndef MODEL_H\n")
    f.write("#define MODEL_H\n\n")

    for name, param in loaded_model.named_parameters():
        if "weight" not in name: # and "bias" not in name:
            #print(f"Skipping {name}")
            continue
        print(f"Processing {name}")

        # convert to numpy array
        data = param.detach().cpu().numpy()
        scale_max = np.max(np.abs(data))
        scale = SCALE_FACTOR / scale_max
        #print("    max", data.max(), "min", data.min(), end="; ")
        #print("scale_max:", scale_max.round(2), end="; ")
        #print("scale:", scale.round(2))

        # quantize weights/biases using SCALE_FACTOR
        data = np.clip(np.round(data * (scale)), -SCALE_FACTOR, SCALE_FACTOR) \
               .astype(np.int8)

        # write C array format
        flat_data = data.flatten()
        if BW_W == 4:
            # pack 2 4-bit values into a single byte
            flat_data = [(flat_data[i] & 0xf) | ((flat_data[i+1] & 0xf) << 4)
                         for i in range(0, len(flat_data), 2)]
            # cast to ensure it's int8
            flat_data = [np.int8(x) for x in flat_data]

        nn_size_bytes += len(flat_data)
        # write number of inputs and outputs
        layer_name = name.replace('.', '_')
        f.write(f"#define {layer_name.upper()}_IN {data.shape[1]}\n")
        f.write(f"#define {layer_name.upper()}_OUT {data.shape[0]}\n")
        f.write(f"static const int8_t {layer_name}[] = {{")
        f.write(", ".join(map(str, flat_data)))
        f.write("};\n\n")

    f.write("#endif // MODEL_H\n")

print(f"NN size {nn_size_bytes/1024:.2f} KB")
