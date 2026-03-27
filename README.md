# tree-sitter-odoc

Tree-sitter grammar for [odoc](https://ocaml.github.io/odoc/), the OCaml documentation markup language.

## Overview

This grammar provides syntax highlighting and parsing support for odoc markup, used in OCaml documentation comments (`(** ... *)`) and standalone `.mld` files. It is based directly on the canonical [odoc-parser](https://github.com/ocaml/odoc/tree/master/src/parser) source.

## Features

- **Complete odoc markup support**: Parses all block and inline constructs
- **External scanner**: Context-sensitive lexing for code blocks, verbatim, math, inline code, and light list bullets
- **Syntax highlighting**: Comprehensive highlight queries for editor integration
- **Language injection**: Code blocks with language tags inject the appropriate language (e.g., `{@ocaml[...]}`)
- **63 test cases**: Extensively tested against real-world `.mld` files

## Supported File Types

- `.mld` - Standalone odoc documentation pages

## Supported Constructs

### Block-level

| Construct | Syntax |
|-----------|--------|
| Heading | `{1 title}`, `{2:label title}` |
| Paragraph | Inline content separated by blank lines |
| Code block | `{[code]}`, `{@lang[code]}`, `{delim@lang[code]delim}` |
| Verbatim | `{v content v}` |
| Math block | `{math content}` |
| Unordered list (heavy) | `{ul {- item} {- item}}` |
| Ordered list (heavy) | `{ol {- item} {li item}}` |
| Light list | `- item` / `+ item` (at line start) |
| Table (heavy) | `{table {tr {th header} {td cell}}}` |
| Table (light) | `{t cell \| cell}` |
| Module list | `{!modules: A B C}` |
| Raw markup | `{%content%}` |
| Paragraph style | `{L text}`, `{C text}`, `{R text}` |
| Tags | `@author`, `@param`, `@return`, `@deprecated`, etc. |

### Inline

| Construct | Syntax |
|-----------|--------|
| Bold | `{b text}` |
| Italic | `{i text}` |
| Emphasis | `{e text}` |
| Superscript | `{^ text}` |
| Subscript | `{_ text}` |
| Code span | `[code]` |
| Math span | `{m content}` |
| Reference | `{!Module.t}`, `{{!Module.t} text}` |
| Link | `{:url}`, `{{:url} text}` |
| Escape sequences | `\{`, `\}`, `\[`, `\]`, `\@` |

## Installation

Nothing published to package repositories just yet, it's still in development.

### From source

```bash
git clone https://github.com/tmcgilchrist/tree-sitter-odoc
cd tree-sitter-odoc
npm install
tree-sitter generate
tree-sitter test
```

## Usage

### With tree-sitter CLI

```bash
tree-sitter parse example.mld
```

### With editors

#### Emacs

This grammar can be used with Emacs 29+ tree-sitter integration or with packages like `tree-sitter-mode`.

#### Neovim

Configure with `nvim-treesitter`:

```lua
parser_config.odoc = {
  install_info = {
    url = "https://github.com/tmcgilchrist/tree-sitter-odoc",
    files = {"src/parser.c", "src/scanner.c"},
  },
  filetype = "mld",
}
```

## Development

### Building

```bash
npm install -g tree-sitter-cli
tree-sitter generate
```

### Testing

```bash
tree-sitter test
```

### Parsing a file

```bash
tree-sitter parse example.mld
```

### Playground

```bash
tree-sitter build --wasm
tree-sitter playground
```

### Debugging

```bash
tree-sitter parse --debug example.mld
```

## Architecture

The grammar uses an **external scanner** (`src/scanner.c`) to handle context-sensitive tokens that cannot be expressed in the regular tree-sitter grammar DSL:

| Token | Purpose |
|-------|---------|
| `_code_block_content` | Scan code block body to `]}` or `]delim}` |
| `_verbatim_content` | Scan verbatim body to `<whitespace>v}` |
| `_inline_code_content` | Scan `[...]` with balanced bracket tracking |
| `_math_block_content` | Scan `{math ...}` with balanced brace tracking |
| `_math_inline_content` | Scan `{m ...}` with balanced brace tracking |
| `_raw_markup_content` | Scan raw markup body to `%}` |
| `_tag_text` | Scan tag text to end of line |
| `_light_list_bullet_dash` | Detect `-` at line start followed by space |
| `_light_list_bullet_plus` | Detect `+` at line start followed by space |

## Resources

- [odoc Documentation](https://ocaml.github.io/odoc/)
- [odoc for Authors](https://ocaml.github.io/odoc/odoc_for_authors.html)
- [Tree-sitter Documentation](https://tree-sitter.github.io/)
- [Grammar Development Guide](https://tree-sitter.github.io/tree-sitter/creating-parsers)

## License

MIT
