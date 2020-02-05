"
I am an union of libphanide events.
"
Class {
	#name : #LibOSIOEventsEvent,
	#superclass : #FFIExternalUnion,
	#classVars : [
		'padding16'
	],
	#pools : [
		'LibOSIOEventsPool'
	],
	#category : #'OSIOEvents-Bindings-Bindings'
}

{ #category : #'field definition' }
LibOSIOEventsEvent class >> fieldsDesc [
	"
	self rebuildFieldAccessors
	"
	^ #(
		uint32 type;
		os_ioevents_event_process_pipe_t processPipe;
		os_ioevents_event_process_finished_t processFinished;
		os_ioevents_event_fsmonitor_t fsmonitor;
		padding16 padding;
	)
]

{ #category : #initialization }
LibOSIOEventsEvent class >> initialize [
	padding16 := FFITypeArray ofType: #size_t size: 16.
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEvent >> fsmonitor [
	"This method was automatically generated"
	^ LibOSIOEventsEventFSMonitor fromHandle: (handle referenceStructAt: 1 length: LibOSIOEventsEventFSMonitor byteSize)
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEvent >> fsmonitor: anObject [
	"This method was automatically generated"
	handle structAt: 1 put: anObject getHandle length: 40.
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEvent >> padding [
	"This method was automatically generated"
	^(FFITypeArray ofType: #FFISizeT size: 16) fromHandle: (handle copyFrom: 1 to: 128)
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEvent >> padding: anObject [
	"This method was automatically generated"
	handle replaceFrom: 1 to: 128 with: anObject getHandle startingAt: 1
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEvent >> processFinished [
	"This method was automatically generated"
	^ LibOSIOEventsEventProcessFinished fromHandle: (handle referenceStructAt: 1 length: LibOSIOEventsEventProcessFinished byteSize)
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEvent >> processFinished: anObject [
	"This method was automatically generated"
	handle structAt: 1 put: anObject getHandle length: 24.
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEvent >> processPipe [
	"This method was automatically generated"
	^ LibOSIOEventsEventProcessPipe fromHandle: (handle referenceStructAt: 1 length: LibOSIOEventsEventProcessPipe byteSize)
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEvent >> processPipe: anObject [
	"This method was automatically generated"
	handle structAt: 1 put: anObject getHandle length: 24.
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEvent >> type [
	"This method was automatically generated"
	^handle unsignedLongAt: 1
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEvent >> type: anObject [
	"This method was automatically generated"
	handle unsignedLongAt: 1 put: anObject
]
