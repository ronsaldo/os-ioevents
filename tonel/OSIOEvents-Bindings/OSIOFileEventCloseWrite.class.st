"
I am an event that notifies that a process has closed a handle accessing the file after modifying it. In platforms where I am not directly supported I may be generated after create and modify events.
"
Class {
	#name : #OSIOFileEventCloseWrite,
	#superclass : #OSIOFileEvent,
	#category : #'OSIOEvents-Bindings-FileSystem'
}

{ #category : #testing }
OSIOFileEventCloseWrite class >> isPreferredInParentFolder [
	^ true
]

{ #category : #testing }
OSIOFileEventCloseWrite >> isCloseWrite [
	^ true
]
