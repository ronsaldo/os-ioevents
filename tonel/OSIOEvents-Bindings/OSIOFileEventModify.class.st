"
I am an event that notifies the modification of a file or a directory. I may not be the final modification of a particular, so the file could be in an inconsistent state.
"
Class {
	#name : #OSIOFileEventModify,
	#superclass : #OSIOFileEvent,
	#category : #'OSIOEvents-Bindings-FileSystem'
}

{ #category : #converting }
OSIOFileEventModify >> asCloseWriteEvent [
	^ OSIOFileEventCloseWrite new fileReference: fileReference
]

{ #category : #testing }
OSIOFileEventModify >> isModify [
	^ true
]
