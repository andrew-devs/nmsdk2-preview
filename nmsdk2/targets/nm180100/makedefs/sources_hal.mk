VPATH += $(HAL)/mcu/apollo3/hal
VPATH += $(HAL)/mcu/apollo3/regs
VPATH += $(HAL)/utils
VPATH += ./hal

HAL_SRC += am_hal_adc.c
HAL_SRC += am_hal_ble.c
HAL_SRC += am_hal_ble_patch.c
HAL_SRC += am_hal_ble_patch_b0.c
HAL_SRC += am_hal_burst.c
HAL_SRC += am_hal_cachectrl.c
HAL_SRC += am_hal_clkgen.c
HAL_SRC += am_hal_cmdq.c
HAL_SRC += am_hal_ctimer.c
HAL_SRC += am_hal_debug.c
HAL_SRC += am_hal_entropy.c
HAL_SRC += am_hal_flash.c
HAL_SRC += am_hal_global.c
HAL_SRC += am_hal_gpio.c
HAL_SRC += am_hal_interrupt.c
HAL_SRC += am_hal_iom.c
HAL_SRC += am_hal_ios.c
HAL_SRC += am_hal_itm.c
HAL_SRC += am_hal_mcuctrl.c
HAL_SRC += am_hal_mspi.c
HAL_SRC += am_hal_pdm.c
HAL_SRC += am_hal_pwrctrl.c
HAL_SRC += am_hal_queue.c
HAL_SRC += am_hal_reset.c
HAL_SRC += am_hal_rtc.c
HAL_SRC += am_hal_scard.c
HAL_SRC += am_hal_secure_ota.c
HAL_SRC += am_hal_security.c
HAL_SRC += am_hal_stimer.c
HAL_SRC += am_hal_sysctrl.c
HAL_SRC += am_hal_systick.c
HAL_SRC += am_hal_tpiu.c
HAL_SRC += am_hal_uart.c
HAL_SRC += am_hal_wdt.c

HAL_SRC += am_util_ble.c
HAL_SRC += am_util_debug.c
HAL_SRC += am_util_delay.c
HAL_SRC += am_util_faultisr.c
HAL_SRC += am_util_id.c
HAL_SRC += am_util_regdump.c
HAL_SRC += am_util_stdio.c
HAL_SRC += am_util_string.c
HAL_SRC += am_util_time.c

VPATH += ./utils
HAL_SRC += eeprom_emulation.c