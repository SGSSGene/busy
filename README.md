# busy - (BUild SYstem)
build system and repository manager for c++ projects.


Main Idea
========
- automatic dependencies discovery
- support of multi repository (in a flat hierachy)
- minimal configuration of projects


Installation
============

```
   git clone git@github.com:SGSSGene/busy.git
   cd busy
   ./bootstrap.sh
   sudo cp busy /usr/bin
```
   

How to use it
=============
If you have a project that is supported by busy you can run following things:
- `busy` will clone all needed repositories and compile all libraries, executables and unittests
- `busy build <specific executable>` will build only a specific executable,library or unittest
- `busy status` gives you an overview over all repositories
- `busy pull` will run `git pull` on all repositories
- `busy push` will run `git push` on all repositories
- `busy git <command>` will run `git <command>` on all repositories
- `busy clean` will clean the workspace (shouldn't be really be needed)

