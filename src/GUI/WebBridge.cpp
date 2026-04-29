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
#include <QTemporaryFile>
#include <QDir>
#include <QTextStream>
#include <QByteArray>
#include <QStringConverter>

namespace qpcr {

//=============================================================================
// Static helpers
//=============================================================================

static QString makeErrorResult(const QString &message)
{
    QJsonObject obj;
    obj["error"] = message;
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

static QString dataframeToCompactJson(const DataFrame &df)
{
    QVariantMap result;

    QStringList columnNames = df.columns();
    QJsonArray rows;
    for (int i = 0; i < df.rowCount(); ++i) {
        QJsonObject row;
        for (const QString &name : columnNames) {
            QVariant value = df.get(i, name);
            if (value.typeId() == QMetaType::Double) {
                row[name] = value.toDouble();
            } else if (value.isNull() || !value.isValid()) {
                row[name] = QJsonValue();
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

    QJsonObject completeResult;
    completeResult["data"] = QJsonDocument::fromJson(result["data"].toString().toUtf8()).array();
    completeResult["columns"] = QJsonDocument::fromJson(result["columns"].toString().toUtf8()).array();

    return QJsonDocument(completeResult).toJson(QJsonDocument::Compact);
}

//=============================================================================
// DataType helper implementations
//=============================================================================

DataFrame& WebBridge::tableRef(DataType type)
{
    switch (type) {
    case DataType::Cq:     return m_cqTable;
    case DataType::Design: return m_designTable;
    case DataType::Concen: return m_concenTable;
    }
    return m_cqTable; // fallback
}

const QString& WebBridge::tableLabel(DataType type)
{
    static const QString s_cq("Cq");
    static const QString s_design("design");
    static const QString s_concen("concentration");
    switch (type) {
    case DataType::Cq:     return s_cq;
    case DataType::Design: return s_design;
    case DataType::Concen: return s_concen;
    }
    return s_cq;
}

//=============================================================================
// Generic data loading implementations
//=============================================================================

QString WebBridge::loadDataFile(DataType type, const QString &filePath)
{
    const QString &label = tableLabel(type);
    emit progressChanged(10, tr("Loading %1 file...").arg(label));

    try {
        QFileInfo fileInfo(filePath);
        QString suffix = fileInfo.suffix().toLower();

        if (suffix == "csv") {
            CSVParser parser;
            tableRef(type) = parser.parse(filePath);
        } else if (suffix == "xlsx" || suffix == "xls") {
#ifdef HAS_OPENXLSX
            ExcelImporter importer;
            tableRef(type) = importer.import(filePath);
#else
            emit errorOccurred(tr("Excel support is not available. Please install OpenXLSX."));
            return "{}";
#endif
        } else {
            emit errorOccurred(tr("Unsupported file format: %1").arg(suffix));
            return "{}";
        }

        DataFrame &table = tableRef(type);
        emit progressChanged(100, tr("%1 file loaded successfully").arg(label));
        emit dataLoaded(true, tr("Loaded %1 rows").arg(table.rowCount()));

        return dataframeToCompactJson(table);
    } catch (const std::exception &e) {
        emit errorOccurred(tr("Failed to load %1 file: %2").arg(label, e.what()));
        return "{}";
    }
}

bool WebBridge::setTableData(DataType type, const QString &jsonData)
{
    const QString &label = tableLabel(type);

    try {
        qDebug() << "=== set" << label << "Data called ===";
        qDebug() << "JSON data length:" << jsonData.length();

        tableRef(type) = DataFrame();

        QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8());
        if (!doc.isArray()) {
            emit errorOccurred(tr("Invalid %1 data format").arg(label));
            return false;
        }

        QJsonArray rows = doc.array();
        if (rows.isEmpty()) {
            emit errorOccurred(tr("Empty %1 data").arg(label));
            return false;
        }

        qDebug() << "Parsed" << rows.size() << "rows from JSON";

        QJsonObject firstRow = rows[0].toObject();
        QStringList columns;
        for (auto it = firstRow.begin(); it != firstRow.end(); ++it) {
            columns.append(it.key());
        }
        qDebug() << "Columns:" << columns;

        QHash<QString, QVector<QVariant>> columnData;
        for (const QString &col : columns) {
            columnData[col] = QVector<QVariant>();
        }

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

        DataFrame &table = tableRef(type);
        for (const QString &col : columns) {
            table.addColumn(col, columnData[col]);
        }

        qDebug() << label << "data loaded:" << table.rowCount() << "rows," << table.columnCount() << "columns";
        if (table.rowCount() > 0) {
            qDebug() << "  First row:" << table.get(0, "Position").toString()
                     << table.get(0, "Gene").toString();
        }
        return true;

    } catch (const std::exception &e) {
        emit errorOccurred(tr("Failed to parse %1 data: %2").arg(label, e.what()));
        return false;
    }
}

QString WebBridge::loadDataFromContent(DataType type, const QString &csvContent)
{
    const QString &label = tableLabel(type);
    emit progressChanged(10, tr("Parsing %1 data...").arg(label));

    try {
        CSVParser parser;
        tableRef(type) = parser.parseString(csvContent);

        DataFrame &table = tableRef(type);
        emit progressChanged(100, tr("%1 data loaded successfully").arg(label));
        emit dataLoaded(true, tr("Loaded %1 rows").arg(table.rowCount()));

        return dataframeToCompactJson(table);

    } catch (const std::exception &e) {
        emit errorOccurred(tr("Failed to parse %1 data: %2").arg(label, e.what()));
        return "{}";
    }
}

QString WebBridge::loadExcelFromBase64(DataType type, const QString &base64Data, int sheetIndex, bool hasHeader)
{
    const QString &label = tableLabel(type);
    emit progressChanged(10, tr("Parsing %1 Excel data...").arg(label));

    try {
        ExcelImporter importer;
        if (!importer.isExcelSupported()) {
            QString err = tr("Excel support is not available. Please install OpenXLSX.");
            emit errorOccurred(err);
            return makeErrorResult(err);
        }

        QByteArray bytes = QByteArray::fromBase64(base64Data.toUtf8());
        if (bytes.isEmpty()) {
            QString err = tr("Invalid or empty Excel base64 payload.");
            emit errorOccurred(err);
            return makeErrorResult(err);
        }

        const QString ext = bytes.startsWith("PK") ? "xlsx" : "xls";
        QTemporaryFile tmp(QDir::tempPath() + QString("/qPCRtools_%1_XXXXXX.%2").arg(label).arg(ext));
        if (!tmp.open()) {
            QString err = tr("Failed to create a temporary file for Excel import.");
            emit errorOccurred(err);
            return makeErrorResult(err);
        }

        const qint64 written = tmp.write(bytes);
        if (written != bytes.size()) {
            QString err = tr("Failed to write Excel data to a temporary file.");
            emit errorOccurred(err);
            return makeErrorResult(err);
        }
        tmp.flush();

        DataFrame &table = tableRef(type);
        table = importer.importSheet(tmp.fileName(), sheetIndex, hasHeader);
        if (table.rowCount() == 0) {
            QString err = importer.lastError();
            if (err.isEmpty()) {
                err = tr("No valid rows found in the Excel file.");
            }
            emit errorOccurred(err);
            return makeErrorResult(err);
        }

        emit progressChanged(100, tr("%1 Excel data loaded successfully").arg(label));
        emit dataLoaded(true, tr("Loaded %1 rows").arg(table.rowCount()));

        return dataframeToCompactJson(table);
    } catch (const std::exception &e) {
        QString err = tr("Failed to parse %1 Excel data: %2").arg(label, e.what());
        emit errorOccurred(err);
        return makeErrorResult(err);
    }
}

//=============================================================================
// Constructor / Destructor
//=============================================================================

WebBridge::WebBridge(QObject *parent)
    : QObject(parent)
    , m_language("zh")
{
}

WebBridge::~WebBridge()
{
}

//=============================================================================
// Public Q_INVOKABLE methods — thin delegates to generic helpers
//=============================================================================

QString WebBridge::loadCqFile(const QString &filePath)
{
    return loadDataFile(DataType::Cq, filePath);
}

bool WebBridge::setCqData(const QString &jsonData)
{
    return setTableData(DataType::Cq, jsonData);
}

QString WebBridge::loadCqFromContent(const QString &csvContent)
{
    return loadDataFromContent(DataType::Cq, csvContent);
}

QString WebBridge::loadCqExcelFromBase64(const QString &base64Data, int sheetIndex, bool hasHeader)
{
    return loadExcelFromBase64(DataType::Cq, base64Data, sheetIndex, hasHeader);
}

QString WebBridge::loadDesignFile(const QString &filePath)
{
    return loadDataFile(DataType::Design, filePath);
}

bool WebBridge::setDesignData(const QString &jsonData)
{
    return setTableData(DataType::Design, jsonData);
}

QString WebBridge::loadDesignFromContent(const QString &csvContent)
{
    return loadDataFromContent(DataType::Design, csvContent);
}

QString WebBridge::loadDesignExcelFromBase64(const QString &base64Data, int sheetIndex, bool hasHeader)
{
    return loadExcelFromBase64(DataType::Design, base64Data, sheetIndex, hasHeader);
}

QString WebBridge::loadConcenFile(const QString &filePath)
{
    return loadDataFile(DataType::Concen, filePath);
}

bool WebBridge::setConcenData(const QString &jsonData)
{
    return setTableData(DataType::Concen, jsonData);
}

QString WebBridge::loadConcenFromContent(const QString &csvContent)
{
    return loadDataFromContent(DataType::Concen, csvContent);
}

QString WebBridge::loadConcenExcelFromBase64(const QString &base64Data, int sheetIndex, bool hasHeader)
{
    return loadExcelFromBase64(DataType::Concen, base64Data, sheetIndex, hasHeader);
}

//=============================================================================
// Calculation methods
//=============================================================================

QString WebBridge::calculateStandardCurve(const QString &params)
{
    emit progressChanged(10, tr("Calculating standard curve..."));

    try {
        QJsonDocument doc = QJsonDocument::fromJson(params.toUtf8());
        if (!doc.isObject()) {
            emit errorOccurred(tr("Invalid parameters"));
            emit calculationCompleted(false, tr("Invalid parameters"));
            return "{}";
        }

        QJsonObject obj = doc.object();
        double lowestConcen = obj["lowestConcen"].toDouble(4.0);
        double highestConcen = obj["highestConcen"].toDouble(4096.0);
        double dilution = obj["dilution"].toDouble(4.0);
        bool byMean = obj["byMean"].toBool(true);

        qDebug() << "=== Standard Curve Calculation ==="
                 << "Low:" << lowestConcen << "High:" << highestConcen
                 << "Dilution:" << dilution << "Mean:" << byMean;

        if (m_cqTable.rowCount() == 0) {
            emit errorOccurred(tr("No Cq data loaded"));
            emit calculationCompleted(false, tr("No Cq data loaded"));
            return "{}";
        }
        if (m_concenTable.rowCount() == 0) {
            emit errorOccurred(tr("No concentration data loaded"));
            emit calculationCompleted(false, tr("No concentration data loaded"));
            return "{}";
        }

        emit progressChanged(50, tr("Performing linear regression..."));

        QVector<StandardCurveResult> results = StandardCurve::calculate(
            m_cqTable, m_concenTable, lowestConcen, highestConcen, dilution, byMean);

        if (results.isEmpty()) {
            emit errorOccurred(tr("No valid results. Please check your data."));
            emit calculationCompleted(false, tr("No valid results. Please check your data."));
            return "{}";
        }

        qDebug() << "Calculated" << results.size() << "standard curves";
        emit progressChanged(100, tr("Standard curve calculation completed"));
        emit calculationCompleted(true, tr("Calculation successful"));

        return jsonFromStandardCurveResults(results);

    } catch (const std::exception &e) {
        emit errorOccurred(tr("Calculation failed: %1").arg(e.what()));
        emit calculationCompleted(false, tr("Calculation failed: %1").arg(e.what()));
        return "{}";
    }
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
        dcParams.controlGroup = obj["controlGroup"].toString().trimmed();

        qDebug() << "=== ΔCt calculation ===" << "Ref:" << dcParams.referenceGene
                 << "Ctrl:" << dcParams.controlGroup << "Stat:" << statMethod;

        emit progressChanged(50, tr("Computing ΔCt values..."));

        ExpressionResult result = m_expressionCalculator.calculateByDeltaCt(dcParams, statMethod);

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

        qDebug() << "=== ΔΔCt calculation ==="
                 << "Cq rows:" << m_cqTable.rowCount()
                 << "Design rows:" << m_designTable.rowCount()
                 << "Ref:" << ddcParams.referenceGene
                 << "Ctrl:" << ddcParams.controlGroup;

        emit progressChanged(50, tr("Computing ΔΔCt values..."));

        ExpressionResult result = m_expressionCalculator.calculateByDeltaDeltaCt(ddcParams, statMethod);

        qDebug() << "Result rows:" << result.table.rowCount()
                 << "Statistics:" << result.statistics.count();

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

    try {
        QJsonDocument doc = QJsonDocument::fromJson(params.toUtf8());
        if (!doc.isObject()) {
            emit errorOccurred(tr("Invalid parameters"));
            emit calculationCompleted(false, tr("Invalid parameters"));
            return "{}";
        }

        QJsonObject obj = doc.object();
        StandardCurveParams scParams;
        scParams.cqTable = m_cqTable;
        scParams.designTable = m_designTable;
        scParams.referenceGene = obj["referenceGene"].toString().trimmed();
        scParams.controlGroup = obj["controlGroup"].toString().trimmed();

        if (scParams.referenceGene.isEmpty()) {
            emit errorOccurred(tr("Reference gene is required"));
            emit calculationCompleted(false, tr("Reference gene is required"));
            return "{}";
        }
        if (scParams.controlGroup.isEmpty()) {
            emit errorOccurred(tr("Control group is required"));
            emit calculationCompleted(false, tr("Control group is required"));
            return "{}";
        }
        if (m_cqTable.rowCount() == 0) {
            emit errorOccurred(tr("No Cq data loaded"));
            emit calculationCompleted(false, tr("No Cq data loaded"));
            return "{}";
        }
        if (m_designTable.rowCount() == 0) {
            emit errorOccurred(tr("No design data loaded"));
            emit calculationCompleted(false, tr("No design data loaded"));
            return "{}";
        }

        qDebug() << "=== Standard Curve Expression ==="
                 << "Ref:" << scParams.referenceGene
                 << "Ctrl:" << scParams.controlGroup
                 << "Stat:" << statMethod;

        emit progressChanged(50, tr("Computing quantities from efficiency values..."));

        ExpressionResult result = m_expressionCalculator.calculateByStandardCurve(scParams, statMethod);

        qDebug() << "Result rows:" << result.table.rowCount()
                 << "Statistics:" << result.statistics.count();

        if (result.table.rowCount() == 0) {
            emit errorOccurred(tr("No valid results. Please check your data."));
            emit calculationCompleted(false, tr("No valid results. Please check your data."));
            return "{}";
        }

        emit progressChanged(100, tr("Standard curve calculation completed"));
        emit calculationCompleted(true, tr("Calculation successful"));
        return jsonFromResult(result);

    } catch (const std::exception &e) {
        emit errorOccurred(tr("Calculation failed: %1").arg(e.what()));
        emit calculationCompleted(false, tr("Calculation failed: %1").arg(e.what()));
        return "{}";
    }
}

//=============================================================================
// Export methods
//=============================================================================

bool WebBridge::writeTableToCSV(const QJsonArray &table, const QString &filePath, bool writeBOM)
{
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

    if (writeBOM) out << "\xEF\xBB\xBF";

    QJsonObject firstRow = table[0].toObject();
    QStringList headers = firstRow.keys();
    out << headers.join(",") << "\n";

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
                if (strVal.contains(",") || strVal.contains("\"") || strVal.contains("\n")) {
                    strVal = "\"" + strVal.replace("\"", "\"\"") + "\"";
                }
                values.append(strVal);
            }
        }
        out << values.join(",") << "\n";
    }

    file.close();
    return true;
}

bool WebBridge::exportToCSV(const QString &data, const QString &filePath)
{
    emit progressChanged(10, tr("Exporting to CSV..."));

    QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
    if (!doc.isObject() || !doc.object().contains("table") || !doc.object()["table"].isArray()) {
        emit errorOccurred(tr("Invalid data format for export"));
        return false;
    }

    QJsonArray table = doc.object()["table"].toArray();

    if (!writeTableToCSV(table, filePath, false)) return false;

    emit progressChanged(100, tr("Export completed"));
    emit dataLoaded(true, tr("Exported %1 rows to CSV").arg(table.size()));
    return true;
}

bool WebBridge::exportToExcel(const QString &data, const QString &filePath)
{
    emit progressChanged(10, tr("Exporting to Excel (CSV format)..."));

    QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
    if (!doc.isObject() || !doc.object().contains("table") || !doc.object()["table"].isArray()) {
        emit errorOccurred(tr("Invalid data format for export"));
        return false;
    }

    QJsonArray table = doc.object()["table"].toArray();

    if (!writeTableToCSV(table, filePath, true)) return false;

    emit progressChanged(100, tr("Export completed"));
    emit dataLoaded(true, tr("Exported %1 rows to Excel (CSV format)").arg(table.size()));
    return true;
}

//=============================================================================
// Utility methods
//=============================================================================

QString WebBridge::getSupportedFileTypes()
{
    QJsonObject obj;
    obj["csv"] = "*.csv";
    obj["excel"] = "*.xlsx *.xls";
    obj["all"] = "*.*";
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

QString WebBridge::getAppVersion()
{
    return "1.0.0";
}

void WebBridge::setLanguage(const QString &language)
{
    m_language = language;
}

QString WebBridge::getLanguage()
{
    return m_language;
}

QString WebBridge::showFileDialog(const QString &title, const QString &filter)
{
    QWidget *parent = qobject_cast<QWidget*>(this->parent());
    return QFileDialog::getOpenFileName(parent, title, "", filter);
}

QString WebBridge::showSaveDialog(const QString &title, const QString &filter, const QString &defaultName)
{
    QWidget *parent = qobject_cast<QWidget*>(this->parent());
    return QFileDialog::getSaveFileName(parent, title, defaultName, filter);
}

void WebBridge::showMessage(const QString &title, const QString &message)
{
    QWidget *parent = qobject_cast<QWidget*>(this->parent());
    QMessageBox::information(parent, title, message);
}

//=============================================================================
// JSON conversion helpers
//=============================================================================

QVariantMap WebBridge::dataframeToVariantMap(const DataFrame &df)
{
    QVariantMap result;

    QStringList columnNames = df.columns();

    QJsonArray rows;
    for (int i = 0; i < df.rowCount(); ++i) {
        QJsonObject row;
        for (const QString &name : columnNames) {
            QVariant value = df.get(i, name);
            if (value.typeId() == QMetaType::Double) {
                row[name] = value.toDouble();
            } else if (value.isNull() || !value.isValid()) {
                row[name] = QJsonValue();
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

    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
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

    // Convert raw data table to JSON
    QJsonArray rawDataArray;
    QStringList rawCols = result.rawData.columns();
    for (int i = 0; i < result.rawData.rowCount(); ++i) {
        QJsonObject row;
        for (const QString &col : rawCols) {
            QVariant value = result.rawData.get(i, col);
            if (value.typeId() == QMetaType::Double) {
                row[col] = value.toDouble();
            } else {
                row[col] = value.toString();
            }
        }
        rawDataArray.append(row);
    }
    obj["rawData"] = rawDataArray;

    // Convert statistics to JSON
    QJsonArray stats;
    for (const StatisticalResult &stat : result.statistics) {
        QJsonObject s;
        s["gene"] = stat.gene;
        s["group1"] = stat.group1;
        s["group2"] = stat.group2;
        s["pValue"] = stat.pValue;
        s["significance"] = stat.significance;
        if (!std::isnan(stat.tStatistic)) s["tStatistic"] = stat.tStatistic;
        if (!std::isnan(stat.fStatistic)) s["fStatistic"] = stat.fStatistic;
        stats.append(s);
    }
    obj["statistics"] = stats;

    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

QString WebBridge::jsonFromStandardCurveResults(const QVector<StandardCurveResult>& results)
{
    QJsonObject obj;
    obj["method"] = "standardCurve";

    QJsonArray tableData;
    for (const StandardCurveResult& result : results) {
        QJsonObject row;
        row["Gene"] = result.gene;
        row["Formula"] = result.formula;
        row["Slope"] = result.slope;
        row["Intercept"] = result.intercept;
        row["R2"] = result.rSquared;
        row["PValue"] = result.pValue;
        row["Efficiency"] = result.efficiency;
        row["MinCq"] = result.minCq;
        row["MaxCq"] = result.maxCq;
        tableData.append(row);
    }
    obj["table"] = tableData;

    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

} // namespace qpcr
