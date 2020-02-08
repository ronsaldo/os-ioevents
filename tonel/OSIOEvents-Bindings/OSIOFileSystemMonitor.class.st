"
I am a monitor for the file system.

I provide capabilites on monitoring a directory or a file for changes.

"
Class {
	#name : #OSIOFileSystemMonitor,
	#superclass : #Object,
	#instVars : [
		'session',
		'registeredFilesMutex',
		'registeredFiles'
	],
	#classInstVars : [
		'uniqueInstance'
	],
	#category : #'OSIOEvents-Bindings-FileSystem'
}

{ #category : #'event subscriptions' }
OSIOFileSystemMonitor class >> on: aPath when: anEvent do: aBlock [
	^ self uniqueInstance on: aPath when: anEvent do: aBlock
]

{ #category : #accessing }
OSIOFileSystemMonitor class >> uniqueInstance [
	^ uniqueInstance ifNil: [ uniqueInstance := self new ].
]

{ #category : #'session management' }
OSIOFileSystemMonitor >> checkSession [
	session ~~ Smalltalk session ifTrue: [
		self initializeForNewSession
	]
]

{ #category : #'as yet unclassified' }
OSIOFileSystemMonitor >> destroyWatcher: watcher [
	OSIOEventsDriver isAvailable ifFalse: [ ^ nil ].

	registeredFilesMutex critical: [
		registeredFiles at: watcher fileReference ifPresent: [ :registeredWatcher |
			registeredWatcher == watcher ifTrue: [
				registeredFiles removeKey: watcher fileReference
			]
		]
	].

	OSIOEventsDriver uniqueInstance destroyFSMonitor: watcher handle
]

{ #category : #'event subscriptions' }
OSIOFileSystemMonitor >> for: aPath [
	| watchedFileReference watchedElement |
	OSIOEventsDriver isAvailable ifFalse: [ ^ nil ].
	
	watchedFileReference := aPath asFileReference asAbsolute.
	self checkSession.
	watchedFileReference exists ifFalse: [ ^ nil ].
	
	registeredFilesMutex critical: [
		watchedElement := registeredFiles at: watchedFileReference
			ifAbsentPut: [ OSIOFileSystemMonitorWatchedElement for: watchedFileReference ]
	].

	^ watchedElement
]

{ #category : #initialization }
OSIOFileSystemMonitor >> initialize [
	super initialize.
	
]

{ #category : #initialization }
OSIOFileSystemMonitor >> initializeForNewSession [
	registeredFilesMutex := Semaphore forMutualExclusion.
	registeredFiles := Dictionary new.
	session := Smalltalk session.
]

{ #category : #'event subscriptions' }
OSIOFileSystemMonitor >> on: aPath element: basename when: anEvent do: aBlock [
	^ (self for: aPath)
		ifNil: [ OSIOEventsNullEventSubscription uniqueInstance ]
		ifNotNil: [ :watcher | watcher onChild: basename subscribeEvent: anEvent withAction: aBlock]
]

{ #category : #'event subscriptions' }
OSIOFileSystemMonitor >> on: aPath when: anEvent do: aBlock [
	| watchedFileReference parent |
	OSIOEventsDriver isAvailable ifFalse: [ ^ OSIOEventsNullEventSubscription uniqueInstance ].
	
	watchedFileReference := aPath asFileReference asAbsolute.
	(watchedFileReference isFile and: [
		(anEvent isPreferredInParentFolder and: [OSIOEventsDriver uniqueInstance fsMonitorCanWatchDirectoryFileModifications ]) or:
		[ OSIOEventsDriver uniqueInstance fsMonitorCanWatchFiles not]]) ifTrue: [
		parent := watchedFileReference parent.
		^ self on: parent element: watchedFileReference basename when: anEvent do: aBlock
	].
	
	^ (self for: aPath)
		ifNil: [ OSIOEventsNullEventSubscription uniqueInstance ]
		ifNotNil: [ :watcher | watcher subscribeEvent: anEvent withAction: aBlock]
]
