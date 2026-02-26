#pragma once
#ifndef GEOM_CHECKER_H
#define GEOM_CHECKER_H

#include <QObject>
#include <QList>
#include <QString>

// OpenCASCADE forward declarations
class TopoDS_Shape;
class TopoDS_Face;
class TopoDS_Edge;
class Bnd_Box;

/**
 * @brief 几何检查结果项
 */
struct CheckResultItem
{
    enum Type {
        DuplicateFace = 1,
        InvalidFace,
        SmallFace,
        SliverFace,
        IntersectingFace,
        SelfIntersectingFace,
        Spike,
        RedundantEdge,
        SmallEdge,
        RedundantVertex,
        Gap
    };

    Type    type;
    int     index;
    QString description;
    double  value;  // 面积、长度等

    QString getTypeString() const;
    QString getDisplayString() const;
};

/**
 * @brief 几何检查器
 *
 * 参照SpaceClaim的检查功能，检查几何中的各种问题：
 * - 重复面：存在重复面
 * - 错误面：存在错误面
 * - 微小面：存在微小面
 * - 狭长面：存在狭长面
 * - 微小边：存在微小边
 * - 等等
 */
class GeomChecker : public QObject
{
    Q_OBJECT

public:
    explicit GeomChecker(QObject* parent = nullptr);
    ~GeomChecker() override = default;

    // 主检查函数
    bool checkShape(const TopoDS_Shape& shape);

    // 检查选项
    enum CheckOption {
        CheckNone                  = 0,
        CheckDuplicateFaces        = 1 << 0,
        CheckInvalidFaces          = 1 << 1,
        CheckSmallFaces            = 1 << 2,
        CheckSliverFaces           = 1 << 3,
        CheckIntersectingFaces     = 1 << 4,
        CheckSelfIntersectingFace  = 1 << 5,
        CheckSpikes                = 1 << 6,
        CheckRedundantEdges        = 1 << 7,
        CheckSmallEdges            = 1 << 8,
        CheckRedundantVertices     = 1 << 9,
        CheckGaps                  = 1 << 10,
        CheckAllFaces              = CheckDuplicateFaces | CheckInvalidFaces | CheckSmallFaces | CheckSliverFaces |
                                    CheckIntersectingFaces | CheckSelfIntersectingFace | CheckSpikes,
        CheckAllEdges              = CheckRedundantEdges | CheckSmallEdges | CheckRedundantVertices | CheckGaps,
        CheckAll                   = CheckAllFaces | CheckAllEdges
    };
    Q_DECLARE_FLAGS(CheckOptions, CheckOption)

    void setCheckOptions(CheckOptions options) { m_options = options; }
    CheckOptions getCheckOptions() const { return m_options; }

    // 参数
    void setTolerance(double tol) { m_tolerance = tol; }
    void setMinFaceArea(double area) { m_minFaceArea = area; }
    void setMaxAspectRadio(double ratio) { m_maxAspectRatio = ratio; }
    void setMinEdgeLength(double length) { m_minEdgeLength = length; }

    // 结果
    const QList<CheckResultItem>& getResults() const { return m_results; }
    int getProblemCount() const { return m_results.size(); }
    int getProblemCount(CheckOption option) const;
    QList<CheckResultItem> getProblemsByType(CheckResultItem::Type type) const;

    void clearResults() { m_results.clear(); }

signals:
    void progressUpdated(int percent);
    void errorOccurred(const QString& error);

private:
    // 辅助函数 - 不使用OpenCASCADE复杂库
    double computeFaceAreaSimple(const TopoDS_Face& face) const;
    double computeEdgeLengthSimple(const TopoDS_Edge& edge) const;

    // 检查实现 - 简化版本避免OpenCASCADE复杂的库
    bool checkInvalidFaces(const TopoDS_Shape& shape);
    bool checkSmallFaces(const TopoDS_Shape& shape);
    bool checkSmallEdges(const TopoDS_Shape& shape);

    // 占位函数（将来实现）
    bool checkDuplicateFaces(const TopoDS_Shape& shape);
    bool checkSliverFaces(const TopoDS_Shape& shape);
    bool checkIntersectingFaces(const TopoDS_Shape& shape);
    bool checkSelfIntersectingFace(const TopoDS_Shape& shape);
    bool checkSpikes(const TopoDS_Shape& shape);
    bool checkRedundantEdges(const TopoDS_Shape& shape);
    bool checkRedundantVertices(const TopoDS_Shape& shape);
    bool checkGaps(const TopoDS_Shape& shape);

    void setError(const QString& error) { emit errorOccurred(error); }

private:
    CheckOptions        m_options = CheckAllFaces | CheckSmallEdges;
    double              m_tolerance = 0.01;
    double              m_minFaceArea = 1e-6;
    double              m_maxAspectRatio = 10.0;
    double              m_minEdgeLength = 1e-4;

    QList<CheckResultItem> m_results;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(GeomChecker::CheckOptions)

#endif // GEOM_CHECKER_H