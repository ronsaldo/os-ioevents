"
I am a file event that notifies that file attributes such as access permissions have changed.
"
Class {
	#name : #OSIOFileEventAttrib,
	#superclass : #OSIOFileEvent,
	#category : #'OSIOEvents-Bindings-FileSystem'
}

{ #category : #testing }
OSIOFileEventAttrib >> isAttrib [
	^ true
]
