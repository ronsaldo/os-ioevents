Class {
	#name : #OSIOEventsLineFlushingWriteStream,
	#superclass : #ZnCharacterWriteStream,
	#instVars : [
		'flushOnNewLines'
	],
	#category : #'OSIOEvents-Bindings-Streams'
}

{ #category : #accessing }
OSIOEventsLineFlushingWriteStream >> flushOnNewLines [
	^ flushOnNewLines
]

{ #category : #accessing }
OSIOEventsLineFlushingWriteStream >> flushOnNewLines: anObject [
	flushOnNewLines := anObject
]

{ #category : #accessing }
OSIOEventsLineFlushingWriteStream >> nextPut: aCharacter [
	super nextPut: aCharacter.
	(flushOnNewLines and: [String crlf includes: aCharacter]) ifTrue: [
		stream flush
	]
	
]

{ #category : #accessing }
OSIOEventsLineFlushingWriteStream >> nextPutAll: aCollection [
	| newLineHaveBeenSeen result |
	result := super nextPutAll: aCollection.
	flushOnNewLines ifFalse: [ ^ result ].
	newLineHaveBeenSeen := false.
	(aCollection includesAnyOf: String crlf) ifTrue: [
		stream flush
	]
]
