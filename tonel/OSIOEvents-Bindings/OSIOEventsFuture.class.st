"
I represent a future value.
"
Class {
	#name : #OSIOEventsFuture,
	#superclass : #Object,
	#instVars : [
		'value',
		'mutex',
		'ready',
		'readyCondition'
	],
	#category : #'OSIOEvents-Bindings-Syncronization'
}

{ #category : #initialization }
OSIOEventsFuture >> initialize [
	super initialize.
	ready := false.
	mutex := Semaphore forMutualExclusion.
	readyCondition := OSIOEventsConditionVariable new.
]

{ #category : #accessing }
OSIOEventsFuture >> value [
	mutex critical: [ 
		[ ready ] whileFalse: [
			readyCondition wait: mutex
		].
	
		^ value
	]
]

{ #category : #accessing }
OSIOEventsFuture >> value: aValue [
	mutex critical: [ 
		ready ifTrue: [
			self error: 'Future value can only be set once.'
		].

		value := aValue.
		ready := true.
		readyCondition signalAll.
	].
	^ aValue
]
