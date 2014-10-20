gboost-rpc
==========

gboost-rpc is an implementation of google protobuf rpc based on boost ASIO.

Dependency
------------

It definitely depends on google protobuf and boost asio. It also uses glog as logger, gflags as command line arguments parser and boost thread. You can also link you program to tcmalloc if you want to improve the performance of gboost-rpc caused that gboost-rpc does not manage memories by itself.

The benchmark tool is as examples shown to you and it needs to link to tcmalloc and google profiler.

At last, you need a compiler supports C++ 11 to build the whole gboost-rpc.

Cross-platform
--------------

gboost-rpc could be compiled on both Linux and Windows. You can CMake it on Linux, but build visual studio project totally by youself. And Windows will be not supported when I add some Linux-only feature, such as sendfile.

Compilation
-----------

```
cmake .
make
```

or for release version
```
cmake . -DCMAKE_BUILD_TYPE=Release
make
```
