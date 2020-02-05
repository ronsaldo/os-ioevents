"
I am a Phanide event dispatcher. I take care of managing event subscriptions, and delivering events to the subscribers.
"
Class {
	#name : #OSIOEventsEventDispatcher,
	#superclass : #Object,
	#instVars : [
		'mutex',
		'subscriptionRegistry',
		'childDispatchers',
		'parentDispatcher',
		'childEventClassCounts',
		'owner'
	],
	#category : #'OSIOEvents-Bindings-Event'
}

{ #category : #'as yet unclassified' }
OSIOEventsEventDispatcher >> decrementChildEventClassCount: anEventClass [
	mutex critical: [
		childEventClassCounts at: anEventClass put: (childEventClassCounts at: anEventClass ifAbsent: [0]) + 1
	].
	owner ifNotNil: [ owner subscriptionRemoved ]
]

{ #category : #'dispatching events' }
OSIOEventsEventDispatcher >> deliverChild: aChild event: anEvent [
	| childDispatcher |
	childDispatcher := mutex critical: [
		childDispatchers at: aChild ifAbsent: [ nil ]
	].

	childDispatcher ifNotNil: [ childDispatcher deliverEvent: anEvent ]
]

{ #category : #'dispatching events' }
OSIOEventsEventDispatcher >> deliverEvent: anEvent [
	| subscriptions |
	subscriptions := mutex critical: [
		(subscriptionRegistry at: anEvent class ifAbsent: [ #() ]) asArray
	].
	subscriptions do: [ :each | each deliverEvent: anEvent ]
]

{ #category : #testing }
OSIOEventsEventDispatcher >> hasAnySubscriptionForEventClasses: eventClasses [
	mutex critical: [ 
		eventClasses do: [ :cls |
			subscriptionRegistry at: cls ifPresent: [ :subscriptions |
				subscriptions ifNotEmpty: [ 
					^ true
				]
			].
			(childEventClassCounts at: cls ifAbsent: [ 0 ]) > 0 ifTrue: [ ^ true ]
		].
	].
	^ false
]

{ #category : #testing }
OSIOEventsEventDispatcher >> hasSubscriptions [
	mutex critical: [ 
		subscriptionRegistry valuesDo: [ :subscriptions | subscriptions ifNotEmpty: [ ^ true ]].
		childEventClassCounts valuesDo: [ :count | count > 0 ifTrue: [ ^ true ] ].
	].
	^ false
]

{ #category : #'as yet unclassified' }
OSIOEventsEventDispatcher >> incrementChildEventClassCount: anEventClass [
	mutex critical: [
		childEventClassCounts at: anEventClass put: (childEventClassCounts at: anEventClass ifAbsent: [0]) + 1
	]
]

{ #category : #initialization }
OSIOEventsEventDispatcher >> initialize [
	super initialize.
	mutex := Mutex new.
	subscriptionRegistry := IdentityDictionary new.
	childDispatchers := Dictionary new.
	childEventClassCounts := IdentityDictionary new.
	parentDispatcher := WeakArray new: 1.
]

{ #category : #'as yet unclassified' }
OSIOEventsEventDispatcher >> onChild: basename subscribeEvent: anEvent withAction: aBlock [
	^ mutex critical: [
		(childDispatchers at: basename ifAbsentPut: [
			self class new parentDispatcher: self
		]) subscribeEvent: anEvent withAction: aBlock
	]
]

{ #category : #accessing }
OSIOEventsEventDispatcher >> owner [
	^ owner
]

{ #category : #accessing }
OSIOEventsEventDispatcher >> owner: anObject [
	owner := anObject
]

{ #category : #initialization }
OSIOEventsEventDispatcher >> parentDispatcher [
	^ parentDispatcher first
]

{ #category : #initialization }
OSIOEventsEventDispatcher >> parentDispatcher: aParentDispatcher [
	parentDispatcher at: 1 put: aParentDispatcher
]

{ #category : #removing }
OSIOEventsEventDispatcher >> removeSubscription: aSubscription [
	mutex critical: [ 
		subscriptionRegistry at: aSubscription eventClass ifPresent: [ :perClassSubscriptions |
			perClassSubscriptions remove: aSubscription ifAbsent: []
		].
		self parentDispatcher ifNotNil: [ :pd |
			pd decrementChildEventClassCount: aSubscription eventClass
		]
	].

	owner ifNotNil: [ owner subscriptionRemoved ]
]

{ #category : #'as yet unclassified' }
OSIOEventsEventDispatcher >> subscribeEvent: anEventClass withAction: aBlock [
	^ mutex critical: [
		| perEventSubscriptions subscription |
		perEventSubscriptions := subscriptionRegistry at: anEventClass ifAbsentPut: [ IdentitySet new ].
		subscription := OSIOEventsEventSubscription new
			dispatcher: self;
			eventClass: anEventClass;
			action: aBlock;
			yourself.
		perEventSubscriptions add: subscription.
		self parentDispatcher ifNotNil: [ :pd | parentDispatcher incrementChildEventClassCount: anEventClass ].
		subscription
	]
]
