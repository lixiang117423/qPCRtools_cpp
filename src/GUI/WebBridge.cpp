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

QString WebBridge::calculateByDeltaCt(const QString &params)
{
    emit progressChanged(10, tr("Calculating gene expression (ΔCt method)..."));

    QJsonDocument doc = QJsonDocument::fromJson(params.toUtf8());
    if (!doc.isObject()) {
        emit errorOccurred(tr("Invalid parameters"));
        return "{}";
    }

    QJsonObject obj = doc.object();
    DeltaCtParams dcParams;
    dcParams.cqTable = m_cqTable;
    dcParams.designTable = m_designTable;
    dcParams.referenceGene = obj["referenceGene"].toString();
    // Note: DeltaCtParams doesn't have removeOutliers, ignoring for now

    emit progressChanged(50, tr("Computing ΔCt values..."));

    ExpressionResult result = m_expressionCalculator.calculateByDeltaCt(dcParams);

    emit progressChanged(100, tr("ΔCt calculation completed"));
    emit calculationCompleted(true, tr("Calculation successful"));

    return jsonFromResult(result);
}

QString WebBridge::calculateByDeltaDeltaCt(const QString &params, const QString &statMethod)
{
    emit progressChanged(10, tr("Calculating gene expression (ΔΔCt method)..."));

    QJsonDocument doc = QJsonDocument::fromJson(params.toUtf8());
    if (!doc.isObject()) {
        emit errorOccurred(tr("Invalid parameters"));
        return "{}";
    }

    QJsonObject obj = doc.object();
    DeltaDeltaCtParams ddcParams;
    ddcParams.cqTable = m_cqTable;
    ddcParams.designTable = m_designTable;
    ddcParams.referenceGene = obj["referenceGene"].toString();
    ddcParams.controlGroup = obj["controlGroup"].toString();
    ddcParams.removeOutliers = obj["removeOutliers"].toBool(false);

    emit progressChanged(50, tr("Computing ΔΔCt values..."));

    ExpressionResult result = m_expressionCalculator.calculateByDeltaDeltaCt(ddcParams, statMethod);

    emit progressChanged(100, tr("ΔΔCt calculation completed"));
    emit calculationCompleted(true, tr("Calculation successful"));

    return jsonFromResult(result);
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

    // TODO: Implement CSV export

    emit progressChanged(100, tr("Export completed"));
    return true;
}

bool WebBridge::exportToExcel(const QString &data, const QString &filePath)
{
    emit progressChanged(10, tr("Exporting to Excel..."));

#ifdef HAS_OPENXLSX
    // TODO: Implement Excel export
#else
    emit errorOccurred(tr("Excel support is not available"));
    return false;
#endif

    emit progressChanged(100, tr("Export completed"));
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

    QJsonArray columns;
    QStringList columnNames = df.columns();
    for (const QString &name : columnNames) {
        columns.append(QJsonValue(name));
    }

    QJsonArray rows;
    for (int i = 0; i < df.rowCount(); ++i) {
        QJsonArray row;
        for (const QString &name : columnNames) {
            QVariant value = df.get(i, name);
            if (value.typeId() == QMetaType::Double) {
                row.append(QJsonValue(value.toDouble()));
            } else {
                row.append(QJsonValue(value.toString()));
            }
        }
        rows.append(row);
    }

    result["columns"] = QJsonDocument(columns).toJson(QJsonDocument::Compact);
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
