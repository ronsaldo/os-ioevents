"
This event notifies that a file has been opened by a process. I may not always be supported, or be disabled for performance reasons.
"
Class {
	#name : #OSIOFileEventOpen,
	#superclass : #OSIOFileEvent,
	#category : #'OSIOEvents-Bindings-FileSystem'
}

{ #category : #testing }
OSIOFileEventOpen >> isOpen [
	^ true
]
