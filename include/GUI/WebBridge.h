#ifndef WEBBRIDGE_H
#define WEBBRIDGE_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include "../Core/StandardCurve.h"
#include "../Core/ExpressionCalculator.h"
#include "../Data/DataFrame.h"

namespace qpcr {

/**
 * @brief C++和JavaScript之间的通信桥梁
 *
 * 这个类暴露给JavaScript，提供qPCR数据分析的核心功能
 */
class WebBridge : public QObject
{
    Q_OBJECT

public:
    explicit WebBridge(QObject *parent = nullptr);
    ~WebBridge();

    /**
     * @brief 加载Cq数据文件
     * @param filePath 文件路径
     * @return JSON格式的数据预览
     */
    Q_INVOKABLE QString loadCqFile(const QString &filePath);

    /**
     * @brief 从JSON加载Cq数据
     * @param jsonData JSON格式的Cq数据
     * @return 是否成功
     */
    Q_INVOKABLE bool setCqData(const QString &jsonData);

    /**
     * @brief 从CSV字符串加载Cq数据
     * @param csvContent CSV格式的字符串内容
     * @return JSON格式的数据预览
     */
    Q_INVOKABLE QString loadCqFromContent(const QString &csvContent);

    /**
     * @brief 加载实验设计文件
     * @param filePath 文件路径
     * @return JSON格式的设计信息
     */
    Q_INVOKABLE QString loadDesignFile(const QString &filePath);

    /**
     * @brief 从JSON加载实验设计数据
     * @param jsonData JSON格式的设计数据
     * @return 是否成功
     */
    Q_INVOKABLE bool setDesignData(const QString &jsonData);

    /**
     * @brief 从CSV字符串加载Design数据
     * @param csvContent CSV格式的字符串内容
     * @return JSON格式的设计信息
     */
    Q_INVOKABLE QString loadDesignFromContent(const QString &csvContent);

    /**
     * @brief 计算标准曲线
     * @param params JSON格式的参数
     * @return JSON格式的结果
     */
    Q_INVOKABLE QString calculateStandardCurve(const QString &params);

    /**
     * @brief 使用ΔCt方法计算基因表达
     * @param params JSON格式的参数
     * @param statMethod 统计方法
     * @return JSON格式的结果
     */
    Q_INVOKABLE QString calculateByDeltaCt(const QString &params, const QString &statMethod = "t.test");

    /**
     * @brief 使用ΔΔCt方法计算基因表达
     * @param params JSON格式的参数
     * @param statMethod 统计方法
     * @return JSON格式的结果
     */
    Q_INVOKABLE QString calculateByDeltaDeltaCt(const QString &params, const QString &statMethod);

    /**
     * @brief 使用标准曲线方法计算基因表达
     * @param params JSON格式的参数
     * @param statMethod 统计方法
     * @return JSON格式的结果
     */
    Q_INVOKABLE QString calculateByStandardCurve(const QString &params, const QString &statMethod);

    /**
     * @brief 导出结果到CSV
     * @param data JSON格式的数据
     * @param filePath 保存路径
     * @return 是否成功
     */
    Q_INVOKABLE bool exportToCSV(const QString &data, const QString &filePath);

    /**
     * @brief 导出结果到Excel
     * @param data JSON格式的数据
     * @param filePath 保存路径
     * @return 是否成功
     */
    Q_INVOKABLE bool exportToExcel(const QString &data, const QString &filePath);

    /**
     * @brief 获取支持的文件类型
     * @return JSON格式的文件类型列表
     */
    Q_INVOKABLE QString getSupportedFileTypes();

    /**
     * @brief 获取应用版本信息
     * @return 版本字符串
     */
    Q_INVOKABLE QString getAppVersion();

    /**
     * @brief 设置界面语言
     * @param language "zh" 或 "en"
     */
    Q_INVOKABLE void setLanguage(const QString &language);

    /**
     * @brief 获取当前语言
     * @return 语言代码
     */
    Q_INVOKABLE QString getLanguage();

    /**
     * @brief 显示文件选择对话框
     * @param title 对话框标题
     * @param filter 文件过滤器
     * @return 选中的文件路径，如果取消则返回空字符串
     */
    Q_INVOKABLE QString showFileDialog(const QString &title, const QString &filter);

    /**
     * @brief 显示保存文件对话框
     * @param title 对话框标题
     * @param filter 文件过滤器
     * @param defaultName 默认文件名（可选）
     * @return 保存的文件路径，如果取消则返回空字符串
     */
    Q_INVOKABLE QString showSaveDialog(const QString &title, const QString &filter, const QString &defaultName = "");

    /**
     * @brief 显示消息提示
     * @param title 标题
     * @param message 消息内容
     */
    Q_INVOKABLE void showMessage(const QString &title, const QString &message);

signals:
    /**
     * @brief 数据加载完成信号
     * @param success 是否成功
     * @param message 消息
     */
    void dataLoaded(bool success, const QString &message);

    /**
     * @brief 计算完成信号
     * @param success 是否成功
     * @param message 消息
     */
    void calculationCompleted(bool success, const QString &message);

    /**
     * @brief 错误信号
     * @param error 错误信息
     */
    void errorOccurred(const QString &error);

    /**
     * @brief 进度更新信号
     * @param progress 进度值 0-100
     * @param message 进度消息
     */
    void progressChanged(int progress, const QString &message);

private:
    DataFrame m_cqTable;
    DataFrame m_designTable;
    QString m_language;
    StandardCurve m_standardCurve;
    ExpressionCalculator m_expressionCalculator;

    // 辅助方法
    QVariantMap dataframeToVariantMap(const DataFrame &df);
    DataFrame variantMapToDataframe(const QVariantMap &map);
    QString jsonFromResult(const StandardCurveResult &result);
    QString jsonFromResult(const ExpressionResult &result);
};

} // namespace qpcr

#endif // WEBBRIDGE_H
