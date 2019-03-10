# Description

 A simple command line clipboard manager.

# Features

 - Minimal: CLI oriented, no GTK/QT required.
 - Keeps the CLIPBOARD and PRIMARY X11 selections in sync.
 - Supports unicode.
 - Keeps track of all recorded items.
 - UNIXy: Copies STDIN to the clipboard and can print clipboard contents (e.g clio -p0)

# Usage

```
> clio -d # Start clio
> echo some text|clio # 'some text' is now in both X11 selections
# Copy and paste a few things...
> clio -l # List all clipboard items
> clio -p0 # Print item 0 in full
```
