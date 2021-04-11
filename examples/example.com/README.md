# hypno-lua-filter

This is an example Hypno web app written in Lua.  



## Files & Directories

app/\*.lua - Models written with Lua.

misc/\* - SSL certificates, logs and more go here.

sql/\* - SQL files for execution.

static/\*  - All of the static assets that come with the project.

tests/\* - All tests are here.

vendor/ - Local copy of dependencies that aren't distributed with Autoconf yet.

views/\* - Views written in Mustache.

Makefile - Recipes to create the shared library. 

config.lua - Configuration file for website



## Building 

The default Makefile builds a shared object.

Run `make` with no arguments at the top level to build it.

Tests can be run using `make test`.



