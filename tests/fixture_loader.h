// 黄金参考 fixtures 的加载工具（header-only）。用 Qt JSON 解析。
#pragma once

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVector>

#include <gtest/gtest.h>

#include "test_config.h"  // QPCR_TEST_FIXTURES_DIR（CMake 生成）

#include <algorithm>
#include <cmath>

namespace testfix {

// 读取 fixtures 目录下的某个 JSON 文件，解析失败时直接令用例失败。
inline QJsonDocument load_json(const QString& name) {
    QFile f(QString(QPCR_TEST_FIXTURES_DIR) + "/" + name);
    if (!f.open(QIODevice::ReadOnly)) {
        ADD_FAILURE() << "cannot open fixture: " << name.toStdString()
                      << " (dir=" << QPCR_TEST_FIXTURES_DIR << ")";
        return {};
    }
    QJsonParseError err{};
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError) {
        ADD_FAILURE() << "JSON parse error in " << name.toStdString()
                      << ": " << err.errorString().toStdString();
        return {};
    }
    return doc;
}

inline QVector<double> to_doubles(const QJsonArray& a) {
    QVector<double> v;
    v.reserve(a.size());
    for (const QJsonValue& e : a) v.append(e.toDouble());
    return v;
}

// 容差比较：绝对 1e-6 或相对 1e-4，取较大者。兼顾接近 0 的 p 值与较大的统计量。
inline void expect_close(double actual, double expected) {
    double tol = std::max(1e-6, std::abs(expected) * 1e-4);
    EXPECT_NEAR(actual, expected, tol);
}

}  // namespace testfix
