" Vim syntax file
" Language: Hexabus Compiler Language
" Maintainer: Hexabus Dev Team
" Latest Revision: nn. July 2012

syntax clear

if exists("b:current_syntax")
  finish
endif

syn match hbcComment "#.*$" contains=hbcTodo
syn keyword hbcTodo TODO FIXME XXX NOTE

" Alias definitions
syn match hbcIPAddr /\<\x\{1,4}:\x\{1,4}:\x\{1,4}:\x\{1,4}:\x\{1,4}:\x\{1,4}:\x\{1,4}:\x\{1,4}\>/
syn keyword hbcAliasCommand ip nextgroup=hbcIPAddr
syn match hbcList '{(\d+,)*\d+}'
syn keyword hbcAliasCommand eids nextgroup=hbcList
syn keyword hbcAliasCommand endpoint nextgroup=hbcLoaclEPDef
" syn match hbcLocalEPDef TODO
syn match hbcAliasName '\h\w*' nextgroup=hbcAliasDef
syn region hbcAliasDef start="{" end="}" fold transparent contains=hbcAliasCommand,hbcIPAddr,hbcList,hbcLocalEPDef
syn keyword hbcAlias alias nextgroup=hbcAliasName

let b:current_syntax = "hbc"
hi def link hbcTodo           Todo
hi def link hbcComment        Comment
hi def link hbcAlias          Statement
hi def link hbcAliasCommand   Statement
hi def link hbcAliasName      Type
hi def link hbcIPAddr         Constant


" For reference:
" hi Keyword
" hi Statement
" hi Constant
" hi Number
" hi PreProc
" hi Function
" hi Identifier
" hi Type
" hi Special
" hi String
" hi Comment
" hi Todo
