#ifndef CSVPARSER_H
#define CSVPARSER_H

#include "Data/DataFrame.h"
#include <QString>

namespace qpcr {

/**
 * @brief CSV 文件解析器
 *
 * 支持功能:
 * - 标准 CSV 格式
 * - 自定义分隔符 (逗号、制表符、分号)
 * - 引号包裹的字段
 * - 转义字符处理
 */
class CSVParser
{
public:
    CSVParser();
    explicit CSVParser(const QString& filePath);

    /**
     * @brief 解析 CSV 文件
     * @param path 文件路径
     * @param hasHeader 是否有标题行
     * @return DataFrame 对象
     */
    DataFrame parse(const QString& path, bool hasHeader = true);

    /**
     * @brief 从字符串解析 CSV 内容
     * @param content CSV 格式的字符串内容
     * @param hasHeader 是否有标题行
     * @return DataFrame 对象
     */
    DataFrame parseString(const QString& content, bool hasHeader = true);

    /**
     * @brief 设置分隔符
     * @param separator 分隔符字符
     */
    void setSeparator(QChar separator);

    /**
     * @brief 设置文本限定符
     * @param quote 字符 (通常是 " 或 ')
     */
    void setQuoteChar(QChar quote);

    /**
     * @brief 设置是否跳过空行
     * @param skip true 跳过, false 保留
     */
    void setSkipEmptyLines(bool skip);

    /**
     * @brief 设置编码
     * @param encoding 编码名称
     */
    void setEncoding(const QString& encoding);

    /**
     * @brief 获取解析错误信息
     * @return 错误信息字符串
     */
    QString errorString() const;

    /**
     * @brief 预览文件前几行
     * @param path 文件路径
     * @param lines 行数
     * @return 字符串列表
     */
    static QStringList preview(const QString& path, int lines = 10);

private:
    QString parseLine(const QString& line, QStringList& fields);
    QVariant convertValue(const QString& str, int columnIndex);

    QChar m_separator = ',';
    QChar m_quoteChar = '"';
    bool m_skipEmptyLines = true;
    QString m_encoding = "UTF-8";
    QString m_errorString;
};

} // namespace qpcr

#endif // CSVPARSER_H
