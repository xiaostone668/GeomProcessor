#include "GeomProcessorWindow.h"

#include "ParasolidChecker.h"
#include "ParasolidCheckRepairWidget.h"

#include <QMenuBar>
#include <QToolBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QApplication>
#include <QStatusBar>
#include <QStandardPaths>
#include <QDir>
#include <QUuid>
#include <QFile>
#include <QCoreApplication>
#include <QTreeWidget>
#include <QDebug>
#include <algorithm>
#include <QCheckBox>

// OCC
#include <Aspect_DisplayConnection.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <V3d_View.hxx>
#include <TopExp_Explorer.hxx>
#include <TopExp.hxx>
#include <TopAbs.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <Geom_Curve.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>

GeomProcessorWindow::GeomProcessorWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUI();
    initOCC();

    m_processor = new GeomProcessor(this);
    m_checker   = new GeomChecker(this);
    m_parasolidChecker = new ParasolidChecker(this);
    m_receiver  = new GeomReceiver(this);
    
    // 设置CAD检查Widget的checker（必须在m_checker创建之后）
    if (m_cadCheckWidget) {
        m_cadCheckWidget->setChecker(m_checker);
    }
    
    // 设置Parasolid检查Widget的checker（必须在m_parasolidChecker创建之后）
    if (m_parasolidCheckWidget) {
        m_parasolidCheckWidget->setChecker(m_parasolidChecker);
    }

    // Processor signals
    connect(m_processor, &GeomProcessor::shapeChanged,
            this, &GeomProcessorWindow::displayCurrentShape);
    connect(m_processor, &GeomProcessor::shapeChanged,
            this, &GeomProcessorWindow::refreshFaceList);
    connect(m_processor, &GeomProcessor::shapeChanged,
            this, &GeomProcessorWindow::refreshGeometryTree);
    connect(m_processor, &GeomProcessor::shapeChanged,
            this, &GeomProcessorWindow::updateStatusInfo);
    connect(m_processor, &GeomProcessor::shapeChanged, this, [this](){
        if (m_cadCheckWidget && m_processor) {
            m_cadCheckWidget->setShape(m_processor->getShape());
        }
    });
    connect(m_processor, &GeomProcessor::progressUpdated,
            m_progressBar, &QProgressBar::setValue);
    connect(m_processor, &GeomProcessor::errorOccurred, this, [this](const QString& e){
        m_statusLabel->setText("错误: " + e);
        QMessageBox::warning(this, "几何处理错误", e);
    });

    // Checker signals
    connect(m_checker, &GeomChecker::progressUpdated,
            m_progressBar, &QProgressBar::setValue);
    connect(m_checker, &GeomChecker::errorOccurred, this, [this](const QString& e){
        m_statusLabel->setText("检查错误: " + e);
        QMessageBox::warning(this, "几何检查错误", e);
    });
    
    // Parasolid Checker signals
    connect(m_parasolidChecker, &ParasolidChecker::progressUpdated,
            m_progressBar, &QProgressBar::setValue);
    connect(m_parasolidChecker, &ParasolidChecker::errorOccurred, this, [this](const QString& e){
        m_statusLabel->setText("Parasolid检查错误: " + e);
        QMessageBox::warning(this, "Parasolid检查错误", e);
    });

    // Receiver signals
    connect(m_receiver, &GeomReceiver::geometryReceived,
            this, &GeomProcessorWindow::onGeometryReceived);
    connect(m_receiver, &GeomReceiver::statusChanged, m_ipcLabel, &QLabel::setText);
    connect(m_receiver, &GeomReceiver::errorOccurred, this, [this](const QString& e){
        m_ipcLabel->setText("IPC 错误: " + e);
    });

    m_receiver->start();

    setWindowTitle("GeomProcessor - 几何数据处理器");
    resize(1280, 800);
}

GeomProcessorWindow::~GeomProcessorWindow()
{
    if (m_receiver) m_receiver->stop();
}

void GeomProcessorWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    // 重新定位几何信息标签到左下角
    if (m_geomInfoLabel && m_geomInfoLabel->isVisible()) {
        QWidget* cw = centralWidget();
        if (!cw) return;
        const int margin = 10;
        QPoint pos = cw->mapTo(this,
            QPoint(margin, cw->height() - m_geomInfoLabel->height() - margin));
        m_geomInfoLabel->move(pos);
    }
}

void GeomProcessorWindow::updateGeomInfoLabel(const QString& text)
{
    if (!m_geomInfoLabel) return;
    m_geomInfoLabel->setText(text);
    m_geomInfoLabel->adjustSize();
    QWidget* cw = centralWidget();
    if (!cw) return;
    const int margin = 10;
    QPoint pos = cw->mapTo(this,
        QPoint(margin, cw->height() - m_geomInfoLabel->height() - margin));
    m_geomInfoLabel->move(pos);
    m_geomInfoLabel->raise();
    m_geomInfoLabel->show();
}

// ---------------------------------------------------------------------------
// UI Setup
// ---------------------------------------------------------------------------

void GeomProcessorWindow::setupUI()
{
    setupMenuBar();
    setupToolBar();

    // Central: OCC view (created before initOCC so the widget exists)
    m_occWidget = new OccViewWidget(this);
    setCentralWidget(m_occWidget);

    setupOperationPanel();
    setupCheckPanel();
    setupCadCheckPanel();  // CAD检查修复停靠面板
    setupParasolidCheckPanel();  // 新增：Parasolid检查修复停靠面板
    setupFaceList();
    setupGeometryTree();
    setupStatusBar();

    // 几何信息悬浮标签（浮在 3D 视图左下角，父对象为主窗口）
    m_geomInfoLabel = new QLabel(this);
    m_geomInfoLabel->setStyleSheet(
        "QLabel { background-color: rgba(0,0,0,160); color: #e0e0e0; "
        "padding: 6px 10px; border-radius: 4px; "
        "font-family: Consolas, monospace; font-size: 11px; }");
    m_geomInfoLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_geomInfoLabel->hide();
}

void GeomProcessorWindow::setupMenuBar()
{
    auto* fileMenu = menuBar()->addMenu(tr("文件(&F)"));
    auto* loadAct  = new QAction(tr("打开 STEP 文件..."), this);
    loadAct->setShortcut(QKeySequence::Open);
    connect(loadAct, &QAction::triggered, this, &GeomProcessorWindow::onLoadSTEP);
    fileMenu->addAction(loadAct);

    auto* saveAct = new QAction(tr("保存 STEP 文件..."), this);
    saveAct->setShortcut(QKeySequence::Save);
    connect(saveAct, &QAction::triggered, this, &GeomProcessorWindow::onSaveSTEP);
    fileMenu->addAction(saveAct);

    auto* closeAct = new QAction(tr("关闭文档"), this);
    closeAct->setShortcut(QKeySequence::Close);
    connect(closeAct, &QAction::triggered, this, &GeomProcessorWindow::onCloseDocument);
    fileMenu->addAction(closeAct);

    fileMenu->addSeparator();
    auto* exitAct = new QAction(tr("退出"), this);
    connect(exitAct, &QAction::triggered, qApp, &QApplication::quit);
    fileMenu->addAction(exitAct);

    auto* opMenu = menuBar()->addMenu(tr("操作(&O)"));
    auto* stitchAct = new QAction(tr("缝合 Shell"), this);
    connect(stitchAct, &QAction::triggered, this, &GeomProcessorWindow::onStitchShells);
    opMenu->addAction(stitchAct);

    auto* healAct = new QAction(tr("修复形状"), this);
    connect(healAct, &QAction::triggered, this, &GeomProcessorWindow::onHealShape);
    opMenu->addAction(healAct);

    auto* delAct = new QAction(tr("删除选中面"), this);
    connect(delAct, &QAction::triggered, this, &GeomProcessorWindow::onDeleteSelectedFaces);
    opMenu->addAction(delAct);

    auto* sendAct = new QAction(tr("发送结果到 SimulationTool"), this);
    connect(sendAct, &QAction::triggered, this, &GeomProcessorWindow::onSendResultBack);
    opMenu->addAction(sendAct);
}

void GeomProcessorWindow::setupToolBar()
{
    auto* tb = addToolBar(tr("操作"));

    auto* loadAct   = tb->addAction(tr("📂 打开"));
    connect(loadAct, &QAction::triggered, this, &GeomProcessorWindow::onLoadSTEP);

    auto* saveAct   = tb->addAction(tr("💾 保存"));
    connect(saveAct, &QAction::triggered, this, &GeomProcessorWindow::onSaveSTEP);

    auto* closeAct  = tb->addAction(tr("❌ 关闭文档"));
    connect(closeAct, &QAction::triggered, this, &GeomProcessorWindow::onCloseDocument);

    tb->addSeparator();

    auto* stitchAct = tb->addAction(tr("🔗 缝合"));
    connect(stitchAct, &QAction::triggered, this, &GeomProcessorWindow::onStitchShells);

    auto* healAct   = tb->addAction(tr("🔧 修复"));
    connect(healAct, &QAction::triggered, this, &GeomProcessorWindow::onHealShape);

    auto* delAct    = tb->addAction(tr("✂ 删除面"));
    connect(delAct, &QAction::triggered, this, &GeomProcessorWindow::onDeleteSelectedFaces);

    auto* offsetAct = tb->addAction(tr("📐 偏移"));
    connect(offsetAct, &QAction::triggered, this, &GeomProcessorWindow::onOffsetShape);

    tb->addSeparator();

    auto* checkAct  = tb->addAction(tr("🔍 检查OCC版"));
    connect(checkAct, &QAction::triggered, this, &GeomProcessorWindow::onRunGeometryCheck);

    auto* cadCheckAct = tb->addAction(tr("🔧 CAD检查修复"));
    connect(cadCheckAct, &QAction::triggered, this, [this](){
        // 切换到OCC检查器并显示停靠面板
        if (m_cadCheckWidget) {
            m_cadCheckWidget->setChecker(m_checker);
            m_cadCheckDock->setVisible(true);
            m_cadCheckDock->raise();
        }
    });

    auto* paraCheckAct = tb->addAction(tr("🔍 Parasolid检查"));
    connect(paraCheckAct, &QAction::triggered, this, [this](){
        // 显示Parasolid检查停靠面板
        if (!m_parasolidCheckDock) {
            QMessageBox::warning(this, "错误", QString::fromUtf8("Parasolid检查面板未初始化"));
            return;
        }
        
        if (!m_parasolidCheckWidget) {
            QMessageBox::warning(this, "错误", QString::fromUtf8("Parasolid检查Widget未初始化"));
            return;
        }
        
        m_parasolidCheckDock->setVisible(true);
        m_parasolidCheckDock->raise();
        
        // 验证面板是否显示
        m_statusLabel->setText(QString::fromUtf8("Parasolid检查面板已显示"));
    });

    tb->addSeparator();

    auto* sendAct   = tb->addAction(tr("📤 发回"));
    connect(sendAct, &QAction::triggered, this, &GeomProcessorWindow::onSendResultBack);
}

void GeomProcessorWindow::setupOperationPanel()
{
    m_opDock = new QDockWidget(tr("操作参数"), this);

    auto* w   = new QWidget(m_opDock);
    auto* lay = new QVBoxLayout(w);

    // Stitch
    auto* stitchBox = new QGroupBox(tr("缝合 (Stitch)"));
    auto* sf = new QFormLayout(stitchBox);
    m_stitchTolSpin = new QDoubleSpinBox();
    m_stitchTolSpin->setRange(1e-6, 10.0);
    m_stitchTolSpin->setValue(0.1);
    m_stitchTolSpin->setDecimals(6);
    sf->addRow(tr("容差:"), m_stitchTolSpin);
    m_convertToSolidCheck = new QCheckBox(tr("水密时转换为实体"));
    m_convertToSolidCheck->setChecked(false);
    sf->addRow(m_convertToSolidCheck);
    auto* stitchBtn = new QPushButton(tr("执行缝合"));
    connect(stitchBtn, &QPushButton::clicked, this, &GeomProcessorWindow::onStitchShells);
    sf->addRow(stitchBtn);
    lay->addWidget(stitchBox);

    // Heal
    auto* healBox = new QGroupBox(tr("形状修复 (Heal)"));
    auto* hf = new QFormLayout(healBox);
    m_healPrecSpin = new QDoubleSpinBox();
    m_healPrecSpin->setRange(1e-7, 1.0);
    m_healPrecSpin->setValue(1e-4);
    m_healPrecSpin->setDecimals(7);
    hf->addRow(tr("精度:"), m_healPrecSpin);
    auto* healBtn = new QPushButton(tr("执行修复"));
    connect(healBtn, &QPushButton::clicked, this, &GeomProcessorWindow::onHealShape);
    hf->addRow(healBtn);
    lay->addWidget(healBox);

    // Offset
    auto* offsetBox = new QGroupBox(tr("偏移 (Offset)"));
    auto* of = new QFormLayout(offsetBox);
    m_offsetSpin = new QDoubleSpinBox();
    m_offsetSpin->setRange(-100.0, 100.0);
    m_offsetSpin->setValue(1.0);
    m_offsetSpin->setDecimals(3);
    of->addRow(tr("偏移量:"), m_offsetSpin);
    auto* offsetBtn = new QPushButton(tr("执行偏移"));
    connect(offsetBtn, &QPushButton::clicked, this, &GeomProcessorWindow::onOffsetShape);
    of->addRow(offsetBtn);
    lay->addWidget(offsetBox);

    // Send back
    auto* sendBtn = new QPushButton(tr("📤 发送结果到 SimulationTool"));
    sendBtn->setStyleSheet("background-color:#2196F3;color:white;font-weight:bold;");
    connect(sendBtn, &QPushButton::clicked, this, &GeomProcessorWindow::onSendResultBack);
    lay->addWidget(sendBtn);

    lay->addStretch();
    m_opDock->setWidget(w);
    addDockWidget(Qt::RightDockWidgetArea, m_opDock);
}

void GeomProcessorWindow::setupCheckPanel()
{
    m_checkDock = new QDockWidget(tr("几何检查OCC版"), this);

    auto* w   = new QWidget(m_checkDock);
    auto* lay = new QVBoxLayout(w);

    // 检查项选择
    auto* faceGroup = new QGroupBox(tr("面类型"));
    faceGroup->setCheckable(true);
    faceGroup->setChecked(true);
    auto* faceLay = new QVBoxLayout(faceGroup);

    QCheckBox* cbDupFace    = new QCheckBox(tr("重复面：存在重复面"));
    QCheckBox* cbInvFace    = new QCheckBox(tr("错误面：存在错误面"));
    QCheckBox* cbSmallFace  = new QCheckBox(tr("微小面：存在微小面"));
    cbDupFace->setChecked(true);
    cbInvFace->setChecked(true);
    cbSmallFace->setChecked(true);
    cbDupFace->setProperty("checkOption", (int)GeomChecker::CheckDuplicateFaces);
    cbInvFace->setProperty("checkOption", (int)GeomChecker::CheckInvalidFaces);
    cbSmallFace->setProperty("checkOption", (int)GeomChecker::CheckSmallFaces);
    
    // 连接复选框状态改变信号
    connect(cbDupFace, &QCheckBox::stateChanged, this, &GeomProcessorWindow::onCheckOptionChanged);
    connect(cbInvFace, &QCheckBox::stateChanged, this, &GeomProcessorWindow::onCheckOptionChanged);
    connect(cbSmallFace, &QCheckBox::stateChanged, this, &GeomProcessorWindow::onCheckOptionChanged);

    faceLay->addWidget(cbDupFace);
    faceLay->addWidget(cbInvFace);
    faceLay->addWidget(cbSmallFace);
    lay->addWidget(faceGroup);

    // 检查参数
    auto* paramBox = new QGroupBox(tr("检查参数"));
    auto* paramLay = new QFormLayout(paramBox);
    QDoubleSpinBox* tolSpin = new QDoubleSpinBox();
    tolSpin->setRange(1e-6, 1.0);
    tolSpin->setValue(0.01);
    tolSpin->setDecimals(6);
    paramLay->addRow(tr("公差:"), tolSpin);
    lay->addWidget(paramBox);

    // 执行检查按钮
    auto* checkBtn = new QPushButton(tr("🔍 执行检查OCC版"));
    checkBtn->setStyleSheet("background-color:#4CAF50;color:white;font-weight:bold;padding:8px;");
    connect(checkBtn, &QPushButton::clicked, this, &GeomProcessorWindow::onRunGeometryCheck);
    lay->addWidget(checkBtn);

    lay->addSpacing(10);
    auto* resLabel = new QLabel(tr("检查结果:"));
    resLabel->setStyleSheet("font-weight:bold;");
    lay->addWidget(resLabel);

    m_checkResultTree = new QTreeWidget();
    m_checkResultTree->setHeaderLabels(QStringList() << tr("类型") << tr("索引") << tr("描述"));
    m_checkResultTree->setColumnWidth(0, 100);
    m_checkResultTree->setColumnWidth(1, 60);
    m_checkResultTree->setAlternatingRowColors(true);
    connect(m_checkResultTree, &QTreeWidget::itemDoubleClicked,
            this, &GeomProcessorWindow::onCheckResultItemDoubleClicked);
    lay->addWidget(m_checkResultTree);

    lay->addStretch();
    m_checkDock->setWidget(w);
    addDockWidget(Qt::RightDockWidgetArea, m_checkDock);
}

void GeomProcessorWindow::updateCheckResults()
{
    m_checkResultTree->clear();
    
    const QList<CheckResultItem>& results = m_checker->getResults();
    for (const auto& result : results) {
        auto* item = new QTreeWidgetItem(m_checkResultTree);
        item->setText(0, result.getTypeString());
        item->setText(1, QString::number(result.index));
        item->setText(2, result.getDisplayString());
        
        switch (result.type) {
            case CheckResultItem::InvalidFace:
            case CheckResultItem::SelfIntersectingFace:
                item->setForeground(0, QBrush(Qt::red));
                item->setForeground(2, QBrush(Qt::red));
                break;
            case CheckResultItem::SmallFace:
            case CheckResultItem::SmallEdge:
                item->setForeground(0, QBrush(QColor(255, 165, 0))); 
                break;
            default:
                item->setForeground(0, QBrush(Qt::darkBlue));
                break;
        }
    }
    
    m_checkResultTree->resizeColumnToContents(0);
    m_checkResultTree->resizeColumnToContents(1);
}

void GeomProcessorWindow::onRunGeometryCheck()
{
    if (!m_processor->hasShape()) {
        QMessageBox::information(this, "提示", "请先加载 STEP 文件或等待 SimulationTool 发送几何");
        return;
    }

    m_statusLabel->setText("正在检查几何...");
    m_progressBar->setValue(0);

    m_checkResultTree->clear();

    if (m_checker->checkShape(m_processor->getShape())) {
        updateCheckResults();
        
        int numProblems = m_checker->getProblemCount();
        if (numProblems == 0) {
            m_statusLabel->setText("检查完成，未发现错误");
            // 在结果树中添加一条提示信息
            auto* item = new QTreeWidgetItem(m_checkResultTree);
            item->setText(0, QString::fromUtf8("✓ 通过"));
            item->setText(1, "");
            item->setText(2, QString::fromUtf8("检查完成，未发现错误"));
            item->setForeground(0, QBrush(Qt::darkGreen));
            item->setForeground(2, QBrush(Qt::darkGreen));
        } else {
            m_statusLabel->setText(QString("检查完成，发现 %1 个问题").arg(numProblems));
        }
    }
}

void GeomProcessorWindow::onCheckOptionChanged()
{
    GeomChecker::CheckOptions options = GeomChecker::CheckNone;
    
    QList<QCheckBox*> allChecks = m_checkDock->widget()->findChildren<QCheckBox*>();
    for (auto* cb : allChecks) {
        if (cb->isChecked()) {
            GeomChecker::CheckOption opt = 
                static_cast<GeomChecker::CheckOption>(cb->property("checkOption").toInt());
            options |= opt;
        }
    }
    
    m_checker->setCheckOptions(options);
}

void GeomProcessorWindow::onCheckResultItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    if (!item) return;
    m_statusLabel->setText(tr("选中问题: %1").arg(item->text(2)));
}

void GeomProcessorWindow::setupFaceList()
{
    m_faceDock = new QDockWidget(tr("面列表 (选中后可删除)"), this);
    m_faceList = new QListWidget(m_faceDock);
    m_faceList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(m_faceList, &QListWidget::itemSelectionChanged,
            this, &GeomProcessorWindow::onFaceListSelectionChanged);
    m_faceDock->setWidget(m_faceList);
    addDockWidget(Qt::LeftDockWidgetArea, m_faceDock);
}

void GeomProcessorWindow::setupGeometryTree()
{
    m_geomTreeDock = new QDockWidget(tr("几何体浏览树"), this);
    m_geomTree = new QTreeWidget(m_geomTreeDock);
    m_geomTree->setHeaderLabels(QStringList() << tr("类型") << tr("索引") << tr("信息"));
    m_geomTree->setColumnWidth(0, 100);
    m_geomTree->setColumnWidth(1, 60);
    m_geomTree->setAlternatingRowColors(true);
    m_geomTree->setRootIsDecorated(true);
    m_geomTreeDock->setWidget(m_geomTree);
    addDockWidget(Qt::LeftDockWidgetArea, m_geomTreeDock);
}

void GeomProcessorWindow::setupStatusBar()
{
    m_statusLabel = new QLabel(tr("就绪"));
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setMinimumWidth(150);

    m_ipcLabel = new QLabel(tr("IPC: 未连接"));
    m_ipcLabel->setMinimumWidth(300);

    statusBar()->addWidget(m_statusLabel, 1);
    statusBar()->addWidget(m_progressBar);
    statusBar()->addPermanentWidget(m_ipcLabel);
}

// ---------------------------------------------------------------------------
// OCC
// ---------------------------------------------------------------------------

void GeomProcessorWindow::initOCC()
{
    try {
        m_displayConnection = new Aspect_DisplayConnection();
        m_graphicDriver     = new OpenGl_GraphicDriver(m_displayConnection);

        m_viewer = new V3d_Viewer(m_graphicDriver);
        m_viewer->SetDefaultLights();
        m_viewer->SetLightOn();

        m_context = new AIS_InteractiveContext(m_viewer);

        if (m_occWidget) {
            m_occWidget->init(m_viewer, m_context);
        }
    }
    catch (const Standard_Failure& e) {
        QMessageBox::critical(this, tr("OCC 初始化失败"),
            QString::fromLatin1(e.GetMessageString()));
    }
}

// ---------------------------------------------------------------------------
// IPC Slot
// ---------------------------------------------------------------------------

void GeomProcessorWindow::onGeometryReceived(const QString& stepFile, uint32_t seqNo)
{
    m_currentSeq   = seqNo;
    m_lastGeomFile = stepFile;

    m_statusLabel->setText(QString("正在加载几何 #%1 ...").arg(seqNo));
    m_progressBar->setValue(0);

    if (m_processor->loadSTEP(stepFile)) {
        m_statusLabel->setText(
            QString("已加载几何 #%1 | %2 个面 | %3 个实体")
            .arg(seqNo).arg(m_processor->numFaces()).arg(m_processor->numSolids()));
    } else {
        m_statusLabel->setText("加载失败: " + m_processor->getLastError());
        m_receiver->sendError(m_processor->getLastError());
    }
}

// ---------------------------------------------------------------------------
// Operation Slots
// ---------------------------------------------------------------------------

void GeomProcessorWindow::onStitchShells()
{
    if (!m_processor->hasShape()) {
        QMessageBox::information(this, "提示", "请先加载 STEP 文件或等待 SimulationTool 发送几何");
        return;
    }
    m_statusLabel->setText("正在缝合...");
    m_progressBar->setValue(0);
    if (m_processor->stitchShells(m_stitchTolSpin->value())) {
        int sewn = m_processor->numSewnShells();
        int remaining = m_processor->numShells();
        QString resultText;
        
        if (sewn > 0) {
            if (remaining == 1) {
                resultText = QString("缝合完成 | %1 个Shell已合并为1个Shell | %2 个面 | %3 个实体")
                    .arg(sewn).arg(m_processor->numFaces()).arg(m_processor->numSolids());
            } else {
                resultText = QString("缝合完成 | %1 个Shell已合并为%2个Shell | %3 个面 | %4 个实体")
                    .arg(sewn).arg(remaining).arg(m_processor->numFaces()).arg(m_processor->numSolids());
            }
        } else {
            resultText = QString("缝合完成 | 无Shell被合并 | %1 个Shell | %2 个面 | %3 个实体")
                .arg(remaining).arg(m_processor->numFaces()).arg(m_processor->numSolids());
        }
        
        m_statusLabel->setText(resultText);
        
        // 如果勾选了"水密时转换为实体"，尝试转换
        bool converted = false;
        if (m_convertToSolidCheck->isChecked()) {
            if (m_processor->convertShellToSolid()) {
                converted = true;
            }
        }
        
        // 自动保存并发送结果回SimulationTool
        if (autoSaveAndSend()) {
            // 在shapeChanged更新信息后，添加"已发送"信息
            QString currentText = m_statusLabel->text();
            if (converted) {
                m_statusLabel->setText(QString("缝合完成并转换为实体 | 已发送 | %1 个面 | %2 个Shell | %3 个实体")
                    .arg(m_processor->numFaces()).arg(0).arg(m_processor->numSolids()));
            } else {
                m_statusLabel->setText(currentText + " | 已自动保存并发送");
            }
        }
    }
}

void GeomProcessorWindow::onDeleteSelectedFaces()
{
    if (!m_processor->hasShape()) return;

    std::vector<int> selected;
    for (auto* item : m_faceList->selectedItems()) {
        selected.push_back(item->data(Qt::UserRole).toInt());
    }
    if (selected.empty()) {
        QMessageBox::information(this, "提示", "请在左侧面列表中选择要删除的面");
        return;
    }

    m_statusLabel->setText(QString("正在删除 %1 个面...").arg(selected.size()));
    m_progressBar->setValue(0);
    if (m_processor->deleteFaces(selected)) {
        m_statusLabel->setText(QString("删除完成 | 剩余 %1 个面").arg(m_processor->numFaces()));
    }
}

void GeomProcessorWindow::onHealShape()
{
    if (!m_processor->hasShape()) return;
    m_statusLabel->setText("正在修复形状...");
    m_progressBar->setValue(0);
    if (m_processor->healShape(m_healPrecSpin->value())) {
        m_statusLabel->setText("修复完成");
    }
}

void GeomProcessorWindow::onOffsetShape()
{
    if (!m_processor->hasShape()) return;
    m_statusLabel->setText("正在偏移...");
    m_progressBar->setValue(0);
    if (m_processor->offsetShape(m_offsetSpin->value())) {
        m_statusLabel->setText("偏移完成");
    }
}

void GeomProcessorWindow::onLoadSTEP()
{
    QString path = QFileDialog::getOpenFileName(this,
        tr("打开 STEP 文件"), QString(),
        tr("STEP Files (*.step *.stp);;All Files (*.*)"));
    if (path.isEmpty()) return;
    m_statusLabel->setText("正在加载...");
    if (!m_processor->loadSTEP(path)) {
        QMessageBox::warning(this, "加载失败", m_processor->getLastError());
    }
}

void GeomProcessorWindow::onSaveSTEP()
{
    if (!m_processor->hasShape()) {
        QMessageBox::information(this, "提示", "没有几何体可保存");
        return;
    }
    QString path = QFileDialog::getSaveFileName(this,
        tr("保存 STEP 文件"), QString(),
        tr("STEP Files (*.step *.stp);;All Files (*.*)"));
    if (path.isEmpty()) return;
    if (m_processor->saveSTEP(path)) {
        m_statusLabel->setText("已保存: " + path);
    } else {
        QMessageBox::warning(this, "保存失败", m_processor->getLastError());
    }
}

void GeomProcessorWindow::onCloseDocument()
{
    if (!m_processor->hasShape()) {
        m_statusLabel->setText("没有打开的文档");
        return;
    }

    // 清除当前几何体
    if (m_processor) {
        m_processor->clearShape();
    }

    // 清除OCC显示
    if (m_context) {
        m_context->RemoveAll(true);
    }

    // 清除界面信息
    m_faceList->clear();
    m_geomTree->clear();
    
    // 清除检查结果
    m_checkResultTree->clear();
    if (m_cadCheckWidget) {
        m_cadCheckWidget->setShape(TopoDS_Shape());
    }

    // 清除浮动信息标签
    if (m_geomInfoLabel) {
        m_geomInfoLabel->hide();
    }

    m_statusLabel->setText("文档已关闭");
    m_progressBar->setValue(0);
    
    // 重置序列号和文件路径
    m_currentSeq = 0;
    m_lastGeomFile.clear();
}

void GeomProcessorWindow::onSendResultBack()
{
    if (!m_processor->hasShape()) {
        QMessageBox::information(this, "提示", "没有几何体可发送");
        return;
    }

    // 将结果保存到 GeomProcessor\example\returned\<GUID>.stp
    // 可执行文件在 build/Release，向上两级即为 GeomProcessor 根目录
    QString appDir = QCoreApplication::applicationDirPath();
    QString returnedDir = QDir::cleanPath(appDir + "/../../example/returned");
    if (!QDir(returnedDir).exists()) {
        returnedDir = QDir::cleanPath(appDir + "/../example/returned");
    }
    if (!QDir(returnedDir).exists()) {
        returnedDir = QDir::cleanPath(appDir + "/example/returned");
    }
    // 确保目录存在
    QDir().mkpath(returnedDir);

    // Qt 5.6 doesn't have QUuid::WithoutBraces, so remove braces manually
    QString guid    = QUuid::createUuid().toString();
    guid.remove('{').remove('}');
    QString tmpFile = QDir(returnedDir).filePath(guid + ".stp");

    if (!m_processor->saveSTEP(tmpFile)) {
        // 回退到系统临时目录
        QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        tmpFile = QDir(tmpDir).filePath("geomproc_result_" + guid + ".stp");
        if (!m_processor->saveSTEP(tmpFile)) {
            QMessageBox::warning(this, "错误", "保存临时 STEP 文件失败: " + m_processor->getLastError());
            return;
        }
    }

    if (m_receiver->sendResult(tmpFile)) {
        m_statusLabel->setText("结果已发送至 SimulationTool: " + tmpFile);
    } else {
        QMessageBox::warning(this, "IPC 错误", "发送失败，SimulationTool 可能未运行");
    }
}

void GeomProcessorWindow::onFaceListSelectionChanged()
{
    int n = m_faceList->selectedItems().size();
    if (n > 0) {
        m_statusLabel->setText(QString("已选择 %1 个面（点击工具栏[删除面]执行）").arg(n));
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void GeomProcessorWindow::refreshFaceList()
{
    m_faceList->clear();
    if (!m_processor->hasShape()) return;

    int idx = 1;
    TopExp_Explorer exp(m_processor->getShape(), TopAbs_FACE);
    for (; exp.More(); exp.Next(), ++idx) {
        auto* item = new QListWidgetItem(QString("面 %1").arg(idx));
        item->setData(Qt::UserRole, idx);
        m_faceList->addItem(item);
    }
}

void GeomProcessorWindow::refreshGeometryTree()
{
    m_geomTree->clear();
    if (!m_processor->hasShape()) return;

    // 统计总数
    int totalSolids = 0;
    int totalShells = 0;
    int totalFaces = 0;
    int totalEdges = 0;
    
    TopExp_Explorer solidExp(m_processor->getShape(), TopAbs_SOLID);
    for (; solidExp.More(); solidExp.Next()) totalSolids++;
    
    TopExp_Explorer shellExp(m_processor->getShape(), TopAbs_SHELL);
    for (; shellExp.More(); shellExp.Next()) totalShells++;
    
    TopExp_Explorer faceExp(m_processor->getShape(), TopAbs_FACE);
    for (; faceExp.More(); faceExp.Next()) totalFaces++;
    
    TopExp_Explorer edgeExp(m_processor->getShape(), TopAbs_EDGE);
    for (; edgeExp.More(); edgeExp.Next()) totalEdges++;
    
    // 添加根节点
    auto* root = new QTreeWidgetItem(m_geomTree);
    root->setText(0, QString::fromUtf8("几何体"));
    root->setText(1, "");
    root->setText(2, QString::fromUtf8("%1 个实体, %2 个Shell, %3 个面, %4 条边")
        .arg(totalSolids).arg(totalShells).arg(totalFaces).arg(totalEdges));
    root->setForeground(0, QBrush(Qt::darkBlue));
    
    // 优先显示Solids（如果存在）
    if (totalSolids > 0) {
        int solidIdx = 1;
        for (TopExp_Explorer solidIter(m_processor->getShape(), TopAbs_SOLID); 
             solidIter.More(); solidIter.Next(), solidIdx++) {
            TopoDS_Solid solid = TopoDS::Solid(solidIter.Current());
            
            auto* solidItem = new QTreeWidgetItem(root);
            solidItem->setText(0, QString::fromUtf8("实体 %1").arg(solidIdx));
            solidItem->setText(1, QString::number(solidIdx));
            
            // 统计该实体的面数
            int solidFaceCount = 0;
            int solidEdgeCount = 0;
            for (TopExp_Explorer exp(solid, TopAbs_FACE); exp.More(); exp.Next()) solidFaceCount++;
            for (TopExp_Explorer exp(solid, TopAbs_EDGE); exp.More(); exp.Next()) solidEdgeCount++;
            solidItem->setText(2, QString::fromUtf8("%1 个面, %2 条边").arg(solidFaceCount).arg(solidEdgeCount));
            solidItem->setForeground(0, QBrush(QColor(128, 0, 128))); // 紫色
            
            // 遍历该实体的所有Faces
            int faceIdx = 1;
            for (TopExp_Explorer faceIter(solid, TopAbs_FACE); 
                 faceIter.More(); faceIter.Next(), faceIdx++) {
                TopoDS_Face face = TopoDS::Face(faceIter.Current());
                
                auto* faceItem = new QTreeWidgetItem(solidItem);
                faceItem->setText(0, QString::fromUtf8("面 %1").arg(faceIdx));
                faceItem->setText(1, QString::number(faceIdx));
                
                // 统计该面的边数
                int faceEdgeCount = 0;
                for (TopExp_Explorer exp(face, TopAbs_EDGE); exp.More(); exp.Next()) faceEdgeCount++;
                faceItem->setText(2, QString::fromUtf8("%1 条边").arg(faceEdgeCount));
                faceItem->setForeground(0, QBrush(QColor(0, 100, 200))); // 蓝色
                
                // 计算面的面积
                double faceArea = 0.0;
                try {
                    Bnd_Box box;
                    BRepBndLib::Add(face, box);
                    double xMin, yMin, zMin, xMax, yMax, zMax;
                    box.Get(xMin, yMin, zMin, xMax, yMax, zMax);
                    double dx = xMax - xMin;
                    double dy = yMax - yMin;
                    double dz = zMax - zMin;
                    double dims[3] = {dx, dy, dz};
                    std::sort(dims, dims + 3);
                    faceArea = dims[1] * dims[2];
                } catch (...) {
                    faceArea = 0.0;
                }
                
                // 在面节点下添加面积信息作为子节点
                auto* areaItem = new QTreeWidgetItem(faceItem);
                areaItem->setText(0, QString::fromUtf8("  面积"));
                areaItem->setText(1, "");
                areaItem->setText(2, QString("%1").arg(faceArea, 0, 'g', 6));
                areaItem->setForeground(0, QBrush(Qt::gray));
                
                // 遍历该面的所有Edges
                int edgeIdx = 1;
                for (TopExp_Explorer edgeIter(face, TopAbs_EDGE); 
                     edgeIter.More(); edgeIter.Next(), edgeIdx++) {
                    TopoDS_Edge edge = TopoDS::Edge(edgeIter.Current());
                    
                    auto* edgeItem = new QTreeWidgetItem(faceItem);
                    edgeItem->setText(0, QString::fromUtf8("  边 %1").arg(edgeIdx));
                    edgeItem->setText(1, QString::number(edgeIdx));
                    
                    // 计算边的长度
                    double edgeLength = 0.0;
                    try {
                        Standard_Real first, last;
                        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
                        if (!curve.IsNull()) {
                            GeomAdaptor_Curve adaptor(curve);
                            edgeLength = GCPnts_AbscissaPoint::Length(adaptor, first, last);
                        }
                    } catch (...) {
                        edgeLength = 0.0;
                    }
                    
                    edgeItem->setText(2, QString::fromUtf8("长度: %1").arg(edgeLength));
                    edgeItem->setForeground(0, QBrush(Qt::darkGreen));
                }
            }
            
            solidItem->setExpanded(true);
        }
    } else {
        // 如果没有Solid，显示Shells
        int shellIdx = 1;
        for (TopExp_Explorer shellIter(m_processor->getShape(), TopAbs_SHELL); 
             shellIter.More(); shellIter.Next(), shellIdx++) {
        TopoDS_Shell shell = TopoDS::Shell(shellIter.Current());
        
        auto* shellItem = new QTreeWidgetItem(root);
        shellItem->setText(0, QString::fromUtf8("Shell %1").arg(shellIdx));
        shellItem->setText(1, QString::number(shellIdx));
        
        // 统计该Shell的面数
        int shellFaceCount = 0;
        int shellEdgeCount = 0;
        for (TopExp_Explorer exp(shell, TopAbs_FACE); exp.More(); exp.Next()) shellFaceCount++;
        for (TopExp_Explorer exp(shell, TopAbs_EDGE); exp.More(); exp.Next()) shellEdgeCount++;
        shellItem->setText(2, QString::fromUtf8("%1 个面, %2 条边").arg(shellFaceCount).arg(shellEdgeCount));
        shellItem->setForeground(0, QBrush(QColor(128, 0, 128))); // 紫色
        
        // 遍历该Shell的所有Faces
        int faceIdx = 1;
        for (TopExp_Explorer faceIter(shell, TopAbs_FACE); 
             faceIter.More(); faceIter.Next(), faceIdx++) {
            TopoDS_Face face = TopoDS::Face(faceIter.Current());
            
            auto* faceItem = new QTreeWidgetItem(shellItem);
            faceItem->setText(0, QString::fromUtf8("面 %1").arg(faceIdx));
            faceItem->setText(1, QString::number(faceIdx));
            
            // 统计该面的边数
            int faceEdgeCount = 0;
            for (TopExp_Explorer exp(face, TopAbs_EDGE); exp.More(); exp.Next()) faceEdgeCount++;
            faceItem->setText(2, QString::fromUtf8("%1 条边").arg(faceEdgeCount));
            faceItem->setForeground(0, QBrush(QColor(0, 100, 200))); // 蓝色
            
            // 计算面的面积
            double faceArea = 0.0;
            try {
                Bnd_Box box;
                BRepBndLib::Add(face, box);
                double xMin, yMin, zMin, xMax, yMax, zMax;
                box.Get(xMin, yMin, zMin, xMax, yMax, zMax);
                double dx = xMax - xMin;
                double dy = yMax - yMin;
                double dz = zMax - zMin;
                double dims[3] = {dx, dy, dz};
                std::sort(dims, dims + 3);
                faceArea = dims[1] * dims[2];
            } catch (...) {
                faceArea = 0.0;
            }
            
            // 在面节点下添加面积信息作为子节点
            auto* areaItem = new QTreeWidgetItem(faceItem);
            areaItem->setText(0, QString::fromUtf8("  面积"));
            areaItem->setText(1, "");
            areaItem->setText(2, QString("%1").arg(faceArea, 0, 'g', 6));
            areaItem->setForeground(0, QBrush(Qt::gray));
            
            // 遍历该面的所有Edges
            int edgeIdx = 1;
            for (TopExp_Explorer edgeIter(face, TopAbs_EDGE); 
                 edgeIter.More(); edgeIter.Next(), edgeIdx++) {
                TopoDS_Edge edge = TopoDS::Edge(edgeIter.Current());
                
                auto* edgeItem = new QTreeWidgetItem(faceItem);
                edgeItem->setText(0, QString::fromUtf8("  边 %1").arg(edgeIdx));
                edgeItem->setText(1, QString::number(edgeIdx));
                
                // 计算边的长度
                double edgeLength = 0.0;
                try {
                    Standard_Real first, last;
                    Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
                    if (!curve.IsNull()) {
                        GeomAdaptor_Curve adaptor(curve);
                        edgeLength = GCPnts_AbscissaPoint::Length(adaptor, first, last);
                    }
                } catch (...) {
                    edgeLength = 0.0;
                }
                
                edgeItem->setText(2, QString::fromUtf8("长度: %1").arg(edgeLength));
                edgeItem->setForeground(0, QBrush(Qt::darkGreen));
            }
        }
        
        shellItem->setExpanded(true);
        }
    }
    
    root->setExpanded(true);
    m_geomTree->resizeColumnToContents(0);
    m_geomTree->resizeColumnToContents(1);
}

void GeomProcessorWindow::displayCurrentShape()
{
    if (!m_context.IsNull()) {
        m_processor->displayShape(m_context, true);
    }
}

void GeomProcessorWindow::updateStatusInfo()
{
    int nFaces   = m_processor->numFaces();
    int nShells  = m_processor->numShells();
    int nSolids  = m_processor->numSolids();
    
    // 如果有Solid，Shell内嵌在Solid中，不应显示Shell数量
    QString shellInfo;
    if (nSolids > 0) {
        shellInfo = "";  // 不显示Shell信息
    } else {
        shellInfo = QString("%1 个Shell | ").arg(nShells);
    }
    
    m_statusLabel->setText(
        QString("几何就绪 | %1 个面 | %2 %3 个实体").arg(nFaces).arg(shellInfo).arg(nSolids));
    // 更新左下角悬浮信息
    updateGeomInfoLabel(
        QString("面: %1   Shell: %2   实体: %3").arg(nFaces).arg(nSolids > 0 ? 0 : nShells).arg(nSolids));
}

bool GeomProcessorWindow::autoSaveAndSend()
{
    if (!m_processor->hasShape()) {
        return false;
    }
    
    // 保存结果到临时文件
    QString appDir = QCoreApplication::applicationDirPath();
    QString returnedDir = QDir::cleanPath(appDir + "/../../example/returned");
    if (!QDir(returnedDir).exists()) {
        returnedDir = QDir::cleanPath(appDir + "/../example/returned");
    }
    if (!QDir(returnedDir).exists()) {
        returnedDir = QDir::cleanPath(appDir + "/example/returned");
    }
    QDir().mkpath(returnedDir);
    
    QString guid    = QUuid::createUuid().toString();
    guid.remove('{').remove('}');
    QString tmpFile = QDir(returnedDir).filePath(guid + ".stp");
    
    if (!m_processor->saveSTEP(tmpFile)) {
        QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        tmpFile = QDir(tmpDir).filePath("geomproc_result_" + guid + ".stp");
        if (!m_processor->saveSTEP(tmpFile)) {
            return false;
        }
    }
    
    // 发送回SimulationTool
    return m_receiver->sendResult(tmpFile);
}

// ---------------------------------------------------------------------------
// CAD检查修复停靠面板设置
// ---------------------------------------------------------------------------

void GeomProcessorWindow::setupCadCheckPanel()
{
    m_cadCheckWidget = new CadCheckRepairWidget(this);
    // setChecker将在构造函数中调用，因为m_checker在此时还未创建
    
    // 连接信号
    connect(m_cadCheckWidget, &CadCheckRepairWidget::checkingFinished,
            this, [this](int problemCount) {
        m_statusLabel->setText(QString::fromUtf8("CAD检查完成，发现 %1 个问题").arg(problemCount));
    });
    connect(m_cadCheckWidget, &CadCheckRepairWidget::repairRequested,
            this, [this](const QList<int>& problemIndices) {
        // TODO: 实现修复功能
        m_statusLabel->setText(QString::fromUtf8("修复功能待实现。选中了 %1 个问题").arg(problemIndices.size()));
    });
    connect(m_cadCheckWidget, &CadCheckRepairWidget::highlightObject,
            this, [this](int index) {
        m_statusLabel->setText(QString::fromUtf8("高亮显示元素索引: %1").arg(index));
    });
    
    m_cadCheckDock = new QDockWidget(QString::fromUtf8("CAD检查修复"), this);
    m_cadCheckDock->setWidget(m_cadCheckWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_cadCheckDock);
}

// ---------------------------------------------------------------------------
// Parasolid检查修复停靠面板设置
// ---------------------------------------------------------------------------

void GeomProcessorWindow::setupParasolidCheckPanel()
{
    m_parasolidCheckWidget = new ParasolidCheckRepairWidget(this);
    // setChecker将在构造函数中调用
    
    // 连接信号
    connect(m_parasolidCheckWidget, &ParasolidCheckRepairWidget::checkingFinished,
            this, [this](int problemCount) {
        m_statusLabel->setText(QString::fromUtf8("Parasolid检查完成，发现 %1 个问题").arg(problemCount));
    });
    connect(m_parasolidCheckWidget, &ParasolidCheckRepairWidget::repairRequested,
            this, [this](const QList<int>& problemIndices) {
        // TODO: 实现Parasolid修复功能
        m_statusLabel->setText(QString::fromUtf8("Parasolid修复功能待实现。选中了 %1 个问题").arg(problemIndices.size()));
    });
    connect(m_parasolidCheckWidget, &ParasolidCheckRepairWidget::highlightObject,
            this, [this](int index) {
        m_statusLabel->setText(QString::fromUtf8("高亮显示Parasolid元素索引: %1").arg(index));
    });
    
    m_parasolidCheckDock = new QDockWidget(QString::fromUtf8("Parasolid检查修复"), this);
    m_parasolidCheckDock->setWidget(m_parasolidCheckWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_parasolidCheckDock);
    
    // 默认隐藏Parasolid面板，让用户通过工具栏按钮来显示
    m_parasolidCheckDock->setVisible(false);
}
