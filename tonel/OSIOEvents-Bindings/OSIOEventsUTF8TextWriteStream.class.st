"
I am a stream that encodes text with UTF-8 and writes it into a decorated stream.

TODO: Deprecate this in favor of the Zinc stream
"
Class {
	#name : #OSIOEventsUTF8TextWriteStream,
	#superclass : #Stream,
	#instVars : [
		'stream',
		'flushOnNewLines'
	],
	#category : #'OSIOEvents-Bindings-Streams'
}

{ #category : #'instance creation' }
OSIOEventsUTF8TextWriteStream class >> on: aStream [
	^ self basicNew stream: aStream
]

{ #category : #writing }
OSIOEventsUTF8TextWriteStream >> cr [
	self nextPut: Character cr.
	flushOnNewLines ifTrue: [
		stream flush
	]
]

{ #category : #writing }
OSIOEventsUTF8TextWriteStream >> crlf [
	self nextPutAll: String crlf.
	flushOnNewLines ifTrue: [
		stream flush
	]
]

{ #category : #accessing }
OSIOEventsUTF8TextWriteStream >> flushOnNewLines [
	^ flushOnNewLines
]

{ #category : #accessing }
OSIOEventsUTF8TextWriteStream >> flushOnNewLines: anObject [
	flushOnNewLines := anObject
]

{ #category : #writing }
OSIOEventsUTF8TextWriteStream >> lf [
	self nextPut: Character lf.
	flushOnNewLines ifTrue: [
		stream flush
	]
]

{ #category : #accessing }
OSIOEventsUTF8TextWriteStream >> nextPut: aCharacter [
	self primitiveNextPut: aCharacter.
	(flushOnNewLines and: [String crlf includes: aCharacter]) ifTrue: [
		stream flush
	]
	
]

{ #category : #accessing }
OSIOEventsUTF8TextWriteStream >> nextPutAll: aCollection [
	| newLineHaveBeenSeen |
	newLineHaveBeenSeen := false.
	aCollection do: [ :element |
		newLineHaveBeenSeen ifFalse: [ 
			newLineHaveBeenSeen := (String crlf includes: element)
		].
		
		self primitiveNextPut: element
	].
	
	(newLineHaveBeenSeen and: [flushOnNewLines]) ifTrue: [
		stream flush
	]
]

{ #category : #accessing }
OSIOEventsUTF8TextWriteStream >> primitiveNextPut: aCharacter [
	| codePoint |
	codePoint := aCharacter codePoint.
	codePoint <= 127 ifTrue: [
		stream nextPut: codePoint
	] ifFalse: [
		self halt.
	].
]

{ #category : #writing }
OSIOEventsUTF8TextWriteStream >> space [
	self nextPut: Character space
]

{ #category : #accessing }
OSIOEventsUTF8TextWriteStream >> stream [
	^ stream
]

{ #category : #accessing }
OSIOEventsUTF8TextWriteStream >> stream: anObject [
	stream := anObject.
	flushOnNewLines := false.
]

{ #category : #writing }
OSIOEventsUTF8TextWriteStream >> tab [
	self nextPut: Character tab
]
