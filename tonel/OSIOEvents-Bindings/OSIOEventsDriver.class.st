"
I represent a driver to the Phanide native API.

The Phanide native API provides facilities for the following features:
- Event driven API for handling subprocess.
- File system monitoring (inotify on Linux)

"
Class {
	#name : #OSIOEventsDriver,
	#superclass : #Object,
	#instVars : [
		'context',
		'session',
		'eventFetchingProcess',
		'processDictionaryMutex',
		'processDictionary',
		'pendingEventsSemaphore',
		'pendingEventsSemaphoreIndex',
		'fsmonitorMutex',
		'fsmonitorDictionary',
		'pendingCoalescingWatcherMutex',
		'pendingCoalescingWatcher',
		'eventCoalescingProcess',
		'fsMonitorCanWatchFiles',
		'fsMonitorCanWatchDirectories',
		'fsMonitorCanWatchDirectoryFileModifications'
	],
	#classVars : [
		'EventDispatchMap'
	],
	#pools : [
		'LibOSIOEventsPool'
	],
	#classInstVars : [
		'uniqueInstance'
	],
	#category : #'OSIOEvents-Bindings-Driver'
}

{ #category : #initialization }
OSIOEventsDriver class >> initialize [
	EventDispatchMap := Dictionary newFromPairs: {
		OS_IOEVENTS_EVENT_TYPE_PROCESS_PIPE_READY . #(processPipe onProcessPipeEvent:) .
		OS_IOEVENTS_EVENT_TYPE_PROCESS_FINISHED . #(processFinished onProcessFinishedEvent:) .
		OS_IOEVENTS_EVENT_TYPE_FSMONITOR . #(fsmonitor onFSMonitorEvent: ).
	}.

]

{ #category : #initialization }
OSIOEventsDriver class >> isAvailable [
	^ LibOSIOEvents isAvailable
]

{ #category : #accessing }
OSIOEventsDriver class >> uniqueInstance [
	^ uniqueInstance ifNil: [ uniqueInstance := self new ].
]

{ #category : #'session checking' }
OSIOEventsDriver >> checkSession [
	session ~~ Smalltalk session ifTrue: [ 
		self initializeForNewSession
	]
]

{ #category : #events }
OSIOEventsDriver >> createEventCoalescingProcess [
	eventCoalescingProcess := [
		self eventCoalescingProcessEntry
	] forkAt: Processor lowIOPriority named: 'PhanideDriver event loop'.

]

{ #category : #events }
OSIOEventsDriver >> createEventFetchingProcess [
	context canSignalExternalSemaphore ifTrue: [ 
		eventFetchingProcess := [
			self eventFetchingProcessEntry
		] forkAt: Processor lowIOPriority named: 'PhanideDriver event loop'.
	] ifFalse: [ 
		eventFetchingProcess := [
			self pollingEventFetchingProcessEntry
		] forkAt: Processor lowIOPriority named: 'PhanideDriver event loop'.
	]

]

{ #category : #'file system' }
OSIOEventsDriver >> destroyFSMonitor: handle [
	| handleValue |
	handleValue := handle handle handle getHandle.
	fsmonitorMutex critical: [
		fsmonitorDictionary at: handleValue ifPresent: [
			handle handle destroyOnContext: context.
			fsmonitorDictionary removeKey: handleValue ifAbsent: []
		]
	]

]

{ #category : #events }
OSIOEventsDriver >> eventCoalescingIteration [
	| currentTime toProcess allPendingWatchers |
	currentTime := Time microsecondClockValue.
	toProcess := OrderedCollection new.
	pendingCoalescingWatcherMutex critical: [
		allPendingWatchers := pendingCoalescingWatcher asArray.
		allPendingWatchers do: [ :watcher |
			watcher nextEventCoalescingTime >= currentTime ifTrue: [
				pendingCoalescingWatcher remove: watcher.
				toProcess add: watcher.
			]
		].
	].

	toProcess ifNotEmpty: [
		toProcess do: [ :watcher | watcher coalesceAndDispatchEvents ]
	]
]

{ #category : #events }
OSIOEventsDriver >> eventCoalescingProcessEntry [
	[
		(session ~~ Smalltalk session or: [context isNil or: [context isNull]]) ifTrue: [ ^ self ].
		self eventCoalescingIteration.
		(Delay forMilliseconds: 100) wait 
	] repeat
]

{ #category : #events }
OSIOEventsDriver >> eventFetchingIteration [
	| event |
	event := LibOSIOEventsEvent new.
	[ context pollEvent: event ] whileTrue: [
		self processEvent: event
	]
]

{ #category : #events }
OSIOEventsDriver >> eventFetchingProcessEntry [
	[
		[
			(session ~~ Smalltalk session or: [context isNil or: [context isNull]]) ifTrue: [ ^ self ].
			pendingEventsSemaphore consumeAllSignals.
			
			self eventFetchingIteration.
			
			pendingEventsSemaphore wait
		] repeat
	] ensure: [ Smalltalk unregisterExternalObject: pendingEventsSemaphore ]
]

{ #category : #accessing }
OSIOEventsDriver >> fsMonitorCanWatchDirectories [
	self checkSession.
	^ fsMonitorCanWatchDirectories
]

{ #category : #accessing }
OSIOEventsDriver >> fsMonitorCanWatchDirectoryFileModifications [
	self checkSession.
	^ fsMonitorCanWatchDirectoryFileModifications
]

{ #category : #accessing }
OSIOEventsDriver >> fsMonitorCanWatchFiles [
	self checkSession.
	^ fsMonitorCanWatchFiles
]

{ #category : #events }
OSIOEventsDriver >> getProcessFromHandle: aHandle [
	| process |
	processDictionaryMutex critical: [
		process := processDictionary at: aHandle ifAbsent: [ nil ].
	].
	^ process
]

{ #category : #initialization }
OSIOEventsDriver >> initialize [
]

{ #category : #'session checking' }
OSIOEventsDriver >> initializeForNewSession [
	eventFetchingProcess ifNotNil: [ eventFetchingProcess terminate ].
	eventCoalescingProcess ifNotNil: [ eventCoalescingProcess terminate ].
	pendingEventsSemaphore ifNotNil: [ 
		Smalltalk unregisterExternalObject: pendingEventsSemaphore
	].
	pendingEventsSemaphore := nil.
	fsMonitorCanWatchFiles := false.
	fsMonitorCanWatchDirectories := false.
	fsMonitorCanWatchDirectoryFileModifications := false.
	
	self isAvailable ifFalse: [ ^ self ].

	pendingEventsSemaphore := Semaphore new.
	pendingEventsSemaphoreIndex := Smalltalk registerExternalObject: pendingEventsSemaphore.
	
	context := LibOSIOEventsContext create: pendingEventsSemaphoreIndex.
	fsMonitorCanWatchFiles := context fsMonitorCanWatchFiles.
	fsMonitorCanWatchDirectories := context fsMonitorCanWatchDirectories.
	fsMonitorCanWatchDirectoryFileModifications := context fsMonitorCanWatchDirectoryFileModifications.
	session := Smalltalk session.
	processDictionaryMutex := Semaphore forMutualExclusion.
	processDictionary := WeakValueDictionary new.

	fsmonitorMutex := Semaphore forMutualExclusion.
	fsmonitorDictionary := WeakValueDictionary new.
	
	pendingCoalescingWatcherMutex := Semaphore forMutualExclusion.
	pendingCoalescingWatcher := WeakSet new.
	self createEventFetchingProcess.
	self createEventCoalescingProcess.
	
]

{ #category : #testing }
OSIOEventsDriver >> isAvailable [
	^ LibOSIOEvents isAvailable
]

{ #category : #'file system' }
OSIOEventsDriver >> onFSMonitorEvent: event [
	| namePointer name handle eventHandle |
	namePointer := event name.
	name := nil.
	(namePointer isNotNil and: [ namePointer isNull not ]) ifTrue: [ 
		name := namePointer readStringUTF8.
		LibOSIOEvents uniqueInstance free: namePointer
	].

	eventHandle := event handle.
	fsmonitorMutex critical: [
		handle := fsmonitorDictionary at: eventHandle getHandle ifAbsent: [ nil ].
		(event mask anyMask: OS_IOEVENTS_FSMONITOR_EVENT_DELETE_SELF) ifTrue: [ 
			(LibOSIOEventsFSMonitor fromHandle: eventHandle) destroyOnContext: context.
			fsmonitorDictionary removeKey: eventHandle getHandle ifAbsent: []
		]
	].

	handle ifNotNil: [
		handle eventHandler ifNotNil: [ :handler |
			handler value: (OSIOFSMonitorWatchEvent new
				handle: handle;
				mask: event mask;
				cookie: event cookie;
				name: name;
				yourself)
		].
	].

]

{ #category : #events }
OSIOEventsDriver >> onProcessFinishedEvent: event [
	| process |
	process := self getProcessFromHandle: event process getHandle.
	process ifNil: [ ^ self ].

	processDictionaryMutex critical: [
		processDictionary removeKey: event process getHandle
	].	
	process finishedWithExitCode: event exitCode.

]

{ #category : #events }
OSIOEventsDriver >> onProcessPipeEvent: event [
	| process |
	process := self getProcessFromHandle: event process getHandle.
	process ifNil: [ ^ self ].

	process signalPipe: event pipeIndex
]

{ #category : #events }
OSIOEventsDriver >> pollingEventFetchingProcessEntry [
	[
		(session ~~ Smalltalk session or: [context isNil or: [context isNull]]) ifTrue: [ ^ self ].
		self eventFetchingIteration.
		
		(Delay forMilliseconds: 100) wait
	] repeat
]

{ #category : #events }
OSIOEventsDriver >> processEvent: anEvent [
	| dispatchMethods |
	dispatchMethods := EventDispatchMap at: anEvent type ifAbsent: [ ^ self ].
	[
		self perform: dispatchMethods second with: (anEvent perform: dispatchMethods first)
	] on: Error do: [ :err |
		err freeze.
		[ err debug ] fork
	]
]

{ #category : #'as yet unclassified' }
OSIOEventsDriver >> queueWatcherForEventCoalescing: watcher in: targetTime [
	pendingCoalescingWatcherMutex critical: [
		watcher nextEventCoalescingTime: Time microsecondClockValue + targetTime.
		pendingCoalescingWatcher add: watcher.
	]
]

{ #category : #process }
OSIOEventsDriver >> spawnShell: aCommand [
	^ self spawnShell: aCommand flags: OS_IOEVENTS_SPAWN_FLAGS_NONE
]

{ #category : #process }
OSIOEventsDriver >> spawnShell: aCommand flags: flags [
	| handle process |
	self isAvailable ifFalse: [ ^ nil].
	
	self checkSession.
	processDictionaryMutex critical: [
		handle := context spawnShell: aCommand flags: flags.
		process := OSIOProcess newWithDriver: self handle: handle flags: flags.
		processDictionary at: handle getHandle getHandle put: process
	].

	^ process.
]

{ #category : #'file system' }
OSIOEventsDriver >> watchDirectory: aPath eventHandler: eventHandler [
	| result handle |
	self isAvailable ifFalse: [ ^ nil].
	
	self checkSession.
	fsmonitorMutex critical: [
		handle := (context watchDirectory: aPath).
		(handle isNil or: [ handle isNull ]) ifTrue: [ ^ nil ].
		
		result := OSIOFSMonitorWatchHandle new
			eventHandler: eventHandler;
			handle: handle;
			yourself.
		fsmonitorDictionary at: handle getHandle getHandle put: result
	].

	^ result
	

]

{ #category : #'file system' }
OSIOEventsDriver >> watchFile: aPath eventHandler: eventHandler [
	| result handle |
	self isAvailable ifFalse: [ ^ nil].
	
	self checkSession.
	fsmonitorMutex critical: [
		handle := (context watchFile: aPath).
		(handle isNil or: [ handle isNull ]) ifTrue: [ ^ nil ].
		
		result := OSIOFSMonitorWatchHandle new
			eventHandler: eventHandler;
			handle: handle;
			yourself.
		fsmonitorDictionary at: handle getHandle getHandle put: result
	].

	^ result
	

]
