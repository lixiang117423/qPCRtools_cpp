#include "Utils/GgplotTheme.h"
#include <QChart>
#include <QValueAxis>
#include <QBarCategoryAxis>
#include <QLegend>
#include <QtMath>

namespace qpcr {

//=============================================================================
// Static color palettes
//=============================================================================

// Nature Publishing Group (NPG) color palette
const QVector<QColor> GgplotTheme::s_npgColors = {
    QColor("#E64B35"),  // Red
    QColor("#4DBBD5"),  // Blue
    QColor("#00A087"),  // Green
    QColor("#3C5488"),  // Dark blue
    QColor("#F39B7F"),  // Orange
    QColor("#8491B4"),  // Light blue
    QColor("#91D1C2"),  // Cyan green
    QColor("#7E6148"),  // Brown
    QColor("#B09C85"),  // Tan
    QColor("#D1E6F3"),  // Light blue
    QColor("#B192C0"),  // Purple
    QColor("#696969")   // Gray
};

// Science (AAAS) color palette
const QVector<QColor> GgplotTheme::s_aaasColors = {
    QColor("#3C5488"),  // Dark blue
    QColor("#F39B7F"),  // Orange
    QColor("#8491B4"),  // Light blue
    QColor("#91D1C2"),  // Cyan green
    QColor("#7E6148"),  // Brown
    QColor("#E64B35"),  // Red
    QColor("#4DBBD5"),  // Blue
    QColor("#00A087"),  // Green
    QColor("#B09C85"),  // Tan
    QColor("#B192C0")   // Purple
};

// Lancet color palette
const QVector<QColor> GgplotTheme::s_lancetColors = {
    QColor("#00468B"),  // Blue
    QColor("#ED0000"),  // Red
    QColor("#42B540"),  // Green
    QColor("#0099B4"),  // Cyan
    QColor("#925E9F"),  // Purple
    QColor("#FF9E00"),  // Orange
    QColor("#B5B500"),  // Yellow
    QColor("#0000FF")   // Pure blue
};

// JCO (Journal of Clinical Oncology) color palette
const QVector<QColor> GgplotTheme::s_jcoColors = {
    QColor("#0073C2"),  // Blue
    QColor("#EFC000"),  // Yellow
    QColor("#868686"),  // Gray
    QColor("#CD534C"),  // Red
    QColor("#7AA6DC"),  // Light blue
    QColor("#003C67"),  // Dark blue
    QColor("#8F7700"),  // Dark yellow
    QColor("#8B6C8C"),  // Purple
    QColor("#D4959D"),  // Pink
    QColor("#B5B500")   // Olive
};

// D3.js categorical color palette
const QVector<QColor> GgplotTheme::s_d3Colors = {
    QColor("#1f77b4"),  // Blue
    QColor("#ff7f0e"),  // Orange
    QColor("#2ca02c"),  // Green
    QColor("#d62728"),  // Red
    QColor("#9467bd"),  // Purple
    QColor("#8c564b"),  // Brown
    QColor("#e377c2"),  // Pink
    QColor("#7f7f7f"),  // Gray
    QColor("#bcbd22"),  // Olive
    QColor("#17becf")   // Cyan
};

// NEJM (New England Journal of Medicine) color palette
const QVector<QColor> GgplotTheme::s_nejmColors = {
    QColor("#BC3C29"),  // Red
    QColor("#0072B5"),  // Blue
    QColor("#E18727"),  // Orange
    QColor("#20854E"),  // Green
    QColor("#7876B1"),  // Purple
    QColor("#6F99AD"),  // Light blue
    QColor("#B55050"),  // Dark red
    QColor("#E2A678")   // Light orange
};

// JAMA color palette
const QVector<QColor> GgplotTheme::s_jamaColors = {
    QColor("#374E55"),  // Dark blue/gray
    QColor("#DF8F44"),  // Orange
    QColor("#00A1D5"),  // Light blue
    QColor("#B24745"),  // Red
    QColor("#79AF97"),  // Green
    QColor("#6A6599"),  // Purple
    QColor("#80796B"),  // Gray
    QColor("#6B6B6B")   // Dark gray
};

//=============================================================================
// Public methods
//=============================================================================

void GgplotTheme::applyToChart(QChart* chart)
{
    if (!chart) return;

    // Background
    chart->setBackgroundRoundness(0);
    chart->setMargins(QMargins(20, 20, 20, 20));
    chart->setBackgroundBrush(QBrush(backgroundColor()));

    // Legend
    if (chart->legend()) {
        chart->legend()->setFont(legendFont());
        chart->legend()->setAlignment(Qt::AlignBottom);
        chart->legend()->setBackgroundVisible(true);
        chart->legend()->setBrush(QBrush(QColor(255, 255, 255, 200)));
        chart->legend()->setPen(QPen(Qt::NoPen));
    }

    // Axes
    auto axes = chart->axes();
    for (auto* axis : axes) {
        if (auto* valueAxis = dynamic_cast<QValueAxis*>(axis)) {
            valueAxis->setGridLinePen(gridPen());
            valueAxis->setLinePen(axisPen());
            valueAxis->setLabelsFont(tickLabelFont());
            valueAxis->setLabelsColor(QColor("#333333"));
            valueAxis->setShadesVisible(false);
            valueAxis->setMinorGridLineVisible(false);
        } else if (auto* categoryAxis = dynamic_cast<QBarCategoryAxis*>(axis)) {
            categoryAxis->setLinePen(axisPen());
            categoryAxis->setLabelsFont(tickLabelFont());
            categoryAxis->setLabelsColor(QColor("#333333"));
            categoryAxis->setGridLineVisible(false);
        }
    }
}

void GgplotTheme::applyClassicTheme(QChart* chart)
{
    if (!chart) return;

    // Classic theme: minimal, no grid lines
    chart->setBackgroundRoundness(0);
    chart->setMargins(QMargins(10, 10, 10, 10));
    chart->setBackgroundBrush(QBrush(QColor(255, 255, 255)));

    if (chart->legend()) {
        chart->legend()->setFont(legendFont());
        chart->legend()->setAlignment(Qt::AlignRight);
        chart->legend()->setBackgroundVisible(false);
    }

    // Remove grid lines
    auto axes = chart->axes();
    for (auto* axis : axes) {
        if (auto* valueAxis = dynamic_cast<QValueAxis*>(axis)) {
            valueAxis->setGridLineVisible(false);
            valueAxis->setLinePen(QPen(Qt::black, 1));
            valueAxis->setLabelsFont(tickLabelFont());
        }
    }
}

void GgplotTheme::applyMinimalTheme(QChart* chart)
{
    if (!chart) return;

    // Minimal theme: light gray grid
    chart->setBackgroundRoundness(0);
    chart->setMargins(QMargins(20, 20, 20, 20));
    chart->setBackgroundBrush(QBrush(QColor(245, 245, 245)));

    if (chart->legend()) {
        chart->legend()->setFont(legendFont());
        chart->legend()->setAlignment(Qt::AlignBottom);
        chart->legend()->setBackgroundVisible(false);
    }

    auto axes = chart->axes();
    for (auto* axis : axes) {
        if (auto* valueAxis = dynamic_cast<QValueAxis*>(axis)) {
            valueAxis->setGridLinePen(QPen(QColor(200, 200, 200), 1, Qt::DashLine));
            valueAxis->setLinePen(QPen(Qt::black, 1));
            valueAxis->setLabelsFont(tickLabelFont());
        }
    }
}

QColor GgplotTheme::paletteColor(ColorPalette palette, int index)
{
    auto colors = palette(palette);
    if (colors.isEmpty()) return QColor();

    // Wrap around if index exceeds palette size
    return colors[index % colors.size()];
}

QVector<QColor> GgplotTheme::palette(ColorPalette palette)
{
    switch (palette) {
    case ColorPalette::NPG:
        return s_npgColors;
    case ColorPalette::AAAS:
        return s_aaasColors;
    case ColorPalette::LANCET:
        return s_lancetColors;
    case ColorPalette::JCO:
        return s_jcoColors;
    case ColorPalette::D3:
        return s_d3Colors;
    case ColorPalette::NEJM:
        return s_nejmColors;
    case ColorPalette::JAMA:
        return s_jamaColors;
    default:
        return s_npgColors;
    }
}

QString GgplotTheme::formatSignificance(const QString& text)
{
    // Format significance stars as HTML
    QString html = QString("<span style='font-size: 14pt; font-weight: bold; color: #000000;'>%1</span>")
                      .arg(text);
    return html;
}

QFont GgplotTheme::defaultFont()
{
    // Helvetica/Arial is ggplot2 default
    QFont font("Helvetica", 11);
    font.setStyleHint(QFont::SansSerif);
    return font;
}

QFont GgplotTheme::axisTitleFont()
{
    QFont font = defaultFont();
    font.setPointSize(12);
    font.setBold(true);
    return font;
}

QFont GgplotTheme::tickLabelFont()
{
    QFont font = defaultFont();
    font.setPointSize(11);
    return font;
}

QFont GgplotTheme::legendFont()
{
    QFont font = defaultFont();
    font.setPointSize(10);
    return font;
}

QPen GgplotTheme::gridPen()
{
    // Light gray dashed line
    return QPen(QColor(220, 220, 220), 1, Qt::DashLine);
}

QPen GgplotTheme::axisPen()
{
    // Black solid line
    return QPen(Qt::black, 1);
}

QColor GgplotTheme::backgroundColor()
{
    // White background
    return QColor(255, 255, 255);
}

QColor GgplotTheme::gridColor()
{
    // Light gray
    return QColor(220, 220, 220);
}

} // namespace qpcr
