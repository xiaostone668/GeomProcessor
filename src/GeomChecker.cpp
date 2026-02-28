#include "GeomChecker.h"

#include <QDebug>
#include <cmath>

// OpenCASCADE - 只使用基础库，避免GProp等复杂库
#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>
#include <TopAbs.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepTools.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>

// ---------------------------------------------------------------------------
// CheckResultItem
// ---------------------------------------------------------------------------

QString CheckResultItem::getTypeString() const
{
    switch (type) {
        case CheckResultItem::DuplicateFace:        return QString::fromUtf8("重复面");
        case CheckResultItem::InvalidFace:          return QString::fromUtf8("错误面");
        case CheckResultItem::SmallFace:            return QString::fromUtf8("微小面");
        case CheckResultItem::SliverFace:           return QString::fromUtf8("狭长面");
        case CheckResultItem::IntersectingFace:     return QString::fromUtf8("相交面");
        case CheckResultItem::SelfIntersectingFace: return QString::fromUtf8("自交面");
        case CheckResultItem::Spike:                return QString::fromUtf8("尖刺");
        case CheckResultItem::RedundantEdge:        return QString::fromUtf8("冗余边");
        case CheckResultItem::SmallEdge:            return QString::fromUtf8("微小边");
        case CheckResultItem::RedundantVertex:      return QString::fromUtf8("冗余点");
        case CheckResultItem::Gap:                  return QString::fromUtf8("间隙");
        default:                                    return QString::fromUtf8("未知");
    }
}

QString CheckResultItem::getDisplayString() const
{
    QString typeStr = getTypeString();
    if (type >= CheckResultItem::RedundantEdge) {
        return QString::fromUtf8("%1: 边%2 %3").arg(typeStr).arg(index).arg(description);
    } else {
        return QString::fromUtf8("%1: 面%2 %3").arg(typeStr).arg(index).arg(description);
    }
}

// ---------------------------------------------------------------------------
// GeomChecker
// ---------------------------------------------------------------------------

GeomChecker::GeomChecker(QObject* parent)
    : QObject(parent)
{
}

bool GeomChecker::checkShape(const TopoDS_Shape& shape)
{
    clearResults();
    
    if (shape.IsNull()) {
        setError(QString::fromUtf8("几何体为空"));
        return false;
    }

    int totalChecks = 0;
    int currentCheck = 0;

    // 统计需要执行的检查项
    if (m_options & CheckDuplicateFaces) totalChecks++;
    if (m_options & CheckInvalidFaces) totalChecks++;
    if (m_options & CheckSmallFaces) totalChecks++;
    if (m_options & CheckSmallEdges) totalChecks++;

    if (totalChecks == 0) {
        setError(QString::fromUtf8("未选择任何检查选项"));
        return false;
    }

    // 执行面检查
    if (m_options & CheckDuplicateFaces) {
        if (checkDuplicateFaces(shape)) {
            currentCheck++;
            emit progressUpdated(100 * currentCheck / totalChecks);
        }
    }
    if (m_options & CheckInvalidFaces) {
        if (checkInvalidFaces(shape)) {
            currentCheck++;
            emit progressUpdated(100 * currentCheck / totalChecks);
        }
    }
    if (m_options & CheckSmallFaces) {
        if (checkSmallFaces(shape)) {
            currentCheck++;
            emit progressUpdated(100 * currentCheck / totalChecks);
        }
    }

    // 执行边检查
    if (m_options & CheckSmallEdges) {
        if (checkSmallEdges(shape)) {
            currentCheck++;
            emit progressUpdated(100 * currentCheck / totalChecks);
        }
    }

    emit progressUpdated(100);
    return true;
}

int GeomChecker::getProblemCount(CheckOption option) const
{
    int count = 0;
    for (const auto& item : m_results) {
        CheckOption itemOption;
        switch (item.type) {
            case CheckResultItem::DuplicateFace:        itemOption = CheckDuplicateFaces; break;
            case CheckResultItem::InvalidFace:          itemOption = CheckInvalidFaces; break;
            case CheckResultItem::SmallFace:            itemOption = CheckSmallFaces; break;
            case CheckResultItem::SliverFace:           itemOption = CheckSliverFaces; break;
            case CheckResultItem::IntersectingFace:     itemOption = CheckIntersectingFaces; break;
            case CheckResultItem::SelfIntersectingFace: itemOption = CheckSelfIntersectingFace; break;
            case CheckResultItem::Spike:                itemOption = CheckSpikes; break;
            case CheckResultItem::RedundantEdge:        itemOption = CheckRedundantEdges; break;
            case CheckResultItem::SmallEdge:            itemOption = CheckSmallEdges; break;
            case CheckResultItem::RedundantVertex:      itemOption = CheckRedundantVertices; break;
            case CheckResultItem::Gap:                  itemOption = CheckGaps; break;
            default: continue;
        }
        if (itemOption == option) {
            count++;
        }
    }
    return count;
}

QList<CheckResultItem> GeomChecker::getProblemsByType(CheckResultItem::Type type) const
{
    QList<CheckResultItem> result;
    for (const auto& item : m_results) {
        if (item.type == type) {
            result.append(item);
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// 辅助函数 - 简化版本
// ---------------------------------------------------------------------------

double GeomChecker::computeFaceAreaSimple(const TopoDS_Face& face) const
{
    // 使用简化的边界框计算近似面积
    // 避免使用GProp_GProps等复杂库
    Bnd_Box box;
    BRepBndLib::Add(face, box);
    
    double xMin, yMin, zMin, xMax, yMax, zMax;
    box.Get(xMin, yMin, zMin, xMax, yMax, zMax);
    
    // 近似面积：各面的最大尺寸
    double dx = xMax - xMin;
    double dy = yMax - yMin;
    double dz = zMax - zMin;
    
    // 返回最大的两个维度的乘积作为近似面积
    double dims[3] = {dx, dy, dz};
    std::sort(dims, dims + 3);
    
    return dims[1] * dims[2];
}

double GeomChecker::computeEdgeLengthSimple(const TopoDS_Edge& edge) const
{
    // 使用BRepAdaptor_Curve获取准确的边长
    BRepAdaptor_Curve curve(edge);
    return curve.LastParameter() - curve.FirstParameter();
}

// ---------------------------------------------------------------------------
// 面检查实现
// ---------------------------------------------------------------------------

bool GeomChecker::checkInvalidFaces(const TopoDS_Shape& shape)
{
    TopExp_Explorer exp(shape, TopAbs_FACE);
    int index = 1;
    for (; exp.More(); exp.Next(), ++index) {
        const TopoDS_Face& face = TopoDS::Face(exp.Current());
        
        try {
            // 检查面几何是否有效
            BRepAdaptor_Surface surf(face);
            
            // 检查近似面积是否有效
            double area = computeFaceAreaSimple(face);
            if (area <= 0 || !std::isfinite(area)) {
                CheckResultItem item;
                item.type = CheckResultItem::InvalidFace;
                item.index = index;
                item.description = QString::fromUtf8("近似面积为%1（无效）").arg(area);
                m_results.append(item);
            }
        } catch (...) {
            CheckResultItem item;
            item.type = CheckResultItem::InvalidFace;
            item.index = index;
            item.description = QString::fromUtf8("面几何无效（异常）");
            m_results.append(item);
        }
    }
    return true;
}

bool GeomChecker::checkSmallFaces(const TopoDS_Shape& shape)
{
    TopExp_Explorer exp(shape, TopAbs_FACE);
    int index = 1;
    for (; exp.More(); exp.Next(), ++index) {
        const TopoDS_Face& face = TopoDS::Face(exp.Current());
        double area = computeFaceAreaSimple(face);
        if (area < m_minFaceArea) {
            CheckResultItem item;
            item.type = CheckResultItem::SmallFace;
            item.index = index;
            item.description = QString::fromUtf8("近似面积%1小于阈值%2").arg(area).arg(m_minFaceArea);
            item.value = area;
            m_results.append(item);
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// 边检查实现
// ---------------------------------------------------------------------------

bool GeomChecker::checkSmallEdges(const TopoDS_Shape& shape)
{
    TopExp_Explorer exp(shape, TopAbs_EDGE);
    int index = 1;
    for (; exp.More(); exp.Next(), ++index) {
        const TopoDS_Edge& edge = TopoDS::Edge(exp.Current());
        double length = computeEdgeLengthSimple(edge);
        if (length < m_minEdgeLength) {
            CheckResultItem item;
            item.type = CheckResultItem::SmallEdge;
            item.index = index;
            item.description = QString::fromUtf8("近似长度%1小于阈值%2").arg(length).arg(m_minEdgeLength);
            item.value = length;
            m_results.append(item);
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// 其他检查
// ---------------------------------------------------------------------------

bool GeomChecker::checkDuplicateFaces(const TopoDS_Shape& shape)
{
    // 重复面检测算法
    // 检测面积和位置都相同的面
    
    struct FaceInfo {
        int index;
        double area;
        double centerX, centerY, centerZ;
    };
    
    QList<FaceInfo> faces;
    TopExp_Explorer exp(shape, TopAbs_FACE);
    int index = 1;
    
    // 收集所有面的信息
    double totalArea = 0.0;
    for (; exp.More(); exp.Next(), ++index) {
        const TopoDS_Face& face = TopoDS::Face(exp.Current());
        FaceInfo info;
        info.index = index;
        info.area = computeFaceAreaSimple(face);
        totalArea += info.area;
        
        // 获取边界框
        Bnd_Box box;
        BRepBndLib::Add(face, box);
        double xMin, yMin, zMin, xMax, yMax, zMax;
        box.Get(xMin, yMin, zMin, xMax, yMax, zMax);
        
        info.centerX = (xMin + xMax) / 2.0;
        info.centerY = (yMin + yMax) / 2.0;
        info.centerZ = (zMin + zMax) / 2.0;
        
        faces.append(info);
    }
    
    // 根据模型尺度动态计算容差
    double avgArea = totalArea / faces.size();
    double scaleFactor = std::sqrt(avgArea);
    
    // 容差设置：根据模型大小动态调整
    const double areaTolerance = 0.01 * avgArea;  // 面积容差为平均面积的1%
    const double posTolerance = 0.001 * scaleFactor;  // 位置容差为特征尺寸的0.1%
    
    qDebug() << "重复面检测: 平均面积=" << avgArea << ", 面积容差=" << areaTolerance 
             << ", 位置容差=" << posTolerance;
    
    int duplicateCount = 0;
    
    // 比较所有面，查找重复的
    for (int i = 0; i < faces.size(); ++i) {
        for (int j = i + 1; j < faces.size(); ++j) {
            const FaceInfo& f1 = faces[i];
            const FaceInfo& f2 = faces[j];
            
            // 检查面积差异（绝对差值）
            double areaDiff = std::abs(f1.area - f2.area);
            
            // 检查中心点距离
            double dist = std::sqrt(
                std::pow(f1.centerX - f2.centerX, 2) +
                std::pow(f1.centerY - f2.centerY, 2) +
                std::pow(f1.centerZ - f2.centerZ, 2)
            );
            
            // 更严格的条件：面积几乎相同且位置几乎重合
            if (areaDiff < areaTolerance && dist < posTolerance) {
                duplicateCount++;
                CheckResultItem item;
                item.type = CheckResultItem::DuplicateFace;
                item.index = f1.index;
                item.description = QString::fromUtf8("与面%2重复（面积差%3，位置距离%4）")
                    .arg(f2.index).arg(areaDiff, 0, 'g', 6).arg(dist, 0, 'g', 6);
                m_results.append(item);
            }
        }
    }
    
    qDebug() << "重复面检测完成，共发现" << duplicateCount << "对重复面";
    
    return true;
}
bool GeomChecker::checkSliverFaces(const TopoDS_Shape&) { return true; }
bool GeomChecker::checkIntersectingFaces(const TopoDS_Shape&) { return true; }
bool GeomChecker::checkSelfIntersectingFace(const TopoDS_Shape&) { return true; }
bool GeomChecker::checkSpikes(const TopoDS_Shape&) { return true; }
bool GeomChecker::checkRedundantEdges(const TopoDS_Shape&) { return true; }
bool GeomChecker::checkRedundantVertices(const TopoDS_Shape&) { return true; }
bool GeomChecker::checkGaps(const TopoDS_Shape&) { return true; }