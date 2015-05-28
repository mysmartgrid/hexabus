syntax clear

syn match hbcNumber "[+-]\?\(\d*\.\d\+\|\d\+\)\(e-\?\d\+\)\?"
syn match hbcIdentifier "[_a-zA-Z][_a-zA-Z0-9]*"
syn match hbcIPLiteral "[0-9a-fA-F:]*" contained
syn match hbcOperator "[-+*/%&|^]\|\(&&\|||\)"
syn match hbcLineComment "//.*"

syn region hbcBlockComent start="/\*" end="\*/"
syn region hbcIP start="(" end=")" contains=hbcIPLiteral contained transparent
syn region hbcDeviceDefinition start="device" end=";" transparent contains=hbcIP,hbcIdentifier,hbcKwDevice
syn region hbcBehaviourDefinition start="behaviour" end="[{:]" transparent contains=hbcOnInBehaviour,hbcIdentifier,hbcKwBehaviour,hbcKwClass
syn region hbcMachineDefinition start="machine" end="[{:]" transparent contains=hbcToplevelDefinition
syn region hbcIncludeLine start="include" end=";"

syn keyword hbcType bool int8 int16 int32 int64 uint8 uint16 uint32 uint64 float typeof
syn keyword hbcToplevelDefinition endpoint machine
syn keyword hbcKwClass class
syn keyword hbcEndpointAccess read write global_write broadcast
syn keyword hbcBoolConstant true false
syn keyword hbcEventKw on entry update from periodic exit always
syn keyword hbcConditional if else switch
syn keyword hbcLabel case default
syn keyword hbcGoto goto
syn keyword hbcKwDevice device
syn keyword hbcKwState state
syn keyword hbcKwBehaviour behaviour
syn keyword hbcOnInBehaviour on contained
syn keyword hbcTodo TODO FIXME contained containedin=hbcBlockComent,hbcLineComment

hi link hbcLineComment         Comment
hi link hbcBlockComent         Comment
hi link hbcType                Type
hi link hbcToplevelDefinition  Type
hi link hbcKwDevice            Type
hi link hbcKwState             Keyword
hi link hbcEndpointAccess      Keyword
hi link hbcBoolConstant        Boolean
hi link hbcNumber              Number
hi link hbcIdentifier          Identifier
hi link hbcIPLiteral           Special
hi link hbcDeviceDefinition    Special
hi link hbcEventKw             Keyword
hi link hbcConditional         Conditional
hi link hbcLabel               Label
hi link hbcOperator            Operator
hi link hbcGoto                Keyword
hi link hbcBehaviourDefinition Type
hi link hbcOnInBehaviour       Type
hi link hbcKwBehaviour         Type
hi link hbcKwClass             Type
hi link hbcIncludeLine         Include
hi link hbcTodo                Todo
