"
I am file related to event. My subclasses are announced when a monitored file or directory suffers some changes.

Some events are directly supported by some platform specific events, and some other events may not be directly supported but in some cases may be inferred. We try to coalesce events in order to reduce spuriuous events, but there is no guarantee that files are in a consistent state after receiving a modification or close write event (e.g. the modification may be incomplete), so it is recommended to handle error if using these events for facilitating live programming.

The names of most of the events come from the inotify linux system call.
"
Class {
	#name : #OSIOFileEvent,
	#superclass : #OSIOEventsEvent,
	#instVars : [
		'fileReference'
	],
	#category : #'OSIOEvents-Bindings-FileSystem'
}

{ #category : #testing }
OSIOFileEvent class >> isPreferredInParentFolder [
	^ false
]

{ #category : #'as yet unclassified' }
OSIOFileEvent >> deliverWithWatcher: aWatcher [
	^ aWatcher deliverProcessedEvent: self
]

{ #category : #accessing }
OSIOFileEvent >> fileReference [
	^ fileReference
]

{ #category : #accessing }
OSIOFileEvent >> fileReference: anObject [
	fileReference := anObject
]

{ #category : #testing }
OSIOFileEvent >> isAccess [
	^ false
]

{ #category : #testing }
OSIOFileEvent >> isAttrib [
	^ false
]

{ #category : #testing }
OSIOFileEvent >> isCloseNoWrite [
	^ false
]

{ #category : #testing }
OSIOFileEvent >> isCloseWrite [
	^ false
]

{ #category : #testing }
OSIOFileEvent >> isCreate [
	^ false
]

{ #category : #testing }
OSIOFileEvent >> isDelete [
	^ false
]

{ #category : #testing }
OSIOFileEvent >> isModify [
	^ false
]

{ #category : #testing }
OSIOFileEvent >> isMoved [
	^ false
]

{ #category : #testing }
OSIOFileEvent >> isOpen [
	^ false
]
