set can-use-hw-watchpoints 0
define asst1
dir ~/cs3231/src/kern/compile/ASST1
target remote unix:.sockets/gdb
b panic
end
