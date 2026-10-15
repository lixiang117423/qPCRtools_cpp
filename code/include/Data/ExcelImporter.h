#ifndef EXCELIMPORTER_H
#define EXCELIMPORTER_H

#include "Data/DataFrame.h"
#include <QString>
#include <QVector>

namespace qpcr {

/**
 * @brief Excel 工作表信息
 */
struct SheetInfo {
    QString name;
    int rowCount;
    int columnCount;
};

/**
 * @brief Excel 文件导入器
 *
 * 使用 OpenXLSX 库读取 .xlsx 文件
 * 如果未编译 OpenXLSX 支持，该类将不可用
 */
class ExcelImporter
{
public:
    ExcelImporter();
    ~ExcelImporter();

    /**
     * @brief 获取所有工作表信息
     * @param path Excel 文件路径
     * @return 工作表信息列表
     */
    static QVector<SheetInfo> listSheets(const QString& path);

    /**
     * @brief 导入指定工作表
     * @param path Excel 文件路径
     * @param sheetIndex 工作表索引 (从 0 开始)
     * @param hasHeader 是否有标题行
     * @return DataFrame 对象
     */
    static DataFrame importSheet(const QString& path, int sheetIndex = 0, bool hasHeader = true);

    /**
     * @brief 导入指定名称的工作表
     * @param path Excel 文件路径
     * @param sheetName 工作表名称
     * @param hasHeader 是否有标题行
     * @return DataFrame 对象
     */
    static DataFrame importSheetByName(const QString& path, const QString& sheetName, bool hasHeader = true);

    /**
     * @brief 检查是否支持 Excel
     * @return true 如果编译时包含 OpenXLSX
     */
    static bool isExcelSupported();

    /**
     * @brief 获取最后的错误信息
     * @return 错误信息字符串
     */
    static QString lastError();

private:
#ifdef HAS_OPENXLSX
    static QString s_lastError;
#endif
};

} // namespace qpcr

#endif // EXCELIMPORTER_H
