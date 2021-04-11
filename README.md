# libhyl

libhyl is a shared module for Hypno allowing users to write webapps in Lua.  It is MVC based and does not require any dependencies outside of Lua, which is most likely present on your system if Hypno is already running.


## Files & Directories

Apps written with libhyl have a simple structure adhering to the following:

app/\*.lua - Models written with Lua go here.

assets/\*  - All of the static assets that come with the project.

private/\* - Private storage, SSL certificates, logs and more go here.

sql/\* - SQL files for execution.

tests/\* - Any tests go here.

vendor/ - Local copy of dependencies that aren't distributed with Autoconf yet.

views/\* - Views written in Mustache or other template language.

config.lua - Configuration file for website



## Building 

The default Makefile builds a shared object.

Run `make` with no arguments at the top level to build it.

Tests can be run using `make test`.



