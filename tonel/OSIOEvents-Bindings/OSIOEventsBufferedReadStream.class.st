"
I am a stream that adds a buffer for reads.

TODO: Deprecate this in favor of the Zinc stream
"
Class {
	#name : #OSIOEventsBufferedReadStream,
	#superclass : #Stream,
	#instVars : [
		'stream',
		'buffer',
		'bufferPosition',
		'bufferSize'
	],
	#classVars : [
		'BufferSize'
	],
	#category : #'OSIOEvents-Bindings-Streams'
}

{ #category : #initialization }
OSIOEventsBufferedReadStream class >> initialize [
	BufferSize := 4096
]

{ #category : #'instance creation' }
OSIOEventsBufferedReadStream class >> on: aStream [
	^ self basicNew stream: aStream
]

{ #category : #accessing }
OSIOEventsBufferedReadStream >> buffer [
	^ buffer
]

{ #category : #accessing }
OSIOEventsBufferedReadStream >> fillBuffer [
	bufferSize := stream readInto: buffer startingAt: 1 count: buffer size.
	bufferPosition := 1.
	^ bufferSize
]

{ #category : #testing }
OSIOEventsBufferedReadStream >> isBinary [
	^ stream isBinary
]

{ #category : #accessing }
OSIOEventsBufferedReadStream >> next [
	| result |
	self remainingElementsInBuffer < 1 ifTrue: [ 
		self fillBuffer < 1 ifTrue: [ 
			^ nil
		]
	].
	
	result := buffer at: bufferPosition.
	bufferPosition := bufferPosition + 1.
	^ result
]

{ #category : #accessing }
OSIOEventsBufferedReadStream >> next: size [
	| result destIndex remainingToRead |
	result := buffer species new: size.
	destIndex := 1.
	remainingToRead := size.
	[remainingToRead > 0 ] whileTrue: [ 
		bufferPosition to: (bufferPosition + remainingToRead - 1 min: bufferSize) do: [ :sourceIndex |
			result at: destIndex put: (buffer at: sourceIndex).
			destIndex := destIndex + 1.
			remainingToRead := remainingToRead - 1.
			bufferPosition := bufferPosition + 1
		].
	
		remainingToRead > 0 ifTrue: [ 
			self fillBuffer < 1 ifTrue: [
				"EOF reached."
				^ result copyFrom: 1 to: destIndex - 1
			]
		].
	].

	^ result
]

{ #category : #accessing }
OSIOEventsBufferedReadStream >> peek [
	self remainingElementsInBuffer < 1 ifTrue: [ 
		self fillBuffer < 1 ifTrue: [ 
			^ nil
		]
	].
	
	^ buffer at: bufferPosition.
]

{ #category : #accessing }
OSIOEventsBufferedReadStream >> remainingElementsInBuffer [
	^ bufferSize - bufferPosition + 1
]

{ #category : #accessing }
OSIOEventsBufferedReadStream >> stream [
	^ stream
]

{ #category : #accessing }
OSIOEventsBufferedReadStream >> stream: anObject [
	stream := anObject.
	buffer := stream isBinary ifTrue: [ ByteArray new: BufferSize ] ifFalse: [ ByteString new: BufferSize ].
	bufferPosition := 1.
	bufferSize := 0
]
