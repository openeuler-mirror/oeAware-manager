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
#ifndef COMMON_TABLE_H
#define COMMON_TABLE_H
#include <vector>
#include <string>

namespace oeaware {
const int DEFAULT_COLUMN_WIDTH = 12;
class Table {
public:
    enum class Alignment {
        LEFT,
        RIGHT
    };
    Table() :hasBorder(true), columnWidth(DEFAULT_COLUMN_WIDTH) {}
    Table(int cols, const std::string &name) : hasBorder(true), columnWidth(DEFAULT_COLUMN_WIDTH)
    {
        Init(cols, name);
    }
    void Init(int cols, const std::string &name);
    bool AddRow(const std::vector<std::string> &row);
    bool RemoveRow(int rowIndex);
    bool SetColumnAlignment(int colIndex, Alignment alignment);
    void SetBorder(bool border);
    void SetColumnWidth(int width);
    void PrintTable() const;
    std::string GetMarkDownTable();
    int GetTableWidth() const;
    std::string GetTableName() const;
    std::string GetContent(int i, int j) const;
    int GetRowCount() const { return data.size(); }
private:
    void PrintBoard() const;
    int columns;
    std::string name;
    bool hasBorder;
    std::vector<std::vector<std::string>> data;
    std::vector<Alignment> alignments;
    std::vector<size_t> columnWidths;
    int columnWidth;
};
}

#endif