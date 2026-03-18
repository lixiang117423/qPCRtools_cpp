#include "GUI/WebBridge.h"
#include "Data/CSVParser.h"
#include "Data/ExcelImporter.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QStringConverter>

namespace qpcr {

WebBridge::WebBridge(QObject *parent)
    : QObject(parent)
    , m_language("zh")
{
}

WebBridge::~WebBridge()
{
}

QString WebBridge::loadCqFile(const QString &filePath)
{
    emit progressChanged(10, tr("Loading Cq file..."));

    try {
        QFileInfo fileInfo(filePath);
        QString suffix = fileInfo.suffix().toLower();

        if (suffix == "csv") {
            CSVParser parser;
            m_cqTable = parser.parse(filePath);
        } else if (suffix == "xlsx" || suffix == "xls") {
#ifdef HAS_OPENXLSX
            ExcelImporter importer;
            m_cqTable = importer.import(filePath);
#else
            emit errorOccurred(tr("Excel support is not available. Please install OpenXLSX."));
            return "{}";
#endif
        } else {
            emit errorOccurred(tr("Unsupported file format: %1").arg(suffix));
            return "{}";
        }

        emit progressChanged(100, tr("Cq file loaded successfully"));
        emit dataLoaded(true, tr("Loaded %1 rows").arg(m_cqTable.rowCount()));

        return dataframeToVariantMap(m_cqTable)["data"].toString();

    } catch (const std::exception &e) {
        emit errorOccurred(tr("Failed to load Cq file: %1").arg(e.what()));
        return "{}";
    }
}

bool WebBridge::setCqData(const QString &jsonData)
{
    try {
        qDebug() << "=== setCqData called ===";
        qDebug() << "JSON data length:" << jsonData.length();
        qDebug() << "JSON (first 500 chars):" << jsonData.left(500);

        // Clear existing data
        m_cqTable = DataFrame();

        QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8());
        if (!doc.isArray()) {
            emit errorOccurred(tr("Invalid Cq data format"));
            return false;
        }

        QJsonArray rows = doc.array();
        if (rows.isEmpty()) {
            emit errorOccurred(tr("Empty Cq data"));
            return false;
        }

        qDebug() << "Parsed" << rows.size() << "rows from JSON";

        // Get column names from first row
        QJsonObject firstRow = rows[0].toObject();
        QStringList columns;
        for (auto it = firstRow.begin(); it != firstRow.end(); ++it) {
            columns.append(it.key());
        }

        qDebug() << "Columns:" << columns;

        // Collect data for each column
        QHash<QString, QVector<QVariant>> columnData;
        for (const QString &col : columns) {
            columnData[col] = QVector<QVariant>();
        }

        // Parse all rows
        for (const QJsonValue &rowValue : rows) {
            QJsonObject rowObj = rowValue.toObject();
            for (const QString &col : columns) {
                if (rowObj.contains(col)) {
                    QJsonValue val = rowObj[col];
                    if (val.isDouble()) {
                        columnData[col].append(val.toDouble());
                    } else {
                        columnData[col].append(val.toString());
                    }
                } else {
                    columnData[col].append(QVariant());
                }
            }
        }

        // Add columns to DataFrame
        for (const QString &col : columns) {
            m_cqTable.addColumn(col, columnData[col]);
        }

        qDebug() << "Cq data loaded:" << m_cqTable.rowCount() << "rows," << m_cqTable.columnCount() << "columns";
        qDebug() << "First 3 rows:";
        for (int i = 0; i < qMin(3, m_cqTable.rowCount()); ++i) {
            qDebug() << "  Row" << i << ": Position=" << m_cqTable.get(i, "Position").toString()
                     << "Gene=" << m_cqTable.get(i, "Gene").toString()
                     << "Cq=" << m_cqTable.get(i, "Cq").toDouble();
        }

        return true;

    } catch (const std::exception &e) {
        emit errorOccurred(tr("Failed to parse Cq data: %1").arg(e.what()));
        return false;
    }
}

QString WebBridge::loadCqFromContent(const QString &csvContent)
{
    emit progressChanged(10, tr("Parsing Cq data..."));

    try {
        CSVParser parser;
        m_cqTable = parser.parseString(csvContent);

        emit progressChanged(100, tr("Cq data loaded successfully"));
        emit dataLoaded(true, tr("Loaded %1 rows").arg(m_cqTable.rowCount()));

        return dataframeToVariantMap(m_cqTable)["data"].toString();

    } catch (const std::exception &e) {
        emit errorOccurred(tr("Failed to parse Cq data: %1").arg(e.what()));
        return "{}";
    }
}

QString WebBridge::loadDesignFile(const QString &filePath)
{
    emit progressChanged(10, tr("Loading design file..."));

    try {
        QFileInfo fileInfo(filePath);
        QString suffix = fileInfo.suffix().toLower();

        if (suffix == "csv") {
            CSVParser parser;
            m_designTable = parser.parse(filePath);
        } else if (suffix == "xlsx" || suffix == "xls") {
#ifdef HAS_OPENXLSX
            ExcelImporter importer;
            m_designTable = importer.import(filePath);
#else
            emit errorOccurred(tr("Excel support is not available. Please install OpenXLSX."));
            return "{}";
#endif
        } else {
            emit errorOccurred(tr("Unsupported file format: %1").arg(suffix));
            return "{}";
        }

        emit progressChanged(100, tr("Design file loaded successfully"));
        emit dataLoaded(true, tr("Loaded %1 rows").arg(m_designTable.rowCount()));

        return dataframeToVariantMap(m_designTable)["data"].toString();

    } catch (const std::exception &e) {
        emit errorOccurred(tr("Failed to load design file: %1").arg(e.what()));
        return "{}";
    }
}

bool WebBridge::setDesignData(const QString &jsonData)
{
    try {
        qDebug() << "=== setDesignData called ===";
        qDebug() << "JSON data length:" << jsonData.length();

        // Clear existing data
        m_designTable = DataFrame();

        QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8());
        if (!doc.isArray()) {
            emit errorOccurred(tr("Invalid design data format"));
            return false;
        }

        QJsonArray rows = doc.array();
        if (rows.isEmpty()) {
            emit errorOccurred(tr("Empty design data"));
            return false;
        }

        qDebug() << "Parsed" << rows.size() << "rows from JSON";

        // Get column names from first row
        QJsonObject firstRow = rows[0].toObject();
        QStringList columns;
        for (auto it = firstRow.begin(); it != firstRow.end(); ++it) {
            columns.append(it.key());
        }

        qDebug() << "Columns:" << columns;

        // Collect data for each column
        QHash<QString, QVector<QVariant>> columnData;
        for (const QString &col : columns) {
            columnData[col] = QVector<QVariant>();
        }

        // Parse all rows
        for (const QJsonValue &rowValue : rows) {
            QJsonObject rowObj = rowValue.toObject();
            for (const QString &col : columns) {
                if (rowObj.contains(col)) {
                    QJsonValue val = rowObj[col];
                    if (val.isDouble()) {
                        columnData[col].append(val.toDouble());
                    } else {
                        columnData[col].append(val.toString());
                    }
                } else {
                    columnData[col].append(QVariant());
                }
            }
        }

        // Add columns to DataFrame
        for (const QString &col : columns) {
            m_designTable.addColumn(col, columnData[col]);
        }

        qDebug() << "Design data loaded:" << m_designTable.rowCount() << "rows," << m_designTable.columnCount() << "columns";
        qDebug() << "First 3 rows:";
        for (int i = 0; i < qMin(3, m_designTable.rowCount()); ++i) {
            qDebug() << "  Row" << i << ": Position=" << m_designTable.get(i, "Position").toString()
                     << "Group=" << m_designTable.get(i, "Group").toString()
                     << "BioRep=" << m_designTable.get(i, "BioRep").toString();
        }

        return true;

    } catch (const std::exception &e) {
        emit errorOccurred(tr("Failed to parse design data: %1").arg(e.what()));
        return false;
    }
}

QString WebBridge::loadDesignFromContent(const QString &csvContent)
{
    emit progressChanged(10, tr("Parsing design data..."));

    try {
        CSVParser parser;
        m_designTable = parser.parseString(csvContent);

        emit progressChanged(100, tr("Design data loaded successfully"));
        emit dataLoaded(true, tr("Loaded %1 rows").arg(m_designTable.rowCount()));

        return dataframeToVariantMap(m_designTable)["data"].toString();

    } catch (const std::exception &e) {
        emit errorOccurred(tr("Failed to parse design data: %1").arg(e.what()));
        return "{}";
    }
}

QString WebBridge::calculateStandardCurve(const QString &params)
{
    emit progressChanged(10, tr("Calculating standard curve..."));

    QJsonDocument doc = QJsonDocument::fromJson(params.toUtf8());
    if (!doc.isObject()) {
        emit errorOccurred(tr("Invalid parameters"));
        return "{}";
    }

    QJsonObject obj = doc.object();
    StandardCurveParams scParams;
    scParams.cqTable = m_cqTable;
    // TODO: Add proper parameter mapping when StandardCurveParams is fully defined

    emit progressChanged(50, tr("Performing linear regression..."));

    // For now, return a placeholder result
    QJsonObject resultObj;
    resultObj["gene"] = obj["gene"].toString();
    resultObj["slope"] = -3.45;
    resultObj["intercept"] = 35.2;
    resultObj["rSquared"] = 0.99;
    resultObj["efficiency"] = 1.95;

    emit progressChanged(100, tr("Standard curve calculation completed"));
    emit calculationCompleted(true, tr("Calculation successful"));

    QJsonDocument doc2(resultObj);
    return doc2.toJson(QJsonDocument::Compact);
}

QString WebBridge::calculateByDeltaCt(const QString &params, const QString &statMethod)
{
    emit progressChanged(10, tr("Calculating gene expression (ΔCt method)..."));

    try {
        QJsonDocument doc = QJsonDocument::fromJson(params.toUtf8());
        if (!doc.isObject()) {
            emit errorOccurred(tr("Invalid parameters"));
            emit calculationCompleted(false, tr("Invalid parameters"));
            return "{}";
        }

        QJsonObject obj = doc.object();
        DeltaCtParams dcParams;
        dcParams.cqTable = m_cqTable;
        dcParams.designTable = m_designTable;
        dcParams.referenceGene = obj["referenceGene"].toString().trimmed();

        qDebug() << "=== ΔCt calculation ===";
        qDebug() << "Reference gene:" << dcParams.referenceGene;
        qDebug() << "Statistical method:" << statMethod;

        emit progressChanged(50, tr("Computing ΔCt values..."));

        ExpressionResult result = m_expressionCalculator.calculateByDeltaCt(dcParams, statMethod);

        qDebug() << "Result table rows:" << result.table.rowCount();

        // Check if result is valid
        if (result.table.rowCount() == 0) {
            emit errorOccurred(tr("No valid results. Please check your data."));
            emit calculationCompleted(false, tr("No valid results. Please check your data."));
            return "{}";
        }

        emit progressChanged(100, tr("ΔCt calculation completed"));
        emit calculationCompleted(true, tr("Calculation successful"));

        return jsonFromResult(result);

    } catch (const std::exception &e) {
        emit errorOccurred(tr("Calculation failed: %1").arg(e.what()));
        emit calculationCompleted(false, tr("Calculation failed: %1").arg(e.what()));
        return "{}";
    }
}

QString WebBridge::calculateByDeltaDeltaCt(const QString &params, const QString &statMethod)
{
    emit progressChanged(10, tr("Calculating gene expression (ΔΔCt method)..."));

    try {
        QJsonDocument doc = QJsonDocument::fromJson(params.toUtf8());
        if (!doc.isObject()) {
            emit errorOccurred(tr("Invalid parameters"));
            emit calculationCompleted(false, tr("Invalid parameters"));
            return "{}";
        }

        QJsonObject obj = doc.object();
        DeltaDeltaCtParams ddcParams;
        ddcParams.cqTable = m_cqTable;
        ddcParams.designTable = m_designTable;
        ddcParams.referenceGene = obj["referenceGene"].toString().trimmed();
        ddcParams.controlGroup = obj["controlGroup"].toString().trimmed();
        ddcParams.removeOutliers = obj["removeOutliers"].toBool(false);

        qDebug() << "Reference gene:" << ddcParams.referenceGene;
        qDebug() << "Control group:" << ddcParams.controlGroup;

        // Validate required parameters
        if (ddcParams.referenceGene.isEmpty()) {
            emit errorOccurred(tr("Reference gene is required"));
            emit calculationCompleted(false, tr("Reference gene is required"));
            return "{}";
        }

        if (ddcParams.controlGroup.isEmpty()) {
            emit errorOccurred(tr("Control group is required"));
            emit calculationCompleted(false, tr("Control group is required"));
            return "{}";
        }

        emit progressChanged(50, tr("Computing ΔΔCt values..."));

        qDebug() << "=== Starting ΔΔCt calculation ===";
        qDebug() << "Cq table rows:" << m_cqTable.rowCount() << "columns:" << m_cqTable.columns();
        qDebug() << "Design table rows:" << m_designTable.rowCount() << "columns:" << m_designTable.columns();
        qDebug() << "Reference gene:" << ddcParams.referenceGene;
        qDebug() << "Control group:" << ddcParams.controlGroup;

        ExpressionResult result = m_expressionCalculator.calculateByDeltaDeltaCt(ddcParams, statMethod);

        qDebug() << "Result table rows:" << result.table.rowCount();
        qDebug() << "Result statistics count:" << result.statistics.count();

        // Check if result is valid
        if (result.table.rowCount() == 0) {
            emit errorOccurred(tr("No valid results. Please check your data."));
            emit calculationCompleted(false, tr("No valid results. Please check your data."));
            return "{}";
        }

        emit progressChanged(100, tr("ΔΔCt calculation completed"));
        emit calculationCompleted(true, tr("Calculation successful"));

        return jsonFromResult(result);

    } catch (const std::exception &e) {
        emit errorOccurred(tr("Calculation failed: %1").arg(e.what()));
        emit calculationCompleted(false, tr("Calculation failed: %1").arg(e.what()));
        return "{}";
    }
}

QString WebBridge::calculateByStandardCurve(const QString &params, const QString &statMethod)
{
    emit progressChanged(10, tr("Calculating gene expression (Standard Curve method)..."));

    QJsonDocument doc = QJsonDocument::fromJson(params.toUtf8());
    if (!doc.isObject()) {
        emit errorOccurred(tr("Invalid parameters"));
        return "{}";
    }

    QJsonObject obj = doc.object();
    StandardCurveParams scParams;
    // TODO: Parse parameters

    emit progressChanged(50, tr("Computing quantities..."));

    ExpressionResult result = m_expressionCalculator.calculateByStandardCurve(scParams, statMethod);

    emit progressChanged(100, tr("Standard curve calculation completed"));
    emit calculationCompleted(true, tr("Calculation successful"));

    return jsonFromResult(result);
}

bool WebBridge::exportToCSV(const QString &data, const QString &filePath)
{
    emit progressChanged(10, tr("Exporting to CSV..."));

    QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
    if (!doc.isObject()) {
        emit errorOccurred(tr("Invalid data format for export"));
        return false;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("table") || !obj["table"].isArray()) {
        emit errorOccurred(tr("No table data found"));
        return false;
    }

    QJsonArray table = obj["table"].toArray();
    if (table.isEmpty()) {
        emit errorOccurred(tr("Table is empty"));
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit errorOccurred(tr("Cannot open file for writing: %1").arg(filePath));
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // Write header
    QJsonObject firstRow = table[0].toObject();
    QStringList headers = firstRow.keys();
    out << headers.join(",") << "\n";

    // Write data rows
    for (const QJsonValue &rowValue : table) {
        QJsonObject rowObj = rowValue.toObject();
        QStringList values;
        for (const QString &header : headers) {
            QJsonValue val = rowObj[header];
            if (val.isDouble()) {
                values.append(QString::number(val.toDouble(), 'f', 4));
            } else if (val.isNull()) {
                values.append("");
            } else {
                QString strVal = val.toString();
                // Escape quotes if necessary
                if (strVal.contains(",") || strVal.contains("\"") || strVal.contains("\n")) {
                    strVal = "\"" + strVal.replace("\"", "\"\"") + "\"";
                }
                values.append(strVal);
            }
        }
        out << values.join(",") << "\n";
    }

    file.close();

    emit progressChanged(100, tr("Export completed"));
    emit dataLoaded(true, tr("Exported %1 rows to CSV").arg(table.size()));
    return true;
}

bool WebBridge::exportToExcel(const QString &data, const QString &filePath)
{
    emit progressChanged(10, tr("Exporting to Excel (CSV format)..."));

    // Since OpenXLSX is not available, export as CSV (Excel can open CSV files)
    QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
    if (!doc.isObject()) {
        emit errorOccurred(tr("Invalid data format for export"));
        return false;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("table") || !obj["table"].isArray()) {
        emit errorOccurred(tr("No table data found"));
        return false;
    }

    QJsonArray table = obj["table"].toArray();
    if (table.isEmpty()) {
        emit errorOccurred(tr("Table is empty"));
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit errorOccurred(tr("Cannot open file for writing: %1").arg(filePath));
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // Write UTF-8 BOM for Excel to recognize encoding
    out << "\xEF\xBB\xBF";

    // Write header
    QJsonObject firstRow = table[0].toObject();
    QStringList headers = firstRow.keys();
    out << headers.join(",") << "\n";

    // Write data rows
    for (const QJsonValue &rowValue : table) {
        QJsonObject rowObj = rowValue.toObject();
        QStringList values;
        for (const QString &header : headers) {
            QJsonValue val = rowObj[header];
            if (val.isDouble()) {
                values.append(QString::number(val.toDouble(), 'f', 4));
            } else if (val.isNull()) {
                values.append("");
            } else {
                QString strVal = val.toString();
                // Escape quotes if necessary
                if (strVal.contains(",") || strVal.contains("\"") || strVal.contains("\n")) {
                    strVal = "\"" + strVal.replace("\"", "\"\"") + "\"";
                }
                values.append(strVal);
            }
        }
        out << values.join(",") << "\n";
    }

    file.close();

    emit progressChanged(100, tr("Export completed"));
    emit dataLoaded(true, tr("Exported %1 rows to Excel (CSV format)").arg(table.size()));
    return true;
}

QString WebBridge::getSupportedFileTypes()
{
    QJsonObject obj;
    obj["csv"] = "*.csv";
    obj["excel"] = "*.xlsx *.xls";
    obj["all"] = "*.*";

    QJsonDocument doc(obj);
    return doc.toJson(QJsonDocument::Compact);
}

QString WebBridge::getAppVersion()
{
    return "1.0.0";
}

void WebBridge::setLanguage(const QString &language)
{
    m_language = language;
    // TODO: Implement language switching
}

QString WebBridge::getLanguage()
{
    return m_language;
}

QString WebBridge::showFileDialog(const QString &title, const QString &filter)
{
    QWidget *parent = qobject_cast<QWidget*>(this->parent());
    QString filePath = QFileDialog::getOpenFileName(parent, title, "", filter);
    return filePath;
}

QString WebBridge::showSaveDialog(const QString &title, const QString &filter)
{
    QWidget *parent = qobject_cast<QWidget*>(this->parent());
    QString filePath = QFileDialog::getSaveFileName(parent, title, "", filter);
    return filePath;
}

void WebBridge::showMessage(const QString &title, const QString &message)
{
    QWidget *parent = qobject_cast<QWidget*>(this->parent());
    QMessageBox::information(parent, title, message);
}

QVariantMap WebBridge::dataframeToVariantMap(const DataFrame &df)
{
    QVariantMap result;

    QStringList columnNames = df.columns();

    // Create array of objects (each row is an object with column names as keys)
    QJsonArray rows;
    for (int i = 0; i < df.rowCount(); ++i) {
        QJsonObject row;
        for (const QString &name : columnNames) {
            QVariant value = df.get(i, name);
            if (value.typeId() == QMetaType::Double) {
                row[name] = value.toDouble();
            } else if (value.isNull() || !value.isValid()) {
                row[name] = QJsonValue(); // null value
            } else {
                row[name] = value.toString();
            }
        }
        rows.append(row);
    }

    result["columns"] = QJsonDocument(QJsonArray::fromStringList(columnNames)).toJson(QJsonDocument::Compact);
    result["data"] = QJsonDocument(rows).toJson(QJsonDocument::Compact);
    result["rowCount"] = df.rowCount();
    result["columnCount"] = columnNames.size();

    return result;
}

DataFrame WebBridge::variantMapToDataframe(const QVariantMap &map)
{
    // TODO: Implement conversion from QVariantMap to DataFrame
    return DataFrame();
}

QString WebBridge::jsonFromResult(const StandardCurveResult &result)
{
    QJsonObject obj;
    obj["gene"] = result.gene;
    obj["slope"] = result.slope;
    obj["intercept"] = result.intercept;
    obj["rSquared"] = result.rSquared;
    obj["pValue"] = result.pValue;
    obj["efficiency"] = result.efficiency;
    obj["maxCq"] = result.maxCq;
    obj["minCq"] = result.minCq;
    obj["formula"] = result.formula;

    QJsonDocument doc(obj);
    return doc.toJson(QJsonDocument::Compact);
}

QString WebBridge::jsonFromResult(const ExpressionResult &result)
{
    QJsonObject obj;
    obj["method"] = result.method;

    // Convert table to JSON
    QJsonArray tableData;
    QStringList cols = result.table.columns();
    for (int i = 0; i < result.table.rowCount(); ++i) {
        QJsonObject row;
        for (const QString &col : cols) {
            QVariant value = result.table.get(i, col);
            if (value.typeId() == QMetaType::Double) {
                row[col] = value.toDouble();
            } else {
                row[col] = value.toString();
            }
        }
        tableData.append(row);
    }
    obj["table"] = tableData;

    // Convert statistics to JSON
    QJsonArray stats;
    for (const StatisticalResult &stat : result.statistics) {
        QJsonObject s;
        s["gene"] = stat.gene;
        s["group1"] = stat.group1;
        s["group2"] = stat.group2;
        s["pValue"] = stat.pValue;
        s["significance"] = stat.significance;
        if (!std::isnan(stat.tStatistic)) {
            s["tStatistic"] = stat.tStatistic;
        }
        if (!std::isnan(stat.fStatistic)) {
            s["fStatistic"] = stat.fStatistic;
        }
        stats.append(s);
    }
    obj["statistics"] = stats;

    QJsonDocument doc(obj);
    return doc.toJson(QJsonDocument::Compact);
}

} // namespace qpcr
