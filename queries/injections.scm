; Inject the language specified in code blocks with language tags
; e.g., {@ocaml[let x = 1]} injects OCaml highlighting
(code_block_with_lang
  (language) @injection.language
  ) @injection.content

; Default code blocks without language tag get OCaml injection
(code_block) @injection.content
(#set! injection.language "ocaml")
