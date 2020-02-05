"
I am a stream that adds a buffer for writes

TODO: Deprecate this in favor of the Zinc stream
"
Class {
	#name : #OSIOEventsBufferedWriteStream,
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
OSIOEventsBufferedWriteStream class >> initialize [
	BufferSize := 4096
]

{ #category : #'instance creation' }
OSIOEventsBufferedWriteStream class >> on: aStream [
	^ self basicNew stream: aStream
]

{ #category : #flushing }
OSIOEventsBufferedWriteStream >> flush [
	self flushBuffer
]

{ #category : #flushing }
OSIOEventsBufferedWriteStream >> flushBuffer [
	| sizeToFlush |
	sizeToFlush := bufferPosition - 1.
	sizeToFlush = 0 ifTrue: [ ^ self ].
	stream write: sizeToFlush from: buffer offset: 1.
	bufferPosition := 1.
]

{ #category : #testing }
OSIOEventsBufferedWriteStream >> isBinary [
	^ stream isBinary
]

{ #category : #flushing }
OSIOEventsBufferedWriteStream >> nextPut: anElement [
	self remainingCapacity < 1 ifTrue: [
		self flushBuffer.
	].

	buffer at: bufferPosition put: anElement.
	bufferPosition := bufferPosition + 1
]

{ #category : #flushing }
OSIOEventsBufferedWriteStream >> nextPutAll: aCollectionOfElements [
	| remainingSize sourceIndex |
	aCollectionOfElements size <= bufferSize ifTrue: [ 
		remainingSize := aCollectionOfElements size.
		sourceIndex := 1.
		bufferPosition to: (bufferPosition + remainingSize - 1 min: bufferSize) do: [ :destIndex |
			buffer at: destIndex put: (aCollectionOfElements at: sourceIndex).
			sourceIndex := sourceIndex + 1.
			bufferPosition := bufferPosition + 1.
		].

		"Flush the buffer and put the remaining data on it."	
		remainingSize > 0 ifTrue: [ 
			self flushBuffer.
			1 to: remainingSize do: [ :destIndex |
				buffer at: destIndex put: (aCollectionOfElements at: sourceIndex).
				sourceIndex := sourceIndex + 1.
				bufferPosition := bufferPosition + 1.
			]
		]
		
	] ifFalse: [
		"Large transfer, write directly into the target stream."
		self flushBuffer.
		stream write: aCollectionOfElements size from: aCollectionOfElements offset: 1
	]
]

{ #category : #accessing }
OSIOEventsBufferedWriteStream >> remainingCapacity [
	^ bufferSize - bufferPosition + 1
]

{ #category : #accessing }
OSIOEventsBufferedWriteStream >> stream [
	^ stream
]

{ #category : #accessing }
OSIOEventsBufferedWriteStream >> stream: anObject [
	stream := anObject.
	buffer := stream isBinary ifTrue: [ ByteArray new: BufferSize ] ifFalse: [ ByteString new: BufferSize ].
	bufferPosition := 1.
	bufferSize := BufferSize.
]
