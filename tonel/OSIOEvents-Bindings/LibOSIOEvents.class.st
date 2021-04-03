"
I have bindings to global libphanide functions.
"
Class {
	#name : #LibOSIOEvents,
	#superclass : #PhaNAPILibrary,
	#pools : [
		'LibOSIOEventsPool'
	],
	#category : #'OSIOEvents-Bindings-Bindings'
}

{ #category : #testing }
LibOSIOEvents class >> isAvailable [
	^ self uniqueInstance isAvailable
]

{ #category : #'library downloading' }
LibOSIOEvents >> fetchLibrary [
	PhaNAPIBintray fetchLibrary: 'os-ioevents' fromIcebergRepoOfClass: self class

]

{ #category : #'library path' }
LibOSIOEvents >> findLibraryPath [
	^ PhaNAPI fullLibraryPathForCurrentPlatform: 'os-ioevents' package: 'os-ioevents'
]

{ #category : #'libgit-finalization' }
LibOSIOEvents >> free: pointer [
	^ self ffiCall: #(void os_ioevents_free(void* pointer))
]

{ #category : #testing }
LibOSIOEvents >> isAvailable [
	| result |
	result := true.
	[
		ExternalAddress loadSymbol: 'os_ioevents_createContext' from: self
	] on: Error do: [ :e | result := false ].
	^ result
]
