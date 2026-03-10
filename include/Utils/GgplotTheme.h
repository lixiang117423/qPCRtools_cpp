#include <QChart>
#include <QColor>
#include <QFont>
#include <QPen>
#include <QBrush>

namespace qpcr {

/**
 * @brief ggplot2 配色方案
 *
 * 从 ggsci 包移植的常用科学期刊配色
 */
enum class ColorPalette {
    NPG,      // Nature Publishing Group
    AAAS,     // Science (AAAS)
    LANCET,   // Lancet
    JCO,      // Journal of Clinical Oncology
    D3,       // D3.js categorical
    NEJM,     // New England Journal of Medicine
    JAMA      // JAMA
};

/**
 * @brief ggplot2 主题样式类
 *
 * 复现 R ggplot2 包的默认风格
 * 包括背景、网格、字体、配色等
 */
class GgplotTheme
{
public:
    /**
     * @brief 应用 ggplot2 主题到图表
     * @param chart QChart 对象
     */
    static void applyToChart(QChart* chart);

    /**
     * @brief 应用简洁主题 (theme_classic)
     * @param chart QChart 对象
     */
    static void applyClassicTheme(QChart* chart);

    /**
     * @brief 应用最小化主题 (theme_minimal)
     * @param chart QChart 对象
     */
    static void applyMinimalTheme(QChart* chart);

    /**
     * @brief 调色板颜色
     * @param palette 配色方案
     * @param index 颜色索引
     * @return QColor 对象
     */
    static QColor paletteColor(ColorPalette palette, int index);

    /**
     * @brief 获取配色方案的所有颜色
     * @param palette 配色方案
     * @return 颜色列表
     */
    static QVector<QColor> palette(ColorPalette palette);

    /**
     * @brief 创建显著性标注文本
     * @param text 星号标记 ("***", "**", "*", "NS")
     * @return 格式化的 HTML 文本
     */
    static QString formatSignificance(const QString& text);

    /**
     * @brief 获取默认字体
     * @return QFont 对象
     */
    static QFont defaultFont();

    /**
     * @brief 获取坐标轴标题字体
     * @return QFont 对象
     */
    static QFont axisTitleFont();

    /**
     * @brief 获取刻度标签字体
     * @return QFont 对象
     */
    static QFont tickLabelFont();

    /**
     * @brief 获取图例字体
     * @return QFont 对象
     */
    static QFont legendFont();

    /**
     * @brief 创建网格画笔
     * @return QPen 对象 (浅灰色虚线)
     */
    static QPen gridPen();

    /**
     * @brief 创建坐标轴画笔
     * @return QPen 对象 (黑色实线)
     */
    static QPen axisPen();

    /**
     * @brief 获取背景色
     * @return QColor (白色)
     */
    static QColor backgroundColor();

    /**
     * @brief 获取网格色
     * @return QColor (浅灰色)
     */
    static QColor gridColor();

private:
    // NPG 配色 (Nature)
    static const QVector<QColor> s_npgColors;

    // AAAS 配色 (Science)
    static const QVector<QColor> s_aaasColors;

    // Lancet 配色
    static const QVector<QColor> s_lancetColors;

    // JCO 配色
    static const QVector<QColor> s_jcoColors;

    // D3 配色
    static const QVector<QColor> s_d3Colors;

    // NEJM 配色
    static const QVector<QColor> s_nejmColors;

    // JAMA 配色
    static const QVector<QColor> s_jamaColors;
};

} // namespace qpcr
