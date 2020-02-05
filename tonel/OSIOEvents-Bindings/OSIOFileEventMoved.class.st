"
I notify that a file was moved. oldFileReference contains the old location, and newFileReference contains the new location.
"
Class {
	#name : #OSIOFileEventMoved,
	#superclass : #OSIOFileEvent,
	#instVars : [
		'newFileReference'
	],
	#category : #'OSIOEvents-Bindings-FileSystem'
}

{ #category : #testing }
OSIOFileEventMoved >> isMoved [
	^ true
]

{ #category : #accessing }
OSIOFileEventMoved >> newFileReference [
	^ newFileReference
]

{ #category : #accessing }
OSIOFileEventMoved >> newFileReference: anObject [
	newFileReference := anObject
]

{ #category : #accessing }
OSIOFileEventMoved >> oldFileReference [
	^ fileReference
]
