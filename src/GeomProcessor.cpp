#include "GeomProcessor.h"

// OpenCASCADE STEP
#include <STEPControl_Reader.hxx>
#include <STEPControl_Writer.hxx>
#include <IFSelect_ReturnStatus.hxx>

// BRep Sewing
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>

// Delete face
#include <TopExp_Explorer.hxx>
#include <TopAbs.hxx>
#include <TopoDS.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <BRepAlgoAPI_Defeaturing.hxx>

// ShapeFix
#include <ShapeFix_Shape.hxx>
#include <ShapeFix_Wireframe.hxx>

// Offset
#include <BRepOffsetAPI_MakeOffsetShape.hxx>

// Bounding box / properties
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>

// AIS
#include <AIS_Shape.hxx>
#include <Quantity_Color.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>

#include <Standard_Failure.hxx>

GeomProcessor::GeomProcessor(QObject* parent)
    : QObject(parent)
{
}

// ---------------------------------------------------------------------------
// Load / Save
// ---------------------------------------------------------------------------

bool GeomProcessor::loadSTEP(const QString& filePath)
{
    try {
        STEPControl_Reader reader;
        IFSelect_ReturnStatus stat =
            reader.ReadFile(filePath.toStdString().c_str());
        if (stat != IFSelect_RetDone) {
            setError(QString("STEP 读取失败: %1").arg(filePath));
            return false;
        }
        reader.TransferRoots();
        m_shape = reader.OneShape();
        if (m_shape.IsNull()) {
            setError("STEP 文件不包含有效几何体");
            return false;
        }
        emit shapeChanged();
        return true;
    }
    catch (const Standard_Failure& e) {
        setError(QString("OCC 错误: %1").arg(e.GetMessageString()));
        return false;
    }
}

bool GeomProcessor::saveSTEP(const QString& filePath) const
{
    if (m_shape.IsNull()) {
        const_cast<GeomProcessor*>(this)->setError("没有可保存的几何体");
        return false;
    }
    try {
        STEPControl_Writer writer;
        writer.Transfer(m_shape, STEPControl_AsIs);
        IFSelect_ReturnStatus stat =
            writer.Write(filePath.toStdString().c_str());
        if (stat != IFSelect_RetDone) {
            const_cast<GeomProcessor*>(this)->setError(
                QString("STEP 写入失败: %1").arg(filePath));
            return false;
        }
        return true;
    }
    catch (const Standard_Failure& e) {
        const_cast<GeomProcessor*>(this)->setError(
            QString("OCC 错误: %1").arg(e.GetMessageString()));
        return false;
    }
}

// ---------------------------------------------------------------------------
// Stitch Shells
// ---------------------------------------------------------------------------

bool GeomProcessor::stitchShells(double tolerance)
{
    if (m_shape.IsNull()) { setError("无几何体，请先加载 STEP"); return false; }

    try {
        emit progressUpdated(10);
        BRepBuilderAPI_Sewing sewing(tolerance);
        sewing.Add(m_shape);

        emit progressUpdated(40);
        sewing.Perform();
        emit progressUpdated(80);

        TopoDS_Shape sewn = sewing.SewedShape();
        if (sewn.IsNull()) {
            setError("缝合结果为空");
            return false;
        }
        m_shape = sewn;
        emit progressUpdated(100);
        emit shapeChanged();
        return true;
    }
    catch (const Standard_Failure& e) {
        setError(QString("缝合失败: %1").arg(e.GetMessageString()));
        return false;
    }
}

// ---------------------------------------------------------------------------
// Delete Faces
// ---------------------------------------------------------------------------

bool GeomProcessor::deleteFaces(const std::vector<int>& faceIndices)
{
    if (m_shape.IsNull()) { setError("无几何体"); return false; }
    if (faceIndices.empty()) { setError("未选择面"); return false; }

    try {
        // Collect the actual TopoDS_Face objects by index
        TopTools_ListOfShape facesToRemove;
        int idx = 0;
        for (TopExp_Explorer exp(m_shape, TopAbs_FACE); exp.More(); exp.Next(), ++idx) {
            for (int fi : faceIndices) {
                if (fi == idx) {
                    facesToRemove.Append(exp.Current());
                    break;
                }
            }
        }

        if (facesToRemove.IsEmpty()) {
            setError("找不到指定面");
            return false;
        }

        emit progressUpdated(20);

        // Use BRepAlgoAPI_Defeaturing to remove faces
        BRepAlgoAPI_Defeaturing def;
        def.SetShape(m_shape);
        def.AddFacesToRemove(facesToRemove);
        def.SetRunParallel(Standard_False);
        def.Build();

        emit progressUpdated(80);

        if (!def.IsDone()) {
            setError("特征删除失败（BRepAlgoAPI_Defeaturing）");
            return false;
        }

        m_shape = def.Shape();
        emit progressUpdated(100);
        emit shapeChanged();
        return true;
    }
    catch (const Standard_Failure& e) {
        setError(QString("删除特征失败: %1").arg(e.GetMessageString()));
        return false;
    }
}

// ---------------------------------------------------------------------------
// Shape Fix / Heal
// ---------------------------------------------------------------------------

bool GeomProcessor::healShape(double precision)
{
    if (m_shape.IsNull()) { setError("无几何体"); return false; }

    try {
        emit progressUpdated(10);
        Handle(ShapeFix_Shape) fixer = new ShapeFix_Shape(m_shape);
        fixer->SetPrecision(precision);
        fixer->SetMinTolerance(precision * 0.1);
        fixer->SetMaxTolerance(precision * 10.0);
        fixer->Perform();
        emit progressUpdated(80);

        TopoDS_Shape fixed = fixer->Shape();
        if (fixed.IsNull()) {
            setError("修复后形状为空");
            return false;
        }
        m_shape = fixed;
        emit progressUpdated(100);
        emit shapeChanged();
        return true;
    }
    catch (const Standard_Failure& e) {
        setError(QString("形状修复失败: %1").arg(e.GetMessageString()));
        return false;
    }
}

// ---------------------------------------------------------------------------
// Offset
// ---------------------------------------------------------------------------

bool GeomProcessor::offsetShape(double offsetVal)
{
    if (m_shape.IsNull()) { setError("无几何体"); return false; }

    try {
        emit progressUpdated(20);
        BRepOffsetAPI_MakeOffsetShape offsetBuilder;
        offsetBuilder.PerformByJoin(m_shape, offsetVal, 1.0e-3);

        emit progressUpdated(80);

        if (!offsetBuilder.IsDone()) {
            setError("偏移操作失败");
            return false;
        }

        m_shape = offsetBuilder.Shape();
        emit progressUpdated(100);
        emit shapeChanged();
        return true;
    }
    catch (const Standard_Failure& e) {
        setError(QString("偏移失败: %1").arg(e.GetMessageString()));
        return false;
    }
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

int GeomProcessor::numFaces() const
{
    int n = 0;
    for (TopExp_Explorer e(m_shape, TopAbs_FACE); e.More(); e.Next()) ++n;
    return n;
}

int GeomProcessor::numSolids() const
{
    int n = 0;
    for (TopExp_Explorer e(m_shape, TopAbs_SOLID); e.More(); e.Next()) ++n;
    return n;
}

// ---------------------------------------------------------------------------
// Display
// ---------------------------------------------------------------------------

void GeomProcessor::displayShape(const Handle(AIS_InteractiveContext)& ctx, bool fitAll)
{
    if (m_shape.IsNull() || ctx.IsNull()) return;

    if (!m_aisShape.IsNull()) {
        ctx->Remove(m_aisShape, Standard_False);
        m_aisShape.Nullify();
    }

    m_aisShape = new AIS_Shape(m_shape);
    m_aisShape->SetColor(Quantity_NOC_CYAN1);
    m_aisShape->SetDisplayMode(AIS_Shaded);
    ctx->Display(m_aisShape, Standard_False);

    if (fitAll) {
        Handle(V3d_Viewer) viewer = ctx->CurrentViewer();
        if (!viewer.IsNull()) {
            viewer->InitActiveViews();
            if (viewer->MoreActiveViews()) {
                Handle(V3d_View) view = viewer->ActiveView();
                if (!view.IsNull()) {
                    view->FitAll();
                    view->Redraw();
                }
            }
        }
    }
}

void GeomProcessor::clearDisplay(const Handle(AIS_InteractiveContext)& ctx)
{
    if (!m_aisShape.IsNull() && !ctx.IsNull()) {
        ctx->Remove(m_aisShape, Standard_True);
        m_aisShape.Nullify();
    }
}

void GeomProcessor::setError(const QString& e)
{
    m_lastError = e;
    emit errorOccurred(e);
}
