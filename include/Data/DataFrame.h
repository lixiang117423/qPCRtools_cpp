#ifndef DATAFRAME_H
#define DATAFRAME_H

#include <QVector>
#include <QString>
#include <QVariant>
#include <QHash>
#include <QSharedPointer>
#include <functional>

namespace qpcr {

/**
 * @brief 数据行表示
 */
class Row {
public:
    Row() = default;
    explicit Row(const QHash<QString, QVariant>& data);

    QVariant value(const QString& columnName) const;
    void setValue(const QString& columnName, const QVariant& value);
    QStringList columns() const;

    bool hasColumn(const QString& columnName) const;

private:
    QHash<QString, QVariant> m_data;
};

/**
 * @brief 数据框类 - 类似 R data.frame
 */
class DataFrame
{
public:
    DataFrame() = default;

    // 工厂方法
    static DataFrame fromCSV(const QString& path, bool hasHeader = true);
    static DataFrame fromExcel(const QString& path, int sheetIndex = 0, bool hasHeader = true);
    static DataFrame fromVariantMap(const QVariantMap& data);

    // 保存
    bool saveCSV(const QString& path) const;
    bool saveExcel(const QString& path) const;

    // 基本属性
    int rowCount() const;
    int columnCount() const;
    QStringList columns() const;
    QSet<QString> columnSet() const;

    // 数据类型
    QVariant::Type columnType(const QString& columnName) const;

    // 列操作
    bool hasColumn(const QString& columnName) const;
    DataFrame selectColumns(const QStringList& colNames) const;
    DataFrame renameColumn(const QString& oldName, const QString& newName) const;

    // 添加列
    void addColumn(const QString& name, const QVector<QVariant>& data);
    void addColumn(const QString& name, const QVariant& defaultValue);

    // 数据访问
    QVariant get(int row, const QString& column) const;
    void set(int row, const QString& column, const QVariant& value);
    Row getRow(int row) const;

    // 向量访问
    QVector<QVariant> getColumn(const QString& columnName) const;
    QVector<double> getNumericColumn(const QString& columnName) const;
    QVector<QString> getStringColumn(const QString& columnName) const;

    // 唯一值
    template<typename T>
    QVector<T> uniqueValues(const QString& column) const;

    // 数据操作
    DataFrame filter(const std::function<bool(const Row&)>& predicate) const;
    DataFrame groupBy(const QString& column) const;
    DataFrame join(const DataFrame& other, const QString& keyColumn) const;
    DataFrame orderBy(const QString& column, bool ascending = true) const;

    // 聚合
    DataFrame aggregate(const QString& groupColumn,
                        const QString& valueColumn,
                        const std::function<double(const QVector<double>&)>& func,
                        const QString& resultName = "result") const;

    // 统计
    QVariant sum(const QString& column) const;
    double mean(const QString& column) const;
    double median(const QString& column) const;
    double sd(const QString& column) const;
    double min(const QString& column) const;
    double max(const QString& column) const;
    int count(const QString& column) const;

    // 转换
    QVariantMap toMap() const;
    QString toString() const;
    QString toHTML() const;

    // 迭代器
    class Iterator {
    public:
        Iterator(const DataFrame* df, int row);
        Row operator*() const;
        Iterator& operator++();
        bool operator!=(const Iterator& other) const;

    private:
        const DataFrame* m_df;
        int m_row;
    };

    Iterator begin() const;
    Iterator end() const;

private:
    QStringList m_columnNames;
    QHash<QString, QVector<QVariant>> m_data;
    int m_rowCount = 0;
};

} // namespace qpcr

#endif // DATAFRAME_H
