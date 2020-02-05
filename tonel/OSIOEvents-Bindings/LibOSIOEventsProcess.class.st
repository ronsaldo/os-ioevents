"
I am a handle to a subprocess that is created by using libphanide.
"
Class {
	#name : #LibOSIOEventsProcess,
	#superclass : #LibOSIOEventsHandle,
	#category : #'OSIOEvents-Bindings-Bindings'
}

{ #category : #actions }
LibOSIOEventsProcess >> destroy [
	^ self ffiCall: #(void os_ioevents_process_free(self))
]

{ #category : #actions }
LibOSIOEventsProcess >> kill [
	^ self ffiCall: #(void os_ioevents_process_kill(self))
]

{ #category : #pipes }
LibOSIOEventsProcess >> pipe: aPipeIndex readInto: aBuffer offset: aBufferOffset count: aBufferSize [
	^ self ffiCall: #(long os_ioevents_process_pipe_read(self, int aPipeIndex, ByteArray aBuffer, size_t aBufferOffset, size_t aBufferSize))
]

{ #category : #pipes }
LibOSIOEventsProcess >> pipe: aPipeIndex writeFrom: aBuffer offset: aBufferOffset count: aBufferSize [
	^ self ffiCall: #(long os_ioevents_process_pipe_write(self, int aPipeIndex, ByteArray aBuffer, size_t aBufferOffset, size_t aBufferSize))
]

{ #category : #actions }
LibOSIOEventsProcess >> terminate [
	^ self ffiCall: #(void os_ioevents_process_terminate(self))
]
