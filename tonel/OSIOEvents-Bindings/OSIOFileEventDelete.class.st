"
I am an event that notifies the deletion of a file in a directory.
"
Class {
	#name : #OSIOFileEventDelete,
	#superclass : #OSIOFileEvent,
	#category : #'OSIOEvents-Bindings-FileSystem'
}

{ #category : #testing }
OSIOFileEventDelete >> isDelete [
	^ true
]
