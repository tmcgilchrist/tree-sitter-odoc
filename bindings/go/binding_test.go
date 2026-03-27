package tree_sitter_odoc_test

import (
	"testing"

	tree_sitter "github.com/tree-sitter/go-tree-sitter"
	tree_sitter_odoc "github.com/tmcgilchrist/tree-sitter-odoc/bindings/go"
)

func TestCanLoadGrammar(t *testing.T) {
	language := tree_sitter.NewLanguage(tree_sitter_odoc.Language())
	if language == nil {
		t.Errorf("Error loading Odoc grammar")
	}
}
