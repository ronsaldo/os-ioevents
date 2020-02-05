"
I represent a handle to something that is watched on the filesystem.
"
Class {
	#name : #OSIOFSMonitorWatchHandle,
	#superclass : #Object,
	#instVars : [
		'handle',
		'eventHandler'
	],
	#category : #'OSIOEvents-Bindings-Driver'
}

{ #category : #accessing }
OSIOFSMonitorWatchHandle >> eventHandler [
	^ eventHandler
]

{ #category : #accessing }
OSIOFSMonitorWatchHandle >> eventHandler: anObject [
	eventHandler := anObject
]

{ #category : #accessing }
OSIOFSMonitorWatchHandle >> handle [
	^ handle
]

{ #category : #accessing }
OSIOFSMonitorWatchHandle >> handle: anObject [
	handle := anObject
]

{ #category : #'initialize-release' }
OSIOFSMonitorWatchHandle >> supportedEventMask [
	^ handle supportedEventMask
]
