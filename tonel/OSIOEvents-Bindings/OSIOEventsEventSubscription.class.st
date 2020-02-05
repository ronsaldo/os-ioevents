"
I am a particular Phanide event subscription. I can be removed with the #unsubscribe message.
"
Class {
	#name : #OSIOEventsEventSubscription,
	#superclass : #OSIOEventsAbstractEventSubscription,
	#instVars : [
		'dispatcher',
		'eventClass',
		'action'
	],
	#category : #'OSIOEvents-Bindings-Event'
}

{ #category : #accessing }
OSIOEventsEventSubscription >> action [
	^ action
]

{ #category : #accessing }
OSIOEventsEventSubscription >> action: anObject [
	action := anObject
]

{ #category : #'dispatching events' }
OSIOEventsEventSubscription >> deliverEvent: anEvent [
	UIManager default defer: [ 
		action cull: anEvent cull: self
	].

]

{ #category : #accessing }
OSIOEventsEventSubscription >> dispatcher [
	^ dispatcher
]

{ #category : #accessing }
OSIOEventsEventSubscription >> dispatcher: anObject [
	dispatcher := anObject
]

{ #category : #accessing }
OSIOEventsEventSubscription >> eventClass [
	^ eventClass
]

{ #category : #accessing }
OSIOEventsEventSubscription >> eventClass: anObject [
	eventClass := anObject
]

{ #category : #'dispatching events' }
OSIOEventsEventSubscription >> unsubscribe [
	dispatcher ifNotNil: [ :d |
		d removeSubscription: self
	].
]
