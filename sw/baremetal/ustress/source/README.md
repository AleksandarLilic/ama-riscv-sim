> [!NOTE]
> Cloned from https://gitlab.arm.com/telemetry-solution/telemetry-solution/-/tree/main/tools/ustress
> Changes are made to workloads used for this project, guarding with `#ifdef`
> The writeup below is for the source repo only

# UStress Validation Suite

UStress is a validation suite comprising a set of micro-architecture workloads that stress some of the major CPU resources, like branch prediction units, execution units (arithmetic and memory), caches, and TLBs.

**These workloads are not intended to be used as benchmarks to compare peformance**, but instead to trigger various performance bottleneck scenarios in the CPU.

Some of these workloads also vary by micro-architecture, and as such, results may not be directly comparable between CPUs.

## Supported CPUs

Some of the included workloads require micro-architectural information for the target CPU in order to function.

This information is currently provided for the follow CPUs:
* Neoverse N1
* Neoverse V1
* Neoverse N2

To support a new CPU/micro-architecture, add the relevant information to *cpuinfo.h*.

## Workloads

| Category          | Workloads                                                              |
| ---               | ---                                                                    |
| Branch            | branch_direct_workload, branch_indirect_workload, call_return_workload |
| Data Cache        | l1d_cache_workload, l2d_cache_workload                                 |
| Instruction Cache | l1i_cache_workload                                                     |
| Data TLB          | l1d_tlb_workload                                                       |
| Arithmetic Units  | div32_workload, …, fpdiv_workload, …, mul64_workload                   |
| Memory Subsystem  | memcpy_workload, store_buffer_full_workload, load_after_store_workload |

## Building UStress Workloads

### AArch64 GNU/Linux Builds

**Makefile** supports native GCC and LLVM/Clang AArch64 GNU/Linux builds.

The target CPU must be specified via `make CPU=<CPU>`, where supported CPU values are defined in **cpuinfo.h**.
For example, to build UStress for a Neoverse N1 CPU:

```bash
make CPU=neoverse-n1
```

### Windows-On-Arm Builds

**Makefile** supports native LLVM/Clang `target=arm64-pc-windows-msvc` builds.

Cross build: Users may want to open MSVC cross environment on their x64 machine with `vcvarsx86_arm64.bat`. **Makefile** supporting `_WIN32` builds is tested in this environment.

Note: `l1i_cache_workload` and `memcpy_workload` are currently disabled on Windows due to compilation issues.

## Running UStress Workloads

Once compiled, workloads can be run by executing the correspond binary for each workload. E.g.:

```bash
./l2d_cache_workload
```

The iteration count (and therefore runtime) can be increased by specifying the optional `<multiplier>`. For example, to execute `l2d_cache_workload`, increasing the iteration count by 2.5x:
```
./l2d_cache_workload 2.5
```

See `<workload_name> --help` for usage information. E.g.:
```bash
./l2d_cache_workload --help
```
```
usage:
        ./l2d_cache_workload [MULTIPLIER]
        ./l2d_cache_workload [--help]

        MULTIPLIER      Multiply the number of iterations performed by this workload. (Default: 1.0)
```
