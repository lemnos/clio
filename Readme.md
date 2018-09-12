# Description

 A simple command line clipboard manager.

# Features

 - Minimal: Command line oriented, no GTK/QT required.
 - Keeps the various X11 selections in sync.
 - Supports unicode.
 - Records all copied items.
 - Unixy: Copies STDIN directly into the clipboard for easy integration.

# Usage

```
> clio -d # Start clio
> echo some text|clio # 'some text' is now in both X11 selections
# Copy and paste a few things...
> clio -l # List all clipboard items
> clio -p0 # Print item 0 in full
```
