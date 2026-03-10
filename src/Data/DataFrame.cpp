#include "Data/DataFrame.h"
#include "Data/CSVParser.h"
#include "Data/ExcelImporter.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QVariantMap>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace qpcr {

//=============================================================================
// Row implementation
//=============================================================================

Row::Row(const QHash<QString, QVariant>& data)
    : m_data(data)
{
}

QVariant Row::value(const QString& columnName) const {
    return m_data.value(columnName);
}

void Row::setValue(const QString& columnName, const QVariant& value) {
    m_data[columnName] = value;
}

QStringList Row::columns() const {
    return m_data.keys();
}

bool Row::hasColumn(const QString& columnName) const {
    return m_data.contains(columnName);
}

//=============================================================================
// DataFrame implementation
//=============================================================================

DataFrame DataFrame::fromCSV(const QString& path, bool hasHeader) {
    CSVParser parser;
    return parser.parse(path, hasHeader);
}

DataFrame DataFrame::fromExcel(const QString& path, int sheetIndex, bool hasHeader) {
#ifdef HAS_OPENXLSX
    ExcelImporter importer;
    return importer.importSheet(path, sheetIndex, hasHeader);
#else
    Q_UNUSED(path);
    Q_UNUSED(sheetIndex);
    Q_UNUSED(hasHeader);
    qWarning() << "Excel support not enabled. Recompile with OpenXLSX library.";
    return DataFrame();
#endif
}

DataFrame DataFrame::fromVariantMap(const QVariantMap& data) {
    DataFrame df;

    for (auto it = data.begin(); it != data.end(); ++it) {
        const QVariantList& list = it.value().toList();
        QVector<QVariant> column;
        column.reserve(list.size());

        for (const auto& item : list) {
            column.append(item);
        }

        df.addColumn(it.key(), column);
    }

    return df;
}

bool DataFrame::saveCSV(const QString& path) const {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // Write header
    out << m_columnNames.join(",") << "\n";

    // Write data
    for (int i = 0; i < m_rowCount; ++i) {
        QStringList values;
        for (const QString& col : m_columnNames) {
            QVariant val = m_data[col][i];
            QString str = val.toString();

            // Escape quotes and wrap in quotes if contains comma
            if (str.contains(",")) {
                str = "\"" + str.replace("\"", "\"\"") + "\"";
            }
            values.append(str);
        }
        out << values.join(",") << "\n";
    }

    file.close();
    return true;
}

bool DataFrame::saveExcel(const QString& path) const {
#ifdef HAS_OPENXLSX
    // Implement with OpenXLSX
    Q_UNUSED(path);
    return false;
#else
    Q_UNUSED(path);
    qWarning() << "Excel export not enabled.";
    return false;
#endif
}

int DataFrame::rowCount() const {
    return m_rowCount;
}

int DataFrame::columnCount() const {
    return m_columnNames.size();
}

QStringList DataFrame::columns() const {
    return m_columnNames;
}

QSet<QString> DataFrame::columnSet() const {
    return QSet<QString>(m_columnNames.begin(), m_columnNames.end());
}

QVariant::Type DataFrame::columnType(const QString& columnName) const {
    if (!hasColumn(columnName) || m_data[columnName].isEmpty()) {
        return QVariant::Invalid;
    }
    return m_data[columnName].first().type();
}

bool DataFrame::hasColumn(const QString& columnName) const {
    return m_data.contains(columnName);
}

DataFrame DataFrame::selectColumns(const QStringList& colNames) const {
    DataFrame result;
    for (const QString& col : colNames) {
        if (hasColumn(col)) {
            result.addColumn(col, m_data[col]);
        }
    }
    return result;
}

DataFrame DataFrame::renameColumn(const QString& oldName, const QString& newName) const {
    if (!hasColumn(oldName)) {
        return *this;
    }

    DataFrame result = *this;
    result.m_data[newName] = result.m_data.take(oldName);

    for (int i = 0; i < result.m_columnNames.size(); ++i) {
        if (result.m_columnNames[i] == oldName) {
            result.m_columnNames[i] = newName;
            break;
        }
    }

    return result;
}

void DataFrame::addColumn(const QString& name, const QVector<QVariant>& data) {
    if (!m_columnNames.contains(name)) {
        m_columnNames.append(name);
    }

    m_data[name] = data;

    if (data.size() > m_rowCount) {
        m_rowCount = data.size();
    }

    // Fill other columns with default values if needed
    for (const QString& col : m_columnNames) {
        if (col != name && m_data[col].size() < m_rowCount) {
            m_data[col].resize(m_rowCount);
        }
    }
}

void DataFrame::addColumn(const QString& name, const QVariant& defaultValue) {
    QVector<QVariant> data(m_rowCount, defaultValue);
    addColumn(name, data);
}

QVariant DataFrame::get(int row, const QString& column) const {
    if (row < 0 || row >= m_rowCount || !hasColumn(column)) {
        return QVariant();
    }
    return m_data[column][row];
}

void DataFrame::set(int row, const QString& column, const QVariant& value) {
    if (!hasColumn(column)) {
        return;
    }

    // Expand DataFrame if needed
    if (row >= m_rowCount) {
        for (const QString& col : m_columnNames) {
            m_data[col].resize(row + 1);
        }
        m_rowCount = row + 1;
    }

    m_data[column][row] = value;
}

Row DataFrame::getRow(int row) const {
    if (row < 0 || row >= m_rowCount) {
        return Row();
    }

    QHash<QString, QVariant> rowData;
    for (const QString& col : m_columnNames) {
        rowData[col] = m_data[col][row];
    }

    return Row(rowData);
}

QVector<QVariant> DataFrame::getColumn(const QString& columnName) const {
    return m_data.value(columnName);
}

QVector<double> DataFrame::getNumericColumn(const QString& columnName) const {
    QVector<double> result;
    const auto& col = m_data.value(columnName);

    for (const auto& val : col) {
        bool ok = false;
        double num = val.toDouble(&ok);
        if (ok) {
            result.append(num);
        } else {
            result.append(qQNaN());
        }
    }

    return result;
}

QVector<QString> DataFrame::getStringColumn(const QString& columnName) const {
    QVector<QString> result;
    const auto& col = m_data.value(columnName);

    for (const auto& val : col) {
        result.append(val.toString());
    }

    return result;
}

template<typename T>
QVector<T> DataFrame::uniqueValues(const QString& column) const {
    QSet<T> uniqueSet;
    const auto& col = m_data.value(column);

    for (const auto& val : col) {
        uniqueSet.insert(val.value<T>());
    }

    return uniqueSet.values().toVector();
}

// Explicit instantiation for common types
template QVector<QString> DataFrame::uniqueValues<QString>(const QString&) const;
template QVector<double> DataFrame::uniqueValues<double>(const QString&) const;
template QVector<int> DataFrame::uniqueValues<int>(const QString&) const;

DataFrame DataFrame::filter(const std::function<bool(const Row&)>& predicate) const {
    DataFrame result;

    // Copy column structure
    for (const QString& col : m_columnNames) {
        result.addColumn(col, QVector<QVariant>());
    }

    // Filter rows
    for (int i = 0; i < m_rowCount; ++i) {
        Row row = getRow(i);
        if (predicate(row)) {
            for (const QString& col : m_columnNames) {
                result.m_data[col].append(m_data[col][i]);
            }
            result.m_rowCount++;
        }
    }

    return result;
}

DataFrame DataFrame::groupBy(const QString& column) const {
    DataFrame result;

    if (!hasColumn(column)) {
        return result;
    }

    auto groups = uniqueValues<QString>(column);

    for (const auto& group : groups) {
        // Create a filtered dataframe for this group
        auto groupDf = filter([column, group](const Row& row) {
            return row.value(column).toString() == group;
        });

        // Add to result
        for (const QString& col : m_columnNames) {
            QString colName = col + "_" + group;
            // ... implement grouping logic
        }
    }

    return result;
}

DataFrame DataFrame::join(const DataFrame& other, const QString& keyColumn) const {
    DataFrame result;

    // Get all unique column names
    QSet<QString> allCols = columnSet() + other.columnSet();

    // Build index for the other dataframe
    QHash<QString, int> otherIndex;
    for (int i = 0; i < other.rowCount(); ++i) {
        QString key = other.get(i, keyColumn).toString();
        if (!otherIndex.contains(key)) {
            otherIndex[key] = i;
        }
    }

    // Join
    for (int i = 0; i < m_rowCount; ++i) {
        QString key = get(i, keyColumn).toString();

        for (const QString& col : allCols) {
            QVariant value;
            if (hasColumn(col)) {
                value = get(i, col);
            } else if (other.hasColumn(col) && otherIndex.contains(key)) {
                value = other.get(otherIndex[key], col);
            }

            if (!result.hasColumn(col)) {
                result.addColumn(col, QVector<QVariant>());
            }
            result.m_data[col].append(value);
        }
    }

    result.m_columnNames = allCols.values().toVector();
    result.m_rowCount = m_rowCount;

    return result;
}

DataFrame DataFrame::orderBy(const QString& column, bool ascending) const {
    DataFrame result = *this;

    if (!hasColumn(column)) {
        return result;
    }

    // Create sort indices
    QVector<int> indices(m_rowCount);
    std::iota(indices.begin(), indices.end(), 0);

    std::sort(indices.begin(), indices.end(), [this, column, ascending](int a, int b) {
        QVariant valA = get(a, column);
        QVariant valB = get(b, column);

        // Compare QVariants
        if (valA.userType() == QMetaType::Double && valB.userType() == QMetaType::Double) {
            double dA = valA.toDouble();
            double dB = valB.toDouble();
            if (dA < dB) return ascending ? true : false;
            if (dA > dB) return ascending ? false : true;
        } else {
            // String comparison
            QString strA = valA.toString();
            QString strB = valB.toString();
            if (strA < strB) return ascending ? true : false;
            if (strA > strB) return ascending ? false : true;
        }
        return false;
    });

    // Reorder all columns
    for (QString& col : result.m_columnNames) {
        QVector<QVariant> oldData = result.m_data[col];
        for (int i = 0; i < m_rowCount; ++i) {
            result.m_data[col][i] = oldData[indices[i]];
        }
    }

    return result;
}

QVariant DataFrame::sum(const QString& column) const {
    auto nums = getNumericColumn(column);
    double total = 0.0;
    for (double val : nums) {
        if (!std::isnan(val)) {
            total += val;
        }
    }
    return total;
}

double DataFrame::mean(const QString& column) const {
    auto nums = getNumericColumn(column);
    QVector<double> validNums;

    for (double val : nums) {
        if (!std::isnan(val)) {
            validNums.append(val);
        }
    }

    if (validNums.isEmpty()) {
        return qQNaN();
    }

    return std::accumulate(validNums.begin(), validNums.end(), 0.0) / validNums.size();
}

double DataFrame::median(const QString& column) const {
    auto nums = getNumericColumn(column);
    QVector<double> validNums;

    for (double val : nums) {
        if (!std::isnan(val)) {
            validNums.append(val);
        }
    }

    if (validNums.isEmpty()) {
        return qQNaN();
    }

    std::sort(validNums.begin(), validNums.end());
    int n = validNums.size();

    if (n % 2 == 0) {
        return (validNums[n/2 - 1] + validNums[n/2]) / 2.0;
    } else {
        return validNums[n/2];
    }
}

double DataFrame::sd(const QString& column) const {
    auto nums = getNumericColumn(column);
    QVector<double> validNums;

    for (double val : nums) {
        if (!std::isnan(val)) {
            validNums.append(val);
        }
    }

    if (validNums.size() < 2) {
        return qQNaN();
    }

    double m = mean(column);
    double sumSqDiff = 0.0;

    for (double val : validNums) {
        double diff = val - m;
        sumSqDiff += diff * diff;
    }

    return std::sqrt(sumSqDiff / (validNums.size() - 1));
}

double DataFrame::min(const QString& column) const {
    auto nums = getNumericColumn(column);
    QVector<double> validNums;

    for (double val : nums) {
        if (!std::isnan(val)) {
            validNums.append(val);
        }
    }

    if (validNums.isEmpty()) {
        return qQNaN();
    }

    return *std::min_element(validNums.begin(), validNums.end());
}

double DataFrame::max(const QString& column) const {
    auto nums = getNumericColumn(column);
    QVector<double> validNums;

    for (double val : nums) {
        if (!std::isnan(val)) {
            validNums.append(val);
        }
    }

    if (validNums.isEmpty()) {
        return qQNaN();
    }

    return *std::max_element(validNums.begin(), validNums.end());
}

int DataFrame::count(const QString& column) const {
    if (!hasColumn(column)) {
        return 0;
    }

    int count = 0;
    const auto& col = m_data[column];

    for (const auto& val : col) {
        if (!val.isNull() && !val.toString().isEmpty()) {
            count++;
        }
    }

    return count;
}

QVariantMap DataFrame::toMap() const {
    QVariantMap map;

    for (const QString& col : m_columnNames) {
        QVariantList list;

        for (int i = 0; i < m_rowCount; ++i) {
            list.append(m_data[col][i]);
        }

        map[col] = list;
    }

    return map;
}

QString DataFrame::toString() const {
    QString result;

    // Header
    result += m_columnNames.join("\t") + "\n";

    // Data
    for (int i = 0; i < m_rowCount; ++i) {
        QStringList values;
        for (const QString& col : m_columnNames) {
            values.append(m_data[col][i].toString());
        }
        result += values.join("\t") + "\n";
    }

    return result;
}

QString DataFrame::toHTML() const {
    QString html = "<table border='1'>\n";

    // Header
    html += "<tr>";
    for (const QString& col : m_columnNames) {
        html += "<th>" + col + "</th>";
    }
    html += "</tr>\n";

    // Data
    for (int i = 0; i < m_rowCount; ++i) {
        html += "<tr>";
        for (const QString& col : m_columnNames) {
            html += "<td>" + m_data[col][i].toString() + "</td>";
        }
        html += "</tr>\n";
    }

    html += "</table>";
    return html;
}

//=============================================================================
// Iterator implementation
//=============================================================================

DataFrame::Iterator::Iterator(const DataFrame* df, int row)
    : m_df(df), m_row(row)
{
}

Row DataFrame::Iterator::operator*() const {
    return m_df->getRow(m_row);
}

DataFrame::Iterator& DataFrame::Iterator::operator++() {
    ++m_row;
    return *this;
}

bool DataFrame::Iterator::operator!=(const Iterator& other) const {
    return m_row != other.m_row;
}

DataFrame::Iterator DataFrame::begin() const {
    return Iterator(this, 0);
}

DataFrame::Iterator DataFrame::end() const {
    return Iterator(this, m_rowCount);
}

} // namespace qpcr
