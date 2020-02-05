"
I am a stream that decodes UTF8 binary data from a decorated stream.

TODO: Deprecate this in favor of the Zinc stream
"
Class {
	#name : #OSIOEventsUTF8TextReadStream,
	#superclass : #Stream,
	#instVars : [
		'stream',
		'peekValue'
	],
	#category : #'OSIOEvents-Bindings-Streams'
}

{ #category : #'instance creation' }
OSIOEventsUTF8TextReadStream class >> on: aStream [
	^ self basicNew stream: aStream
]

{ #category : #testing }
OSIOEventsUTF8TextReadStream >> isBinary [
	^ false
]

{ #category : #accessing }
OSIOEventsUTF8TextReadStream >> lines [
	| line |
	^ Array streamContents: [ :out |
		[ line := self nextLine ] whileNotNil: [ 
			out nextPut: line
		]
	]
]

{ #category : #accessing }
OSIOEventsUTF8TextReadStream >> next [
	| result |
	result := self peek.
	peekValue := nil.
	^ result
]

{ #category : #accessing }
OSIOEventsUTF8TextReadStream >> next: n [
	| result c |
	result := ByteString new: n.
	1 to: n do: [ :i |
		c := self next.
		c ifNil: [
			^ result copyFrom: 1 to: i - 1
		].
		result at: i put: c
	].

	^ result
]

{ #category : #accessing }
OSIOEventsUTF8TextReadStream >> nextLine [
	| continue char result |
	result := ByteString streamContents: [ :out |
		continue := true.
		[ continue ] whileTrue: [ 
			char := self next.
			char = Character cr ifTrue: [ 
				self next.
				self peek = Character lf ifTrue: [ self next ].
				continue := false.
			] ifFalse: [
				char = Character lf ifTrue: [ 
					continue := false.
				] ifFalse: [
					char ifNil: [
						continue := false.
					] ifNotNil: [ 
						out nextPut: char
					]
				]
			]
		]
	].

	(char isNil and: [ result isEmpty ]) ifTrue: [ ^ nil ].
	^ result
]

{ #category : #accessing }
OSIOEventsUTF8TextReadStream >> peek [
	| b |
	peekValue ifNotNil: [ ^ peekValue ].
	
	b := stream next.
	b ifNil: [ ^ nil ].
	
	b <= 127 ifTrue: [
		peekValue := b asCharacter 
	] ifFalse: [
		self flag: 'TODO: Decode the UTF-8 character.'.
		self halt.
	].

	^ peekValue
]

{ #category : #accessing }
OSIOEventsUTF8TextReadStream >> stream [
	^ stream
]

{ #category : #accessing }
OSIOEventsUTF8TextReadStream >> stream: anObject [
	stream := anObject
]
