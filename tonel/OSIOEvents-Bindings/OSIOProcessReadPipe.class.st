"
I represent a read-only pipe from a process.
"
Class {
	#name : #OSIOProcessReadPipe,
	#superclass : #OSIOProcessPipe,
	#instVars : [
		'peeked'
	],
	#category : #'OSIOEvents-Bindings-Driver'
}

{ #category : #testing }
OSIOProcessReadPipe >> atEnd [
	^ self peek isNil.
]

{ #category : #'as yet unclassified' }
OSIOProcessReadPipe >> bufferedStream [
	^ ZnBufferedReadStream on: self
]

{ #category : #accessing }
OSIOProcessReadPipe >> next [
	"Answer the next byte from this pipe, or nil if at the end of the file."
	peeked ifNotNil: [
		| result |
		result := peeked.
		peeked := nil.
		^ result
	].

	^ (self next: 1) ifEmpty: [ nil ] ifNotEmpty: [ :data | data first ]
]

{ #category : #accessing }
OSIOProcessReadPipe >> next: n [
	"Return a string with the next n characters of the filestream in it."

	^ self next: n into: (ByteArray new: n)
]

{ #category : #accessing }
OSIOProcessReadPipe >> next: n into: aBuffer [
	| resultCount |
	resultCount := self read: n into: aBuffer.
	^ resultCount = n
		ifTrue: [ aBuffer ]
		ifFalse: [ aBuffer copyFrom: 1 to: resultCount ].
]

{ #category : #accessing }
OSIOProcessReadPipe >> peek [
	peeked ifNotNil: [ ^ peeked ].
	^ peeked := self next
]

{ #category : #accessing }
OSIOProcessReadPipe >> read: n into: aBuffer [
	^ self read: n into: aBuffer offset: 1
]

{ #category : #accessing }
OSIOProcessReadPipe >> read: n into: aBuffer offset: aBufferOffset [
	| resultCount startOffset readCount initialReadCount |
	startOffset := aBufferOffset.
	readCount := n.
	initialReadCount := 0.
	
	peeked ifNotNil: [
		aBuffer at: aBufferOffset put: peeked.
		startOffset := startOffset + 1.
		readCount := readCount - 1.
		peeked := nil.
		initialReadCount := 1.
		readCount = 0 ifTrue: [ ^ initialReadCount ]
	].
	
	mutex critical: [
		[
			"Wait for the pipe to be ready"
			[processHasFinished or: [ready or: [initialReadCount > 0]]] whileFalse: [ readyCondition wait: mutex ].
			
			resultCount := process handle
				ifNil: [ OS_IOEVENTS_PIPE_ERROR_CLOSED ]
				ifNotNil: [:handle | handle pipe: index readInto: aBuffer offset: startOffset - 1 count: readCount].
			resultCount = OS_IOEVENTS_PIPE_ERROR_WOULD_BLOCK ifTrue: [
				ready := false.
			].
		] doWhileFalse: [ processHasFinished or: [ready or: [initialReadCount > 0]]]
	].

	(resultCount <= 0 and: [initialReadCount > 0]) ifTrue: [
		^ initialReadCount
	].

	"Treat a read from a closed pipe as a read of size 1."
	resultCount = OS_IOEVENTS_PIPE_ERROR_CLOSED ifTrue: [ 
		resultCount := 0.
	].

	self convertPipeError: resultCount.
	^ initialReadCount + resultCount
]

{ #category : #accessing }
OSIOProcessReadPipe >> readInto: aBuffer startingAt: aBufferOffset count: n [
	^ self read: n into: aBuffer offset: aBufferOffset
]

{ #category : #'as yet unclassified' }
OSIOProcessReadPipe >> textStream [
	^ ZnCharacterReadStream on: self bufferedStream
]
