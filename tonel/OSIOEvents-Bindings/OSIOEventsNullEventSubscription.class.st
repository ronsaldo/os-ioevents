"
I am a null phanide event subscription. All of my operations are a nop.
"
Class {
	#name : #OSIOEventsNullEventSubscription,
	#superclass : #OSIOEventsAbstractEventSubscription,
	#classInstVars : [
		'uniqueInstance'
	],
	#category : #'OSIOEvents-Bindings-Event'
}

{ #category : #accessing }
OSIOEventsNullEventSubscription class >> uniqueInstance [
	^ uniqueInstance ifNil: [ uniqueInstance := self new ]
]

{ #category : #'dispatching events' }
OSIOEventsNullEventSubscription >> deliverEvent: anEvent [
	"Do nothing"
]

{ #category : #'dispatching events' }
OSIOEventsNullEventSubscription >> unsubscribe [
	"Do nothing"
]
