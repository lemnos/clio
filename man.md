% CLIO(1)
% Aetnaeus
% December 2018

# NAME

clio - a command line clipboard manager

# SYNOPSIS 

clio [-d] [-l] [-p num]

# DESCRIPTION

A minimalistic command line clipboard manager featuring selection
synchronization and history.

# USAGE

clio runs as a daemon and must be started before it begins recording clipboard
activity, adding 'clio -d' to your xinitrc is a good way to ensure that it
is always running for in your X session.

# OPTIONS

**-d**
: Start clio as a daemon.

**-l** 
: List all recorded pastables.

**-p**  *n*
: Print the *n*th pastable to STDOUT.

**-a**  *n*
: Raise the *n*th pastable to the top of history and copy it to the clipboard.

**-c**
: Clear all pastables in memory.

**-h**
: Prints options.

# EXAMPLES

```
> echo 'lorem ipsum'|clio  # Copy 'lorem ipsum' to the clipboard.
> clio -l                  # List all recorded clipboard items (one per line, possibly truncated).
> clip -a 1                # Copy the second history item to the active clipboard.
> clio -p 0                # Print the contents of the first history item (i.e the active one).
```
