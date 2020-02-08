"
I am a watched element by the file system monitor.
"
Class {
	#name : #OSIOFileSystemMonitorWatchedElement,
	#superclass : #Object,
	#instVars : [
		'handle',
		'fileReference',
		'pendingUnprocessedEvents',
		'nextEventCoalescingTime',
		'dispatcher',
		'isDirectory',
		'needsExplicitChildPoll',
		'childrenState',
		'parentWatcher',
		'childrenWatchers',
		'childrenStateMutex',
		'supportedEventMask'
	],
	#pools : [
		'LibOSIOEventsPool'
	],
	#category : #'OSIOEvents-Bindings-FileSystem'
}

{ #category : #'instance creation' }
OSIOFileSystemMonitorWatchedElement class >> for: aFileReference [
	^ self basicNew initializeFor: aFileReference
]

{ #category : #private }
OSIOFileSystemMonitorWatchedElement >> checkChildrenWatcherExistenceIfNeeded [
	needsExplicitChildPoll ifFalse: [ ^ self ].
	childrenStateMutex critical: [
		self needsChildrenWatchers ifTrue: [
			childrenWatchers ifEmpty: [ 
				childrenState keys asArray do: [ :key | self ensureChildWatcherFor: key ]
			]
		] ifFalse: [
			childrenWatchers keys asArray do: [ :key | self removeChildWatcherFor: key ].
		]
	]
]

{ #category : #private }
OSIOFileSystemMonitorWatchedElement >> checkDestroyCondition [
	(parentWatcher isNil and: [dispatcher hasSubscriptions not]) ifTrue: [
		self destroy
	].
]

{ #category : #'events-processing' }
OSIOFileSystemMonitorWatchedElement >> coalesceAndDispatchEvents [
	| coalescedElementDictionary coalescedEvents pendingUnprocessedEvent lastCoalescedEvent coalescedEvent |
	coalescedElementDictionary := Dictionary new.
	coalescedEvents := OrderedCollection new.
	lastCoalescedEvent := nil.
	[pendingUnprocessedEvent := pendingUnprocessedEvents nextOrNil] whileNotNil: [
		coalescedEvent := coalescedElementDictionary at: pendingUnprocessedEvent name ifAbsentPut: [
			| syntheticEvent |
			syntheticEvent := pendingUnprocessedEvent copy.
			coalescedEvents add: syntheticEvent.
			syntheticEvent
		].
		coalescedEvent mask: (coalescedEvent mask bitOr: pendingUnprocessedEvent mask).
		
		lastCoalescedEvent ifNotNil: [ 
			((coalescedEvent mask anyMask: OS_IOEVENTS_FSMONITOR_EVENT_MOVED_FROM) and:
			[ lastCoalescedEvent mask anyMask: OS_IOEVENTS_FSMONITOR_EVENT_MOVED_TO]) ifTrue: [
				coalescedEvents add: (OSIOFileEventMoved new
					fileReference: coalescedEvent fileReference;
					newFileReference: lastCoalescedEvent fileReference;
					yourself)
			]. 
			((coalescedEvent mask anyMask: OS_IOEVENTS_FSMONITOR_EVENT_MOVED_TO) and:
			[ lastCoalescedEvent mask anyMask: OS_IOEVENTS_FSMONITOR_EVENT_MOVED_FROM]) ifTrue: [
				coalescedEvents add: (OSIOFileEventMoved new
					fileReference: lastCoalescedEvent fileReference;
					newFileReference: coalescedEvent fileReference;
					yourself)
			]. 
		].
		
		lastCoalescedEvent := coalescedEvent
	].

	coalescedEvents do: [ :each |
		each deliverWithWatcher: self
	].

]

{ #category : #'events-processing' }
OSIOFileSystemMonitorWatchedElement >> deliverProcessedEvent: processedEvent [
	| isForMyself |
	isForMyself := processedEvent fileReference = fileReference.
	processedEvent isModify ifTrue: [
		(needsExplicitChildPoll and: [ isForMyself ]) ifTrue: [ 
			self pollNewChildrenStateAndDeliverEvents
		].
	].

	dispatcher deliverEvent: processedEvent.
	isForMyself ifTrue: [
		parentWatcher ifNotNil: [ :p | parentWatcher deliverProcessedEvent: processedEvent ].
		"Synthesize additional events, in case there are not supported by the underlying platform."
		processedEvent isModify ifTrue: [ 
			(fileReference exists
			and: [ fileReference isDirectory not
			and: [ (supportedEventMask anyMask: OS_IOEVENTS_FSMONITOR_EVENT_CLOSE_WRITE) not ]]) ifTrue: 	[
			self deliverProcessedEvent: processedEvent asCloseWriteEvent
			]
		].
	] ifFalse: [
		dispatcher deliverChild: fileReference basename event: processedEvent		
	].

]

{ #category : #initialization }
OSIOFileSystemMonitorWatchedElement >> destroy [
	OSIOFileSystemMonitor uniqueInstance destroyWatcher: self.
	handle := nil.
]

{ #category : #'events-processing' }
OSIOFileSystemMonitorWatchedElement >> doPollNewChildrenStateAndDeliverEvents [
	| newChildrenState needsChildrenWatchers |
	newChildrenState := self fetchCurrentChildrenState.
	needsChildrenWatchers := self needsChildrenWatchers.
	
	childrenState keysAndValuesDo: [ :key :oldState |
		newChildrenState at: key ifAbsent: [
			self deliverProcessedEvent: (OSIOFileEventDelete new
				fileReference: oldState fileReference).
			needsChildrenWatchers ifTrue: [
				self removeChildWatcherFor: key
			]
		]
	].

	newChildrenState keysAndValuesDo: [ :key :newState |
		needsChildrenWatchers ifTrue: [
			self ensureChildWatcherFor: key
		].

		childrenState at: key ifPresent: [ :oldState |
			oldState size ~= newState size ifTrue: [ 
				self deliverProcessedEvent: (OSIOFileEventModify new
					fileReference: oldState fileReference)
			].
		] ifAbsent: [ 
			self 
				deliverProcessedEvent: (OSIOFileEventCreate new
					fileReference: newState fileReference);
				deliverProcessedEvent: (OSIOFileEventCloseWrite new
					fileReference: newState fileReference)
		]
	].

	childrenState := newChildrenState
]

{ #category : #'events-processing' }
OSIOFileSystemMonitorWatchedElement >> enqueueEventProcessing: rawEvent [
	| eventWithTimesTamp elementFileReference mask |
	elementFileReference := rawEvent name ifNil: [ fileReference ] ifNotNil: [ fileReference / rawEvent name ].
	mask := rawEvent mask.
	eventWithTimesTamp := OSIOFileSystemMonitorUnprocessedEvent new
		mask: mask;
		fileReference: elementFileReference;
		name: rawEvent name;
		timeStamp: Time microsecondClockValue;
		yourself.
	pendingUnprocessedEvents nextPut: eventWithTimesTamp.
	OSIOEventsDriver uniqueInstance queueWatcherForEventCoalescing: self in: 200000
]

{ #category : #'events-processing' }
OSIOFileSystemMonitorWatchedElement >> ensureChildWatcherFor: childName [
	childrenWatchers at: childName ifAbsentPut: [
		| watcher |
		watcher := OSIOFileSystemMonitor uniqueInstance for: childName.
		self assert: watcher parentWatcher isNil.
		watcher parentWatcher: self.
		watcher
	]

]

{ #category : #'events-processing' }
OSIOFileSystemMonitorWatchedElement >> fetchCurrentChildrenState [
	| newChildrenState |
	newChildrenState := Dictionary new.
	
	fileReference children do: [ :child |
		| newState |
		newState := OSIOFileSystemMonitorFileState for: child.
		newChildrenState at: newState name put: newState
	].
	^ newChildrenState
]

{ #category : #accessing }
OSIOFileSystemMonitorWatchedElement >> fileReference [
	^ fileReference
]

{ #category : #accessing }
OSIOFileSystemMonitorWatchedElement >> fileReference: anObject [
	fileReference := anObject
]

{ #category : #accessing }
OSIOFileSystemMonitorWatchedElement >> handle [
	^ handle
]

{ #category : #accessing }
OSIOFileSystemMonitorWatchedElement >> handle: anObject [
	handle := anObject
]

{ #category : #initialization }
OSIOFileSystemMonitorWatchedElement >> initializeFor: aFileReference [
	fileReference := aFileReference.
	pendingUnprocessedEvents := WaitfreeQueue new.
	dispatcher := OSIOEventsEventDispatcher new owner: self.
	isDirectory := aFileReference isDirectory.
	needsExplicitChildPoll := isDirectory and: [ OSIOEventsDriver uniqueInstance fsMonitorCanWatchDirectoryFileModifications not ].
	childrenWatchers := Dictionary new.
	childrenStateMutex := Semaphore forMutualExclusion.
	
	isDirectory ifTrue: [
		needsExplicitChildPoll ifTrue: [ childrenState := self fetchCurrentChildrenState ].
		handle := OSIOEventsDriver uniqueInstance watchDirectory: aFileReference fullName eventHandler:
			[ :event | self enqueueEventProcessing: event ]
	] ifFalse: [
		handle := OSIOEventsDriver uniqueInstance watchFile: aFileReference fullName eventHandler:
			[ :event | self enqueueEventProcessing: event ]
	].
	supportedEventMask := handle supportedEventMask.

]

{ #category : #testing }
OSIOFileSystemMonitorWatchedElement >> needsChildrenWatchers [
	^ needsExplicitChildPoll and: [ dispatcher hasAnySubscriptionForEventClasses: { 
		OSIOFileEventCloseWrite
	} ]
]

{ #category : #accessing }
OSIOFileSystemMonitorWatchedElement >> nextEventCoalescingTime [
	^ nextEventCoalescingTime
]

{ #category : #accessing }
OSIOFileSystemMonitorWatchedElement >> nextEventCoalescingTime: anObject [
	nextEventCoalescingTime := anObject
]

{ #category : #'event subscriptions' }
OSIOFileSystemMonitorWatchedElement >> onChild: basename subscribeEvent: anEvent withAction: aBlock [
	| subscription |
	subscription := dispatcher onChild: basename subscribeEvent: anEvent withAction: aBlock.
	self checkChildrenWatcherExistenceIfNeeded.
	^ subscription
]

{ #category : #accessing }
OSIOFileSystemMonitorWatchedElement >> parentWatcher [
	^ parentWatcher
]

{ #category : #accessing }
OSIOFileSystemMonitorWatchedElement >> parentWatcher: anObject [
	parentWatcher := anObject.
	parentWatcher ifNil: [ self checkDestroyCondition ].
]

{ #category : #'events-processing' }
OSIOFileSystemMonitorWatchedElement >> pollNewChildrenStateAndDeliverEvents [
	childrenStateMutex critical: [ 
		self doPollNewChildrenStateAndDeliverEvents
	]
]

{ #category : #'events-processing' }
OSIOFileSystemMonitorWatchedElement >> removeChildWatcherFor: childName [
	childrenWatchers at: childName ifPresent: [ :existent |
		existent parentWatcher: nil.
		childrenWatchers removeKey: childName
	]
]

{ #category : #'event subscriptions' }
OSIOFileSystemMonitorWatchedElement >> subscribeEvent: anEvent withAction: aBlock [
	| subscription |
	subscription := dispatcher subscribeEvent: anEvent withAction: aBlock.
	self checkChildrenWatcherExistenceIfNeeded.
	^ subscription
]

{ #category : #private }
OSIOFileSystemMonitorWatchedElement >> subscriptionRemoved [
	self checkChildrenWatcherExistenceIfNeeded.
	self checkDestroyCondition.
]
