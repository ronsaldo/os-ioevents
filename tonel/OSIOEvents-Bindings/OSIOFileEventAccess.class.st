"
I am an event that notifies that a file is being accessed by some process. I may not be always supported.
"
Class {
	#name : #OSIOFileEventAccess,
	#superclass : #OSIOFileEvent,
	#category : #'OSIOEvents-Bindings-FileSystem'
}

{ #category : #testing }
OSIOFileEventAccess >> isAccess [
	^ true
]
