"
I am a generic handle from libphanide.
"
Class {
	#name : #LibOSIOEventsHandle,
	#superclass : #FFIExternalObject,
	#pools : [
		'LibOSIOEventsPool'
	],
	#category : #'OSIOEvents-Bindings-Bindings'
}

{ #category : #'library path' }
LibOSIOEventsHandle class >> ffiLibraryName [
	^ LibOSIOEvents
]

{ #category : #'library path' }
LibOSIOEventsHandle >> ffiLibraryName [
	^ LibOSIOEvents
]
