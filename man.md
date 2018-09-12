% CLIO(1)
% Aetnaeus
% December 2018

# NAME

clio - a command line clipboard manager

# SYNOPSIS 

clio [-d] [-l] [-p num]

# DESCRIPTION

A minimalistic command line clipboard manager. Text can be piped into clio and
made pastable (like xclip). 

# OPTIONS

**-d**
: Start clio as a daemon.

**-l** 
: List all recorded pastables.

**-p**  *n*
: Print the *n*th pastable to STDOUT.

**-a**  *n*
: Raise the *n*th pastable to the top of history and copy it to the clipboard.

**-h**
: Prints options.
