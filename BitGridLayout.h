#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>

namespace BitGridCodec::detail
{
constexpr std::size_t rowWidth(std::size_t cells)
{
    return 2 * cells + 2;
}

constexpr std::size_t gridSize(std::size_t cells, std::size_t rows, std::size_t newlineSize)
{
    return (rows + 2) * (rowWidth(cells) + newlineSize);
}

inline std::string makeGrid(std::size_t cells, std::size_t rows)
{
    const auto width = rowWidth(cells);
    const auto stride = width + 1;
    std::string grid(gridSize(cells, rows, 1), '#');
    for (std::size_t row = 0; row < rows + 2; ++row)
    {
        const auto start = row * stride;
        if (row == 0 || row == rows + 1)
        {
            grid[start] = grid[start + width - 1] = '+';
            std::fill_n(grid.data() + start + 1, width - 2, '-');
        }
        else
        {
            grid[start] = grid[start + width - 1] = '|';
        }
        grid[start + width] = '\n';
    }
    return grid;
}

// Callers validate the row width before checking the border.
inline bool horizontalBorder(std::string_view line)
{
    return line.front() == '+' && line.back() == '+' &&
           line.substr(1, line.size() - 2).find_first_not_of('-') == std::string_view::npos;
}

inline bool verticalBorder(std::string_view line)
{
    return line.front() == '|' && line.back() == '|';
}
} // namespace BitGridCodec::detail
