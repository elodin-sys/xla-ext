# XLA

Precompiled [XLA](https://github.com/openxla/xla) static libraries.

#### `XLA_TARGET`

The default value is `cpu`, which implies the final the binary supports targeting
only the host CPU.

| Value | Target environment |
| --- | --- |
| cpu | |
| tpu | libtpu |
| cuda120 | CUDA 12.1+, cuDNN 8.9+ |
| cuda118 | CUDA 11.8+, cuDNN 8.6+ |
| cuda | CUDA x.y, cuDNN (building from source only) |
| rocm | ROCm (building from source only) |

To use XLA with NVidia GPU you need [CUDA](https://developer.nvidia.com/cuda-downloads)
and [cuDNN](https://developer.nvidia.com/cudnn) compatible with your GPU drivers.
See [the installation instructions](https://docs.nvidia.com/deeplearning/cudnn/install-guide/index.html)
and [the cuDNN support matrix](https://docs.nvidia.com/deeplearning/cudnn/support-matrix/index.html)
for version compatibility. To use precompiled XLA libraries specify a target matching
your CUDA version (like `cuda118`). You can find your CUDA version by running `nvcc --version`
(note that `nvidia-smi` shows the highest supported CUDA version, not the installed one).
When building from source it's enough to specify `cuda` as the target.

Note that all the precompiled libraries assume glibc 2.31 or newer.

##### Notes for ROCm

For GPU support, we primarily rely on CUDA, because of the popularity and availability
in the cloud. In case you use ROCm and it does not work, please open up an issue and
we will be happy to help.

When you encounter errors at runtime, you may want to set `ROCM_PATH=/opt/rocm-5.7.0`
and `LD_LIBRARY_PATH="/opt/rocm-5.7.0/lib"` (with your respective version). For further
issues, feel free to open an issue.

## Building from source

Type `make`.

You will need the following installed in your system for the compilation:

  * [Git](https://git-scm.com/) for fetching XLA source
  * [Bazel v7.4.1](https://bazel.build/) for compiling XLA
  * [Python3](https://python.org) with NumPy installed for compiling XLA

### GPU support

To build binaries with GPU support, you need all the GPU-specific dependencies (CUDA, ROCm),
then you can build with either `XLA_TARGET=cuda` or `XLA_TARGET=rocm`. See the `XLA_TARGET`
for more details.

### TPU support

All you need is setting `XLA_TARGET=tpu`.

## Runtime flags

You can further configure XLA runtime options with `XLA_FLAGS`,
see: [xla/debug_options_flags.cc](https://github.com/openxla/xla/blob/main/xla/debug_options_flags.cc)
for the list of available flags.

## Release process

To publish a new version of this package:

1. Update version in `mix.exs`.
2. Create and push a new tag.
3. Wait for the release workflow to build all the binaries.
4. Publish the release from draft.
5. Publish the package to Hex.

## License

Note that the build artifacts are a result of compiling XLA, hence are under
the respective license. See [XLA](https://github.com/openxla/xla).

```text
Copyright (c) 2020 Sean Moriarity

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at [http://www.apache.org/licenses/LICENSE-2.0](http://www.apache.org/licenses/LICENSE-2.0)

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```
