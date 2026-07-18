# Day 16 — Kernel Debugging

## ftrace
- Used function_graph tracer, filtered to mpu_probe/mpu_read via set_graph_function
- Traced full call chain: mpu_read -> i2c_smbus_read_i2c_block_data -> i2c_dw_xfer (i2c_designware_core)
  -> regmap_read/write -> dw_reg_read/dw_reg_write (actual hardware register access)
- Observed kernel yielding CPU via schedule() while waiting on I2C hardware completion (wait_for_completion_timeout)
- Learned to read duration markers (+/!/#/$) to spot bottlenecks
- Hit same inv_mpu6050_i2c binding race as Day 15 mid-session; resolved by unbinding built-in driver's modules

## dyndbg
- Added dev_dbg() line to mpu_read() in mpu6050_driver.c
- Confirmed CONFIG_DYNAMIC_DEBUG and CONFIG_DYNAMIC_DEBUG_CORE are NOT set in this
  Raspberry Pi OS kernel (checked via /boot/config-$(uname -r))
- dyndbg unavailable on this kernel without a full kernel rebuild
- TODO: enable CONFIG_DYNAMIC_DEBUG when building custom Yocto kernel in Day 20

## Deferred to Day 20
- Test dyndbg properly once a custom-built kernel with CONFIG_DYNAMIC_DEBUG=y is available
