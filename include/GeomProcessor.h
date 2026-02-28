#pragma once
#ifndef GEOM_PROCESSOR_H
#define GEOM_PROCESSOR_H

#include <QObject>
#include <QString>
#include <vector>

// OpenCASCADE
#include <TopoDS_Shape.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>

/**
 * @brief 几何处理核心模块
 *
 * 提供以下操作：
 *   - 加载 / 保存 STEP 文件
 *   - 缝合（Sew/Stitch shells → solid）
 *   - 删除特征面（Delete faces + 愈合剩余体）
 *   - 形状修复（ShapeFix）
 *   - 在 AIS 上下文中显示
 */
class GeomProcessor : public QObject
{
    Q_OBJECT
public:
    explicit GeomProcessor(QObject* parent = nullptr);
    ~GeomProcessor() override = default;

    // ---- 加载 / 保存 ----
    bool loadSTEP(const QString& filePath);
    bool saveSTEP(const QString& filePath) const;

    // ---- 几何操作 ----

    /** 缝合：将所有 Shell 缝合为 Solid（BRepBuilderAPI_Sewing） */
    bool stitchShells(double tolerance = 1.0e-3);

    /** 将水密的Shell转换为Solid */
    bool convertShellToSolid();

    /** 删除选中的面（索引从 0 起）并愈合缺口 */
    bool deleteFaces(const std::vector<int>& faceIndices);

    /** 形状修复（ShapeFix_Shape） */
    bool healShape(double precision = 1.0e-4);

    /** 偏移所有面（Shell Offset） */
    bool offsetShape(double offsetVal);

    // ---- 查询 ----
    bool           hasShape() const { return !m_shape.IsNull(); }
    TopoDS_Shape   getShape() const { return m_shape; }
    int            numFaces() const;
    int            numShells() const;
    int            numSolids() const;
    QString        getLastError() const { return m_lastError; }
    
    // ---- 缝合查询 ----
    int            numSewnShells() const { return m_numSewnShells; }

    // ---- 显示 ----
    void displayShape(const Handle(AIS_InteractiveContext)& ctx, bool fitAll = true);
    void clearDisplay(const Handle(AIS_InteractiveContext)& ctx);

signals:
    void shapeChanged();
    void progressUpdated(int pct);
    void errorOccurred(const QString& msg);

private:
    void setError(const QString& e);

    TopoDS_Shape       m_shape;
    Handle(AIS_Shape)  m_aisShape;
    QString            m_lastError;
    int                m_numSewnShells = 0;
};

#endif // GEOM_PROCESSOR_H
