/******************************************************************************
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#include "table.h"
#include <iostream>
#include <iomanip>
#include "oeaware/utils.h"

namespace oeaware {
void Table::Init(int cols, const std::string &tempName)
{
    columns = cols;
    name = tempName;
    alignments.resize(cols, Alignment::LEFT);
    columnWidths.resize(cols, columnWidth);
}
bool Table::AddRow(const std::vector<std::string> &row)
{
    int size = row.size();
    if (size != columns) {
        return false;
    }
    for (const auto &str : row) {
        if (str != "") {  // only add non-empty row to table
            data.emplace_back(row);
            return true;
        }
    }
    return false;
}

bool Table::RemoveRow(int rowIndex)
{
    int size = data.size();
    if (rowIndex < 0 || rowIndex >= size) {
        return false;
    }
    data.erase(data.begin() + rowIndex);
    return true;
}

int Table::GetTableWidth() const
{
    return columns * (columnWidth + 1) + columns + 1;
}

std::string Table::GetTableName() const
{
    return name;
}

std::string Table::GetContent(int i, int j) const
{
    int size = data.size();
    if (i < 0 || i >= size) {
        return "";
    }
    size = data[i].size();
    if (j < 0 || j >= size) {
        return "";
    }
    return data[i][j];
}

void Table::SetBorder(bool border)
{
    hasBorder = border;
}

void Table::SetColumnWidth(int width)
{
    columnWidth = width;
    for (auto &w : columnWidths) {
        w = width;
    }
}

bool Table::SetColumnAlignment(int colIndex, Alignment alignment)
{
    if (colIndex < 0 || colIndex >= columns) {
        return false;
    }
    alignments[colIndex] = alignment;
    return true;
}

std::vector<std::string> SplitCellContent(const std::string &content, size_t width)
{
    std::vector<std::string> contentSplit;
    std::stringstream ss(content);
    std::vector<std::string> lines;
    std::string tmp;
    // step 1: split by '\n'
    while (std::getline(ss, tmp, '\n')) {
        contentSplit.push_back(tmp);
    }
    // step2: split by max width
    for (const auto &line : contentSplit) {
        size_t start = 0;
        while (start < line.length()) {
            size_t end = std::min(start + width, line.size());
            lines.emplace_back(line.substr(start, end - start));
            start = end;
        }
    }

    return lines;
}

void Table::PrintBoard() const
{
    std::cout << "+";
    for (size_t i = 0; i < columnWidths.size(); ++i) {
        std::cout << std::string(columnWidths[i] + 1, '-') << "+";
    }
    std::cout << std::endl;
}

void Table::PrintTable() const
{
    if (hasBorder) {
        PrintBoard();
    }
    for (const auto &row : data) {
        std::vector<std::vector<std::string>> cellLines;
        size_t maxLines = 0;
        for (size_t i = 0; i < row.size(); ++i) {
            auto lines = SplitCellContent(row[i], columnWidths[i]);
            cellLines.emplace_back(lines);
            maxLines = std::max(maxLines, lines.size());
        }
        for (size_t line = 0; line < maxLines; ++line) {
            if (hasBorder) {
                std::cout << "|";
            }
            for (size_t col = 0; col < row.size(); ++col) {
                if (line < cellLines[col].size()) {
                    if (alignments[col] == Alignment::LEFT) {
                        std::cout << std::left  << std::setw(columnWidths[col]) << cellLines[col][line] << " ";
                    } else {
                        std::cout << std::right  << std::setw(columnWidths[col]) << cellLines[col][line] << " ";
                    }
                } else {
                    std::cout << std::string(columnWidths[col] + 1, ' ');
                }
                if (hasBorder) {
                    std::cout << "|";
                }
            }
            std::cout << std::endl;
        }
        if (hasBorder) {
            PrintBoard();
        }
    }
}

std::string Table::GetMarkDownTable()
{
    std::string ret = "";

    for (size_t i = 0; i < data.size(); ++i) {
        for (size_t j = 0; j < data[i].size(); ++j) {
            std::string md = ReplaceString(data[i][j], "\n", "<br>");
            ret += "| " + md + " ";
        }
        ret += "|\n";
        if (!i) {
            for (size_t j = 0; j < data[i].size(); ++j) {
                ret += "| --- ";
            }
            ret += "|\n";
        }
    }
    return ret;
}
}