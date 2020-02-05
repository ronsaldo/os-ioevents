"
A OSIOEventsConditionTest is a test class for testing the behavior of OSIOEventsCondition
"
Class {
	#name : #OSIOEventsConditionTest,
	#superclass : #TestCase,
	#category : #'OSIOEvents-Bindings-Tests'
}

{ #category : #tests }
OSIOEventsConditionTest >> testCondition [
	| condition mutex entry leave |
	condition := OSIOEventsConditionVariable new.
	mutex := Semaphore forMutualExclusion.
	entry := false.
	leave := false.
	[
		mutex critical: [ 
			entry := true.
			condition wait: mutex.
			leave := true.
		]
	] fork.

	(Delay forMilliseconds: 50) wait.
	self assert: entry.
	self deny: leave.
	condition signal.
	(Delay forMilliseconds: 50) wait.
	self assert: entry.
	
]
