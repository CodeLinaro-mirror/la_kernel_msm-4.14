human_arch	= ARMv9
build_arch	= arm64
defconfig	= defconfig
flavours	= imx
build_image	= Image
kernel_file	= arch/$(build_arch)/boot/Image
install_file	= vmlinux
no_dumpfile = true
uefi_signed     = true

vdso		= vdso_install

do_tools_usbip  = true
do_tools_cpupower = true
do_tools_perf   = true
do_tools_perf_jvmti = true
do_tools_perf_python = true
do_tools_bpftool = true
do_tools_rtla = true

do_dtbs		= true
