Class {
	#name : #BaselineOfOSIOEvents,
	#superclass : #BaselineOf,
	#category : #BaselineOfOSIOEvents
}

{ #category : #baselines }
BaselineOfOSIOEvents >> baseline: spec [
	<baseline>

	spec for: #common do: [
		spec postLoadDoIt: #postLoadActions.
		spec
			baseline: #'PhaNAPI' with: [
				spec
					repository: 'github://ronsaldo/phanapi' ].
		spec
			package: #'OSIOEvents-Bindings' with: [
				spec requires: #(#'PhaNAPI')
			].
	].
]

{ #category : #doits }
BaselineOfOSIOEvents >> postLoadActions [
	(Smalltalk globals at: #LibOSIOEvents) fetchLibrary
]
