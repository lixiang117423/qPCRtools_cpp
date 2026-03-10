#include "Data/CSVParser.h"
#include <QFile>
#include <QTextStream>
#include <QStringConverter>
#include <QDebug>

namespace qpcr {

CSVParser::CSVParser()
{
}

CSVParser::CSVParser(const QString& filePath)
{
    Q_UNUSED(filePath);
}

void CSVParser::setSeparator(QChar separator) {
    m_separator = separator;
}

void CSVParser::setQuoteChar(QChar quote) {
    m_quoteChar = quote;
}

void CSVParser::setSkipEmptyLines(bool skip) {
    m_skipEmptyLines = skip;
}

void CSVParser::setEncoding(const QString& encoding) {
    m_encoding = encoding;
}

QString CSVParser::errorString() const {
    return m_errorString;
}

DataFrame CSVParser::parse(const QString& path, bool hasHeader) {
    m_errorString.clear();

    QFile file(path);
    if (!file.exists()) {
        m_errorString = QString("File not found: %1").arg(path);
        return DataFrame();
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_errorString = QString("Cannot open file: %1").arg(path);
        return DataFrame();
    }

    QTextStream in(&file);

    // Set encoding
    if (m_encoding == "UTF-8") {
        in.setEncoding(QStringConverter::Utf8);
    } else if (m_encoding == "GBK") {
        in.setEncoding(QStringConverter::System);
    } else {
        in.setAutoDetectUnicode(true);
    }

    DataFrame df;
    QStringList columnNames;
    int currentRow = 0;

    while (!in.atEnd()) {
        QString line = in.readLine();

        // Skip empty lines if configured
        if (m_skipEmptyLines && line.trimmed().isEmpty()) {
            continue;
        }

        QStringList fields;
        parseLine(line, fields);

        if (fields.isEmpty()) {
            continue;
        }

        if (currentRow == 0 && hasHeader) {
            // First row is header
            columnNames = fields;

            // Initialize columns in DataFrame
            for (const QString& name : columnNames) {
                df.addColumn(name, QVector<QVariant>());
            }
        } else {
            // Data row
            if (columnNames.isEmpty() && !hasHeader) {
                // Auto-generate column names
                for (int i = 0; i < fields.size(); ++i) {
                    columnNames.append(QString("V%1").arg(i + 1));
                    df.addColumn(columnNames[i], QVector<QVariant>());
                }
            }

            // Add data to columns
            int rowIndex = currentRow - (hasHeader ? 1 : 0);
            for (int i = 0; i < qMin(fields.size(), columnNames.size()); ++i) {
                QVariant value = convertValue(fields[i], i);

                if (!df.hasColumn(columnNames[i])) {
                    df.addColumn(columnNames[i], QVariant());
                }

                // Use set method to add data
                df.set(rowIndex, columnNames[i], value);
            }

            // Fill missing columns with QVariant()
            for (int i = fields.size(); i < columnNames.size(); ++i) {
                if (!df.hasColumn(columnNames[i])) {
                    df.addColumn(columnNames[i], QVariant());
                }
                df.set(rowIndex, columnNames[i], QVariant());
            }
        }

        currentRow++;
    }

    file.close();

    return df;
}

QString CSVParser::parseLine(const QString& line, QStringList& fields) {
    fields.clear();

    QString currentField;
    bool inQuotes = false;

    for (int i = 0; i < line.length(); ++i) {
        QChar c = line[i];

        if (c == m_quoteChar) {
            // Check for escaped quote (double quote)
            if (i + 1 < line.length() && line[i + 1] == m_quoteChar) {
                currentField += m_quoteChar;
                i++; // Skip next quote
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == m_separator && !inQuotes) {
            fields.append(currentField.trimmed());
            currentField.clear();
        } else {
            currentField += c;
        }
    }

    // Add last field
    fields.append(currentField.trimmed());

    return QString();
}

QVariant CSVParser::convertValue(const QString& str, int columnIndex) {
    if (str.isEmpty()) {
        return QVariant();
    }

    // Try to convert to integer
    bool ok = false;
    int intValue = str.toInt(&ok);
    if (ok && str == QString::number(intValue)) {
        return intValue;
    }

    // Try to convert to double
    double doubleValue = str.toDouble(&ok);
    if (ok) {
        return doubleValue;
    }

    // Keep as string
    return str;
}

QStringList CSVParser::preview(const QString& path, int lines) {
    QStringList result;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return result;
    }

    QTextStream in(&file);
    in.setAutoDetectUnicode(true);

    int count = 0;
    while (!in.atEnd() && count < lines) {
        result.append(in.readLine());
        count++;
    }

    file.close();
    return result;
}

} // namespace qpcr
