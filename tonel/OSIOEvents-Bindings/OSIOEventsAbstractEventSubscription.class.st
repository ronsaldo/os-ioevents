"
I represent an abstract phanide event subscription.
"
Class {
	#name : #OSIOEventsAbstractEventSubscription,
	#superclass : #Object,
	#category : #'OSIOEvents-Bindings-Event'
}

{ #category : #'dispatching events' }
OSIOEventsAbstractEventSubscription >> deliverEvent: anEvent [
	self subclassResponsibility
]

{ #category : #'dispatching events' }
OSIOEventsAbstractEventSubscription >> unsubscribe [
	self subclassResponsibility
]
