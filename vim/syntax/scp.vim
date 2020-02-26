if exists("b:current_syntax")
	finish
endif

let b:current_syntax = "scp"

syn keyword Keyword fn

syn keyword Keyword extern return struct link

syn keyword Type void char int float

syn keyword cOperator link

syn match Constant '\d\+'

syn region String start='"' end='"'

syn region Comment start="//" end="$"

"TODO: Rename and link
