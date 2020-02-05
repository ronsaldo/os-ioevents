"
I am a libphanide process finished event.
"
Class {
	#name : #LibOSIOEventsEventProcessFinished,
	#superclass : #FFIExternalStructure,
	#classVars : [
		'OFFSET_EXITCODE',
		'OFFSET_PROCESS',
		'OFFSET_TYPE'
	],
	#pools : [
		'LibOSIOEventsPool'
	],
	#category : #'OSIOEvents-Bindings-Bindings'
}

{ #category : #'field definition' }
LibOSIOEventsEventProcessFinished class >> fieldsDesc [
	"
	self rebuildFieldAccessors
	"
	^ #(
		uint32 type;
		os_ioevents_process_t process;
		int32 exitCode;
	)
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEventProcessFinished >> exitCode [
	"This method was automatically generated"
	^handle signedLongAt: OFFSET_EXITCODE
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEventProcessFinished >> exitCode: anObject [
	"This method was automatically generated"
	handle signedLongAt: OFFSET_EXITCODE put: anObject
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEventProcessFinished >> process [
	"This method was automatically generated"
	^ExternalData fromHandle: (handle pointerAt: OFFSET_PROCESS) type: ExternalType void asPointerType
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEventProcessFinished >> process: anObject [
	"This method was automatically generated"
	handle pointerAt: OFFSET_PROCESS put: anObject getHandle.
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEventProcessFinished >> type [
	"This method was automatically generated"
	^handle unsignedLongAt: OFFSET_TYPE
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEventProcessFinished >> type: anObject [
	"This method was automatically generated"
	handle unsignedLongAt: OFFSET_TYPE put: anObject
]
