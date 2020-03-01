if exists("b:current_syntax")
	finish
endif

let b:current_syntax = "gh"

syn keyword Keyword fn if

syn keyword Statement return

syn keyword Keyword struct true false

syn keyword Type void char int float

syn keyword cOperator link extern

syn match Constant '\d\+'

syn region String start='"' end='"'

syn region Comment start="//" end="$"

"TODO: Rename and link
