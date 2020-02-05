Metacello new
  baseline: 'OSIOEvents';
  repository: 'gitlocal://.';
  load.

"Save the image"
(Smalltalk saveAs: 'osioevents')
    ifFalse: [ Smalltalk snapshot: false andQuit: true ].
