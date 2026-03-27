import XCTest
import SwiftTreeSitter
import TreeSitterOdoc

final class TreeSitterOdocTests: XCTestCase {
    func testCanLoadGrammar() throws {
        let parser = Parser()
        let language = Language(language: tree_sitter_odoc())
        XCTAssertNoThrow(try parser.setLanguage(language),
                         "Error loading Odoc grammar")
    }
}
