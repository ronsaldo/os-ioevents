"
I am an event that notifies the creation of a file in a directory.
"
Class {
	#name : #OSIOFileEventCreate,
	#superclass : #OSIOFileEvent,
	#category : #'OSIOEvents-Bindings-FileSystem'
}

{ #category : #testing }
OSIOFileEventCreate >> isCreate [
	^ true
]
