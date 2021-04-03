# OS IO Events - Bindings to non-blocking IO Events in Pharo.

----
## Loading in a Pharo image

```smalltalk
Metacello new
  baseline: 'OSIOEvents';
  repository: 'github://ronsaldo/os-ioevents';
  load.
```

## FileSystem monitor example
The following script is an example on how to use the file system monitoring library:

```smalltalk
OSIOFileSystemMonitor on: '.' asFileReference when: OSIOFileEventCreate do: [ :ev |
	Transcript show: 'Create '; show: ev fileReference; cr.
].
OSIOFileSystemMonitor on: '.' asFileReference when: OSIOFileEventDelete do: [ :ev |
	Transcript show: 'Delete '; show: ev fileReference; cr.
].
subscription := OSIOFileSystemMonitor on: '.' asFileReference when: OSIOFileEventCloseWrite do: [ :ev |
	Transcript show: 'Close Write '; show: ev fileReference; cr.
].
"
subscription unsubscribe
"
```
