"
I am an event for watched element using the low-level file system monitoring API.
"
Class {
	#name : #OSIOFSMonitorWatchEvent,
	#superclass : #Object,
	#instVars : [
		'handle',
		'name',
		'cookie',
		'mask'
	],
	#category : #'OSIOEvents-Bindings-Driver'
}

{ #category : #accessing }
OSIOFSMonitorWatchEvent >> cookie [
	^ cookie
]

{ #category : #accessing }
OSIOFSMonitorWatchEvent >> cookie: anObject [
	cookie := anObject
]

{ #category : #accessing }
OSIOFSMonitorWatchEvent >> handle [
	^ handle
]

{ #category : #accessing }
OSIOFSMonitorWatchEvent >> handle: anObject [
	handle := anObject
]

{ #category : #accessing }
OSIOFSMonitorWatchEvent >> mask [
	^ mask
]

{ #category : #accessing }
OSIOFSMonitorWatchEvent >> mask: anObject [
	mask := anObject
]

{ #category : #accessing }
OSIOFSMonitorWatchEvent >> name [
	^ name
]

{ #category : #accessing }
OSIOFSMonitorWatchEvent >> name: anObject [
	name := anObject
]
