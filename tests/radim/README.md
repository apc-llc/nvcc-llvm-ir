# How to debug the LLVM backend through llc

```
marcusmae@M17xR4:~/apc/nvcc-llvm-ir/tests/radim$ export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/opt/cuda/bin:/home/marcusmae/forge/llvm-trunk/install_debug/bin
marcusmae@M17xR4:~/apc/nvcc-llvm-ir/tests/radim$ gdb llc
GNU gdb (Ubuntu 7.7-0ubuntu3.1) 7.7
Copyright (C) 2014 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.  Type "show copying"
and "show warranty" for details.
This GDB was configured as "x86_64-linux-gnu".
Type "show configuration" for configuration details.
For bug reporting instructions, please see:
<http://www.gnu.org/software/gdb/bugs/>.
Find the GDB manual and other documentation resources online at:
<http://www.gnu.org/software/gdb/documentation/>.
For help, type "help".
Type "apropos word" to search for commands related to "word"...
Reading symbols from llc...done.
(gdb) r -march=r600 test.ll -o -
Starting program: /home/marcusmae/forge/llvm-trunk/install_debug/bin/llc -march=r600 test.ll -o -
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/x86_64-linux-gnu/libthread_db.so.1".
warning: ignoring debug info with an invalid version (0) in test.ll
	.text
.global_Z8mykernelv_modified
_Z8mykernelv_modified:                  ; @_Z8mykernelv_modified
	.section	.AMDGPU.config
.long	165992
.long	1
.long	165900
.long	0
.long	166120
.long	0
	.text
; BB#0:
	CF_END
	PAD

	.section	.AMDGPU.csdata
	;SQ_PGM_RESOURCES:STACK_SIZE = 0
	.no_dead_strip	_Z8mykernelv_modified
	.text
EndOfTextLabel:
[Inferior 1 (process 11686) exited normally]
```

