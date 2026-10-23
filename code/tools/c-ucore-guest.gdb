set pagination off
set confirm off
set print pretty on
set architecture riscv:rv64

file bin/kernel
target remote localhost:1234

break kern_init
break slub_check
