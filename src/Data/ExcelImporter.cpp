#include "Data/ExcelImporter.h"
#include <QDebug>

namespace qpcr {

#ifdef HAS_OPENXLSX
#include <OpenXLSX.hpp>
QString ExcelImporter::s_lastError;
#endif

ExcelImporter::ExcelImporter()
{
}

ExcelImporter::~ExcelImporter()
{
}

bool ExcelImporter::isExcelSupported()
{
#ifdef HAS_OPENXLSX
    return true;
#else
    return false;
#endif
}

QString ExcelImporter::lastError()
{
#ifdef HAS_OPENXLSX
    return s_lastError;
#else
    return QString("Excel support not enabled. Recompile with OpenXLSX library.");
#endif
}

QVector<SheetInfo> ExcelImporter::listSheets(const QString& path)
{
#ifdef HAS_OPENXLSX
    QVector<SheetInfo> sheets;
    s_lastError.clear();

    try {
        OpenXLSX::XLDocument doc;
        doc.open(path.toStdString());
        auto workbook = doc.workbook();

        for (const auto& name : workbook.worksheetNames()) {
            auto worksheet = workbook.worksheet(name);

            SheetInfo info;
            info.name = QString::fromStdString(name);
            info.rowCount = worksheet.rowCount();
            info.columnCount = worksheet.columnCount();

            sheets.append(info);
        }

        doc.close();

    } catch (const std::exception& e) {
        s_lastError = QString("Error reading Excel file: %1").arg(e.what());
        qWarning() << s_lastError;
    }

    return sheets;
#else
    Q_UNUSED(path);
    qWarning() << lastError();
    return QVector<SheetInfo>();
#endif
}

DataFrame ExcelImporter::importSheet(const QString& path, int sheetIndex, bool hasHeader)
{
#ifdef HAS_OPENXLSX
    DataFrame df;
    s_lastError.clear();

    try {
        OpenXLSX::XLDocument doc;
        doc.open(path.toStdString());
        auto workbook = doc.workbook();

        if (sheetIndex < 0 || sheetIndex >= workbook.worksheetNames().size()) {
            s_lastError = QString("Sheet index %1 out of range").arg(sheetIndex);
            doc.close();
            return df;
        }

        auto sheetName = workbook.worksheetNames()[sheetIndex];
        auto worksheet = workbook.worksheet(sheetName);

        // Get dimensions
        int firstRow = worksheet.firstRow();
        int lastRow = worksheet.lastRow();
        int firstCol = worksheet.firstColumn();
        int lastCol = worksheet.lastColumn();

        if (lastRow < 1 || lastCol < 1) {
            s_lastError = "Empty worksheet";
            doc.close();
            return df;
        }

        // Read header
        QStringList columnNames;
        if (hasHeader) {
            for (int col = firstCol; col <= lastCol; ++col) {
                auto cell = worksheet.cell(firstRow, col);
                QString value = QString::fromStdString(cell.value().asString());

                if (value.isEmpty()) {
                    value = QString("V%1").arg(col - firstCol + 1);
                }

                columnNames.append(value);
                df.addColumn(value, QVector<QVariant>());
            }
        } else {
            // Generate column names
            for (int col = firstCol; col <= lastCol; ++col) {
                QString name = QString("V%1").arg(col - firstCol + 1);
                columnNames.append(name);
                df.addColumn(name, QVector<QVariant>());
            }
        }

        // Read data rows
        int dataStartRow = hasHeader ? firstRow + 1 : firstRow;

        for (int row = dataStartRow; row <= lastRow; ++row) {
            for (int col = firstCol; col <= lastCol; ++col) {
                auto cell = worksheet.cell(row, col);
                QVariant value;

                // Try to get the value as string first
                std::string strValue = cell.value().asString();

                if (strValue.empty()) {
                    value = QVariant();
                } else {
                    // Try to convert to number
                    QString qstr = QString::fromStdString(strValue);
                    bool ok = false;
                    double num = qstr.toDouble(&ok);
                    if (ok) {
                        value = num;
                    } else {
                        value = qstr;
                    }
                }

                // Add to dataframe column
                QString colName = columnNames[col - firstCol];
                auto& column = df.m_data[colName];
                column.append(value);
            }
        }

        df.m_rowCount = lastRow - dataStartRow + 1;
        doc.close();

    } catch (const std::exception& e) {
        s_lastError = QString("Error importing Excel file: %1").arg(e.what());
        qWarning() << s_lastError;
    }

    return df;
#else
    Q_UNUSED(path);
    Q_UNUSED(sheetIndex);
    Q_UNUSED(hasHeader);
    qWarning() << lastError();
    return DataFrame();
#endif
}

DataFrame ExcelImporter::importSheetByName(const QString& path, const QString& sheetName, bool hasHeader)
{
#ifdef HAS_OPENXLSX
    DataFrame df;
    s_lastError.clear();

    try {
        OpenXLSX::XLDocument doc;
        doc.open(path.toStdString());
        auto workbook = doc.workbook();

        // Find sheet index
        int sheetIndex = -1;
        const auto& names = workbook.worksheetNames();
        for (int i = 0; i < names.size(); ++i) {
            if (QString::fromStdString(names[i]) == sheetName) {
                sheetIndex = i;
                break;
            }
        }

        if (sheetIndex < 0) {
            s_lastError = QString("Sheet '%1' not found").arg(sheetName);
            doc.close();
            return df;
        }

        doc.close();
        return importSheet(path, sheetIndex, hasHeader);

    } catch (const std::exception& e) {
        s_lastError = QString("Error importing Excel file: %1").arg(e.what());
        qWarning() << s_lastError;
    }

    return df;
#else
    Q_UNUSED(path);
    Q_UNUSED(sheetName);
    Q_UNUSED(hasHeader);
    qWarning() << lastError();
    return DataFrame();
#endif
}

} // namespace qpcr
