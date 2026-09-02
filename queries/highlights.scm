; Headings
(heading_marker) @markup.heading
(heading_marker_with_label) @markup.heading
(heading
  "}" @markup.heading)

; Style markup
(bold
  "{b" @markup.bold
  "}" @markup.bold)
(bold) @markup.bold

(italic
  "{i" @markup.italic
  "}" @markup.italic)
(italic) @markup.italic

(emphasis
  "{e" @markup.italic
  "}" @markup.italic)
(emphasis) @markup.italic

(superscript
  "{^" @markup.italic
  "}" @markup.italic)

(subscript
  "{_" @markup.italic
  "}" @markup.italic)

; Code
(code_span
  "[" @punctuation.bracket
  "]" @punctuation.bracket) @markup.raw

(code_block
  "{[" @punctuation.bracket
  "]}" @punctuation.bracket) @markup.raw.block

(code_block_with_lang
  "{@" @punctuation.bracket
  "]}" @punctuation.bracket) @markup.raw.block

(code_block_with_lang
  (code_block_open_delimiter) @punctuation.bracket
  "]" @punctuation.bracket
  "}" @punctuation.bracket) @markup.raw.block

; A delimited code block may be followed by its results:
; {delim@lang[code]delim[results]}.  The results are markup, not raw text, so
; only the code and the delimiters are highlighted here.
(code_block_with_lang
  (code_block_open_delimiter) @punctuation.bracket
  (code_block_content) @markup.raw.block
  "]}" @punctuation.bracket)

(language) @label
(code_block_open_delimiter) @label
(code_block_delimiter_close) @label

; Verbatim
(verbatim_block) @markup.raw.block

; Math
(math_span
  "{m" @punctuation.bracket
  "}" @punctuation.bracket) @markup.math

(math_block
  "{math" @punctuation.bracket
  "}" @punctuation.bracket) @markup.math

; References
(simple_reference
  "{!" @punctuation.bracket
  "}" @punctuation.bracket)
(reference_target) @markup.link

(reference_with_text
  "{{!" @punctuation.bracket
  "}" @punctuation.bracket)

(simple_link
  "{:" @punctuation.bracket
  "}" @punctuation.bracket)
(link_target) @markup.link.url

(link_with_text
  "{{:" @punctuation.bracket
  "}" @punctuation.bracket)

; Media ({image!...}, {video:...}, {audio!...} and replacement-text forms)
(simple_media
  "}" @punctuation.bracket)
(media_with_text
  "}" @punctuation.bracket)
(media_target) @markup.link.url

; Lists (heavy)
(unordered_list
  "{ul" @keyword
  "}" @keyword)

(ordered_list
  "{ol" @keyword
  "}" @keyword)

(dash_list_item
  "{-" @punctuation.special
  "}" @punctuation.special)

(li_list_item
  "{li" @punctuation.special
  "}" @punctuation.special)

; Tables (heavy)
(table_heavy
  "{table" @keyword
  "}" @keyword)

(table_row
  "{tr" @keyword
  "}" @keyword)

(table_header_cell
  "{th" @keyword
  "}" @keyword)

(table_data_cell
  "{td" @keyword
  "}" @keyword)

; Tables (light)
(table_light
  "{t" @keyword
  "}" @keyword)

(table_separator) @punctuation.delimiter

; Module list
(module_list
  "{!modules:" @keyword
  "}" @keyword)
(module_name) @module

; Raw markup
(raw_markup
  "{%" @punctuation.bracket
  "%}" @punctuation.bracket)

; Paragraph style
(paragraph_style
  "{L" @keyword)
(paragraph_style
  "{C" @keyword)
(paragraph_style
  "{R" @keyword)
(paragraph_style
  "}" @keyword)

; Tags — simple token rules are highlighted as whole nodes
(tag_author
  "@author" @attribute)
(tag_deprecated) @attribute
(tag_param
  "@param" @attribute)
(param_name) @variable.parameter
(tag_raise
  (raise_name) @type)
(tag_return) @attribute
(tag_see
  "@see" @attribute)
(see_url) @markup.link.url
(see_file) @string
(see_doc) @string
(tag_since
  "@since" @attribute)
(tag_before
  "@before" @attribute)
(before_version) @string
(tag_version
  "@version" @attribute)
(tag_canonical
  "@canonical" @attribute)
(tag_inline) @attribute
(tag_open) @attribute
(tag_closed) @attribute
(tag_hidden) @attribute
(tag_children_order) @attribute
(tag_toc_status) @attribute
(tag_order_category) @attribute
(tag_short_title) @attribute

; Escape sequences
(escape_sequence) @string.escape

; Words (plain text has no highlight — this is intentional)
