"
I am handle to a context object in libphanide.
"
Class {
	#name : #LibOSIOEventsContext,
	#superclass : #LibOSIOEventsHandle,
	#category : #'OSIOEvents-Bindings-Bindings'
}

{ #category : #operations }
LibOSIOEventsContext class >> create [
	^ self create: 0
]

{ #category : #operations }
LibOSIOEventsContext class >> create: semaphoreIndex [
	^ self ffiCall: #(os_ioevents_context_t os_ioevents_createContext(size_t semaphoreIndex))
]

{ #category : #events }
LibOSIOEventsContext >> canCreateNamedExtraPipes [
	^ self isCapabilitySupported: OS_IOEVENTS_CAPABILITY_NAMED_EXTRA_PIPES
]

{ #category : #events }
LibOSIOEventsContext >> canCreateNumberedExtraPipes [
	^ self isCapabilitySupported: OS_IOEVENTS_CAPABILITY_NUMBERED_EXTRA_PIPES
]

{ #category : #events }
LibOSIOEventsContext >> canSignalExternalSemaphore [
	^ self isCapabilitySupported: OS_IOEVENTS_CAPABILITY_EXTERNAL_SEMAPHORE_SIGNALING
]

{ #category : #initialize }
LibOSIOEventsContext >> destroy [
	^ self ffiCall: #(void os_ioevents_destroyContext(this))
]

{ #category : #events }
LibOSIOEventsContext >> fsMonitorCanWatchDirectories [
	^ self isCapabilitySupported: OS_IOEVENTS_CAPABILITY_FSMONITOR_WATCH_DIRECTORIES
]

{ #category : #events }
LibOSIOEventsContext >> fsMonitorCanWatchDirectoryFileModifications [
	^ self isCapabilitySupported: OS_IOEVENTS_CAPABILITY_FSMONITOR_WATCH_DIRECTORY_FILE_MODIFICATIONS
]

{ #category : #events }
LibOSIOEventsContext >> fsMonitorCanWatchFiles [
	^ self isCapabilitySupported: OS_IOEVENTS_CAPABILITY_FSMONITOR_WATCH_FILES
]

{ #category : #events }
LibOSIOEventsContext >> fsMonitorHasCookie [
	^ self isCapabilitySupported: OS_IOEVENTS_CAPABILITY_FSMONITOR_COOKIE
]

{ #category : #initialization }
LibOSIOEventsContext >> initialize [ 
	handle value = 0 ifTrue: [ 
		self error: 'Error creating new context' ].
	FFIExternalResourceManager addResource: self data: handle
]

{ #category : #events }
LibOSIOEventsContext >> isCapabilitySupported: capability [
	^ self ffiCall: #(bool os_ioevents_isCapabilitySupported(self, int capability))
]

{ #category : #events }
LibOSIOEventsContext >> pollEvent: anEvent [
	^ self ffiCall: #(bool os_ioevents_pollEvent(self, os_ioevents_event_t *anEvent))
]

{ #category : #events }
LibOSIOEventsContext >> pushEvent: anEvent [
	^ self ffiCall: #(int os_ioevents_pushEvent(self, os_ioevents_event_t *anEvent))
]

{ #category : #process }
LibOSIOEventsContext >> spawn: path arguments: arguments [
	^ self spawn: path arguments: arguments flags: OS_IOEVENTS_SPAWN_FLAGS_NONE
]

{ #category : #process }
LibOSIOEventsContext >> spawn: path arguments: arguments flags: flags [
	^ self ffiCall: #(os_ioevents_process_t os_ioevents_process_spawnInPath(self, String path, String* arguments, int flags))
]

{ #category : #process }
LibOSIOEventsContext >> spawnInPath: file arguments: arguments [
	^ self spawnInPath: file arguments: arguments flags: OS_IOEVENTS_SPAWN_FLAGS_NONE
]

{ #category : #process }
LibOSIOEventsContext >> spawnInPath: file arguments: arguments flags: flags [
	^ self ffiCall: #(os_ioevents_process_t os_ioevents_process_spawnInPath(self, String file, String* arguments, int flags))
]

{ #category : #process }
LibOSIOEventsContext >> spawnShell: command [
	^ self spawnShell: command flags: OS_IOEVENTS_SPAWN_FLAGS_NONE
]

{ #category : #process }
LibOSIOEventsContext >> spawnShell: command flags: flags [
	^ self ffiCall: #(os_ioevents_process_t os_ioevents_process_spawnShell(self, String command, int flags))
]

{ #category : #events }
LibOSIOEventsContext >> waitEvent: anEvent [
	^ self ffiCall: #(bool os_ioevents_waitEvent(self, os_ioevents_event_t *anEvent))
]

{ #category : #'file system' }
LibOSIOEventsContext >> watchDirectory: path [
	^ self ffiCall: #(os_ioevents_fsmonitor_handle_t os_ioevents_fsmonitor_watchDirectory(self, String path))
]

{ #category : #'file system' }
LibOSIOEventsContext >> watchFile: path [
	^ self ffiCall: #(os_ioevents_fsmonitor_handle_t os_ioevents_fsmonitor_watchFile(self, String path))
]
