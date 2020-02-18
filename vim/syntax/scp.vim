if exists("b:current_syntax")
	finish
endif

let b:current_syntax = "scp"

syn keyword Keyword fn

syn keyword Keyword extern return

syn keyword Type void char int

syn match Constant '\d\+'

syn region String start='"' end='"'

syn region Comment start="//" end="$"

"TODO: Rename and link
