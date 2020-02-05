Class {
	#name : #OSIOFileSystemMonitorFileState,
	#superclass : #Object,
	#instVars : [
		'fileReference',
		'isDirectory',
		'size',
		'name'
	],
	#category : #'OSIOEvents-Bindings-FileSystem'
}

{ #category : #'instance creation' }
OSIOFileSystemMonitorFileState class >> for: aFileReference [
	^ self new
		fileReference: aFileReference;
		isDirectory: aFileReference isDirectory;
		size: aFileReference size;
		yourself
]

{ #category : #comparing }
OSIOFileSystemMonitorFileState >> <= other [
	^ self name <= other name
]

{ #category : #accessing }
OSIOFileSystemMonitorFileState >> fileReference [
	^ fileReference
]

{ #category : #accessing }
OSIOFileSystemMonitorFileState >> fileReference: anObject [
	fileReference := anObject
]

{ #category : #accessing }
OSIOFileSystemMonitorFileState >> isDirectory [
	^ isDirectory
]

{ #category : #accessing }
OSIOFileSystemMonitorFileState >> isDirectory: anObject [
	isDirectory := anObject
]

{ #category : #accessing }
OSIOFileSystemMonitorFileState >> name [
	^ name ifNil: [name := fileReference basename]
]

{ #category : #printing }
OSIOFileSystemMonitorFileState >> printOn: aStream [
	self name printOn: aStream
]

{ #category : #accessing }
OSIOFileSystemMonitorFileState >> size [
	^ size
]

{ #category : #accessing }
OSIOFileSystemMonitorFileState >> size: anObject [
	size := anObject
]
