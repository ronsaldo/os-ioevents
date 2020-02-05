Class {
	#name : #OSIOFileSystemMonitorUnprocessedEvent,
	#superclass : #Object,
	#instVars : [
		'mask',
		'name',
		'fileReference',
		'timeStamp'
	],
	#pools : [
		'LibOSIOEventsPool'
	],
	#category : #'OSIOEvents-Bindings-FileSystem'
}

{ #category : #'as yet unclassified' }
OSIOFileSystemMonitorUnprocessedEvent >> deliverWithWatcher: watcher [
	(mask anyMask: OS_IOEVENTS_FSMONITOR_EVENT_ACCESS) ifTrue: [
		watcher deliverProcessedEvent: (OSIOFileEventAccess new
			fileReference: fileReference;
			yourself)
	].
	(mask anyMask: OS_IOEVENTS_FSMONITOR_EVENT_ATTRIB) ifTrue: [ 
		watcher deliverProcessedEvent: (OSIOFileEventAttrib new
			fileReference: fileReference;
			yourself)
	].
	(mask anyMask: OS_IOEVENTS_FSMONITOR_EVENT_CLOSE_WRITE) ifTrue: [ 
		watcher deliverProcessedEvent: (OSIOFileEventCloseWrite new
			fileReference: fileReference;
			yourself).
	].
	(mask anyMask: OS_IOEVENTS_FSMONITOR_EVENT_CLOSE_NOWRITE) ifTrue: [ 
		watcher deliverProcessedEvent: (OSIOFileEventCloseNoWrite new
			fileReference: fileReference;
			yourself)
	].
	(mask anyMask: OS_IOEVENTS_FSMONITOR_EVENT_CREATE) ifTrue: [ 
		watcher deliverProcessedEvent: (OSIOFileEventCreate new
			fileReference: fileReference;
			yourself)
	].
	(mask anyMask: OS_IOEVENTS_FSMONITOR_EVENT_DELETE) ifTrue: [ 
		watcher deliverProcessedEvent: (OSIOFileEventDelete new
			fileReference: fileReference;
			yourself)
	].
	(mask anyMask: OS_IOEVENTS_FSMONITOR_EVENT_DELETE_SELF) ifTrue: [ 
		watcher deliverProcessedEvent: (OSIOFileEventDelete new
			fileReference: fileReference;
			yourself)
	].
	(mask anyMask: OS_IOEVENTS_FSMONITOR_EVENT_MODIFY) ifTrue: [ 
		watcher deliverProcessedEvent: (OSIOFileEventModify new
			fileReference: fileReference;
			yourself).
	].

	(mask anyMask: OS_IOEVENTS_FSMONITOR_EVENT_OPEN) ifTrue: [ 
		watcher deliverProcessedEvent: (OSIOFileEventOpen new
			fileReference: fileReference;
			yourself)
	].
]

{ #category : #accessing }
OSIOFileSystemMonitorUnprocessedEvent >> fileReference [
	^ fileReference
]

{ #category : #accessing }
OSIOFileSystemMonitorUnprocessedEvent >> fileReference: anObject [
	fileReference := anObject
]

{ #category : #accessing }
OSIOFileSystemMonitorUnprocessedEvent >> mask [
	^ mask
]

{ #category : #accessing }
OSIOFileSystemMonitorUnprocessedEvent >> mask: anObject [
	mask := anObject
]

{ #category : #accessing }
OSIOFileSystemMonitorUnprocessedEvent >> name [
	^ name
]

{ #category : #accessing }
OSIOFileSystemMonitorUnprocessedEvent >> name: anObject [
	name := anObject
]

{ #category : #accessing }
OSIOFileSystemMonitorUnprocessedEvent >> timeStamp [
	^ timeStamp
]

{ #category : #accessing }
OSIOFileSystemMonitorUnprocessedEvent >> timeStamp: anObject [
	timeStamp := anObject
]
