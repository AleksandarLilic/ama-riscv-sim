import sys
import os
import numpy as np
import torch
import torch.nn as nn

from brevitas.nn import QuantLinear, QuantReLU
from brevitas.quant import Int8ActPerTensorFloat, Int8WeightPerTensorFloat

BW_W = 8
BW_A = 8

WQ = Int8WeightPerTensorFloat
AQ = Int8ActPerTensorFloat

NIN = 16 # x-dim of the input, square image assumed
FCDIM = 64
BIAS = False

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

if len(sys.argv) < 2:
    print("Usage: python convert_model.py <model.pth>")
    sys.exit(1)

pth_input = sys.argv[1]
if (not os.path.isfile(pth_input)):
    print(f"Error: {pth_input} not found")
    sys.exit(1)

header_out = pth_input.replace(".pth", ".h")
print(f"Converting {pth_input} to {header_out}")

loaded_model = QuantizedMLP()
loaded_model.load_state_dict(torch.load(pth_input), strict=False)

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

        # quantize weights/biases to int8 using SCALE_FACTOR
        data_int8 = np.clip(np.round(data * (scale)), -127, 127).astype(np.int8)

        # write C array format
        flat_data = data_int8.flatten()
        # write number of inputs and outputs
        f.write(f"#define {name.replace('.', '_').upper()}_IN {data_int8.shape[1]}\n")
        f.write(f"#define {name.replace('.', '_').upper()}_OUT {data_int8.shape[0]}\n")
        f.write(f"static const int8_t {name.replace('.', '_')}[] = {{")
        f.write(", ".join(map(str, flat_data)))
        f.write("};\n\n")

    f.write("#endif // MODEL_H\n")
