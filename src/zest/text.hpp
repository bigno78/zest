#pragma once

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>


template<typename Line>
class LineBufferImpl
{
public:
    const Line& get_line(int index) const { return lines_[index]; }
    Line& get_line(int index) { return lines_[index]; }

    size_t line_count() const { return lines_.size(); }

    void add_line(int index, const std::string& line)
    {
        if (index == lines_.size())
        {
            lines_.push_back(line);
            return;
        }

        size_t old_size = lines_.size();
        lines_.resize(lines_.size() + 1);

        for (int i = old_size; i > index; --i)
        {
            lines_[i - 1] = std::move(lines_[i]);
        }
        lines_[index] = line;
    }

    void append_line(const std::string& line)
    {
        add_line(line_count(), line);
    }

private:
    std::vector<Line> lines_;
};

using LineBuffer = LineBufferImpl<std::string>;

inline LineBuffer load_file(const std::string& path)
{
    std::ifstream input_stream(path);

    if (!input_stream)
        throw std::runtime_error("Cannot open file '" + path + "'");

    LineBuffer buffer;

    std::string line;
    while (std::getline(input_stream, line))
    {
        buffer.append_line(line);
    }

    return buffer;
}
