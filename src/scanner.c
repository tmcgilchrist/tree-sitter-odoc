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
};

// Scanner state: track whether we're at line start for light list detection
typedef struct {
  bool at_line_start;
} Scanner;

static void advance(TSLexer *lexer) { lexer->advance(lexer, false); }

static void skip(TSLexer *lexer) { lexer->advance(lexer, true); }

static void mark_end(TSLexer *lexer) { lexer->mark_end(lexer); }

static bool eof(TSLexer *lexer) { return lexer->eof(lexer); }

static int32_t lookahead(TSLexer *lexer) { return lexer->lookahead; }

// Scan code block content: everything until ']<delim>}' where delim matches
static bool scan_code_block_content(TSLexer *lexer) {
  bool has_content = false;

  while (!eof(lexer)) {
    if (lookahead(lexer) == ']') {
      mark_end(lexer);
      advance(lexer);

      // Check for '}' (end of basic code block: ]})
      if (lookahead(lexer) == '}') {
        lexer->result_symbol = CODE_BLOCK_CONTENT;
        return has_content;
      }

      // Check for delimiter chars followed by } or [
      int buf_len = 0;
      while ((lookahead(lexer) >= 'a' && lookahead(lexer) <= 'z') ||
             (lookahead(lexer) >= 'A' && lookahead(lexer) <= 'Z') ||
             (lookahead(lexer) >= '0' && lookahead(lexer) <= '9') ||
             lookahead(lexer) == '_') {
        buf_len++;
        advance(lexer);
      }

      if (buf_len > 0 && (lookahead(lexer) == '}' || lookahead(lexer) == '[')) {
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

// Scan light list bullet: '-' or '+' at column 0 (start of line)
// tree-sitter provides get_column via the lexer
static bool scan_light_list_bullet(TSLexer *lexer,
                                    const bool *valid_symbols) {
  // Use get_column to check if we're at the start of a line
  // (after skipping horizontal whitespace which is in extras)
  uint32_t col = lexer->get_column(lexer);

  if (col == 0) {
    // Skip leading horizontal whitespace
    while (lookahead(lexer) == ' ' || lookahead(lexer) == '\t') {
      skip(lexer);
    }
  }

  // Only consider this a list bullet if we're at column 0
  // (or after only whitespace that was already skipped by extras)
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
  Scanner *scanner = calloc(1, sizeof(Scanner));
  if (scanner) {
    scanner->at_line_start = true;
  }
  return scanner;
}

void tree_sitter_odoc_external_scanner_destroy(void *payload) {
  free(payload);
}

unsigned tree_sitter_odoc_external_scanner_serialize(void *payload,
                                                     char *buffer) {
  (void)payload;
  (void)buffer;
  return 0;
}

void tree_sitter_odoc_external_scanner_deserialize(void *payload,
                                                    const char *buffer,
                                                    unsigned length) {
  (void)payload;
  (void)buffer;
  (void)length;
}

bool tree_sitter_odoc_external_scanner_scan(void *payload, TSLexer *lexer,
                                             const bool *valid_symbols) {
  (void)payload;

  // If all externals are valid, we're in error recovery — bail out.
  // This prevents the scanner from greedily consuming content when
  // the parser hasn't committed to a specific construct yet.
  bool all_valid = true;
  for (int i = CODE_BLOCK_CONTENT; i <= LIGHT_LIST_BULLET_PLUS; i++) {
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
    return scan_code_block_content(lexer);
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

  if (valid_symbols[LIGHT_LIST_BULLET_DASH] ||
      valid_symbols[LIGHT_LIST_BULLET_PLUS]) {
    return scan_light_list_bullet(lexer, valid_symbols);
  }

  return false;
}
