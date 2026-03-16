#pragma once
#ifndef PARASOLID_CHECKER_H
#define PARASOLID_CHECKER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QVariant>

#include "GeomChecker.h"  // 复用相同的检查结果类型

/**
 * @brief Parasolid几何检查器
 * 
 * 使用Parasolid API实现几何检查功能
 * 注意：需要Parasolid SDK和有效许可证
 */
class ParasolidChecker : public QObject
{
    Q_OBJECT
public:
    explicit ParasolidChecker(QObject* parent = nullptr);
    ~ParasolidChecker() override;

    // 检查操作
    bool checkShape(const QVariant& parasolidData);
    
    // 配置
    void setCheckOptions(GeomChecker::CheckOptions options);
    GeomChecker::CheckOptions getCheckOptions() const { return m_options; }
    
    // 结果
    int getProblemCount() const;
    const QList<CheckResultItem>& getResults() const { return m_results; }
    
    // 精度设置
    void setDistancePrecision(double tol) { m_distanceTol = tol; }
    void setAnglePrecision(double tol) { m_angleTol = tol; }
    double getDistancePrecision() const { return m_distanceTol; }
    double getAnglePrecision() const { return m_angleTol; }

signals:
    void progressUpdated(int pct);
    void errorOccurred(const QString& msg);
    void checkFinished(bool success);

private:
    void addResult(CheckResultItem::Type type, int index, const QString& desc);
    
    // 检查方法
    void checkSmallFaces();
    void checkDuplicateFaces();
    void checkIntersectingFaces();
    void checkSmallEdges();
    void checkGaps();
    
    GeomChecker::CheckOptions m_options;
    QList<CheckResultItem> m_results;
    
    double m_distanceTol;  // 距离精度
    double m_angleTol;     // 角度精度
    
    QVariant m_parasolidData;  // Parasolid数据（可以是PK_BODY_t等）
};

#endif // PARASOLID_CHECKER_H