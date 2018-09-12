# busy - (BUild SYstem)
build system and repository manager for c++ projects.


Main Idea
=========
- automatic dependencies discovery
- minimal configuration of projects


Installation
============

```
   git clone https://github.com/SGSSGene/busy.git
   cd busy
   ./bootstrap.sh
   sudo cp busy /usr/bin
```
   

How to use it
=============
If you have a project that is supported by busy you can run following things:
- `busy` will compile all libraries, executables and unittests
- `busy build <specific executable>` will build only a specific executable,library or unittest
- `busy clean` will clean the workspace (shouldn't be really be needed)
- `busy cleanall` will clean the workspace (shouldn't be really be needed)


Target forms:
- executable
- static library
- shared library (headers available)
- plugin library  (a shared library without headers)


