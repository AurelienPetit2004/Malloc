LD_LIBRARY_PATH is an environment variable which defines the path where the shared library should
be found. Here is an example of how to use it:

gcc main.c -L. -lmalloc -o main

LD_LIBRARY_PATH=. ./main

LD_PRELOAD is also an environment variable but defines directly the library to preload instead of specifying the path. Here is an example of how to use it:

LD_PRELOAD=./libmalloc.so ls
