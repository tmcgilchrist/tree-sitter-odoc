#include "tree_sitter/parser.h"

#include <string.h>

// Token types — must match the order in grammar.js externals array
enum TokenType {
  CODE_BLOCK_CONTENT,
  VERBATIM_CONTENT,
  INLINE_CODE_CONTENT,
  MATH_BLOCK_CONTENT,
  MATH_INLINE_CONTENT,
  RAW_MARKUP_CONTENT,
  TAG_TEXT,
  LIGHT_LIST_BULLET_DASH,
  LIGHT_LIST_BULLET_PLUS,
  CODE_BLOCK_OPEN_DELIMITER,
  CODE_BLOCK_OPEN,
  CODE_BLOCK_OPEN_BRACKET,
  INLINE_NEWLINE,
};

// odoc's delim_char: ['a'-'z' 'A'-'Z' '0'-'9' '_'].  odoc puts no bound on the
// length of a delimiter; anything longer than this is treated as a mismatch,
// which degrades to "the block runs to the end of the file" rather than to a
// wrong parse.
#define MAX_DELIMITER_LENGTH 32

// Scanner state: the delimiter of the code block currently being scanned, so
// that {delim@lang[ ... ]delim} only ends at its own terminator and not at the
// first ']}' that happens to appear in the code.
typedef struct {
  uint8_t delimiter_length;
  char delimiter[MAX_DELIMITER_LENGTH];
} Scanner;

static bool is_delimiter_char(int32_t c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || c == '_';
}

static void advance(TSLexer *lexer) { lexer->advance(lexer, false); }

static void skip(TSLexer *lexer) { lexer->advance(lexer, true); }

static void mark_end(TSLexer *lexer) { lexer->mark_end(lexer); }

static bool eof(TSLexer *lexer) { return lexer->eof(lexer); }

static int32_t lookahead(TSLexer *lexer) { return lexer->lookahead; }

// Scan a code block opener: '{[', '{@' or '{' delim_char* '@'.  The delimiter
// is kept in the scanner state for scan_code_block_content below.
static bool scan_code_block_open(Scanner *scanner, TSLexer *lexer,
                                 const bool *valid_symbols) {
  if (lookahead(lexer) != '{') {
    return false;
  }
  advance(lexer);

  if (lookahead(lexer) == '[') {
    if (!valid_symbols[CODE_BLOCK_OPEN_BRACKET]) {
      return false;
    }
    advance(lexer);
    mark_end(lexer);
    scanner->delimiter_length = 0;
    lexer->result_symbol = CODE_BLOCK_OPEN_BRACKET;
    return true;
  }

  char delimiter[MAX_DELIMITER_LENGTH];
  unsigned length = 0;
  while (is_delimiter_char(lookahead(lexer))) {
    if (length < MAX_DELIMITER_LENGTH) {
      delimiter[length] = (char)lookahead(lexer);
    }
    length++;
    advance(lexer);
  }

  if (lookahead(lexer) != '@' || length > MAX_DELIMITER_LENGTH) {
    return false;
  }

  if (length > 0 ? !valid_symbols[CODE_BLOCK_OPEN_DELIMITER]
                 : !valid_symbols[CODE_BLOCK_OPEN]) {
    return false;
  }

  advance(lexer);
  mark_end(lexer);
  memcpy(scanner->delimiter, delimiter, length);
  scanner->delimiter_length = (uint8_t)length;
  lexer->result_symbol =
      length > 0 ? CODE_BLOCK_OPEN_DELIMITER : CODE_BLOCK_OPEN;
  return true;
}

// Scan code block content: everything until the terminator matching the
// delimiter the opener recorded — ']<delim>}', or ']<delim>[' for the result
// block a delimited code block may carry.  odoc's lexer treats a terminator
// with any other delimiter as part of the code.
static bool scan_code_block_content(Scanner *scanner, TSLexer *lexer) {
  bool has_content = false;

  while (!eof(lexer)) {
    if (lookahead(lexer) == ']') {
      mark_end(lexer);
      advance(lexer);

      unsigned length = 0;
      bool matches = true;
      while (is_delimiter_char(lookahead(lexer))) {
        if (length >= scanner->delimiter_length ||
            (char)lookahead(lexer) != scanner->delimiter[length]) {
          matches = false;
        }
        length++;
        advance(lexer);
      }

      if (matches && length == scanner->delimiter_length &&
          (lookahead(lexer) == '}' ||
           (lookahead(lexer) == '[' && scanner->delimiter_length > 0))) {
        lexer->result_symbol = CODE_BLOCK_CONTENT;
        return has_content;
      }

      has_content = true;
      continue;
    }

    has_content = true;
    advance(lexer);
  }

  if (has_content) {
    mark_end(lexer);
    lexer->result_symbol = CODE_BLOCK_CONTENT;
    return true;
  }
  return false;
}

// Scan verbatim content: everything until <space_char>v}
static bool scan_verbatim_content(TSLexer *lexer) {
  bool has_content = false;

  while (!eof(lexer)) {
    int32_t c = lookahead(lexer);
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      mark_end(lexer);
      advance(lexer);
      if (lookahead(lexer) == 'v') {
        advance(lexer);
        if (lookahead(lexer) == '}') {
          lexer->result_symbol = VERBATIM_CONTENT;
          return has_content;
        }
        has_content = true;
        continue;
      }
      has_content = true;
      continue;
    }

    has_content = true;
    advance(lexer);
  }

  if (has_content) {
    mark_end(lexer);
    lexer->result_symbol = VERBATIM_CONTENT;
    return true;
  }
  return false;
}

// Scan inline code content: everything inside [...] with balanced brackets
static bool scan_inline_code_content(TSLexer *lexer) {
  int depth = 0;
  bool has_content = false;

  while (!eof(lexer)) {
    if (lookahead(lexer) == '\\') {
      advance(lexer);
      if (lookahead(lexer) == '[' || lookahead(lexer) == ']') {
        advance(lexer);
        has_content = true;
        continue;
      }
      has_content = true;
      continue;
    }

    if (lookahead(lexer) == '[') {
      depth++;
      has_content = true;
      advance(lexer);
      continue;
    }

    if (lookahead(lexer) == ']') {
      if (depth == 0) {
        mark_end(lexer);
        lexer->result_symbol = INLINE_CODE_CONTENT;
        return has_content;
      }
      depth--;
      has_content = true;
      advance(lexer);
      continue;
    }

    has_content = true;
    advance(lexer);
  }

  if (has_content) {
    mark_end(lexer);
    lexer->result_symbol = INLINE_CODE_CONTENT;
    return true;
  }
  return false;
}

// Scan math block content: balanced braces with \{ \} escapes
static bool scan_math_block_content(TSLexer *lexer) {
  int depth = 0;
  bool has_content = false;

  while (!eof(lexer)) {
    if (lookahead(lexer) == '\\') {
      advance(lexer);
      if (lookahead(lexer) == '{' || lookahead(lexer) == '}') {
        advance(lexer);
        has_content = true;
        continue;
      }
      has_content = true;
      continue;
    }

    if (lookahead(lexer) == '{') {
      depth++;
      has_content = true;
      advance(lexer);
      continue;
    }

    if (lookahead(lexer) == '}') {
      if (depth == 0) {
        mark_end(lexer);
        lexer->result_symbol = MATH_BLOCK_CONTENT;
        return has_content;
      }
      depth--;
      has_content = true;
      advance(lexer);
      continue;
    }

    has_content = true;
    advance(lexer);
  }

  if (has_content) {
    mark_end(lexer);
    lexer->result_symbol = MATH_BLOCK_CONTENT;
    return true;
  }
  return false;
}

// Scan math inline content: same as math block
static bool scan_math_inline_content(TSLexer *lexer) {
  int depth = 0;
  bool has_content = false;

  while (!eof(lexer)) {
    if (lookahead(lexer) == '\\') {
      advance(lexer);
      if (lookahead(lexer) == '{' || lookahead(lexer) == '}') {
        advance(lexer);
        has_content = true;
        continue;
      }
      has_content = true;
      continue;
    }

    if (lookahead(lexer) == '{') {
      depth++;
      has_content = true;
      advance(lexer);
      continue;
    }

    if (lookahead(lexer) == '}') {
      if (depth == 0) {
        mark_end(lexer);
        lexer->result_symbol = MATH_INLINE_CONTENT;
        return has_content;
      }
      depth--;
      has_content = true;
      advance(lexer);
      continue;
    }

    has_content = true;
    advance(lexer);
  }

  if (has_content) {
    mark_end(lexer);
    lexer->result_symbol = MATH_INLINE_CONTENT;
    return true;
  }
  return false;
}

// Scan raw markup content: everything until %}
static bool scan_raw_markup_content(TSLexer *lexer) {
  bool has_content = false;

  while (!eof(lexer)) {
    if (lookahead(lexer) == '%') {
      mark_end(lexer);
      advance(lexer);
      if (lookahead(lexer) == '}') {
        lexer->result_symbol = RAW_MARKUP_CONTENT;
        return has_content;
      }
      has_content = true;
      continue;
    }

    has_content = true;
    advance(lexer);
  }

  if (has_content) {
    mark_end(lexer);
    lexer->result_symbol = RAW_MARKUP_CONTENT;
    return true;
  }
  return false;
}


// Scan tag text: rest of the line after a tag
static bool scan_tag_text(TSLexer *lexer) {
  bool has_content = false;

  while (!eof(lexer)) {
    int32_t c = lookahead(lexer);
    if (c == '\n' || c == '\r') {
      break;
    }
    has_content = true;
    advance(lexer);
  }

  if (has_content) {
    mark_end(lexer);
    lexer->result_symbol = TAG_TEXT;
    return true;
  }
  return false;
}

// Scan the newline that lets brace-delimited markup span lines.  A newline
// that starts a blank line is not one: odoc rejects a blank line inside such
// markup, and stopping here keeps unclosed markup from swallowing the page.
static bool scan_inline_newline(TSLexer *lexer) {
  if (lookahead(lexer) != '\n' && lookahead(lexer) != '\r') {
    return false;
  }

  if (lookahead(lexer) == '\r') {
    advance(lexer);
  }
  if (lookahead(lexer) == '\n') {
    advance(lexer);
  }
  mark_end(lexer);

  while (lookahead(lexer) == ' ' || lookahead(lexer) == '\t') {
    advance(lexer);
  }
  if (eof(lexer) || lookahead(lexer) == '\n' || lookahead(lexer) == '\r') {
    return false;
  }

  lexer->result_symbol = INLINE_NEWLINE;
  return true;
}

// Scan light list bullet: '-' or '+' introducing an item.  The caller has
// already established that only whitespace precedes it on the line.
static bool scan_light_list_bullet(TSLexer *lexer,
                                    const bool *valid_symbols) {
  if (lookahead(lexer) == '-' && valid_symbols[LIGHT_LIST_BULLET_DASH]) {
    advance(lexer);
    mark_end(lexer);
    // Must be followed by space (otherwise it's a word or minus)
    if (lookahead(lexer) == ' ' || lookahead(lexer) == '\t') {
      lexer->result_symbol = LIGHT_LIST_BULLET_DASH;
      return true;
    }
  }

  if (lookahead(lexer) == '+' && valid_symbols[LIGHT_LIST_BULLET_PLUS]) {
    advance(lexer);
    mark_end(lexer);
    // Must be followed by space
    if (lookahead(lexer) == ' ' || lookahead(lexer) == '\t') {
      lexer->result_symbol = LIGHT_LIST_BULLET_PLUS;
      return true;
    }
  }

  return false;
}

// External scanner API

void *tree_sitter_odoc_external_scanner_create(void) {
  return calloc(1, sizeof(Scanner));
}

void tree_sitter_odoc_external_scanner_destroy(void *payload) {
  free(payload);
}

unsigned tree_sitter_odoc_external_scanner_serialize(void *payload,
                                                     char *buffer) {
  Scanner *scanner = (Scanner *)payload;
  buffer[0] = (char)scanner->delimiter_length;
  memcpy(buffer + 1, scanner->delimiter, scanner->delimiter_length);
  return 1 + scanner->delimiter_length;
}

void tree_sitter_odoc_external_scanner_deserialize(void *payload,
                                                    const char *buffer,
                                                    unsigned length) {
  Scanner *scanner = (Scanner *)payload;
  scanner->delimiter_length = 0;
  if (length > 0) {
    scanner->delimiter_length = (uint8_t)buffer[0];
    memcpy(scanner->delimiter, buffer + 1, scanner->delimiter_length);
  }
}

bool tree_sitter_odoc_external_scanner_scan(void *payload, TSLexer *lexer,
                                             const bool *valid_symbols) {
  Scanner *scanner = (Scanner *)payload;

  // If all externals are valid, we're in error recovery — bail out.
  // This prevents the scanner from greedily consuming content when
  // the parser hasn't committed to a specific construct yet.
  bool all_valid = true;
  for (int i = CODE_BLOCK_CONTENT; i <= INLINE_NEWLINE; i++) {
    if (!valid_symbols[i]) {
      all_valid = false;
      break;
    }
  }
  if (all_valid) return false;

  // These scanners should only run when the parser has committed to being
  // inside the specific construct (i.e., the opening delimiter has been
  // consumed and the parser expects content tokens).

  if (valid_symbols[CODE_BLOCK_CONTENT] && !valid_symbols[INLINE_CODE_CONTENT]) {
    return scan_code_block_content(scanner, lexer);
  }

  if (valid_symbols[VERBATIM_CONTENT]) {
    return scan_verbatim_content(lexer);
  }

  if (valid_symbols[INLINE_CODE_CONTENT]) {
    return scan_inline_code_content(lexer);
  }

  if (valid_symbols[MATH_BLOCK_CONTENT] && !valid_symbols[MATH_INLINE_CONTENT]) {
    return scan_math_block_content(lexer);
  }

  if (valid_symbols[MATH_INLINE_CONTENT]) {
    return scan_math_inline_content(lexer);
  }

  if (valid_symbols[RAW_MARKUP_CONTENT]) {
    return scan_raw_markup_content(lexer);
  }

  if (valid_symbols[TAG_TEXT]) {
    return scan_tag_text(lexer);
  }

  if (valid_symbols[INLINE_NEWLINE] &&
      (lookahead(lexer) == '\n' || lookahead(lexer) == '\r')) {
    return scan_inline_newline(lexer);
  }

  // A code block opener and a light list bullet both start a block, so they
  // can be valid in the same state.  Extras are skipped by the internal lexer,
  // which does not run before this scanner, so the indentation in front of
  // either is ours to skip — but only a bullet that is the first thing on its
  // line starts a list ('a - b' is a paragraph, not a list).
  bool code_block_open_valid = valid_symbols[CODE_BLOCK_OPEN_DELIMITER] ||
                               valid_symbols[CODE_BLOCK_OPEN] ||
                               valid_symbols[CODE_BLOCK_OPEN_BRACKET];

  if (code_block_open_valid || valid_symbols[LIGHT_LIST_BULLET_DASH] ||
      valid_symbols[LIGHT_LIST_BULLET_PLUS]) {
    bool at_line_start = lexer->get_column(lexer) == 0;

    while (lookahead(lexer) == ' ' || lookahead(lexer) == '\t') {
      skip(lexer);
    }

    if (lookahead(lexer) == '{' && code_block_open_valid) {
      return scan_code_block_open(scanner, lexer, valid_symbols);
    }

    if (at_line_start && (valid_symbols[LIGHT_LIST_BULLET_DASH] ||
                          valid_symbols[LIGHT_LIST_BULLET_PLUS])) {
      return scan_light_list_bullet(lexer, valid_symbols);
    }
  }

  return false;
}
