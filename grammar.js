/// <reference types="tree-sitter-cli/dsl" />

/**
 * Inline content of a brace-delimited construct.
 *
 * odoc's parser turns a newline inside delimited markup into a space, so
 * `{b foo\nbar}` spans lines just fine, but a blank line is not allowed there
 * (see `delimited_inline_element_list` in odoc's `syntax.ml`).  Hence
 * `_inline_newline`, which the external scanner refuses to produce in front of
 * a blank line: markup left unclosed while editing then stops at the end of
 * its paragraph instead of swallowing the rest of the page.
 *
 * @param {GrammarSymbols<string>} $
 * @returns {RuleBuilder<string>}
 */
const inline_content = ($) => repeat(choice($._inline, $._inline_newline));

module.exports = grammar({
  name: 'odoc',

  extras: () => [/[ \t]/],

  externals: ($) => [
    $.code_block_content,
    $._verbatim_content,
    $._inline_code_content,
    $._math_block_content,
    $._math_inline_content,
    $._raw_markup_content,
    $._tag_text,
    $._light_list_bullet_dash,
    $._light_list_bullet_plus,
    // Every code block opener is external so that the scanner knows which
    // delimiter, if any, the block was opened with and stops its content at
    // the matching terminator rather than at the first ']}' in the code.
    $.code_block_open_delimiter,
    '{@',
    '{[',
    $._inline_newline,
  ],

  conflicts: () => [],

  rules: {
    document: ($) =>
      repeat(choice($._block, $._newline, $._blank_line)),

    // ---------------------------------------------------------------
    // Block-level elements
    // ---------------------------------------------------------------

    _block: ($) =>
      choice(
        $.heading,
        $.paragraph,
        $.code_block,
        $.code_block_with_lang,
        $.verbatim_block,
        $.math_block,
        $.unordered_list,
        $.ordered_list,
        $.light_list,
        $.table_heavy,
        $.table_light,
        $.module_list,
        $.tag,
        $.paragraph_style,
      ),

    // ---------------------------------------------------------------
    // Heading: {N content} or {N:label content}
    // ---------------------------------------------------------------

    heading: ($) =>
      choice(
        seq($.heading_marker_with_label, inline_content($), '}'),
        seq($.heading_marker, inline_content($), '}'),
      ),

    // Combined tokens to prevent the word rule from consuming the colon
    heading_marker: () => token(seq('{', /[0-9]+/)),
    heading_marker_with_label: () => token(seq('{', /[0-9]+/, ':', /[^ \t\n\r}]+/)),

    // ---------------------------------------------------------------
    // Paragraph: sequence of inline elements
    // ---------------------------------------------------------------

    paragraph: ($) => prec.right(repeat1($._inline)),

    // ---------------------------------------------------------------
    // Inline elements
    // ---------------------------------------------------------------

    // '|' separates cells in a light table, so the table rule below needs the
    // inline elements without it.
    _inline: ($) => choice($._inline_without_bar, $.bar),

    _inline_without_bar: ($) =>
      choice(
        $.word,
        $.escape_sequence,
        $.code_span,
        $.bold,
        $.italic,
        $.emphasis,
        $.superscript,
        $.subscript,
        $.math_span,
        $.raw_markup,
        $.simple_reference,
        $.reference_with_text,
        $.simple_link,
        $.link_with_text,
        $.simple_media,
        $.media_with_text,
        $.minus,
        $.plus,
        $.at,
        $.backslash,
        $.right_bracket,
        $.left_brace,
      ),

    // Word: anything not special. Follows the odoc lexer:
    //   word_char (word_char | bullet_char | '@')*
    // where word_char excludes markup_char ({, }, [, ], @, |), space_char, bullet_char (-, +)
    // and includes escaped markup chars (\\{, \\}, etc.)
    // Also: bullet_char (word_char | bullet_char | '@')+ is a word
    // (a bullet char followed by more chars becomes a word token)
    word: () => token(
      choice(
        // Standard word: starts with a non-special character (NOT escaped char)
        seq(
          /[^\s{}\[\]@|\\+\-]/,
          repeat(
            choice(
              /[^\s{}\[\]@|\\]/,
              seq('\\', /[{}\[\]@]/),
              /[+\-@]/,
            ),
          ),
        ),
        // Bullet-char-started word: - or + followed by at least one more char
        seq(
          /[+\-]/,
          repeat1(
            choice(
              /[^\s{}\[\]@|\\]/,
              seq('\\', /[{}\[\]@]/),
              /[+\-@]/,
            ),
          ),
        ),
      ),
    ),

    escape_sequence: () => token(seq('\\', /[{}\[\]@]/)),

    // Standalone minus and plus (not followed by word chars)
    minus: () => '-',
    plus: () => '+',

    // Standalone punctuation that is only structural in specific contexts:
    // '|' separates cells inside {t ...} tables, '@' introduces tags, and
    // '\' leads an escape sequence.  When they don't form one of those
    // constructs odoc treats them as literal text (it emits `Bar, warns and
    // recovers on unknown tags / stray backslashes), so we surface them as
    // their own inline tokens rather than producing ERROR nodes.
    bar: () => '|',
    at: () => '@',
    backslash: () => '\\',
    // An unpaired ']' is literal text (odoc warns and emits it as a word).
    right_bracket: () => ']',
    // A '{' that opens none of the constructs below is literal text too: odoc
    // warns and recovers rather than giving up on the rest of the page, and so
    // should we — a half-typed '{' is a very common state in an editor.
    left_brace: () => '{',

    // ---------------------------------------------------------------
    // Style markup: {b ...}, {i ...}, {e ...}, {^ ...}, {_ ...}
    // ---------------------------------------------------------------

    bold: ($) => seq('{b', inline_content($), '}'),
    italic: ($) => seq('{i', inline_content($), '}'),
    emphasis: ($) => seq('{e', inline_content($), '}'),
    superscript: ($) => seq('{^', inline_content($), '}'),
    subscript: ($) => seq('{_', inline_content($), '}'),

    // ---------------------------------------------------------------
    // Paragraph style: {L ...}, {C ...}, {R ...}
    // ---------------------------------------------------------------

    paragraph_style: ($) =>
      seq(
        choice('{L', '{C', '{R'),
        inline_content($),
        '}',
      ),

    // ---------------------------------------------------------------
    // Code span: [content]
    // ---------------------------------------------------------------

    code_span: ($) =>
      seq(
        '[',
        optional($._inline_code_content),
        ']',
      ),

    // ---------------------------------------------------------------
    // Code block: {[content]} or {delim@lang[content]delim}
    // ---------------------------------------------------------------

    code_block: ($) =>
      seq(
        '{[',
        optional($.code_block_content),
        ']}',
      ),

    code_block_with_lang: ($) =>
      choice(
        // Without delimiter: {@lang[content]} or {@lang meta[content]}
        seq(
          '{@',
          $.language,
          optional($.code_block_meta),
          '[',
          optional($.code_block_content),
          ']}',
        ),
        // No language tag: {@[content]} (odoc accepts this with a warning).
        // A dedicated branch keeps it unambiguous with the language branch
        // above (whose greedy code_block_meta would otherwise absorb the
        // language when it is made optional).
        seq(
          '{@',
          '[',
          optional($.code_block_content),
          ']}',
        ),
        // With delimiter: {delim@lang[content]delim}, optionally followed by
        // a result block: {delim@lang[content]delim[blocks]}.  Only delimited
        // blocks may carry results, and the results always end with ']}'
        // (odoc's `Right_code_delimiter`), whatever the delimiter is.
        seq(
          $.code_block_open_delimiter,
          $.language,
          optional($.code_block_meta),
          '[',
          optional($.code_block_content),
          ']',
          optional($.code_block_delimiter_close),
          choice(
            '}',
            seq('[', optional($.code_block_output), ']}'),
          ),
        ),
      ),

    // Results of evaluating a code block, e.g. the toplevel output or the
    // compiler error mdx writes back into the page.  Unlike the code itself
    // this is odoc markup, not raw text.
    code_block_output: ($) =>
      repeat1(choice($._block, $._newline, $._blank_line)),

    // Combined token to avoid standalone '{' interfering with other {-prefixed tokens
    code_block_open_delimiter: () => token(seq('{', /[a-zA-Z0-9_]+/, '@')),
    code_block_delimiter_close: () => /[a-zA-Z0-9_]+/,
    language: () => /[a-zA-Z0-9_\-]+/,
    code_block_meta: () => /[^\[]+/,

    // ---------------------------------------------------------------
    // Verbatim block: {v content v}
    // ---------------------------------------------------------------

    verbatim_block: ($) =>
      seq(
        '{v',
        optional($._verbatim_content),
        token(seq(/[ \t\n\r]/, 'v}')),
      ),

    // ---------------------------------------------------------------
    // Math: {m content} and {math content}
    // ---------------------------------------------------------------

    math_span: ($) =>
      seq(
        '{m',
        optional($._math_inline_content),
        '}',
      ),

    math_block: ($) =>
      seq(
        '{math',
        optional($._math_block_content),
        '}',
      ),

    // ---------------------------------------------------------------
    // References: {!target}, {{!target} text}, {:url}, {{:url} text}
    // ---------------------------------------------------------------

    simple_reference: ($) =>
      seq(
        '{!',
        $.reference_target,
        '}',
      ),

    reference_with_text: ($) =>
      seq(
        '{{!',
        $.reference_target,
        '}',
        inline_content($),
        '}',
      ),

    simple_link: ($) =>
      seq(
        '{:',
        $.link_target,
        '}',
      ),

    link_with_text: ($) =>
      seq(
        '{{:',
        $.link_target,
        '}',
        inline_content($),
        '}',
      ),

    reference_target: () => /[^}]+/,
    link_target: () => /[^}]+/,

    // ---------------------------------------------------------------
    // Media: {image!ref}, {image:url}, {video!...}, {audio!...} and the
    // replacement-text forms {{image!ref} caption}, etc.  The '!' variants
    // take a reference target, the ':' variants a link/URL target.  These
    // openers are matched ahead of {i (italic) and {v (verbatim) by maximal
    // munch, so e.g. {video!...} no longer mis-parses as a verbatim block.
    // ---------------------------------------------------------------

    simple_media: ($) =>
      seq(
        choice('{image!', '{image:', '{video!', '{video:', '{audio!', '{audio:'),
        $.media_target,
        '}',
      ),

    media_with_text: ($) =>
      seq(
        choice('{{image!', '{{image:', '{{video!', '{{video:', '{{audio!', '{{audio:'),
        $.media_target,
        '}',
        inline_content($),
        '}',
      ),

    media_target: () => /[^}]+/,

    // ---------------------------------------------------------------
    // Lists (heavy syntax): {ul {- item} ...} and {ol {li item} ...}
    // ---------------------------------------------------------------

    unordered_list: ($) =>
      seq(
        '{ul',
        repeat(choice($._list_item, $._newline, $._blank_line)),
        '}',
      ),

    ordered_list: ($) =>
      seq(
        '{ol',
        repeat(choice($._list_item, $._newline, $._blank_line)),
        '}',
      ),

    _list_item: ($) =>
      choice($.dash_list_item, $.li_list_item),

    dash_list_item: ($) =>
      seq(
        '{-',
        repeat(choice($._block, $._newline, $._blank_line)),
        '}',
      ),

    li_list_item: ($) =>
      seq(
        '{li',
        repeat(choice($._block, $._newline, $._blank_line)),
        '}',
      ),

    // ---------------------------------------------------------------
    // Lists (light syntax): - item / + item
    // Uses external scanner to detect bullets at line start only.
    // ---------------------------------------------------------------

    // Items carry the newline that separates them (see `light_list_item`),
    // so the list is just a run of adjacent items.  A blank line is not part
    // of an item, so it ends the list, as it does in odoc.
    light_list: ($) => prec.right(repeat1($.light_list_item)),

    // An item runs until the next bullet of the same kind, a blank line or a
    // closing brace: its content is a paragraph, so it spans single newlines
    // (odoc's [shorthand_list_items]).
    light_list_item: ($) =>
      prec.right(seq(
        choice($._light_list_bullet_dash, $._light_list_bullet_plus),
        inline_content($),
      )),

    // ---------------------------------------------------------------
    // Tables (heavy): {table {tr {th ...} {td ...}} ...}
    // ---------------------------------------------------------------

    table_heavy: ($) =>
      seq(
        '{table',
        repeat(choice($.table_row, $._newline, $._blank_line)),
        '}',
      ),

    table_row: ($) =>
      seq(
        '{tr',
        repeat(choice($.table_header_cell, $.table_data_cell, $._newline, $._blank_line)),
        '}',
      ),

    table_header_cell: ($) =>
      seq(
        '{th',
        repeat(choice($._block, $._newline, $._blank_line)),
        '}',
      ),

    table_data_cell: ($) =>
      seq(
        '{td',
        repeat(choice($._block, $._newline, $._blank_line)),
        '}',
      ),

    // ---------------------------------------------------------------
    // Tables (light): {t ...}
    // ---------------------------------------------------------------

    // Light table: cells separated by pipes, rows by newlines.  A cell holds
    // inline elements — odoc allows markup such as {e ...} inside one — but no
    // newline, so a cell never spans rows.
    table_light: ($) =>
      seq(
        '{t',
        repeat(choice($.table_light_cell, $.table_separator, $._newline)),
        '}',
      ),

    table_separator: () => '|',
    table_light_cell: ($) => prec.right(repeat1($._inline_without_bar)),

    // ---------------------------------------------------------------
    // Module list: {!modules: A B C}
    // ---------------------------------------------------------------

    module_list: ($) =>
      seq(
        '{!modules:',
        // odoc lexes everything up to the '}' as the module list, blank lines
        // included, so this is not `inline_content`.
        repeat(choice($.module_name, $._newline, $._blank_line)),
        '}',
      ),

    module_name: () => /[^ \t\n\r}]+/,

    // ---------------------------------------------------------------
    // Raw markup: {%target:content%} or {%content%}
    // ---------------------------------------------------------------

    raw_markup: ($) =>
      seq(
        '{%',
        optional($._raw_markup_content),
        '%}',
      ),

    // ---------------------------------------------------------------
    // Tags: @author, @param, @deprecated, etc.
    // ---------------------------------------------------------------

    // Tags are recognized as standalone tokens. The tag content (for tags like
    // @deprecated, @param, @return) is parsed as subsequent blocks in the
    // document — the odoc parser handles the tag-content association at a
    // higher level. Tree-sitter just recognizes the tag markers.
    tag: ($) =>
      choice(
        $.tag_author,
        $.tag_deprecated,
        $.tag_param,
        $.tag_raise,
        $.tag_return,
        $.tag_see,
        $.tag_since,
        $.tag_before,
        $.tag_version,
        $.tag_canonical,
        $.tag_inline,
        $.tag_open,
        $.tag_closed,
        $.tag_hidden,
        $.tag_children_order,
        $.tag_toc_status,
        $.tag_order_category,
        $.tag_short_title,
      ),

    tag_author: ($) => seq('@author', optional($._tag_text)),
    tag_deprecated: () => '@deprecated',
    tag_param: ($) => seq('@param', $.param_name),
    tag_raise: ($) => seq(choice('@raise', '@raises'), $.raise_name),
    tag_return: () => choice('@return', '@returns'),
    tag_see: ($) =>
      choice(
        seq('@see', '<', $.see_url, '>'),
        seq('@see', '\'', $.see_file, '\''),
        seq('@see', '\x22', $.see_doc, '\x22'),
      ),
    tag_since: ($) => seq('@since', optional($._tag_text)),
    tag_before: ($) => seq('@before', $.before_version),
    tag_version: ($) => seq('@version', optional($._tag_text)),
    tag_canonical: ($) => seq('@canonical', optional($._tag_text)),
    tag_inline: () => '@inline',
    tag_open: () => '@open',
    tag_closed: () => '@closed',
    tag_hidden: () => '@hidden',
    // Page/index tags (odoc >= 2.4).  Like @deprecated these are bare
    // markers; their argument (e.g. the child order) follows as block content.
    tag_children_order: () => '@children_order',
    tag_toc_status: () => '@toc_status',
    tag_order_category: () => '@order_category',
    tag_short_title: () => '@short_title',

    param_name: () => /[^ \t\n\r]+/,
    raise_name: () => /[^ \t\n\r]+/,
    before_version: () => /[^ \t\n\r]+/,
    see_url: () => /[^>]*/,
    see_file: () => /[^']*/,
    see_doc: () => /[^"]*/,

    // ---------------------------------------------------------------
    // Whitespace tokens
    // ---------------------------------------------------------------

    _newline: () => /\n|\r\n/,
    _blank_line: () => /(\n|\r\n)([ \t]*(\n|\r\n))+/,
  },
});
