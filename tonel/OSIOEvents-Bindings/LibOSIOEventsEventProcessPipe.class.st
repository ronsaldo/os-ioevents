"
I am a libphanide process event that notifies that a pipe is ready for sending or receiving data from a child process.
"
Class {
	#name : #LibOSIOEventsEventProcessPipe,
	#superclass : #FFIExternalStructure,
	#classVars : [
		'OFFSET_PIPEINDEX',
		'OFFSET_PROCESS',
		'OFFSET_TYPE'
	],
	#pools : [
		'LibOSIOEventsPool'
	],
	#category : #'OSIOEvents-Bindings-Bindings'
}

{ #category : #'field definition' }
LibOSIOEventsEventProcessPipe class >> fieldsDesc [
	"
	self rebuildFieldAccessors
	"
	^ #(
		uint32 type;
		os_ioevents_process_t process;
		uint32 pipeIndex;
	)
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEventProcessPipe >> pipeIndex [
	"This method was automatically generated"
	^handle unsignedLongAt: OFFSET_PIPEINDEX
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEventProcessPipe >> pipeIndex: anObject [
	"This method was automatically generated"
	handle unsignedLongAt: OFFSET_PIPEINDEX put: anObject
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEventProcessPipe >> process [
	"This method was automatically generated"
	^ExternalData fromHandle: (handle pointerAt: OFFSET_PROCESS) type: ExternalType void asPointerType
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEventProcessPipe >> process: anObject [
	"This method was automatically generated"
	handle pointerAt: OFFSET_PROCESS put: anObject getHandle.
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEventProcessPipe >> type [
	"This method was automatically generated"
	^handle unsignedLongAt: OFFSET_TYPE
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEventProcessPipe >> type: anObject [
	"This method was automatically generated"
	handle unsignedLongAt: OFFSET_TYPE put: anObject
]
