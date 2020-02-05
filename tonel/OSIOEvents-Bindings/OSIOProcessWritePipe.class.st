"
I represent a write only pipe from a process.
"
Class {
	#name : #OSIOProcessWritePipe,
	#superclass : #OSIOProcessPipe,
	#category : #'OSIOEvents-Bindings-Driver'
}

{ #category : #'as yet unclassified' }
OSIOProcessWritePipe >> bufferedStream [
	^ OSIOEventsBufferedWriteStream on: self
]

{ #category : #accessing }
OSIOProcessWritePipe >> nextPut: anInteger [

	^ self nextPutAll: (ByteArray with: anInteger)
]

{ #category : #accessing }
OSIOProcessWritePipe >> nextPutAll: aByteArray [
	self write: aByteArray size from: aByteArray
]

{ #category : #'as yet unclassified' }
OSIOProcessWritePipe >> textStream [
	^ (OSIOEventsUTF8TextWriteStream on: self bufferedStream)
		flushOnNewLines: true;
		yourself
]

{ #category : #writing }
OSIOProcessWritePipe >> write: n from: aBuffer [
	^ self write: n from: aBuffer offset: 1
]

{ #category : #writing }
OSIOProcessWritePipe >> write: n from: aBuffer offset: aBufferOffset [
	| resultCount |
	mutex critical: [
		[
			"Wait for the pipe to be ready"
			[processHasFinished or: [ready]] whileFalse: [ readyCondition wait: mutex ].

			resultCount := process handle
				ifNil: [ OS_IOEVENTS_PIPE_ERROR_CLOSED ]
				ifNotNil: [:handle | handle pipe: index writeFrom: aBuffer offset: aBufferOffset - 1 count: n ].
			resultCount = OS_IOEVENTS_PIPE_ERROR_WOULD_BLOCK ifTrue: [ 
				ready := false.
			].
		] doWhileTrue: [ ready not ]
	].

	"Treat a read from a closed pipe as a read of size 1."
	resultCount = OS_IOEVENTS_PIPE_ERROR_CLOSED ifTrue: [ 
		resultCount := 0.
	].

	self convertPipeError: resultCount.
	^ resultCount
]
