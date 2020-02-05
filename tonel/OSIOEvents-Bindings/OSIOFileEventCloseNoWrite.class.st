"
I am an event that notifies that a process has closed a handle accessing the file without modifying.
"
Class {
	#name : #OSIOFileEventCloseNoWrite,
	#superclass : #OSIOFileEvent,
	#category : #'OSIOEvents-Bindings-FileSystem'
}

{ #category : #testing }
OSIOFileEventCloseNoWrite >> isCloseNoWrite [
	^ true
]
