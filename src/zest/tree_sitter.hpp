#pragma once

#include <zest/text.hpp>

#include <tree_sitter/api.h>

#include <memory>

namespace zest
{

namespace tree_sitter
{

inline void delete_parser(TSParser* parser)
{
    ts_parser_delete(parser);
}

inline void delete_tree(TSTree* tree)
{
    ts_tree_delete(tree);
}

inline void delete_query(TSQuery* query)
{
    ts_query_delete(query);
}

inline void delete_query_cursor(TSQueryCursor* query_cursor)
{
    ts_query_cursor_delete(query_cursor);
}

using ParserPtr = std::unique_ptr<TSParser, decltype(delete_parser)*>;
using TreePtr = std::unique_ptr<TSTree, decltype(delete_tree)*>;
using QueryPtr = std::unique_ptr<TSQuery, decltype(delete_query)*>;
using QueryCursorPtr = std::unique_ptr<TSQueryCursor,
                                       decltype(delete_query_cursor)*>;


ParserPtr init();
QueryPtr init_highlight_queries(const TSParser* parser);
QueryCursorPtr init_query_cursor();

TreePtr parse_text(TSParser* parser, LineBuffer& line_buff);


} // namespace tree_sitter

} // namespace zest
