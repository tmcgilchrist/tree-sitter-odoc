//! This crate provides Odoc language support for the [tree-sitter] parsing library.
//!
//! Typically, you will use the [`LANGUAGE`] constant to add this language to a
//! tree-sitter [`Parser`], and then use the parser to parse some code:
//!
//! ```
//! let code = r#"
//! "#;
//! let mut parser = tree_sitter::Parser::new();
//! let language = tree_sitter_odoc::LANGUAGE;
//! parser
//!     .set_language(&language.into())
//!     .expect("Error loading Odoc parser");
//! let tree = parser.parse(code, None).unwrap();
//! assert!(!tree.root_node().has_error());
//! ```
//!
//! [`Parser`]: https://docs.rs/tree-sitter/0.25.10/tree_sitter/struct.Parser.html
//! [tree-sitter]: https://tree-sitter.github.io/

use tree_sitter_language::LanguageFn;

extern "C" {
    fn tree_sitter_odoc() -> *const ();
}

/// The tree-sitter [`LanguageFn`] for this grammar.
pub const LANGUAGE: LanguageFn = unsafe { LanguageFn::from_raw(tree_sitter_odoc) };

/// The content of the [`node-types.json`] file for this grammar.
///
/// [`node-types.json`]: https://tree-sitter.github.io/tree-sitter/using-parsers/6-static-node-types
pub const NODE_TYPES: &str = include_str!("../../src/node-types.json");

/// The syntax highlighting query for this grammar.
pub const HIGHLIGHTS_QUERY: &str = include_str!("../../queries/highlights.scm");

/// The language injection query for this grammar.
pub const INJECTIONS_QUERY: &str = include_str!("../../queries/injections.scm");

#[cfg(test)]
mod tests {
    #[test]
    fn test_can_load_grammar() {
        let mut parser = tree_sitter::Parser::new();
        parser
            .set_language(&super::LANGUAGE.into())
            .expect("Error loading Odoc parser");
    }

    // Editors load the queries through these constants, so a query that
    // names a node or field the grammar no longer has must fail here.
    #[test]
    fn test_highlights_query_is_valid() {
        tree_sitter::Query::new(&super::LANGUAGE.into(), super::HIGHLIGHTS_QUERY)
            .expect("Invalid highlights query");
    }

    #[test]
    fn test_injections_query_is_valid() {
        tree_sitter::Query::new(&super::LANGUAGE.into(), super::INJECTIONS_QUERY)
            .expect("Invalid injections query");
    }

    #[test]
    fn test_node_types_matches_grammar() {
        let language: tree_sitter::Language = super::LANGUAGE.into();
        for kind in ["document", "paragraph", "code_block"] {
            assert!(
                super::NODE_TYPES.contains(&format!("\"type\": \"{kind}\"")),
                "node-types.json has no {kind} node"
            );
            assert_ne!(
                language.id_for_node_kind(kind, true),
                0,
                "grammar has no {kind} node"
            );
        }
    }
}
