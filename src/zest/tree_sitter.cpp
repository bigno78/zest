#include "tree_sitter.hpp"

#include <zest/highlight/captures.hpp>
#include <zest/highlight/queries.hpp>

#include <iostream>


using namespace zest::tree_sitter;


extern "C" TSLanguage* tree_sitter_cpp();

static char newline = '\n';

static const char* get_text_chunk(void* payload,
                                  uint32_t byte_index,
                                  TSPoint position,
                                  uint32_t* bytes_read)
{
    const LineBuffer& line_buff = *(LineBuffer*)payload;

    if (position.row >= line_buff.line_count())
    {
        *bytes_read = 0;
        return nullptr;
    }

    const std::string* row = &line_buff.get_line(position.row);

    if (position.column == row->size())
    {
        *bytes_read = 1;
        return &newline;
    }

    *bytes_read = row->size() - position.column;
    return row->c_str() + position.column;
}

ParserPtr zest::tree_sitter::init()
{
    ParserPtr parser(ts_parser_new(), delete_parser);
    ts_parser_set_language(parser.get(), tree_sitter_cpp());
    return parser;
}

QueryPtr zest::tree_sitter::init_highlight_queries(const TSParser* parser)
{
        const TSLanguage* lang = ts_parser_language(parser);

    uint32_t err_offset;
    TSQueryError err_type;

    TSQuery* raw_query =
        ts_query_new(lang, zest::highlight::cpp_queries,
                     std::strlen(zest::highlight::cpp_queries),
                     &err_offset, &err_type);

    if (!raw_query)
    {
        std::cerr << "Query error at " << err_offset << "\n";
        std::cerr << std::string_view(zest::highlight::cpp_queries + err_offset) << "\n";
        switch(err_type)
        {
            case TSQueryErrorSyntax:
                std::cout << "syntax\n";
                break;
            case TSQueryErrorNodeType:
                std::cout << "node_type\n";
                break;
            case TSQueryErrorField:
                std::cout << "field\n";
                break;
            case TSQueryErrorCapture:
                std::cout << "capture\n";
                break;
            case TSQueryErrorNone:
                std::cout << "none\n";
                break;
            default:
                std::cout << "idk\n";
                break;
        }
        throw std::runtime_error("Failed to load queries.");
    }

    return zest::tree_sitter::QueryPtr(raw_query,
                                       zest::tree_sitter::delete_query);
}

QueryCursorPtr zest::tree_sitter::init_query_cursor()
{
    return zest::tree_sitter::QueryCursorPtr(
            ts_query_cursor_new(),
            zest::tree_sitter::delete_query_cursor);
}

TreePtr zest::tree_sitter::parse_text(TSParser* parser, LineBuffer& line_buff)
{
    TSInput input{
        &line_buff,
        get_text_chunk,
        TSInputEncodingUTF8
    };

    ts_parser_reset(parser);
    TSTree* tree_raw = ts_parser_parse(parser, NULL, input);

    return TreePtr(tree_raw, delete_tree);
}
