"
I represent a condition variable.
"
Class {
	#name : #OSIOEventsConditionVariable,
	#superclass : #Object,
	#instVars : [
		'mutex',
		'waitingQueue'
	],
	#category : #'OSIOEvents-Bindings-Syncronization'
}

{ #category : #initialization }
OSIOEventsConditionVariable >> initialize [
	mutex := Semaphore forMutualExclusion.
	waitingQueue := OrderedCollection new.
	
]

{ #category : #communication }
OSIOEventsConditionVariable >> signal [
	| process |
	mutex critical: [
		waitingQueue ifNotEmpty: [
			process := waitingQueue removeFirst.
			process resume
		].
	]
]

{ #category : #communication }
OSIOEventsConditionVariable >> signalAll [
	mutex critical: [
		waitingQueue do: #resume.
		waitingQueue removeAll
	]
]

{ #category : #communication }
OSIOEventsConditionVariable >> wait: aSemaphoreMutexToRelease [
	"Release the mutex"
	| activeProcess waitFinished |
	mutex wait.
	aSemaphoreMutexToRelease signal.
	activeProcess := Processor activeProcess.
	waitingQueue add: activeProcess.
	
	"Release the mutex and suspend the active process, atomically."
	waitFinished := Semaphore new.
	[
		activeProcess suspend.
		mutex signal.
		waitFinished signal.
	] forkAt: Processor highestPriority.
	waitFinished wait.
	
	"Reacquire the mutex"
	aSemaphoreMutexToRelease wait
]
