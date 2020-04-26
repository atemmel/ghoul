if exists("b:current_syntax")
	finish
endif

let b:current_syntax = "gh"

syn keyword Keyword fn if while for then var

syn keyword Statement return

syn keyword Keyword struct true false volatile

syn keyword Type void char int float bool

syn keyword cOperator link extern import

syn match Constant '\d\+'

syn region String start='"' end='"'

syn region Comment start="//" end="$"

"TODO: Rename and link
