#include "tree_sitter.hpp"

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
