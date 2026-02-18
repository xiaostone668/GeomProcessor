#pragma once
#ifndef GEOM_PROCESSOR_WINDOW_H
#define GEOM_PROCESSOR_WINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QProgressBar>
#include <QListWidget>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QCheckBox>

// OCC
#include <AIS_InteractiveContext.hxx>
#include <V3d_Viewer.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <OpenGl_GraphicDriver.hxx>

// App
#include "OccViewWidget.h"
#include "GeomReceiver.h"
#include "GeomProcessor.h"

/**
 * @brief 应用 B 主窗口
 *
 * 功能：
 *  - 接收 SimulationTool 发来的 STEP 几何
 *  - 展示 3D 视图
 *  - 提供工具栏/操作面板：缝合、删除特征、修复、偏移
 *  - 将结果写回共享内存，通知 SimulationTool
 */
class GeomProcessorWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit GeomProcessorWindow(QWidget* parent = nullptr);
    ~GeomProcessorWindow() override;

protected:
    void resizeEvent(QResizeEvent*) override;

private slots:
    // IPC
    void onGeometryReceived(const QString& stepFile, uint32_t seqNo);

    // Operations
    void onStitchShells();
    void onDeleteSelectedFaces();
    void onHealShape();
    void onOffsetShape();
    void onLoadSTEP();
    void onSaveSTEP();
    void onSendResultBack();

    // Face list selection
    void onFaceListSelectionChanged();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupOperationPanel();
    void setupFaceList();
    void setupStatusBar();
    void initOCC();

    void refreshFaceList();
    void displayCurrentShape();
    void updateStatusInfo();

    // OCC
    Handle(Aspect_DisplayConnection) m_displayConnection;
    Handle(OpenGl_GraphicDriver)     m_graphicDriver;
    Handle(V3d_Viewer)               m_viewer;
    Handle(AIS_InteractiveContext)   m_context;

    OccViewWidget*   m_occWidget    = nullptr;

    // IPC
    GeomReceiver*    m_receiver     = nullptr;
    uint32_t         m_currentSeq   = 0;
    QString          m_lastGeomFile;

    // Processor
    GeomProcessor*   m_processor    = nullptr;

    // Operation panel
    QDockWidget*     m_opDock       = nullptr;
    QDoubleSpinBox*  m_stitchTolSpin= nullptr;
    QDoubleSpinBox*  m_healPrecSpin = nullptr;
    QDoubleSpinBox*  m_offsetSpin   = nullptr;

    // Face list panel
    QDockWidget*     m_faceDock     = nullptr;
    QListWidget*     m_faceList     = nullptr;

    // Status
    QLabel*          m_statusLabel  = nullptr;
    QProgressBar*    m_progressBar  = nullptr;
    QLabel*          m_ipcLabel     = nullptr;
};

#endif // GEOM_PROCESSOR_WINDOW_H
