# Validation programs

These small programs provide reproducible SDK-level evidence for the community
R200 port. They are diagnostics and acceptance tools, not a second driver.

Build the RS2 programs against the R200 fork, never an accidental system copy:

```bash
cmake -S validation -B validation/build \
  -Drealsense2_DIR="$R200_ROOT/rs2_install/lib/cmake/realsense2"
cmake --build validation/build -j4
```

The resulting `rs2_probe`, `rs2_options`, `rs2_stream`, `rs2_restart`,
`rs2_profile_matrix`, `rs2_y16`, `rs2_rate`, and `rs2_pointcloud` binaries are in
`validation/build/`. Their purpose and acceptance criteria are documented in
the repository-level `DEMO_ENTRYPOINTS.md` and `ACCEPTANCE.md`.

RS1 baseline probes are disabled by default. Enable them only when a separate
librealsense 1.x installation is deliberately supplied:

```bash
cmake -S validation -B validation/build-rs1 \
  -Drealsense2_DIR="$R200_ROOT/rs2_install/lib/cmake/realsense2" \
  -DR200_BUILD_RS1_PROBES=ON \
  -DRS1_INCLUDE_DIR=/path/to/rs1/include \
  -DRS1_LIBRARY=/path/to/librealsense.so
```

`xu_test` performs low-level extension-unit operations. Do not use it as a
normal launch path; incorrect arguments can change device state.
