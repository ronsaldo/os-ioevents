"
I am a handle to a file system monitor object that is used for receiving events about the changes in a file or directory, if supported by the underlying platform specific APIs.
"
Class {
	#name : #LibOSIOEventsFSMonitor,
	#superclass : #LibOSIOEventsHandle,
	#category : #'OSIOEvents-Bindings-Bindings'
}

{ #category : #'initialize-release' }
LibOSIOEventsFSMonitor >> destroyOnContext: context [
	^ self ffiCall: #(void os_ioevents_fsmonitor_destroy(os_ioevents_context_t context, self))
]

{ #category : #'initialize-release' }
LibOSIOEventsFSMonitor >> supportedEventMask [
	^ self ffiCall: #(uint os_ioevents_fsmonitor_getSupportedEventMask(self))
]
