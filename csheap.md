**csheap** is a customizable, [dynamic memory allocator](http://en.wikipedia.org/wiki/Dynamic_memory_allocation) written in C.
It was originally written as a replacement for the standard libc malloc and the Windows runtime.

csheap is simple, making it fast and less bug-prone. It can be customized at compile-time with the use of preprocessor defines. It is written in portable ANSI C.

_Why did you create csheap?_

Most C programs use dynamic memory extensively. Big projects rarely rely on the standard libc routines (`malloc`, `free`, etc.) though. Indeed, libc allocate memory from a unique process heap, making it impossible to control or track memory usage.

Windows developers sometimes use the [heap functions](http://msdn.microsoft.com/en-us/library/aa366711(VS.85).aspx) provided by the OS. However, these routines are limited in some aspects. Here are a few examples:
  * Windows Heap, obviously, is not platform independent.
  * If a maximum heap size is set, memory chunks larger than 512Kb cannot be allocated (you need a growable heap to allocate these; growable heaps are difficult to control and should not be used IMO).
  * Heap options are to be set at runtime, when the heap is created. This adds flexibility but also code overhead in the heap routines for checking which options apply or not.

Therefore, a few things csheap does are:
  * Heap creation with a maximum size is mandatory.
  * Allocation of memory **chunks up to 128Mb** in size.
  * Customization to enable: **profiling**, **safe unlinking**, chunk **cookies**, **multithreading**, etc.
  * Customization is done at compile-time, meaning no impact performance-wise for unwanted options.
  * Straight-forward API.

_How to use csheap in my C project?_

  * Link csheap.c to your project.
  * Include csheap.h.
  * Set or unset the `#defines` in csheap.h to specify which options you need.
  * Use the explicitly-named `csheap_xxx` routines to **create**, **destroy**, **reset** or validate a heap, and **alloc**, **free** or **realloc** memory chunks.

_Questions and Answers_

  * What does `csheap_reset()` do? This routine frees all chunks contained in a heap, bringing it to the state it was after being created. In a nutshell, it's an efficient way to do `csheap_destroy()` + `csheap_create()`.