cmd_/home/oem/linux_kernel_kobjex/modules.order := {   echo /home/oem/linux_kernel_kobjex/kobjx_slab_rcu_registry.ko; :; } | awk '!x[$$0]++' - > /home/oem/linux_kernel_kobjex/modules.order
