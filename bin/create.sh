#!/bin/bash -

# -------------------------------------
# create.sh
# =========
# ...
#
# -------------------------------------

DOMAIN=
FULLPATH=

# Show the help
help() {
	cat <<EOF
-d, --domain <arg>    Specify a domain for the new site.
-p, --path <arg>      Specify a path to the new site.
-v, --verbose         Be verbose.
-h, --help            Show help.
EOF
}


if [ $# -lt 1 ]
then
	printf "$0: No options specified.\n" > /dev/stderr
	help
	exit 1
fi



# Get the options 
while [ $# -gt 0 ]
do
	case $1 in
		-p|--path)
			shift
			FULLPATH=$1	
		;;
		-d|--domain)
			shift
			DOMAIN=$1
		;;
		-v|--verbose)
			VERBOSE=1	
		;;
		-h|--help)
			help
			exit 0
		;;
	esac
	shift
done


# If DOMAIN is unspecified, use the path
if [ -z "$DOMAIN" ]
then
	[ -z "$FULLPATH" ] && {
		printf "$0: No --path specified...\n" > /dev/stderr
		exit 1
	}
	DOMAIN=`basename $FULLPATH`
fi


if [ -z "$FULLPATH" ]
then
	FULLPATH=$DOMAIN
fi


# Create folders and whatnot for a new instance of libhyl
mkdir -p $FULLPATH/{app,assets,private,sql,tests,views}/
mkdir -p $FULLPATH/private/{certs,keys,lib,log}/
mkdir -p $FULLPATH/assets/{bin,img}/
ln -s $FULLPATH/tmp /tmp
touch $FULLPATH/robots.txt
# If this were C, I could generate one...
touch $FULLPATH/favicon.ico

# Generate a .gitignore
{
cat <<EOF
# Add nothing under private or tmp
private/
tmp/

# Add no binary assets
assets/bin
assets/img
EOF 
} > $FULLPATH/.gitignore


# Generate a config file
{
cat <<EOF
-------------------------------------------------
-- config.lua
--
-- Configuration file for this Lua-based website.
-------------------------------------------------
return {
	static = {
		"/assets"
	, "/robots.txt"
	, "/favicon.ico"
	}
,	db = "yamama"
, title = "$DOMAIN"
, root = ""
,	routes = {
		["/"] = { model = "single", views = { "single" } }
	}
}
EOF
} > $FULLPATH/config.lua


# I can either modify the file myself or do something else
# lua global-config
# add the current host info
# then rewrite... 
