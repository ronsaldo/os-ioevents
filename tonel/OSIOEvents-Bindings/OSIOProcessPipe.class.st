"
I represent a pipe from a process.
"
Class {
	#name : #OSIOProcessPipe,
	#superclass : #Stream,
	#instVars : [
		'process',
		'index',
		'ready',
		'readyCondition',
		'mutex',
		'processHasFinished'
	],
	#pools : [
		'LibOSIOEventsPool'
	],
	#category : #'OSIOEvents-Bindings-Driver'
}

{ #category : #construction }
OSIOProcessPipe class >> forProcess: aProcess index: aPipeIndex [
	^ self basicNew initializeForProcess: aProcess index: aPipeIndex
]

{ #category : #'as yet unclassified' }
OSIOProcessPipe >> convertPipeError: aPipeResult [
	aPipeResult < 0 ifTrue: [
		self error: 'Got an error from a pipe.'
	]
]

{ #category : #initialization }
OSIOProcessPipe >> initializeForProcess: aProcess index: aPipeIndex [
	process := aProcess.
	index := aPipeIndex.
	mutex := Semaphore forMutualExclusion.
	readyCondition := OSIOEventsConditionVariable new.
	ready := true.
	processHasFinished := false.
]

{ #category : #signalling }
OSIOProcessPipe >> isATTY [
	^ mutex critical: [
		processHasFinished
			ifTrue: [ false ]
			ifFalse: [ process isPipeATTY: index ]
	]

]

{ #category : #testing }
OSIOProcessPipe >> isBinary [
	^ true
]

{ #category : #'accessing structure variables' }
OSIOProcessPipe >> processFinished [
	mutex critical: [ 
		processHasFinished := true.
		readyCondition signalAll.
	].
]

{ #category : #signalling }
OSIOProcessPipe >> setTerminalRows: rows columns: columns [
	^ mutex critical: [
		processHasFinished
			ifTrue: [ false ]
			ifFalse: [ process pipe: index setTTYRows: rows columns: columns ].
	]

]

{ #category : #signalling }
OSIOProcessPipe >> signal [
	mutex critical: [ 
		ready := true.
		readyCondition signalAll
	]
	
]
