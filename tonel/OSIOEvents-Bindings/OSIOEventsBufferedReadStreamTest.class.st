"
A OSIOEventsBufferedReadStreamTest is a test class for testing the behavior of OSIOEventsBufferedReadStream
"
Class {
	#name : #OSIOEventsBufferedReadStreamTest,
	#superclass : #TestCase,
	#category : #'OSIOEvents-Bindings-Tests'
}

{ #category : #tests }
OSIOEventsBufferedReadStreamTest >> testBufferedStream [
	| rawStream stream |
	rawStream := ReadStream on: 'Hello World...'.
	stream := OSIOEventsBufferedReadStream on: rawStream.
	self assert: stream peek equals: $H.
	self assert: stream next equals: $H.
	self assert: (stream next: 3) equals: 'ell'.
	self assert: stream next equals: $o.
	self assert: (stream next: 20) equals: ' World...'.
]
