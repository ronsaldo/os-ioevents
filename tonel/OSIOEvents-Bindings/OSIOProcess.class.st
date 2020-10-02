"
I represent a external subprocess.
"
Class {
	#name : #OSIOProcess,
	#superclass : #Object,
	#instVars : [
		'driver',
		'handle',
		'exitCode',
		'finishedCondition',
		'mutex',
		'pipes'
	],
	#pools : [
		'LibOSIOEventsPool'
	],
	#category : #'OSIOEvents-Bindings-Driver'
}

{ #category : #'instance creation' }
OSIOProcess class >> newWithDriver: driver handle: handle flags: flags [
	^ self basicNew initializeWithDriver: driver handle: handle flags: flags
]

{ #category : #events }
OSIOProcess >> destroy [
	handle ifNil: [ ^ self ].
	mutex critical: [ 
		handle destroy.
		handle := nil.
	].

]

{ #category : #accessing }
OSIOProcess >> driver [
	^ driver
]

{ #category : #accessing }
OSIOProcess >> extraStderr [
	^ pipes at: OS_IOEVENTS_PIPE_INDEX_EXTRA_STDERR + 1
]

{ #category : #accessing }
OSIOProcess >> extraStdin [
	^ pipes at: OS_IOEVENTS_PIPE_INDEX_EXTRA_STDIN + 1
]

{ #category : #accessing }
OSIOProcess >> extraStdout [
	^ pipes at: OS_IOEVENTS_PIPE_INDEX_EXTRA_STDOUT + 1
]

{ #category : #events }
OSIOProcess >> finish [
	self wait.
	self destroy.
]

{ #category : #events }
OSIOProcess >> finishedWithExitCode: anExitCode [
	mutex critical: [
		exitCode := anExitCode.
		finishedCondition signal.
		pipes do: [ :pipe | pipe processFinished]
	].

]

{ #category : #accessing }
OSIOProcess >> handle [
	^ handle
]

{ #category : #initialization }
OSIOProcess >> initializeWithDriver: aDriver handle: aHandle flags: flags [
	driver := aDriver.
	handle := aHandle.
	mutex := Semaphore forMutualExclusion.
	finishedCondition := OSIOEventsConditionVariable new.
	pipes := {
		"stdin" OSIOProcessWritePipe forProcess: self index: 0 .
		"stdout" OSIOProcessReadPipe forProcess: self index: 1 .
	}.
	(flags anyMask: OS_IOEVENTS_SPAWN_FLAGS_OPEN_IN_PSEUDO_TERMINAL) ifFalse: [ 
		pipes := pipes , {
			"stderr" OSIOProcessReadPipe forProcess: self index: 2
		}.
	].
	(flags anyMask: OS_IOEVENTS_SPAWN_FLAGS_OPEN_EXTRA_PIPES) ifTrue: [ 
		pipes := pipes , {
			"extra stdin" OSIOProcessWritePipe forProcess: self index: 3 .
			"extra stdout" OSIOProcessReadPipe forProcess: self index: 4
		}.
		
		(flags anyMask: OS_IOEVENTS_SPAWN_FLAGS_OPEN_EXTRA_PIPES_IN_PSEUDO_TERMINAL) ifFalse: [
			pipes := pipes, {
				"extra stderr" OSIOProcessReadPipe forProcess: self index: 5
			}.
		]
	]
]

{ #category : #waiting }
OSIOProcess >> kill [
	mutex critical: [
		handle ifNotNil: [ handle kill ]
	].
	self finish
]

{ #category : #events }
OSIOProcess >> signalPipe: pipeIndex [
	(pipes at: pipeIndex + 1) signal
]

{ #category : #accessing }
OSIOProcess >> stderr [
	^ pipes at: OS_IOEVENTS_PIPE_INDEX_STDERR + 1
]

{ #category : #accessing }
OSIOProcess >> stdin [
	^ pipes at: OS_IOEVENTS_PIPE_INDEX_STDIN + 1
]

{ #category : #accessing }
OSIOProcess >> stdout [
	^ pipes at: OS_IOEVENTS_PIPE_INDEX_STDOUT + 1
]

{ #category : #waiting }
OSIOProcess >> terminate [
	mutex critical: [
		handle ifNotNil: [ handle terminate ]
	].
	self finish
]

{ #category : #waiting }
OSIOProcess >> wait [
	exitCode ifNotNil: [ ^ exitCode ].
	mutex critical: [
		[exitCode] whileNil: [ 
			finishedCondition wait: mutex
		].
	
		^ exitCode
	]
]
