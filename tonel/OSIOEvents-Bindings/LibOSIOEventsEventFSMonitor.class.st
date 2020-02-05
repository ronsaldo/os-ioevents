"
I am a libphanide FS monitor event.
"
Class {
	#name : #LibOSIOEventsEventFSMonitor,
	#superclass : #FFIExternalStructure,
	#classVars : [
		'OFFSET_COOKIE',
		'OFFSET_HANDLE',
		'OFFSET_MASK',
		'OFFSET_NAME',
		'OFFSET_NAMELENGTH',
		'OFFSET_TYPE'
	],
	#pools : [
		'LibOSIOEventsPool'
	],
	#category : #'OSIOEvents-Bindings-Bindings'
}

{ #category : #'field definition' }
LibOSIOEventsEventFSMonitor class >> fieldsDesc [
	"
	self rebuildFieldAccessors
	"
	^ #(
		uint32 type;
		os_ioevents_fsmonitor_handle_t handle;
    	uint32 mask;
    	uint32 cookie;
    	uint32 nameLength;
    	char* name;
	)
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEventFSMonitor >> cookie [
	"This method was automatically generated"
	^handle unsignedLongAt: OFFSET_COOKIE
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEventFSMonitor >> cookie: anObject [
	"This method was automatically generated"
	handle unsignedLongAt: OFFSET_COOKIE put: anObject
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEventFSMonitor >> handle [
	"This method was automatically generated"
	^ExternalData fromHandle: (handle pointerAt: OFFSET_HANDLE) type: ExternalType void asPointerType
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEventFSMonitor >> handle: anObject [
	"This method was automatically generated"
	handle pointerAt: OFFSET_HANDLE put: anObject getHandle.
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEventFSMonitor >> mask [
	"This method was automatically generated"
	^handle unsignedLongAt: OFFSET_MASK
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEventFSMonitor >> mask: anObject [
	"This method was automatically generated"
	handle unsignedLongAt: OFFSET_MASK put: anObject
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEventFSMonitor >> name [
	"This method was automatically generated"
	^ExternalData fromHandle: (handle pointerAt: OFFSET_NAME) type: ExternalType char asPointerType
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEventFSMonitor >> name: anObject [
	"This method was automatically generated"
	handle pointerAt: OFFSET_NAME put: anObject getHandle.
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEventFSMonitor >> nameLength [
	"This method was automatically generated"
	^handle unsignedLongAt: OFFSET_NAMELENGTH
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEventFSMonitor >> nameLength: anObject [
	"This method was automatically generated"
	handle unsignedLongAt: OFFSET_NAMELENGTH put: anObject
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEventFSMonitor >> type [
	"This method was automatically generated"
	^handle unsignedLongAt: OFFSET_TYPE
]

{ #category : #'accessing structure variables' }
LibOSIOEventsEventFSMonitor >> type: anObject [
	"This method was automatically generated"
	handle unsignedLongAt: OFFSET_TYPE put: anObject
]
