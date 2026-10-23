set pagination off
set confirm off
set print pretty on
set breakpoint pending on

set args -machine virt -nographic -bios default -kernel bin/ucore.img -s -S

break riscv_cpu_tlb_fill
condition 1 address >= 0xffffffffc0000000

break get_physical_address
condition 2 addr >= 0xffffffffc0000000
