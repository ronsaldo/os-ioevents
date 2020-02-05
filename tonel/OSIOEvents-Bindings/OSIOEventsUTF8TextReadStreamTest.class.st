"
A PhanideUTF8TextReadStreamTest is a test class for testing the behavior of PhanideUTF8TextReadStream
"
Class {
	#name : #OSIOEventsUTF8TextReadStreamTest,
	#superclass : #TestCase,
	#category : #'OSIOEvents-Bindings-Tests'
}

{ #category : #tests }
OSIOEventsUTF8TextReadStreamTest >> testTextStream [
	| rawStream stream |
	rawStream := ReadStream on: ('Hello World...') asByteArray.
	stream := OSIOEventsUTF8TextReadStream on: rawStream.
	self assert: stream peek equals: $H.
	self assert: stream next equals: $H.
	self assert: (stream next: 3) equals: 'ell'.
	self assert: stream next equals: $o.
	self assert: (stream next: 20) equals: ' World...'.
]
